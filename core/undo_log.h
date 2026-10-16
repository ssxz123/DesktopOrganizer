#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

namespace doc {

// JSON 撤销日志：每次整理追加一条（时间戳 + src→dst 移动对列表），
// 按条整体撤销，程序重启后仍可撤销。
class UndoLog {
public:
    struct Entry {
        QDateTime timestamp;
        QVector<QPair<QString, QString>> moves; // 源 → 实际目标
    };

    struct UndoResult {
        int undone = 0;      // 成功撤销的条数
        int restored = 0;    // 成功移回的文件数
        QStringList errors;
    };

    explicit UndoLog(const QString &filePath);

    QString filePath() const;

    bool load();
    bool save() const;

    const QVector<Entry> &entries() const;
    int entryCount() const;

    void appendEntry(const QDateTime &timestamp,
                     const QVector<QPair<QString, QString>> &moves);
    void clear();

    // 撤销最近 count 条（最新先撤，条内按反向顺序移回）。
    // 原位置被占用时同样自动改名 "name (1).ext"。已处理的条目从日志移除。
    UndoResult undoLast(int count = 1);

private:
    QString m_filePath;
    QVector<Entry> m_entries;
};

} // namespace doc
