#pragma once

#include "config.h"
#include "desktop_watch.h"
#include "organizer.h"
#include "undo_log.h"

#include <QMainWindow>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 主窗口：预览表格 / 一键整理 / 撤销 / 规则 / 设置 / 实时监控
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onPreview();
    void onExecute();
    void onUndo();
    void onRules();
    void onSettings();
    void onWatchTriggered(const QString &dirPath);

private:
    void reloadConfig();
    void applyAutoWatch();
    void refreshPlanTable(const QVector<doc::MoveItem> &plan);
    void organizeSilently();

    Ui::MainWindow *ui;

    QString m_desktopDir;
    doc::Organizer m_organizer;
    doc::UndoLog m_undoLog;
    doc::Settings m_settings;
    QVector<doc::Rule> m_rules;
    doc::DesktopWatch m_watch;
};
