#pragma once

#include <QString>
#include <QStringList>

namespace doc {

// 内置文件类别
enum class Category {
    Images,
    Documents,
    Videos,
    Audio,
    Archives,
    Programs,
    Other,
};

// 扩展名 → 类别，以及类别 → 默认目标目录名（相对桌面）
class Classifier {
public:
    static Category classify(const QString &fileName);
    static QString categoryName(Category category);
    static QString defaultCategoryDirName(Category category);
    static QStringList extensionList(Category category);

    // 返回小写、无点的扩展名；无扩展名返回空串
    static QString normalizedExtension(const QString &fileName);
};

} // namespace doc
