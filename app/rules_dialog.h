#pragma once

#include "classifier.h"
#include "rules.h"

#include <QDialog>
#include <QMap>

QT_BEGIN_NAMESPACE
namespace Ui { class RulesDialog; }
QT_END_NAMESPACE

// 规则编辑对话框：自定义规则（按优先级排序）+ 类别映射
// 构造时传入当前配置，用户点确定后通过 rules() / categoryDirs() 取回
class RulesDialog : public QDialog {
    Q_OBJECT

public:
    RulesDialog(const QVector<doc::Rule> &rules,
                const QMap<doc::Category, QString> &categoryDirs,
                QWidget *parent = nullptr);
    ~RulesDialog() override;

    QVector<doc::Rule> rules() const;
    QMap<doc::Category, QString> categoryDirs() const;

private slots:
    void onSelectionChanged();
    void onAdd();
    void onDelete();
    void onMoveUp();
    void onMoveDown();
    void onMatchTypeChanged(int index);

private:
    void refreshList(int selectRow = -1);
    void loadForm(int row);
    void commitForm();          // 表单内容写回当前规则
    int currentRow() const;
    static QString ruleDisplayText(const doc::Rule &rule);

    Ui::RulesDialog *ui;
    QVector<doc::Rule> m_rules;               // 工作副本
    QMap<doc::Category, QString> m_categoryDirs;
    bool m_loadingForm = false;
};
