#pragma once

#include "classifier.h"
#include "rules.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace doc {

// 应用设置
struct Settings {
    bool autoOrganize = false;      // 实时监控自动整理
    int debounceMs = 2000;          // 防抖时长
    QStringList whitelist;          // 永不移动的文件名（桌面根目录文件夹天然跳过）
    QMap<Category, QString> categoryDirs; // 类别 → 目标目录名（覆盖默认值）
};

// 配置读写：rules.json / settings.json / undo_log.json，
// 统一存放于 QStandardPaths::AppDataLocation（%APPDATA%/DesktopOrganizer）
class Config {
public:
    static QString appDataDir();
    static QString rulesFilePath();
    static QString settingsFilePath();
    static QString undoLogFilePath();

    static QVector<Rule> loadRules();
    static bool saveRules(const QVector<Rule> &rules);

    static Settings loadSettings();
    static bool saveSettings(const Settings &settings);

private:
    static bool ensureAppDataDir();
};

} // namespace doc
