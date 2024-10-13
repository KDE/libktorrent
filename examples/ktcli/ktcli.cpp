/*
    SPDX-FileCopyrightText: 2010 Joris Guisson <joris.guisson@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ktcli.h"

#include <cstdlib>
#include <ctime>

#include <QCommandLineParser>
#include <QDir>
#include <QRandomGenerator>

#include <interfaces/serverinterface.h>
#include <peer/authenticationmonitor.h>
#include <peer/utpex.h>
#include <util/error.h>
#include <util/fileops.h>
#include <util/functions.h>
#include <util/waitjob.h>
#include <version.h>

using namespace bt;
using namespace Qt::Literals::StringLiterals;

KTCLI::KTCLI(int argc, char **argv)
    : QCoreApplication(argc, argv)
    , tc(new TorrentControl())
    , updates(0)
{
    parser.addPositionalArgument("url", tr("Torrent to open"));
    parser.addOption(QCommandLineOption("port", tr("Port to use"), "<port>", "6881"));
    parser.addOption(QCommandLineOption("tmpdir", tr("Port to use"), "<tmpdir>", QDir::tempPath()));
    parser.addOption(QCommandLineOption("encryption", tr("Whether or not to enable encryption")));
    parser.addOption(QCommandLineOption("pex", tr("Whether or not to enable peer exchange")));
    parser.addOption(QCommandLineOption("utp", tr("Whether or not to use utp")));
    parser.process(*this);

    bt::SetClientInfo("ktcli", bt::MAJOR, bt::MINOR, bt::RELEASE, bt::NORMAL, "KT");
    bt::InitLog("ktcli.log", false, true, false);
    connect(tc.get(), &TorrentInterface::finished, this, &KTCLI::finished);
    connect(this, &QCoreApplication::aboutToQuit, this, &KTCLI::shutdown);
}

KTCLI::~KTCLI()
{
}

bool KTCLI::start()
{
    bool ok = false;
    quint16 port = parser.value("port").toInt(&ok);
    if (!ok)
        port = 1024 + QRandomGenerator::global()->bounded((1 << 16) - 1 - 1024); // Use non-root ports

    if (parser.isSet("encryption")) {
        Out(SYS_GEN | LOG_NOTICE) << "Enabled encryption" << endl;
        ServerInterface::enableEncryption(false);
    }

    if (parser.isSet("pex")) {
        Out(SYS_GEN | LOG_NOTICE) << "Enabled PEX" << endl;
        UTPex::setEnabled(true);
    }

    if (parser.isSet("utp")) {
        Out(SYS_GEN | LOG_NOTICE) << "Enabled UTP" << endl;
        if (!bt::Globals::instance().initUTPServer(port)) {
            Out(SYS_GEN | LOG_NOTICE) << "Failed to listen on port " << port << endl;
            return false;
        }

        ServerInterface::setPort(port);
        ServerInterface::setUtpEnabled(true, true);
        ServerInterface::setPrimaryTransportProtocol(UTP);
    } else {
        if (!bt::Globals::instance().initTCPServer(port)) {
            Out(SYS_GEN | LOG_NOTICE) << "Failed to listen on port " << port << endl;
            return false;
        }
    }

    if (parser.positionalArguments().isEmpty())
        return false;
    return load(QUrl::fromLocalFile(parser.positionalArguments().at(0)));
}

bool KTCLI::load(const QUrl &url)
{
    QDir dir(url.toLocalFile());
    if (dir.exists() && dir.exists("torrent") && dir.exists("stats")) {
        // Load existing torrent
        if (loadFromDir(dir.absolutePath())) {
            tc->start();
            connect(&timer, &QTimer::timeout, this, &KTCLI::update);
            timer.start(250);
            return true;
        }
    } else if (url.isLocalFile()) {
        QString path = url.toLocalFile();
        if (loadFromFile(path)) {
            tc->start();
            connect(&timer, &QTimer::timeout, this, &KTCLI::update);
            timer.start(250);
            return true;
        }
    } else {
        Out(SYS_GEN | LOG_IMPORTANT) << "Non local files not supported" << endl;
    }

    return false;
}

QString KTCLI::tempDir()
{
    QDir tmpdir = QDir(parser.value("tmpdir"));
    int i = 0;
    while (tmpdir.exists(QString("tor%1").arg(i)))
        i++;

    QString sd = QString("tor%1").arg(i);
    if (!tmpdir.mkdir(sd))
        throw bt::Error(QString("Failed to create temporary directory %1").arg(sd));

    tmpdir.cd(sd);
    return tmpdir.absolutePath();
}

bool KTCLI::loadFromFile(const QString &path)
{
    try {
        tc->init(this, bt::LoadFile(path), tempDir(), QDir::currentPath());
        tc->setLoadUrl(QUrl(path));
        tc->createFiles();
        return true;
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << err.toString() << endl;
        return false;
    }
}

bool KTCLI::loadFromDir(const QString &path)
{
    try {
        tc->init(this, bt::LoadFile(path + "/torrent"_L1), path, QString());
        tc->createFiles();
        return true;
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_IMPORTANT) << err.toString() << endl;
        return false;
    }
}

bool KTCLI::notify(QObject *obj, QEvent *ev)
{
    try {
        return QCoreApplication::notify(obj, ev);
    } catch (bt::Error &err) {
        Out(SYS_GEN | LOG_DEBUG) << "Error: " << err.toString() << endl;
    } catch (std::exception &err) {
        Out(SYS_GEN | LOG_DEBUG) << "Error: " << err.what() << endl;
    }

    return true;
}

bool KTCLI::alreadyLoaded(const bt::SHA1Hash &ih) const
{
    Q_UNUSED(ih);
    return false;
}

void KTCLI::mergeAnnounceList(const bt::SHA1Hash &ih, const bt::TrackerTier *trk)
{
    Q_UNUSED(ih);
    Q_UNUSED(trk);
}

void KTCLI::update()
{
    bt::UpdateCurrentTime();
    AuthenticationMonitor::instance().update();
    tc->update();
    updates++;
    if (updates % 20 == 0) {
        Out(SYS_GEN | LOG_DEBUG) << "Speed down " << bt::BytesPerSecToString(tc->getStats().download_rate) << endl;
        Out(SYS_GEN | LOG_DEBUG) << "Speed up   " << bt::BytesPerSecToString(tc->getStats().upload_rate) << endl;
    }
}

void KTCLI::finished(bt::TorrentInterface *tor)
{
    Q_UNUSED(tor);
    Out(SYS_GEN | LOG_NOTICE) << "Torrent fully downloaded" << endl;
    QTimer::singleShot(0, this, &KTCLI::shutdown);
}

void KTCLI::shutdown()
{
    AuthenticationMonitor::instance().shutdown();

    WaitJob *j = new WaitJob(2000);
    tc->stop(j);
    if (j->needToWait())
        j->exec();
    j->deleteLater();

    Globals::instance().shutdownTCPServer();
    Globals::instance().shutdownUTPServer();
    quit();
}

#include "moc_ktcli.cpp"
