#pragma once

#include "classifier.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace doc {

// 自定义规则匹配方式
enum class MatchType {
    Extension,      // 按扩展名：pattern 为 "jpg,png"
    Wildcard,       // 按文件名通配：pattern 为 "*.log"、"report?.txt"
    OlderThanDays,  // 按修改日期：pattern 为天数 "30"（N 天前修改的）
};

// 一条自定义规则：命中后文件移入 targetDir
struct Rule {
    QString id;
    QString name;
    MatchType matchType = MatchType::Extension;
    QString pattern;
    QString targetDir;   // 相对桌面目录或绝对路径
    bool enabled = true;

    static Rule make(MatchType type, const QString &pattern,
                     const QString &targetDir, const QString &name = {});

    bool matches(const QString &fileName, const QDateTime &modified) const;
    QString matchTypeString() const;
};

QJsonObject ruleToJson(const Rule &rule);
Rule ruleFromJson(const QJsonObject &object);

// 规则匹配结果
struct Destination {
    bool matched = false;    // false 表示回落到内置分类
    QString targetDir;       // 相对桌面或绝对路径
    QString viaRuleName;     // 命中的规则名
};

// 按顺序匹配，第一条命中的规则生效（规则优先级 > 内置分类 > 默认“其他”）
Destination matchRules(const QVector<Rule> &rules, const QString &fileName,
                       const QDateTime &modified);

} // namespace doc
