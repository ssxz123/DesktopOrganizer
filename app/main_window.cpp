#include "main_window.h"

#include "rules_dialog.h"
#include "settings_dialog.h"
#include "ui_main_window.h"

#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_desktopDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
    , m_organizer(m_desktopDir)
    , m_undoLog(doc::Config::undoLogFilePath())
{
    ui->setupUi(this);

    ui->desktopPathLabel->setText(QStringLiteral("桌面：%1").arg(m_desktopDir));

    // 预览表格：勾选 / 文件名 / 当前位置 / 将移动到 / 类别
    ui->planTable->setColumnCount(5);
    ui->planTable->setHorizontalHeaderLabels({QStringLiteral("整理"),
                                              QStringLiteral("文件名"),
                                              QStringLiteral("当前位置"),
                                              QStringLiteral("将移动到"),
                                              QStringLiteral("类别")});
    ui->planTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->planTable->setColumnWidth(0, 50);

    if (!m_undoLog.load()) {
        statusBar()->showMessage(QStringLiteral("撤销日志读取失败：%1")
                                     .arg(m_undoLog.filePath()), 10000);
    }

    reloadConfig();
    m_organizer.setUndoLog(&m_undoLog);
    applyAutoWatch();

    connect(ui->actionPreview, &QAction::triggered, this, &MainWindow::onPreview);
    connect(ui->actionExecute, &QAction::triggered, this, &MainWindow::onExecute);
    connect(ui->actionUndo, &QAction::triggered, this, &MainWindow::onUndo);
    connect(ui->actionRules, &QAction::triggered, this, &MainWindow::onRules);
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::onSettings);
    connect(&m_watch, &doc::DesktopWatch::changeDetected,
            this, &MainWindow::onWatchTriggered);

    statusBar()->showMessage(QStringLiteral("就绪。点击「预览」生成整理计划。"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reloadConfig()
{
    m_settings = doc::Config::loadSettings();
    m_rules = doc::Config::loadRules();
    m_organizer.setWhitelist(m_settings.whitelist);
}

void MainWindow::applyAutoWatch()
{
    m_watch.stop();
    if (!m_settings.autoOrganize)
        return;

    m_watch.setDebounce(m_settings.debounceMs);
    if (!m_watch.start(m_desktopDir)) {
        statusBar()->showMessage(QStringLiteral("无法监控桌面目录：%1")
                                     .arg(m_desktopDir), 10000);
        return;
    }
    statusBar()->showMessage(
        QStringLiteral("自动整理已开启（防抖 %1 毫秒）").arg(m_settings.debounceMs), 5000);
}

void MainWindow::refreshPlanTable(const QVector<doc::MoveItem> &plan)
{
    ui->planTable->setRowCount(0);
    ui->planTable->setRowCount(plan.size());

    for (int row = 0; row < plan.size(); ++row) {
        const doc::MoveItem &item = plan.at(row);

        auto *checkItem = new QTableWidgetItem;
        checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        checkItem->setCheckState(item.checked ? Qt::Checked : Qt::Unchecked);
        ui->planTable->setItem(row, 0, checkItem);

        const QFileInfo srcInfo(item.sourcePath);
        const QFileInfo dstInfo(item.destPath);

        auto *nameItem = new QTableWidgetItem(srcInfo.fileName());
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->planTable->setItem(row, 1, nameItem);

        auto *fromItem = new QTableWidgetItem(srcInfo.dir().dirName().isEmpty()
                                                  ? srcInfo.dir().absolutePath()
                                                  : srcInfo.dir().dirName());
        fromItem->setFlags(fromItem->flags() & ~Qt::ItemIsEditable);
        ui->planTable->setItem(row, 2, fromItem);

        auto *toItem = new QTableWidgetItem(
            QStringLiteral("%1/%2").arg(dstInfo.dir().dirName(), dstInfo.fileName()));
        toItem->setFlags(toItem->flags() & ~Qt::ItemIsEditable);
        ui->planTable->setItem(row, 3, toItem);

        auto *categoryItem = new QTableWidgetItem(item.categoryName);
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsEditable);
        ui->planTable->setItem(row, 4, categoryItem);
    }
}

void MainWindow::onPreview()
{
    reloadConfig();
    const QVector<doc::MoveItem> plan =
        m_organizer.buildPlan(m_rules, m_settings.categoryDirs);
    refreshPlanTable(plan);

    if (plan.isEmpty())
        statusBar()->showMessage(QStringLiteral("桌面很整洁，没有需要整理的文件。"));
    else
        statusBar()->showMessage(QStringLiteral("共 %1 个文件待整理，可取消勾选后点「执行整理」。")
                                     .arg(plan.size()));
}

void MainWindow::onExecute()
{
    // 预览表为空时（如直接点执行）先生成计划
    if (ui->planTable->rowCount() == 0)
        onPreview();

    // 以最新扫描重建计划，仅保留用户在表格中的勾选状态
    const QVector<doc::MoveItem> plan =
        m_organizer.buildPlan(m_rules, m_settings.categoryDirs);
    QVector<doc::MoveItem> items;
    items.reserve(plan.size());
    for (int row = 0; row < plan.size() && row < ui->planTable->rowCount(); ++row) {
        doc::MoveItem item = plan.at(row);
        QTableWidgetItem *checkItem = ui->planTable->item(row, 0);
        item.checked = checkItem && checkItem->checkState() == Qt::Checked;
        items.append(item);
    }

    const doc::OrganizeResult result = m_organizer.execute(items);

    QString message = QStringLiteral("整理完成：移动 %1 个文件").arg(result.moved);
    if (result.failed > 0)
        message += QStringLiteral("，失败 %1 个").arg(result.failed);

    if (!result.errors.isEmpty())
        QMessageBox::warning(this, QStringLiteral("整理结果"),
                             message + QStringLiteral("\n\n") + result.errors.join(QLatin1Char('\n')));
    else
        QMessageBox::information(this, QStringLiteral("整理结果"), message);

    onPreview(); // 刷新预览
}

void MainWindow::onUndo()
{
    if (m_undoLog.entryCount() == 0) {
        QMessageBox::information(this, QStringLiteral("撤销"),
                                 QStringLiteral("没有可撤销的整理记录。"));
        return;
    }

    const doc::UndoLog::UndoResult result = m_undoLog.undoLast();
    QString message = QStringLiteral("已撤销 %1 次整理，移回 %2 个文件。")
                          .arg(result.undone).arg(result.restored);
    if (!result.errors.isEmpty())
        QMessageBox::warning(this, QStringLiteral("撤销"),
                             message + QStringLiteral("\n\n") + result.errors.join(QLatin1Char('\n')));
    else
        QMessageBox::information(this, QStringLiteral("撤销"), message);

    onPreview();
}

void MainWindow::onRules()
{
    RulesDialog dialog(m_rules, m_settings.categoryDirs, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_rules = dialog.rules();
    m_settings.categoryDirs = dialog.categoryDirs();
    if (!doc::Config::saveRules(m_rules) || !doc::Config::saveSettings(m_settings))
        QMessageBox::warning(this, QStringLiteral("整理规则"),
                             QStringLiteral("配置保存失败，请检查 %1").arg(doc::Config::appDataDir()));
    reloadConfig();
}

void MainWindow::onSettings()
{
    SettingsDialog dialog(m_settings, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_settings = dialog.settings();
    if (!doc::Config::saveSettings(m_settings))
        QMessageBox::warning(this, QStringLiteral("设置"),
                             QStringLiteral("配置保存失败，请检查 %1").arg(doc::Config::appDataDir()));
    reloadConfig();
    applyAutoWatch();
}

void MainWindow::onWatchTriggered(const QString &dirPath)
{
    if (dirPath != m_desktopDir)
        return;
    organizeSilently();
}

void MainWindow::organizeSilently()
{
    reloadConfig();
    const QVector<doc::MoveItem> plan =
        m_organizer.buildPlan(m_rules, m_settings.categoryDirs);
    if (plan.isEmpty())
        return;

    const doc::OrganizeResult result = m_organizer.execute(plan);
    if (result.moved > 0) {
        statusBar()->showMessage(
            QStringLiteral("自动整理：%1 个文件已归档。").arg(result.moved), 10000);
        onPreview();
    }
}
