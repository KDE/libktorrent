#include <QObject>
#include <QTest>

#include <magnet/magnetlink.h>
#include <util/log.h>

using namespace Qt::Literals::StringLiterals;

class MagnetLinkTest : public QObject
{
    Q_OBJECT

public:
    MagnetLinkTest()
    {
    }
    ~MagnetLinkTest() override
    {
    }

private Q_SLOTS:
    void init()
    {
    }

    void testParsing()
    {
        QString data = QStringLiteral(
            "magnet:?xt=urn:btih:dd8255ecdc7ca55fb0bbf81323d87062db1f6d1c&dn=Big+Buck+Bunny&tr=udp%3A%2F%2Fexplodie.org%3A6969&tr=udp%3A%2F%2Ftracker."
            "coppersurfer.tk%3A6969&tr=udp%3A%2F%2Ftracker.empire-js.us%3A1337&tr=udp%3A%2F%2Ftracker.leechers-paradise.org%3A6969&tr=udp%3A%2F%2Ftracker."
            "opentrackr.org%3A1337&tr=wss%3A%2F%2Ftracker.btorrent.xyz&tr=wss%3A%2F%2Ftracker.fastcast.nz&tr=wss%3A%2F%2Ftracker.openwebtorrent.com&ws=https%"
            "3A%2F%2Fwebtorrent.io%2Ftorrents%2F&xs=https%3A%2F%2Fwebtorrent.io%2Ftorrents%2Fbig-buck-bunny.torrent");
        bt::MagnetLink mlink(data);
        QVERIFY(mlink.isValid());
        QCOMPARE(mlink.displayName(), u"Big Buck Bunny"_s);
        QCOMPARE(mlink.torrent(), u"https://webtorrent.io/torrents/big-buck-bunny.torrent"_s);
        QCOMPARE(mlink.trackers().size(), 8);
        QCOMPARE(mlink.trackers().constFirst(), QUrl(u"udp://explodie.org:6969"_s));

        bt::Uint8 hash[] = {0xdd, 0x82, 0x55, 0xec, 0xdc, 0x7c, 0xa5, 0x5f, 0xb0, 0xbb, 0xf8, 0x13, 0x23, 0xd8, 0x70, 0x62, 0xdb, 0x1f, 0x6d, 0x1c};

        QCOMPARE(mlink.infoHash(), bt::SHA1Hash(hash));
    }

    void testInvalidUrl()
    {
        QStringList invalid;
        invalid << u"dinges:"_s;
        invalid << u"magnet:?xt=dinges"_s;
        invalid << u"magnet:?xt=urn:btih:fe377e017ef52ef"_s;
        invalid << u"magnet:?xt=urn:btih:fe377e017ef52efa83251231b5b991ffae0e77--"_s;

        for (const QString &data : std::as_const(invalid)) {
            bt::MagnetLink mlink(data);
            QVERIFY(!mlink.isValid());
        }
    }
};

QTEST_MAIN(MagnetLinkTest)

#include "magnetlinktest.moc"
