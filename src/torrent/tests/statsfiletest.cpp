/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KSharedConfig>

#include <QObject>
#include <QTemporaryFile>
#include <QTest>

#include <ctime>
#include <torrent/statsfile.h>
#include <util/log.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

QString test_data =
    u"ASSURED_DOWNLOAD_SPEED=0\n\
ASSURED_UPLOAD_SPEED=0\n\
AUTOSTART=0\n\
CUSTOM_OUTPUT_NAME=0\n\
DHT=1\n\
DISPLAY_NAME=\n\
DOWNLOAD_LIMIT=0\n\
ENCODING=UTF-8\n\
IMPORTED=0\n\
MAX_RATIO=3.00\n\
MAX_SEED_TIME=30\n\
OUTPUTDIR=/home/joris/ktorrent/downloads/\n\
PRIORITY=4\n\
QM_CAN_START=0\n\
RESTART_DISK_PREALLOCATION=0\n\
RUNNING_TIME_DL=7042\n\
RUNNING_TIME_UL=7042\n\
TIME_ADDED=1265650102\n\
UPLOADED=0\n\
UPLOAD_LIMIT=0\n\
URL=file:///home/joris/tmp/Killers.torrent\n\
UT_PEX=1\n"_s;

class StatsFileTest : public QEventLoop
{
    Q_OBJECT
public:
private Q_SLOTS:
    void initTestCase()
    {
        bt::InitLog(u"statsfiletest.log"_s, false, false);
        QVERIFY(file.open());
        file.setAutoRemove(true);
        QTextStream out(&file);
        out << test_data;

        const QStringList lines = test_data.split('\n'_L1);
        for (const QString &line : lines) {
            QStringList sl = line.split('='_L1);
            if (sl.count() == 2) {
                keys.append(sl[0]);
                values.append(sl[1]);
            }
        }
    }

    void cleanupTestCase()
    {
    }

    void testRead()
    {
        StatsFile st(file.fileName());

        int idx = 0;
        for (const QString &key : std::as_const(keys)) {
            QVERIFY(st.hasKey(key));
            QVERIFY(st.readString(key) == values[idx++]);
        }

        QVERIFY(st.readInt(u"RUNNING_TIME_DL"_s) == 7042);
        QVERIFY(st.readInt(u"RUNNING_TIME_UL"_s) == 7042);
        QVERIFY(st.readBoolean(u"DHT"_s) == true);
    }

    void testWrite()
    {
        StatsFile sta(file.fileName());
        sta.write(u"DINGES"_s, u"1234"_s);
        sta.sync();

        StatsFile stb(file.fileName());
        QVERIFY(stb.readInt(u"DINGES"_s) == 1234);
    }

private:
    QTemporaryFile file;
    QStringList keys;
    QStringList values;
};

QTEST_MAIN(StatsFileTest)

#include "statsfiletest.moc"
