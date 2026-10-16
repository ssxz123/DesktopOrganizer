#include "config.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace doc {

namespace {

QString categoryKey(Category category)
{
    return QString::number(static_cast<int>(category));
}

Category categoryFromKey(const QString &key)
{
    bool ok = false;
    const int value = key.toInt(&ok);
    if (!ok || value < 0 || value > static_cast<int>(Category::Other))
        return Category::Other;
    return static_cast<Category>(value);
}

} // namespace

QString Config::appDataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Config::rulesFilePath()
{
    return appDataDir() + QStringLiteral("/rules.json");
}

QString Config::settingsFilePath()
{
    return appDataDir() + QStringLiteral("/settings.json");
}

QString Config::undoLogFilePath()
{
    return appDataDir() + QStringLiteral("/undo_log.json");
}

bool Config::ensureAppDataDir()
{
    return QDir().mkpath(appDataDir());
}

QVector<Rule> Config::loadRules()
{
    QVector<Rule> rules;

    QFile file(rulesFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return rules;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return rules;

    const QJsonArray array = document.object()
                                 .value(QStringLiteral("rules")).toArray();
    for (const QJsonValue &value : array)
        rules.append(ruleFromJson(value.toObject()));
    return rules;
}

bool Config::saveRules(const QVector<Rule> &rules)
{
    if (!ensureAppDataDir())
        return false;

    QJsonArray array;
    for (const Rule &rule : rules)
        array.append(ruleToJson(rule));

    QJsonObject root;
    root[QStringLiteral("rules")] = array;

    QFile file(rulesFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) > 0;
}

Settings Config::loadSettings()
{
    Settings settings;

    QFile file(settingsFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return settings;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return settings;

    const QJsonObject object = document.object();
    settings.autoOrganize = object.value(QStringLiteral("auto_organize")).toBool(false);
    settings.debounceMs = object.value(QStringLiteral("debounce_ms")).toInt(2000);

    const QJsonArray whitelist = object.value(QStringLiteral("whitelist")).toArray();
    for (const QJsonValue &value : whitelist)
        settings.whitelist.append(value.toString());

    const QJsonObject categoryDirs = object.value(QStringLiteral("category_dirs")).toObject();
    for (auto it = categoryDirs.constBegin(); it != categoryDirs.constEnd(); ++it)
        settings.categoryDirs.insert(categoryFromKey(it.key()), it.value().toString());

    return settings;
}

bool Config::saveSettings(const Settings &settings)
{
    if (!ensureAppDataDir())
        return false;

    QJsonObject object;
    object[QStringLiteral("auto_organize")] = settings.autoOrganize;
    object[QStringLiteral("debounce_ms")] = settings.debounceMs;

    QJsonArray whitelist;
    for (const QString &name : settings.whitelist)
        whitelist.append(name);
    object[QStringLiteral("whitelist")] = whitelist;

    QJsonObject categoryDirs;
    for (auto it = settings.categoryDirs.constBegin();
         it != settings.categoryDirs.constEnd(); ++it) {
        categoryDirs.insert(categoryKey(it.key()), it.value());
    }
    object[QStringLiteral("category_dirs")] = categoryDirs;

    QFile file(settingsFilePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;
    return file.write(QJsonDocument(object).toJson(QJsonDocument::Indented)) > 0;
}

} // namespace doc
