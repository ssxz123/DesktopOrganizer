#include "undo_log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace doc {

UndoLog::UndoLog(const QString &filePath)
    : m_filePath(filePath)
{
}

QString UndoLog::filePath() const
{
    return m_filePath;
}

bool UndoLog::load()
{
    m_entries.clear();

    QFile file(m_filePath);
    if (!file.exists())
        return true; // 尚无日志，视为成功
    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return false;

    const QJsonArray array = document.object()
                                 .value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.timestamp = QDateTime::fromString(
            object.value(QStringLiteral("timestamp")).toString(), Qt::ISODate);
        const QJsonArray moves = object.value(QStringLiteral("moves")).toArray();
        for (const QJsonValue &moveValue : moves) {
            const QJsonObject move = moveValue.toObject();
            entry.moves.append({move.value(QStringLiteral("src")).toString(),
                                move.value(QStringLiteral("dst")).toString()});
        }
        m_entries.append(entry);
    }
    return true;
}

bool UndoLog::save() const
{
    QJsonArray array;
    for (const Entry &entry : m_entries) {
        QJsonObject object;
        object[QStringLiteral("timestamp")] = entry.timestamp.toString(Qt::ISODate);
        QJsonArray moves;
        for (const auto &[src, dst] : entry.moves) {
            QJsonObject move;
            move[QStringLiteral("src")] = src;
            move[QStringLiteral("dst")] = dst;
            moves.append(move);
        }
        object[QStringLiteral("moves")] = moves;
        array.append(object);
    }

    QJsonObject root;
    root[QStringLiteral("entries")] = array;
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);

    // 先写临时文件再替换，避免中途崩溃损坏日志
    const QString tempPath = m_filePath + QStringLiteral(".tmp");
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    if (file.write(data) != data.size())
        return false;
    file.close();

    QFile::remove(m_filePath);
    return QFile::rename(tempPath, m_filePath);
}

const QVector<UndoLog::Entry> &UndoLog::entries() const
{
    return m_entries;
}

int UndoLog::entryCount() const
{
    return m_entries.size();
}

void UndoLog::appendEntry(const QDateTime &timestamp,
                          const QVector<QPair<QString, QString>> &moves)
{
    Entry entry;
    entry.timestamp = timestamp;
    entry.moves = moves;
    m_entries.append(entry);
}

void UndoLog::clear()
{
    m_entries.clear();
}

// 与 Organizer::uniqueDestination 相同的改名规则
static QString uniquePath(const QString &path, const QSet<QString> &reserved)
{
    const QFileInfo info(path);
    const QDir dir = info.dir();
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix();

    for (int i = 0; i < 100000; ++i) {
        QString candidate;
        if (i == 0) {
            candidate = info.fileName();
        } else {
            candidate = QStringLiteral("%1 (%2)").arg(base).arg(i);
            if (!suffix.isEmpty())
                candidate += QLatin1Char('.') + suffix;
        }
        const QString candidatePath = dir.absoluteFilePath(candidate);
        if (!QFile::exists(candidatePath) && !reserved.contains(candidatePath))
            return candidatePath;
    }
    return {};
}

UndoLog::UndoResult UndoLog::undoLast(int count)
{
    UndoResult result;
    if (count <= 0 || m_entries.isEmpty())
        return result;

    count = qMin(count, m_entries.size());
    QSet<QString> usedPaths;

    for (int i = 0; i < count; ++i) {
        const Entry entry = m_entries.last();
        bool anyRestored = false;

        // 条内按反向顺序移回，尽量还原移动前的文件顺序
        for (auto it = entry.moves.crbegin(); it != entry.moves.crend(); ++it) {
            const QString &dst = it->second;
            const QString &src = it->first;

            if (!QFile::exists(dst)) {
                result.errors.append(QStringLiteral("已找不到文件：%1").arg(dst));
                continue;
            }

            QDir srcDir = QFileInfo(src).dir();
            if (!srcDir.exists() && !srcDir.mkpath(QStringLiteral("."))) {
                result.errors.append(QStringLiteral("无法创建目录：%1").arg(srcDir.absolutePath()));
                continue;
            }

            const QString finalSrc = uniquePath(src, usedPaths);
            if (finalSrc.isEmpty()) {
                result.errors.append(QStringLiteral("无法生成可用文件名：%1").arg(src));
                continue;
            }

            if (!QFile::rename(dst, finalSrc)) {
                if (!QFile::copy(dst, finalSrc) || !QFile::remove(dst)) {
                    result.errors.append(QStringLiteral("撤销失败：%1 → %2").arg(dst, src));
                    continue;
                }
            }

            usedPaths.insert(finalSrc);
            ++result.restored;
            anyRestored = true;
        }

        if (anyRestored)
            ++result.undone;
        m_entries.removeLast();
    }

    save(); // 结果是否落盘都继续，错误已通过 errors 汇报
    return result;
}

} // namespace doc
