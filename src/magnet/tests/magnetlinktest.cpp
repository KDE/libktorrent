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
            "magnet:?xt=urn:btih:fe377e017ef52efa83251231b5b991ffae0e77ae&dn=Indie+Top+50+-+Best+of+Indie"
            "&tr=http%3A%2F%2Fdenis.stalker.h3q.com%3A6969%2Fannounce"
            "&to=http%3A%2F%2Ftorrents.thepiratebay.org%2F5156308%2FIndie_Top_50_-_Best_of_Indie.5156308.TPB.torrent");
        bt::MagnetLink mlink(data);
        QVERIFY(mlink.isValid());
        QCOMPARE(mlink.displayName(), u"Indie Top 50 - Best of Indie"_s);
        QCOMPARE(mlink.torrent(), u"http://torrents.thepiratebay.org/5156308/Indie_Top_50_-_Best_of_Indie.5156308.TPB.torrent"_s);
        QCOMPARE(mlink.trackers().constFirst(), QUrl(u"http://denis.stalker.h3q.com:6969/announce"_s));

        bt::Uint8 hash[] = {0xfe, 0x37, 0x7e, 0x01, 0x7e, 0xf5, 0x2e, 0xfa, 0x83, 0x25, 0x12, 0x31, 0xb5, 0xb9, 0x91, 0xff, 0xae, 0x0e, 0x77, 0xae};

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
