#include "rules.h"

#include <QSignalSpy>
#include <QTest>

using namespace doc;

class TestRules : public QObject {
    Q_OBJECT

private slots:
    void extensionMatch();
    void wildcardMatch();
    void olderThanDays();
    void disabledRuleSkips();
    void firstMatchWins();
    void noMatchFallsThrough();
    void jsonRoundTrip();
};

static QDateTime daysAgo(int days)
{
    return QDateTime::currentDateTime().addDays(-days);
}

void TestRules::extensionMatch()
{
    Rule rule = Rule::make(MatchType::Extension, QStringLiteral("log,tmp"),
                           QStringLiteral("垃圾文件"), QStringLiteral("日志"));
    QVERIFY(rule.matches(QStringLiteral("a.log"), daysAgo(0)));
    QVERIFY(rule.matches(QStringLiteral("b.TMP"), daysAgo(0)));
    QVERIFY(!rule.matches(QStringLiteral("c.txt"), daysAgo(0)));
    QCOMPARE(rule.matchTypeString(), QStringLiteral("扩展名"));
}

void TestRules::wildcardMatch()
{
    Rule rule = Rule::make(MatchType::Wildcard, QStringLiteral("report?.txt"),
                           QStringLiteral("报表"));
    QVERIFY(rule.matches(QStringLiteral("report1.txt"), daysAgo(0)));
    QVERIFY(rule.matches(QStringLiteral("REPORTA.TXT"), daysAgo(0))); // 大小写不敏感
    QVERIFY(!rule.matches(QStringLiteral("report.txt"), daysAgo(0)));
    QVERIFY(!rule.matches(QStringLiteral("report12.txt"), daysAgo(0)));

    Rule allLog = Rule::make(MatchType::Wildcard, QStringLiteral("*.log"),
                             QStringLiteral("日志目录"));
    QVERIFY(allLog.matches(QStringLiteral("anything.log"), daysAgo(0)));
    QVERIFY(!allLog.matches(QStringLiteral("anything.txt"), daysAgo(0)));
}

void TestRules::olderThanDays()
{
    Rule rule = Rule::make(MatchType::OlderThanDays, QStringLiteral("30"),
                           QStringLiteral("旧文件"));
    QVERIFY(rule.matches(QStringLiteral("a.txt"), daysAgo(31)));
    QVERIFY(rule.matches(QStringLiteral("a.txt"), daysAgo(30)));
    QVERIFY(!rule.matches(QStringLiteral("a.txt"), daysAgo(29)));
    QVERIFY(!rule.matches(QStringLiteral("a.txt"), daysAgo(0)));

    Rule invalid = Rule::make(MatchType::OlderThanDays, QStringLiteral("abc"),
                              QStringLiteral("x"));
    QVERIFY(!invalid.matches(QStringLiteral("a.txt"), daysAgo(100)));
}

void TestRules::disabledRuleSkips()
{
    Rule rule = Rule::make(MatchType::Extension, QStringLiteral("txt"),
                           QStringLiteral("别处"));
    rule.enabled = false;
    QVERIFY(!rule.matches(QStringLiteral("a.txt"), daysAgo(0)));
}

void TestRules::firstMatchWins()
{
    QVector<Rule> rules;
    rules.append(Rule::make(MatchType::Extension, QStringLiteral("txt"),
                            QStringLiteral("规则1"), QStringLiteral("规则1")));
    rules.append(Rule::make(MatchType::Wildcard, QStringLiteral("*.txt"),
                            QStringLiteral("规则2"), QStringLiteral("规则2")));

    const Destination destination = matchRules(rules, QStringLiteral("a.txt"), daysAgo(0));
    QVERIFY(destination.matched);
    QCOMPARE(destination.targetDir, QStringLiteral("规则1"));
    QCOMPARE(destination.viaRuleName, QStringLiteral("规则1"));
}

void TestRules::noMatchFallsThrough()
{
    QVector<Rule> rules;
    rules.append(Rule::make(MatchType::Extension, QStringLiteral("log"),
                            QStringLiteral("日志")));
    const Destination destination = matchRules(rules, QStringLiteral("a.jpg"), daysAgo(0));
    QVERIFY(!destination.matched);
    QVERIFY(destination.targetDir.isEmpty());
}

void TestRules::jsonRoundTrip()
{
    Rule rule = Rule::make(MatchType::Wildcard, QStringLiteral("*.md"),
                           QStringLiteral("笔记"), QStringLiteral(" Markdown 文档"));
    rule.enabled = false;

    const Rule restored = ruleFromJson(ruleToJson(rule));
    QCOMPARE(restored.id, rule.id);
    QCOMPARE(restored.name, rule.name);
    QCOMPARE(restored.matchType, rule.matchType);
    QCOMPARE(restored.pattern, rule.pattern);
    QCOMPARE(restored.targetDir, rule.targetDir);
    QCOMPARE(restored.enabled, rule.enabled);
}

QTEST_GUILESS_MAIN(TestRules)
#include "test_rules.moc"
