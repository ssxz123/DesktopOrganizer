#include "undo_log.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

using namespace doc;

class TestUndoLog : public QObject {
    Q_OBJECT

private slots:
    void loadMissingFileOk();
    void saveAndReload();
    void undoOrderIsReverse();
    void undoMultipleEntries();
    void undoWithConflictRenames();
    void undoMissingFileReportsError();
};

void TestUndoLog::loadMissingFileOk()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    UndoLog log(dir.filePath(QStringLiteral("not_exist.json")));
    QVERIFY(log.load());
    QCOMPARE(log.entryCount(), 0);
}

void TestUndoLog::saveAndReload()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("undo.json"));

    {
        UndoLog log(path);
        QVERIFY(log.load());
        log.appendEntry(QDateTime::currentDateTime(),
                        {{QStringLiteral("/a/x.txt"), QStringLiteral("/b/x.txt")}});
        QVERIFY(log.save());
        QCOMPARE(log.entryCount(), 1);
    }

    UndoLog reloaded(path);
    QVERIFY(reloaded.load());
    QCOMPARE(reloaded.entryCount(), 1);
    QCOMPARE(reloaded.entries().first().moves.size(), 1);
    QCOMPARE(reloaded.entries().first().moves.first().first, QStringLiteral("/a/x.txt"));
    QCOMPARE(reloaded.entries().first().moves.first().second, QStringLiteral("/b/x.txt"));
}

void TestUndoLog::undoOrderIsReverse()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // 造出 src/d1.txt、src/d2.txt 并移到 dst/
    QDir().mkpath(dir.filePath(QStringLiteral("src")));
    QDir().mkpath(dir.filePath(QStringLiteral("dst")));
    for (const QString &name : {QStringLiteral("d1.txt"), QStringLiteral("d2.txt")}) {
        QFile file(dir.filePath(QStringLiteral("src/") + name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data");
        file.close();
        QVERIFY(QFile::rename(dir.filePath(QStringLiteral("src/") + name),
                              dir.filePath(QStringLiteral("dst/") + name)));
    }

    UndoLog log(dir.filePath(QStringLiteral("undo.json")));
    log.appendEntry(QDateTime::currentDateTime(),
                    {{dir.filePath(QStringLiteral("src/d1.txt")), dir.filePath(QStringLiteral("dst/d1.txt"))},
                     {dir.filePath(QStringLiteral("src/d2.txt")), dir.filePath(QStringLiteral("dst/d2.txt"))}});

    const UndoLog::UndoResult result = log.undoLast();
    QCOMPARE(result.undone, 1);
    QCOMPARE(result.restored, 2);
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("src/d1.txt"))));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("src/d2.txt"))));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("dst/d1.txt"))));
    QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("dst/d2.txt"))));
}

void TestUndoLog::undoMultipleEntries()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir().mkpath(dir.filePath(QStringLiteral("src")));
    QDir().mkpath(dir.filePath(QStringLiteral("dst")));

    UndoLog log(dir.filePath(QStringLiteral("undo.json")));

    // 两条日志：f1 移动两次（src→dst→src），依次撤销应回到最初状态
    const QString f1 = dir.filePath(QStringLiteral("src/f1.txt"));
    const QString mid = dir.filePath(QStringLiteral("dst/f1.txt"));

    for (const QString &path : {f1, mid}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    log.appendEntry(QDateTime::currentDateTime(), {{f1, mid}});
    QFile::rename(f1, mid);
    log.appendEntry(QDateTime::currentDateTime(), {{mid, f1}});
    QFile::rename(mid, f1);
    QCOMPARE(log.entryCount(), 2);

    // 先撤第二条（mid→f1 的反向），再撤第一条
    const UndoLog::UndoResult result = log.undoLast(2);
    QCOMPARE(result.undone, 2);
    QCOMPARE(result.restored, 2);
    QVERIFY(QFile::exists(f1));
    QVERIFY(!QFile::exists(mid));
    QCOMPARE(log.entryCount(), 0);
}

void TestUndoLog::undoWithConflictRenames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir().mkpath(dir.filePath(QStringLiteral("src")));
    QDir().mkpath(dir.filePath(QStringLiteral("dst")));

    const QString src = dir.filePath(QStringLiteral("src/c.txt"));
    const QString dst = dir.filePath(QStringLiteral("dst/c.txt"));

    // 原位置被新文件占用 → 撤销时自动改名 "c (1).txt"
    for (const QString &path : {src, dst}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("data");
        file.close();
    }

    UndoLog log(dir.filePath(QStringLiteral("undo.json")));
    log.appendEntry(QDateTime::currentDateTime(), {{src, dst}});

    const UndoLog::UndoResult result = log.undoLast();
    QCOMPARE(result.restored, 1);
    QVERIFY(QFile::exists(src));
    QVERIFY(QFile::exists(dir.filePath(QStringLiteral("src/c (1).txt"))));
    QVERIFY(!QFile::exists(dst));
}

void TestUndoLog::undoMissingFileReportsError()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    UndoLog log(dir.filePath(QStringLiteral("undo.json")));
    log.appendEntry(QDateTime::currentDateTime(),
                    {{QStringLiteral("/no/such/file.txt"), QStringLiteral("/no/such/dst.txt")}});

    const UndoLog::UndoResult result = log.undoLast();
    QCOMPARE(result.restored, 0);
    QVERIFY(!result.errors.isEmpty());
    QCOMPARE(log.entryCount(), 0); // 条目仍被移除，避免卡死后续撤销
}

QTEST_GUILESS_MAIN(TestUndoLog)
#include "test_undo_log.moc"
