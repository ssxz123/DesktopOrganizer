#include "organizer.h"

#include "undo_log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace doc {

Organizer::Organizer(const QString &desktopDir)
    : m_desktopDir(desktopDir)
{
}

QString Organizer::desktopDir() const
{
    return m_desktopDir;
}

void Organizer::setDesktopDir(const QString &dir)
{
    m_desktopDir = dir;
}

QStringList Organizer::whitelist() const
{
    return m_whitelist;
}

void Organizer::setWhitelist(const QStringList &names)
{
    m_whitelist = names;
}

QStringList Organizer::scanFiles() const
{
    QDir dir(m_desktopDir);
    if (!dir.exists())
        return {};

    const QSet<QString> whitelist(m_whitelist.begin(), m_whitelist.end());
    const QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot,
                                              QDir::Name | QDir::IgnoreCase);
    QStringList files;
    for (const QString &entry : entries) {
        if (!whitelist.contains(entry))
            files.append(dir.absoluteFilePath(entry));
    }
    return files;
}

QVector<MoveItem> Organizer::buildPlan(const QVector<Rule> &rules,
                                       const QMap<Category, QString> &categoryDirs) const
{
    QVector<MoveItem> plan;
    const QDir desktop(m_desktopDir);

    const QStringList files = scanFiles();
    for (const QString &filePath : files) {
        const QFileInfo info(filePath);
        const QString fileName = info.fileName();

        // 规则命中 → 规则目标目录；否则 → 内置类别目录
        QString targetDir;
        QString viaName;
        const Destination destination = matchRules(rules, fileName, info.lastModified());
        if (destination.matched) {
            targetDir = destination.targetDir;
            viaName = destination.viaRuleName;
        } else {
            const Category category = Classifier::classify(fileName);
            targetDir = categoryDirs.value(category, Classifier::defaultCategoryDirName(category));
            viaName = Classifier::categoryName(category);
        }

        // 目标目录解析：绝对路径直接用；相对路径相对桌面
        QString destDirPath = targetDir;
        if (QDir::isRelativePath(destDirPath))
            destDirPath = desktop.absoluteFilePath(destDirPath);
        destDirPath = QDir(destDirPath).absolutePath();

        // 目标就是桌面本身（如 "." 或空）→ 无需移动
        if (QDir(destDirPath) == desktop)
            continue;

        MoveItem item;
        item.sourcePath = filePath;
        item.destPath = QDir(destDirPath).absoluteFilePath(fileName);
        item.categoryName = viaName;
        plan.append(item);
    }
    return plan;
}

QString Organizer::uniqueDestination(const QString &destPath)
{
    return uniqueDestination(destPath, {});
}

QString Organizer::uniqueDestination(const QString &destPath, const QSet<QString> &reserved)
{
    const QFileInfo info(destPath);
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
    return {}; // 理论上不可达
}

OrganizeResult Organizer::execute(const QVector<MoveItem> &items, bool writeUndo)
{
    OrganizeResult result;

    // 已占用的目标路径（含本次计划内），避免同批文件互相覆盖
    QSet<QString> usedDests;

    for (const MoveItem &item : items) {
        if (!item.checked)
            continue;

        QFileInfo srcInfo(item.sourcePath);
        if (!srcInfo.exists() || !srcInfo.isFile()) {
            ++result.failed;
            result.errors.append(QStringLiteral("源文件不存在：%1").arg(item.sourcePath));
            continue;
        }

        // 目标目录（执行时重新做冲突改名，因为预览后磁盘可能已变化）
        QDir destDir = QFileInfo(item.destPath).dir();
        if (!destDir.exists() && !destDir.mkpath(QStringLiteral("."))) {
            ++result.failed;
            result.errors.append(QStringLiteral("无法创建目录：%1").arg(destDir.absolutePath()));
            continue;
        }

        const QString finalDest = uniqueDestination(item.destPath, usedDests);
        if (finalDest.isEmpty()) {
            ++result.failed;
            result.errors.append(QStringLiteral("无法生成可用文件名：%1").arg(item.destPath));
            continue;
        }

        if (!QFile::rename(item.sourcePath, finalDest)) {
            // 跨卷等场景 rename 失败时退回 复制+删除
            if (!QFile::copy(item.sourcePath, finalDest)
                || !QFile::remove(item.sourcePath)) {
                ++result.failed;
                result.errors.append(QStringLiteral("移动失败：%1 → %2")
                                        .arg(item.sourcePath, finalDest));
                continue;
            }
        }

        usedDests.insert(finalDest);
        result.performedMoves.append({item.sourcePath, finalDest});
        ++result.moved;
    }

    if (writeUndo && m_undoLog && !result.performedMoves.isEmpty()) {
        m_undoLog->appendEntry(QDateTime::currentDateTime(), result.performedMoves);
        if (!m_undoLog->save())
            result.errors.append(QStringLiteral("撤销日志保存失败"));
    }

    return result;
}

void Organizer::setUndoLog(UndoLog *undoLog)
{
    m_undoLog = undoLog;
}

UndoLog *Organizer::undoLog() const
{
    return m_undoLog;
}

} // namespace doc
