#pragma once

#include "rules.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

namespace doc {

class UndoLog; // 前向声明，execute 时可选写日志

// 计划中的一条移动
struct MoveItem {
    QString sourcePath;
    QString destPath;      // 期望目标（执行时可能因冲突再改名）
    QString categoryName;  // 类别名或命中的规则名
    bool checked = true;   // 用户在预览表格中勾选
};

struct OrganizeResult {
    int moved = 0;
    int failed = 0;
    QStringList errors;
    // 实际执行的移动（源 → 实际目标），供撤销/提示使用
    QVector<QPair<QString, QString>> performedMoves;
};

// 整理引擎：扫描桌面 → 生成移动计划 → 执行移动（只移动，不复制不删除）
class Organizer {
public:
    explicit Organizer(const QString &desktopDir);

    QString desktopDir() const;
    void setDesktopDir(const QString &dir);

    // 白名单：这些文件名（不含路径）永不移动；桌面根目录的文件夹天然跳过
    QStringList whitelist() const;
    void setWhitelist(const QStringList &names);

    // 扫描桌面顶层文件（不含文件夹、不递归）
    QStringList scanFiles() const;

    // 生成移动计划（不触碰文件）。categoryDirs 可覆盖默认类别目录名。
    QVector<MoveItem> buildPlan(const QVector<Rule> &rules,
                                const QMap<Category, QString> &categoryDirs = {}) const;

    // 执行计划中被勾选的项；writeUndo 为 true 且设置了 undoLog 时写日志。
    OrganizeResult execute(const QVector<MoveItem> &items, bool writeUndo = true);

    void setUndoLog(UndoLog *undoLog);
    UndoLog *undoLog() const;

    // 目标已存在时生成 "name (1).ext"、"name (2).ext"…
    static QString uniqueDestination(const QString &destPath);
    // 同上，但 reserved 中的路径也视为已占用（用于同批计划内去重）
    static QString uniqueDestination(const QString &destPath,
                                     const QSet<QString> &reserved);

private:
    QString m_desktopDir;
    QStringList m_whitelist;
    UndoLog *m_undoLog = nullptr;
};

} // namespace doc
