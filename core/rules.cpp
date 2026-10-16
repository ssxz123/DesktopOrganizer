#include "rules.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUuid>

namespace doc {

Rule Rule::make(MatchType type, const QString &pattern,
                const QString &targetDir, const QString &ruleName)
{
    Rule rule;
    rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rule.name = ruleName.isEmpty() ? pattern : ruleName;
    rule.matchType = type;
    rule.pattern = pattern.trimmed();
    rule.targetDir = targetDir.trimmed();
    return rule;
}

bool Rule::matches(const QString &fileName, const QDateTime &modified) const
{
    if (!enabled || pattern.isEmpty())
        return false;

    switch (matchType) {
    case MatchType::Extension: {
        const QString ext = Classifier::normalizedExtension(fileName);
        if (ext.isEmpty())
            return false;
        const QStringList wanted = pattern.toLower().split(QLatin1Char(','),
                                                           Qt::SkipEmptyParts);
        return wanted.contains(ext);
    }
    case MatchType::Wildcard: {
        // * / ? 通配，大小写不敏感（跨平台行为一致）
        QString regex = QRegularExpression::escape(pattern);
        regex.replace(QStringLiteral("\\*"), QStringLiteral(".*"))
             .replace(QStringLiteral("\\?"), QStringLiteral("."));
        const QRegularExpression re(QStringLiteral("\\A") + regex + QStringLiteral("\\z"),
                                    QRegularExpression::CaseInsensitiveOption);
        return re.match(QFileInfo(fileName).fileName()).hasMatch();
    }
    case MatchType::OlderThanDays: {
        bool ok = false;
        const int days = pattern.toInt(&ok);
        if (!ok || days < 0 || !modified.isValid())
            return false;
        // 修改时间在未来（时钟偏差）不归档
        if (modified > QDateTime::currentDateTime())
            return false;
        return modified.daysTo(QDateTime::currentDateTime()) >= days;
    }
    }
    return false;
}

QString Rule::matchTypeString() const
{
    switch (matchType) {
    case MatchType::Extension:   return QStringLiteral("扩展名");
    case MatchType::Wildcard:    return QStringLiteral("文件名通配");
    case MatchType::OlderThanDays: return QStringLiteral("修改日期");
    }
    return {};
}

static QString matchTypeToString(MatchType type)
{
    switch (type) {
    case MatchType::Extension:     return QStringLiteral("extension");
    case MatchType::Wildcard:      return QStringLiteral("wildcard");
    case MatchType::OlderThanDays: return QStringLiteral("older_than_days");
    }
    return {};
}

static MatchType matchTypeFromString(const QString &string)
{
    if (string == QLatin1String("wildcard"))
        return MatchType::Wildcard;
    if (string == QLatin1String("older_than_days"))
        return MatchType::OlderThanDays;
    return MatchType::Extension;
}

QJsonObject ruleToJson(const Rule &rule)
{
    QJsonObject object;
    object[QStringLiteral("id")] = rule.id;
    object[QStringLiteral("name")] = rule.name;
    object[QStringLiteral("match_type")] = matchTypeToString(rule.matchType);
    object[QStringLiteral("pattern")] = rule.pattern;
    object[QStringLiteral("target_dir")] = rule.targetDir;
    object[QStringLiteral("enabled")] = rule.enabled;
    return object;
}

Rule ruleFromJson(const QJsonObject &object)
{
    Rule rule;
    rule.id = object.value(QStringLiteral("id")).toString();
    if (rule.id.isEmpty())
        rule.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rule.name = object.value(QStringLiteral("name")).toString();
    rule.matchType = matchTypeFromString(
        object.value(QStringLiteral("match_type")).toString());
    rule.pattern = object.value(QStringLiteral("pattern")).toString();
    rule.targetDir = object.value(QStringLiteral("target_dir")).toString();
    rule.enabled = object.value(QStringLiteral("enabled")).toBool(true);
    return rule;
}

Destination matchRules(const QVector<Rule> &rules, const QString &fileName,
                       const QDateTime &modified)
{
    for (const Rule &rule : rules) {
        if (rule.matches(fileName, modified)) {
            Destination destination;
            destination.matched = true;
            destination.targetDir = rule.targetDir;
            destination.viaRuleName = rule.name;
            return destination;
        }
    }
    return {};
}

} // namespace doc
