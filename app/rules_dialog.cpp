#include "rules_dialog.h"

#include "ui_rules_dialog.h"

#include <QTableWidgetItem>

RulesDialog::RulesDialog(const QVector<doc::Rule> &rules,
                         const QMap<doc::Category, QString> &categoryDirs,
                         QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RulesDialog)
    , m_rules(rules)
    , m_categoryDirs(categoryDirs)
{
    ui->setupUi(this);

    // 类别映射表格：7 个内置类别
    ui->categoryTable->setColumnCount(2);
    ui->categoryTable->setHorizontalHeaderLabels({QStringLiteral("类别"),
                                                  QStringLiteral("目标目录")});
    ui->categoryTable->horizontalHeader()->setStretchLastSection(true);
    ui->categoryTable->setRowCount(static_cast<int>(doc::Category::Other) + 1);
    for (int i = 0; i <= static_cast<int>(doc::Category::Other); ++i) {
        const auto category = static_cast<doc::Category>(i);
        auto *nameItem = new QTableWidgetItem(doc::Classifier::categoryName(category));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        ui->categoryTable->setItem(i, 0, nameItem);
        const QString dir = m_categoryDirs.value(
            category, doc::Classifier::defaultCategoryDirName(category));
        ui->categoryTable->setItem(i, 1, new QTableWidgetItem(dir));
    }
    ui->categoryTable->resizeColumnToContents(0);

    connect(ui->rulesList, &QListWidget::currentRowChanged,
            this, &RulesDialog::onSelectionChanged);
    connect(ui->addButton, &QPushButton::clicked, this, &RulesDialog::onAdd);
    connect(ui->deleteButton, &QPushButton::clicked, this, &RulesDialog::onDelete);
    connect(ui->moveUpButton, &QPushButton::clicked, this, &RulesDialog::onMoveUp);
    connect(ui->moveDownButton, &QPushButton::clicked, this, &RulesDialog::onMoveDown);
    connect(ui->typeCombo, &QComboBox::currentIndexChanged,
            this, &RulesDialog::onMatchTypeChanged);

    refreshList(m_rules.isEmpty() ? -1 : 0);
}

RulesDialog::~RulesDialog()
{
    delete ui;
}

QVector<doc::Rule> RulesDialog::rules() const
{
    return m_rules;
}

QMap<doc::Category, QString> RulesDialog::categoryDirs() const
{
    QMap<doc::Category, QString> result;
    for (int i = 0; i <= static_cast<int>(doc::Category::Other); ++i) {
        const auto category = static_cast<doc::Category>(i);
        const QTableWidgetItem *item = ui->categoryTable->item(i, 1);
        const QString dir = item ? item->text().trimmed() : QString();
        if (!dir.isEmpty() && dir != doc::Classifier::defaultCategoryDirName(category))
            result.insert(category, dir);
    }
    return result;
}

int RulesDialog::currentRow() const
{
    return ui->rulesList->currentRow();
}

QString RulesDialog::ruleDisplayText(const doc::Rule &rule)
{
    const QString flag = rule.enabled ? QString() : QStringLiteral("〔已停用〕");
    return QStringLiteral("%1  %2：%3 → %4")
        .arg(rule.name, rule.matchTypeString(), rule.pattern, rule.targetDir)
        + flag;
}

void RulesDialog::refreshList(int selectRow)
{
    commitForm();
    const int previous = currentRow();
    Q_UNUSED(previous);

    ui->rulesList->clear();
    for (const doc::Rule &rule : std::as_const(m_rules))
        ui->rulesList->addItem(ruleDisplayText(rule));

    if (selectRow >= 0 && selectRow < ui->rulesList->count())
        ui->rulesList->setCurrentRow(selectRow);
    else if (ui->rulesList->count() > 0)
        ui->rulesList->setCurrentRow(ui->rulesList->count() - 1);
    else
        onSelectionChanged();
}

void RulesDialog::loadForm(int row)
{
    m_loadingForm = true;
    if (row < 0 || row >= m_rules.size()) {
        ui->nameEdit->clear();
        ui->patternEdit->clear();
        ui->targetDirEdit->clear();
        ui->typeCombo->setCurrentIndex(0);
        ui->enabledCheck->setChecked(true);
        ui->nameEdit->setEnabled(false);
        ui->patternEdit->setEnabled(false);
        ui->targetDirEdit->setEnabled(false);
        ui->typeCombo->setEnabled(false);
        ui->enabledCheck->setEnabled(false);
        m_loadingForm = false;
        return;
    }

    const doc::Rule &rule = m_rules.at(row);
    ui->nameEdit->setEnabled(true);
    ui->patternEdit->setEnabled(true);
    ui->targetDirEdit->setEnabled(true);
    ui->typeCombo->setEnabled(true);
    ui->enabledCheck->setEnabled(true);
    ui->nameEdit->setText(rule.name);
    ui->typeCombo->setCurrentIndex(static_cast<int>(rule.matchType));
    ui->patternEdit->setText(rule.pattern);
    ui->targetDirEdit->setText(rule.targetDir);
    ui->enabledCheck->setChecked(rule.enabled);
    onMatchTypeChanged(static_cast<int>(rule.matchType));
    m_loadingForm = false;
}

void RulesDialog::commitForm()
{
    if (m_loadingForm)
        return;
    const int row = currentRow();
    if (row < 0 || row >= m_rules.size())
        return;

    doc::Rule &rule = m_rules[row];
    rule.name = ui->nameEdit->text().trimmed();
    rule.matchType = static_cast<doc::MatchType>(ui->typeCombo->currentIndex());
    rule.pattern = ui->patternEdit->text().trimmed();
    rule.targetDir = ui->targetDirEdit->text().trimmed();
    rule.enabled = ui->enabledCheck->isChecked();

    // 同步刷新列表中当前行的显示（不重排选择）
    QListWidgetItem *item = ui->rulesList->item(row);
    if (item)
        item->setText(ruleDisplayText(rule));
}

void RulesDialog::onSelectionChanged()
{
    commitForm(); // 先提交旧选择
    loadForm(currentRow());
}

void RulesDialog::onAdd()
{
    commitForm();
    m_rules.append(doc::Rule::make(doc::MatchType::Extension,
                                   QStringLiteral("txt"),
                                   QStringLiteral("文档"),
                                   QStringLiteral("新规则")));
    refreshList(m_rules.size() - 1);
    ui->nameEdit->setFocus();
    ui->nameEdit->selectAll();
}

void RulesDialog::onDelete()
{
    const int row = currentRow();
    if (row < 0 || row >= m_rules.size())
        return;
    m_rules.remove(row);
    refreshList(qMin(row, m_rules.size() - 1));
}

void RulesDialog::onMoveUp()
{
    const int row = currentRow();
    if (row <= 0)
        return;
    commitForm();
    std::swap(m_rules[row], m_rules[row - 1]);
    refreshList(row - 1);
}

void RulesDialog::onMoveDown()
{
    const int row = currentRow();
    if (row < 0 || row >= m_rules.size() - 1)
        return;
    commitForm();
    std::swap(m_rules[row], m_rules[row + 1]);
    refreshList(row + 1);
}

void RulesDialog::onMatchTypeChanged(int index)
{
    switch (static_cast<doc::MatchType>(index)) {
    case doc::MatchType::Extension:
        ui->patternEdit->setPlaceholderText(QStringLiteral("如：jpg,png（逗号分隔，不含点）"));
        break;
    case doc::MatchType::Wildcard:
        ui->patternEdit->setPlaceholderText(QStringLiteral("如：*.log、report?.txt"));
        break;
    case doc::MatchType::OlderThanDays:
        ui->patternEdit->setPlaceholderText(QStringLiteral("如：30（30 天前修改的）"));
        break;
    }
}
