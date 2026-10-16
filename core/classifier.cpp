#include "classifier.h"

#include <QFileInfo>

namespace doc {

namespace {

struct CategoryInfo {
    const char *name;        // 中文类别名
    const char *dirName;     // 默认目标目录名（桌面下）
    const char *extensions;  // 逗号分隔，不含点
};

constexpr CategoryInfo kCategories[] = {
    {"图片", "图片", "jpg,jpeg,png,gif,bmp,webp,svg,ico"},
    {"文档", "文档", "txt,md,pdf,doc,docx,xls,xlsx,ppt,pptx,csv"},
    {"视频", "视频", "mp4,mkv,avi,mov,wmv,flv"},
    {"音频", "音频", "mp3,flac,wav,m4a,ogg"},
    {"压缩包", "压缩包", "zip,rar,7z,tar,gz"},
    {"程序", "程序", "exe,msi"},
    {"其他", "其他", ""},
};

CategoryInfo infoFor(Category category)
{
    return kCategories[static_cast<int>(category)];
}

} // namespace

QString Classifier::normalizedExtension(const QString &fileName)
{
    const QString suffix = QFileInfo(fileName).suffix();
    return suffix.toLower();
}

Category Classifier::classify(const QString &fileName)
{
    const QString ext = normalizedExtension(fileName);
    if (ext.isEmpty())
        return Category::Other;

    for (int i = 0; i < static_cast<int>(Category::Other); ++i) {
        const QStringList exts = QString::fromLatin1(kCategories[i].extensions).split(QLatin1Char(','));
        if (exts.contains(ext))
            return static_cast<Category>(i);
    }
    return Category::Other;
}

QString Classifier::categoryName(Category category)
{
    return QString::fromUtf8(infoFor(category).name);
}

QString Classifier::defaultCategoryDirName(Category category)
{
    return QString::fromUtf8(infoFor(category).dirName);
}

QStringList Classifier::extensionList(Category category)
{
    const QString raw = QString::fromLatin1(infoFor(category).extensions);
    return raw.isEmpty() ? QStringList() : raw.split(QLatin1Char(','));
}

} // namespace doc
