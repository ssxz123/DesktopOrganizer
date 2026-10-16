#pragma once

#include "config.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

// 设置对话框：自动整理开关 / 防抖时长 / 白名单
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(const doc::Settings &settings, QWidget *parent = nullptr);
    ~SettingsDialog() override;

    doc::Settings settings() const;

private slots:
    void onAdd();
    void onRemove();

private:
    Ui::SettingsDialog *ui;
};
