#include "organizer.h"
#include "undo_log.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

using namespace doc;

class TestOrganizer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void scanSkipsDirsAndWhitelist();
    void buildPlanUsesCategories();
    void customRuleOverridesCategory();
    void relativeAndAbsoluteTargetDirs();
    void executeMovesFiles();
    void conflictAutoRename();
    void uncheckedItemsStay();
    void undoRestoresFiles();

private:
    QTemporaryDir *m_desktop = nullptr;

    QString makeFile(const QString &name, const QString &content = QStringLiteral("x"));
};

void TestOrganizer::initTestCase()
{
    m_desktop = new QTemporaryDir;
    QVERIFY(m_desktop->isValid());
}

void TestOrganizer::cleanupTestCase()
{
    delete m_desktop;
}

QString TestOrganizer::makeFile(const QString &name, const QString &content)
{
    const QString path = m_desktop->filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QTest::qFail("无法创建测试文件", __FILE__, __LINE__);
        return {};
    }
    file.write(content.toUtf8());
    return path;
}

void TestOrganizer::scanSkipsDirsAndWhitelist()
{
    makeFile(QStringLiteral("a.txt"));
    makeFile(QStringLiteral("b.jpg"));
    makeFile(QStringLiteral("keep.me"));
    QDir().mkpath(m_desktop->filePath(QStringLiteral("已有文件夹")));

    Organizer organizer(m_desktop->path());
    organizer.setWhitelist({QStringLiteral("keep.me")});

    const QStringList files = organizer.scanFiles();
    QCOMPARE(files.size(), 2);
    QVERIFY(!files.contains(m_desktop->filePath(QStringLiteral("keep.me"))));
    QVERIFY(!files.contains(m_desktop->filePath(QStringLiteral("已有文件夹"))));
}

void TestOrganizer::buildPlanUsesCategories()
{
    Organizer organizer(m_desktop->path());
    const QVector<MoveItem> plan = organizer.buildPlan({});

    QMap<QString, QString> expected; // 文件名 → 目标目录
    expected.insert(QStringLiteral("a.txt"), QStringLiteral("文档"));
    expected.insert(QStringLiteral("b.jpg"), QStringLiteral("图片"));

    int matched = 0;
    for (const MoveItem &item : plan) {
        if (!expected.contains(QFileInfo(item.sourcePath).fileName()))
            continue;
        ++matched;
        QCOMPARE(QFileInfo(item.destPath).dir().dirName(),
                 expected.value(QFileInfo(item.sourcePath).fileName()));
    }
    QCOMPARE(matched, expected.size());
}

void TestOrganizer::customRuleOverridesCategory()
{
    Organizer organizer(m_desktop->path());
    QVector<Rule> rules;
    rules.append(Rule::make(MatchType::Extension, QStringLiteral("txt"),
                            QStringLiteral("文本文档"), QStringLiteral("TXT 规则")));

    const QVector<MoveItem> plan = organizer.buildPlan(rules);
    bool found = false;
    for (const MoveItem &item : plan) {
        if (QFileInfo(item.sourcePath).fileName() == QLatin1String("a.txt")) {
            found = true;
            QCOMPARE(QFileInfo(item.destPath).dir().dirName(), QStringLiteral("文本文档"));
            QCOMPARE(item.categoryName, QStringLiteral("TXT 规则"));
        }
    }
    QVERIFY(found);
}

void TestOrganizer::relativeAndAbsoluteTargetDirs()
{
    QTemporaryDir other;
    QVERIFY(other.isValid());

    Organizer organizer(m_desktop->path());
    QVector<Rule> rules;
    rules.append(Rule::make(MatchType::Wildcard, QStringLiteral("a.*"),
                            other.filePath(QStringLiteral("绝对目录"))));

    const QVector<MoveItem> plan = organizer.buildPlan(rules);
    bool found = false;
    for (const MoveItem &item : plan) {
        if (QFileInfo(item.sourcePath).fileName() == QLatin1String("a.txt")) {
            found = true;
            QVERIFY(item.destPath.startsWith(other.filePath(QStringLiteral("绝对目录"))));
        }
    }
    QVERIFY(found);
}

void TestOrganizer::executeMovesFiles()
{
    Organizer organizer(m_desktop->path());

    const QVector<MoveItem> plan = organizer.buildPlan({});
    const OrganizeResult result = organizer.execute(plan);

    QVERIFY(result.failed == 0);
    QVERIFY(result.moved >= 2);
    QVERIFY(QFile::exists(m_desktop->filePath(QStringLiteral("文档/a.txt"))));
    QVERIFY(QFile::exists(m_desktop->filePath(QStringLiteral("图片/b.jpg"))));
}

void TestOrganizer::conflictAutoRename()
{
    // 目标目录已存在同名文件 → "name (1).ext"
    QDir().mkpath(m_desktop->filePath(QStringLiteral("文档")));
    makeFile(QStringLiteral("文档/conflict.txt"), QStringLiteral("old"));

    const QString src = makeFile(QStringLiteral("conflict.txt"), QStringLiteral("new"));

    Organizer organizer(m_desktop->path());
    MoveItem item;
    item.sourcePath = src;
    item.destPath = m_desktop->filePath(QStringLiteral("文档/conflict.txt"));
    item.categoryName = QStringLiteral("文档");

    const OrganizeResult result = organizer.execute({item});
    QCOMPARE(result.moved, 1);
    QCOMPARE(result.failed, 0);

    QVERIFY(QFile::exists(m_desktop->filePath(QStringLiteral("文档/conflict.txt"))));
    QVERIFY(QFile::exists(m_desktop->filePath(QStringLiteral("文档/conflict (1).txt"))));

    // 原文件内容保留在两个文件中
    QFile oldFile(m_desktop->filePath(QStringLiteral("文档/conflict.txt")));
    QVERIFY(oldFile.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(oldFile.readAll()), QStringLiteral("old"));
}

void TestOrganizer::uncheckedItemsStay()
{
    const QString src = makeFile(QStringLiteral("stay.txt"));

    Organizer organizer(m_desktop->path());
    MoveItem item;
    item.sourcePath = src;
    item.destPath = m_desktop->filePath(QStringLiteral("文档/stay.txt"));
    item.checked = false;

    const OrganizeResult result = organizer.execute({item});
    QCOMPARE(result.moved, 0);
    QVERIFY(QFile::exists(src));
}

void TestOrganizer::undoRestoresFiles()
{
    const QString undoPath = m_desktop->filePath(QStringLiteral("undo.json"));

    auto *undoLog = new UndoLog(undoPath);
    Organizer organizer(m_desktop->path());
    organizer.setUndoLog(undoLog);

    const QString src = makeFile(QStringLiteral("undo_me.txt"));
    MoveItem item;
    item.sourcePath = src;
    item.destPath = m_desktop->filePath(QStringLiteral("文档/undo_me.txt"));
    item.categoryName = QStringLiteral("文档");

    const OrganizeResult result = organizer.execute({item});
    QCOMPARE(result.moved, 1);
    QCOMPARE(undoLog->entryCount(), 1);
    QVERIFY(!QFile::exists(src));
    QVERIFY(QFile::exists(item.destPath));

    // 重启场景：重新 load 日志再撤销
    delete undoLog;
    auto *restoredLog = new UndoLog(undoPath);
    QVERIFY(restoredLog->load());
    QCOMPARE(restoredLog->entryCount(), 1);

    const UndoLog::UndoResult undoResult = restoredLog->undoLast();
    QCOMPARE(undoResult.undone, 1);
    QCOMPARE(undoResult.restored, 1);
    QVERIFY(undoResult.errors.isEmpty());
    QVERIFY(QFile::exists(src));
    QVERIFY(!QFile::exists(item.destPath));
    QCOMPARE(restoredLog->entryCount(), 0);

    delete restoredLog;
}

QTEST_GUILESS_MAIN(TestOrganizer)
#include "test_organizer.moc"
