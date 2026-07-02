#include <QTest>

#include "snapadapter.h"

class TestSnapAdapter : public QObject {
    Q_OBJECT
private slots:
    void findParsesColumnTable()
    {
        SnapAdapter adapter;
        const auto packages = adapter.parseSearch(QStringLiteral(
            "Name      Version   Publisher   Notes  Summary\n"
            "vlc       3.0.20    videolan**  -      The ultimate media player\n"
            "spotify   1.2.31    spotify**   -      Music for everyone\n"));
        QCOMPARE(packages.size(), 2);
        QCOMPARE(packages.at(0).id, QStringLiteral("vlc"));
        QCOMPARE(packages.at(0).version, QStringLiteral("3.0.20"));
        QCOMPARE(packages.at(0).description, QStringLiteral("The ultimate media player"));
    }

    void listParsesInstalledWithHeldNotes()
    {
        SnapAdapter adapter;
        const auto packages = adapter.parseInstalled(QStringLiteral(
            "Name     Version  Rev    Tracking       Publisher   Notes\n"
            "core22   20240111 1122   latest/stable  canonical** base\n"
            "vlc      3.0.20   3777   latest/stable  videolan**  held\n"));
        QCOMPARE(packages.size(), 2);
        QVERIFY(!packages.at(0).pinned);
        QVERIFY(packages.at(1).pinned);
        QCOMPARE(packages.at(1).source, QStringLiteral("latest/stable"));
    }

    void infoParsesInstalledVersion()
    {
        SnapAdapter adapter;
        const auto packages = adapter.parseInfo(QStringLiteral(
            "name:      vlc\n"
            "summary:   The ultimate media player\n"
            "publisher: VideoLAN**\n"
            "store-url: https://snapcraft.io/vlc\n"
            "installed: 3.0.20 (3777) 320MB -\n"));
        QCOMPARE(packages.size(), 1);
        QCOMPARE(packages.first().installedVersion, QStringLiteral("3.0.20"));
        QCOMPARE(packages.first().homepage, QStringLiteral("https://snapcraft.io/vlc"));
    }

    void mutationsUsePkexecAndHoldIsPin()
    {
        SnapAdapter adapter;
        QCOMPARE(adapter.installCommand("vlc", "").program, QStringLiteral("pkexec"));
        QCOMPARE(adapter.searchCommand("vlc").program, QStringLiteral("snap"));
        QVERIFY(adapter.pinCommand("vlc", "").arguments.contains(QStringLiteral("--hold")));
        QVERIFY(adapter.unpinCommand("vlc", "").arguments.contains(QStringLiteral("--unhold")));
    }
};

QTEST_MAIN(TestSnapAdapter)
#include "tst_snapadapter.moc"
