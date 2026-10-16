#include "classifier.h"

#include <QTest>

using namespace doc;

class TestClassifier : public QObject {
    Q_OBJECT

private slots:
    void builtinMapping();
    void caseInsensitive();
    void noExtensionAndUnknown();
    void categoryNames();
    void extensionLists();
};

void TestClassifier::builtinMapping()
{
    QCOMPARE(Classifier::classify(QStringLiteral("photo.jpg")), Category::Images);
    QCOMPARE(Classifier::classify(QStringLiteral("photo.JPEG")), Category::Images);
    QCOMPARE(Classifier::classify(QStringLiteral("archive.tar.gz")), Category::Archives); // 取最后一段
    QCOMPARE(Classifier::classify(QStringLiteral("notes.md")), Category::Documents);
    QCOMPARE(Classifier::classify(QStringLiteral("movie.mp4")), Category::Videos);
    QCOMPARE(Classifier::classify(QStringLiteral("song.flac")), Category::Audio);
    QCOMPARE(Classifier::classify(QStringLiteral("setup.exe")), Category::Programs);
    QCOMPARE(Classifier::classify(QStringLiteral("data.csv")), Category::Documents);
}

void TestClassifier::caseInsensitive()
{
    QCOMPARE(Classifier::normalizedExtension(QStringLiteral("A.PNG")), QStringLiteral("png"));
    QCOMPARE(Classifier::classify(QStringLiteral("IMG.WEBP")), Category::Images);
    QCOMPARE(Classifier::classify(QStringLiteral("Doc.DOCX")), Category::Documents);
}

void TestClassifier::noExtensionAndUnknown()
{
    QCOMPARE(Classifier::classify(QStringLiteral("README")), Category::Other);
    QCOMPARE(Classifier::classify(QStringLiteral("weird.xyz123")), Category::Other);
    QCOMPARE(Classifier::normalizedExtension(QStringLiteral("noext")), QString());
}

void TestClassifier::categoryNames()
{
    QCOMPARE(Classifier::categoryName(Category::Images), QStringLiteral("图片"));
    QCOMPARE(Classifier::categoryName(Category::Archives), QStringLiteral("压缩包"));
    QCOMPARE(Classifier::categoryName(Category::Other), QStringLiteral("其他"));
}

void TestClassifier::extensionLists()
{
    QVERIFY(Classifier::extensionList(Category::Images).contains(QStringLiteral("png")));
    QVERIFY(Classifier::extensionList(Category::Documents).contains(QStringLiteral("pdf")));
    QVERIFY(Classifier::extensionList(Category::Other).isEmpty());
}

QTEST_GUILESS_MAIN(TestClassifier)
#include "test_classifier.moc"
