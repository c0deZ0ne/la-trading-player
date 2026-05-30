/*************************************************************************************
    garlic-player: SMIL Player for Digital Signage
    Copyright (C) 2016 Nikolaos Saghiadinos <ns@smil-control.com>
    This file is part of the garlic-player source code

    This program is free software: you can redistribute it and/or  modify
    it under the terms of the GNU Affero General Public License, version 3,
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*************************************************************************************/
#include "lib_facade.h"
#include "tools/main_configuration.hpp"
#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <QNetworkProxyQuery>
#include <QUrl>
#include <QDebug>

LibFacade *GlobalLibfacede = nullptr;

#include "logger.h"
#include "../player-c2qml/Java2Cpp.h"

LibFacade::LibFacade(QObject *parent) : QObject(parent)
{
    MyDiscSpace.reset(new SystemInfos::DiscSpace(&MyStorage));
    MyResourceMonitor.setDiscSpace(MyDiscSpace.data());
    MyFreeDiscSpace.reset(new FreeDiscSpace(MyDiscSpace.data()));
    Logger::getInstance().setResourceMonitor(&MyResourceMonitor);

    QNetworkProxyFactory::setUseSystemConfiguration(true);
    QNetworkProxyQuery npq(QUrl(QLatin1String("http://www.google.com")));
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    foreach (QNetworkProxy p, listOfProxies)
       qDebug() << "hostname" << p.hostName();

    connect(&RebootTimer, SIGNAL(reboot(QString)), this, SLOT(reboot(QString)));
    m_downloadProgress = 0.0;
    m_downloadLabel = "";
}

LibFacade::~LibFacade()
{
    killTimer(resource_monitor_timer_id);
}

/**
 * This is needed because qmlRegisterType in player-c2qml did not work
 * with constructor params except QObject
 * So we need to make the dependency injections here
 *
 * @brief LibFacade::init
 * @param config
 */
void LibFacade::init(MainConfiguration *config)
{    
    GlobalLibfacede = this;
    MyConfiguration.reset(config);
    MyInventoryTable.reset(new DB::InventoryTable(this));
    MyInventoryTable.data()->init(MyConfiguration.data()->getPaths("logs"));
    MyFreeDiscSpace.data()->init(MyConfiguration.data()->getPaths("cache"));
    MyFreeDiscSpace.data()->setInventoryTable(MyInventoryTable.data());

#if defined Q_OS_ANDROID
    setGlobalLibFaceForJava(this);
#endif

    MyDiscSpace.data()->init(MyConfiguration.data()->getPaths("cache"));
    MyIndexManager.reset(new Files::IndexManager(MyInventoryTable.data(), MyConfiguration.data(), MyFreeDiscSpace.data(), this));
    connect(MyIndexManager.data(), SIGNAL(readyForLoading()), this, SLOT(loadIndex()));
    connect(MyIndexManager.data(), &Files::IndexManager::downloadFailed, this, &LibFacade::initFailed);

    MyTaskScheduler.reset(new SmilHead::TaskScheduler(MyInventoryTable.data(), MyConfiguration.data(), MyFreeDiscSpace.data(), this));
    connect(MyTaskScheduler.data(), SIGNAL(applyConfiguration()), this, SLOT(changeConfig()));
    connect(MyTaskScheduler.data(), SIGNAL(installSoftware(QString)), this, SLOT(emitInstallSoftware(QString)));
    connect(MyTaskScheduler.data(), SIGNAL(reboot(QString)), this, SLOT(reboot(QString)));
    connect(MyTaskScheduler.data(), SIGNAL(applyCommand(QString,QString)), this, SLOT(applyCommand(QString,QString)));
    
    MyVpnConfiguration.reset(new WireguardConfig(MyConfiguration.data(), this));
    MyVpnConfiguration.data()->load();

    connect(MyConfiguration.data(), SIGNAL(managementPinChanged()), this, SIGNAL(managementPinChanged()));
}

ResourceMonitor *LibFacade::getResourceMonitor()
{
    return &MyResourceMonitor;
}

QString LibFacade::appVersion() const
{
    QString version = MyConfiguration->getVersion();
    QString buildCode = MyConfiguration->getBuildVersion();
    if (!buildCode.isEmpty()) {
        return QString("%1 (%2)").arg(version).arg(buildCode);
    }
    return version;
}

QString LibFacade::managementPin() const
{
    if (MyConfiguration.isNull()) return "0000";
    return MyConfiguration->getManagementPin();
}

void LibFacade::saveVpnConfig()
{
    if (!MyVpnConfiguration.isNull()) {
        MyVpnConfiguration.data()->save();
    }
}

void LibFacade::shutDownParsing()
{
    MyBodyParser.data()->endPlayingBody();
    MyIndexManager.data()->deactivateRefresh();
}

void LibFacade::initParser()
{
    emit initStarted();
    MyIndexManager.data()->init(MyConfiguration.data()->getIndexUri());
    MyIndexManager.data()->lookUpForUpdatedIndex();

    // load index from cache, because if remote check fails
    // or has a long timeout player will be show a white screen
    loadIndex();
}

void LibFacade::initParserWithTemporaryFile(QString uri)
{
    MyIndexManager.data()->init(uri);
    MyIndexManager.data()->lookUpForUpdatedIndex();
   // loadIndex();
}

void LibFacade::setConfigFromExternal(QString config_path, bool restart_smil_parsing)
{
    MyXMLConfiguration.reset(new SmilHead::XMLConfiguration(MyInventoryTable.data(), MyConfiguration.data(), MyFreeDiscSpace.data(), this));
    if (restart_smil_parsing)
        connect(MyXMLConfiguration.data(), SIGNAL(finishedConfiguration()), this, SLOT(reboot()));
    MyXMLConfiguration.data()->processFromLocalFile(config_path);
}

void LibFacade::transferNotify(QString key)
{
    if (MyElementsContainer.isNull())
        return;

    MyBodyParser.data()->triggerNotify(key);
}

void LibFacade::transferAccessKey(QChar key)
{
    if (MyElementsContainer.isNull())
        return;

    MyBodyParser.data()->triggerAccessKey(key);
}

/**
 * When new index/content_url came from an external source like a launcher at runtime
 *
 * @brief LibFacade::reloadWithNewIndex
 * @param index_path
 */
void LibFacade::reloadWithNewIndex(QString index_path)
{
    MyConfiguration->determineIndexUri(index_path);
    initParser();
}

void LibFacade::beginSmilPlaying()
{
    MyBodyParser.data()->startPresentationAfterPreload();
}

QString LibFacade::requestLoaddableMediaPath(QString path)
{
    return MyMediaManager.data()->requestLoadablePath(path);
}

void LibFacade::loadIndex()
{
    // validate
    MyIndexManager.data()->init(MyConfiguration.data()->getIndexUri());
    if (!MyBodyParser.isNull())
    {
        MyBodyParser.data()->endPlayingBody();
        MyIndexManager.data()->deactivateRefresh();
    }

    // Start with this only when it is absolutly sure that in the player component is no activity anymore.
    if (!MyIndexManager.data()->load())
    {
        // Why we are here?
        // index in cache? => wait
        // index is corrupt and there is no ongoing download process for a new index? =>Y reboot
        if (MyIndexManager.data()->getError() == Files::IndexManager::INDEX_CORRUPT && !MyIndexManager.data()->isIndexInDownload())
            reboot("index_broken");

        return;
    }

    MyPlaceHolder.reset(new SmilHead::PlaceHolder(this)); // must init before Filemanager

    initFileManager();
    processHeadParsing();
    configureRebootTimer();
}

void LibFacade::changeConfig()
{
    loadIndex();
}

void LibFacade::initFileManager()
{
    MyMediaModel.reset(new MediaModel(MyFreeDiscSpace.data(), this));
    MyDownloadQueue.reset(new DownloadQueue(MyConfiguration.data(), MyFreeDiscSpace.data(), MyInventoryTable.data(), this));
    connect(MyDownloadQueue.data(), &DownloadQueue::downloadProgress, this, &LibFacade::handleMediaDownloadProgress);
    MyMediaManager.reset(new Files::MediaManager(MyMediaModel.data(), MyDownloadQueue.data(), MyConfiguration.data(), MyFreeDiscSpace.data(), this));
    qDebug() <<  " end initFileManager" ;
}

void LibFacade::processHeadParsing()
{
    MyHeadParser.reset(new HeadParser(MyConfiguration.data(), MyMediaManager.data(), MyInventoryTable.data(), MyPlaceHolder.data(), MyDiscSpace.data(), &MyResourceMonitor, this));
    connect(MyHeadParser.data(), SIGNAL(parsingCompleted()), this, SLOT(processBodyParsing()));

    qDebug() <<  " begin head parsing" ;
    MyHeadParser.data()->parse(MyIndexManager->getHead(), MyTaskScheduler.data());
    qDebug() <<  " end head parsing" ;
}

void LibFacade::processBodyParsing()
{
    MyExpr.reset(new Expr);
    MyElementsContainer.reset(new ElementsContainer(this)); // must be setted, when Layout is known
    MyElementFactory.reset(new ElementFactory(MyMediaManager.data(), MyConfiguration.data(), MyPlaceHolder.data(), MyExpr.data()));

    MyBodyParser.reset(new BodyParser(MyElementFactory.data(), MyMediaManager.data(), MyElementsContainer.data(), this));

    connect(MyBodyParser.data(), SIGNAL(startShowMedia(BaseMedia*)), this, SLOT(emitStartShowMedia(BaseMedia*)));
    connect(MyBodyParser.data(), SIGNAL(stopShowMedia(BaseMedia*)), this, SLOT(emitStopShowMedia(BaseMedia*)));
    connect(MyBodyParser.data(), SIGNAL(resumeShowMedia(BaseMedia*)), this, SLOT(emitResumeShowMedia(BaseMedia*)));
    connect(MyBodyParser.data(), SIGNAL(pauseShowMedia(BaseMedia*)), this, SLOT(emitPauseShowMedia(BaseMedia*)));

    qDebug() <<  " begin preloading" ;
    MySmil.reset(new Smil(this));
    MySmil.data()->preloadParse(MyIndexManager->getSmil());

    MyBodyParser->beginPreloading(MyExpr.data(), MySmil.data(), MyIndexManager->getBody());
    qDebug() <<  " end preloading" ;

    MyIndexManager.data()->activateRefresh(MyHeadParser->getRefreshTime());
    emit readyForPlaying();
}

void LibFacade::configureRebootTimer()
{
    RebootScheduler.reset(new Scheduler(MyConfiguration.data(), &MyWeekDayConverter));
    RebootScheduler.data()->determineNextReboot(QDateTime::currentDateTime());
    RebootTimer.stopTimer();
    if (RebootScheduler.data()->getNextDatetime().isValid())
    {
        RebootTimer.setRebootTime(RebootScheduler.data()->getNextDatetimeInMSecs());
    }
}

void LibFacade::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
    MyResourceMonitor.refresh();

    qInfo(Develop) << MyResourceMonitor.getTotalDiscSpace() << MyResourceMonitor.getFreeDiscSpace();
    qInfo(Develop) << MyResourceMonitor.getTotalMemorySystem() << MyResourceMonitor.getFreeMemorySystem();
    qInfo(Develop) << MyResourceMonitor.getMemoryAppUse() << MyResourceMonitor.getMaxMemoryAppUsed();
    qInfo(Develop) << MyResourceMonitor.getThreadsNumber() << MyResourceMonitor.getMaxThreadsNumber();
}

void LibFacade::emitInstallSoftware(QString file_path)
{
    emit installSoftware(file_path);
}

void LibFacade::reboot(QString task_id)
{
    emit rebootOS(task_id);
}

void LibFacade::applyCommand(QString task_id, QString command)
{
    Q_UNUSED(task_id);
    if (command == "clear_all_caches")
    {
        MyFreeDiscSpace->clearPlayerCache();
        MyFreeDiscSpace->clearWebCache();
    }
    else if (command == "clear_playercache" || command == "clear_cache")
    {
        MyFreeDiscSpace->clearPlayerCache();
    }
    else if (command == "clear_webcache")
    {
        MyFreeDiscSpace->clearWebCache();
    }
    reboot(task_id); // otherwise a black screen
}

void LibFacade::takeScreenshot(QString file_path)
{
    emit screenshot(file_path);
}

void LibFacade::emitStartShowMedia(BaseMedia *media)
{
    emit startShowMedia(media);
    qDebug() << "emitStartShowMedia " << media->getID();
}

void LibFacade::emitStopShowMedia(BaseMedia *media)
{
    emit stopShowMedia(media);
    qDebug() << "emitStopShowMedia " << media->getID();
}

void LibFacade::emitResumeShowMedia(BaseMedia *media)
{
    emit resumeShowMedia(media);
    qDebug() << "emitResumeShowMedia " << media->getID();
}

void LibFacade::emitPauseShowMedia(BaseMedia *media)
{
    emit pauseShowMedia(media);
    qDebug() << "emitPauseShowMedia " << media->getID();
}

void LibFacade::forceSystemReport()
{
    if (!MyHeadParser.isNull())
    {
        MyHeadParser.data()->forceSystemReport();
    }
}

void LibFacade::notifyOtaProgress(qint64 received, qint64 total)
{
    m_otaReceived = received;
    m_otaTotal = total;
    
    if (received >= total && total > 0) {
        m_otaReceived = 0;
        m_otaTotal = 0;
    }
    
    updateDownloadStatus();
}

void LibFacade::handleMediaDownloadProgress(QString src, qint64 received, qint64 total)
{
    Q_UNUSED(src);
    Q_UNUSED(received);
    Q_UNUSED(total);
    // DownloadQueue will be updated to emit aggregate progress later
    updateDownloadStatus();
}

void LibFacade::updateDownloadStatus()
{
    bool wasDownloading = m_isDownloading;
    double oldProgress = m_downloadProgress;
    QString oldLabel = m_downloadLabel;

    // OTA takes precedence
    if (m_otaTotal > 0 || m_otaReceived > 0) {
        m_isDownloading = true;
        double receivedMB = m_otaReceived / (1024.0 * 1024.0);

        if (m_otaTotal > 0) {
            // Known file size — show exact progress: "6.15 / 44.50 MB (14%)"
            m_downloadProgress = (double)m_otaReceived / m_otaTotal;
            double totalMB    = m_otaTotal / (1024.0 * 1024.0);
            int pct           = (int)(m_downloadProgress * 100);
            m_downloadLabel = QString("Updating... %1 / %2 MB  (%3%)")
                    .arg(QString::number(receivedMB, 'f', 2))
                    .arg(QString::number(totalMB,    'f', 2))
                    .arg(pct);
        } else {
            // Chunked / unknown size — QML scanner bar handles the animation,
            // progress = 0.0 keeps the determinate bar invisible.
            m_downloadProgress = 0.0;
            m_downloadLabel = QString("Updating... %1 MB downloaded")
                    .arg(QString::number(receivedMB, 'f', 2));
        }

        static qint64 lastLog = 0;
        if (m_otaReceived - lastLog > 512 * 1024) { // Log every 512KB
             qDebug() << "[LibFacade][OTA] Progress:" << m_otaReceived << "/" << m_otaTotal;
             lastLog = m_otaReceived;
        }
    } else {
        // Fallback to media queue or idle
        m_isDownloading = false;
        m_downloadProgress = 0;
        m_downloadLabel = "";
    }

    if (wasDownloading != m_isDownloading || oldProgress != m_downloadProgress || oldLabel != m_downloadLabel) {
        emit downloadStatusChanged();
    }
}
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

void LibFacade::enrollDevice(QString token, QString playlistUrl)
{
    if (token.isEmpty()) {
        emit initFailed("Enrollment Token is required");
        return;
    }

    if (MyConfiguration->getStaticHardwareId() == "DEVICE_OWNER_REQUIRED") {
        emit initFailed("Security Error: Device Owner status required. Please run ADB setup.");
        return;
    }

    emit initStarted();
    qDebug() << "Enrolling device with token:" << token << "and URL:" << playlistUrl;

    // Persist the URL immediately — don't wait for enrollment success.
    // This ensures the user's intent survives even if the network request fails.
    if (!playlistUrl.isEmpty()) {
        MyConfiguration->setIndexUri(playlistUrl);
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    // Use the same management base URL as the VPN config — single source of truth.
    // Do NOT hardcode the server IP here; always derive it from MyVpnConfiguration.
    QString baseUrl = MyVpnConfiguration.isNull()
                          ? QStringLiteral("https://api.la-trading-cms.co.uk")
                          : MyVpnConfiguration->getManagementBaseUrl();
    QUrl url(baseUrl + "/api/v1/devices/vpn-register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json = MyConfiguration->getSystemMetadata();
    json["enrollmentToken"] = token;
    
    // Override playlistUrl if a specific one was provided in the UI
    if (!playlistUrl.isEmpty()) {
        json["playlistUrl"] = playlistUrl;
    }
    
    // Include Wireguard Public Key if available
    if (!MyVpnConfiguration.isNull()) {
        json["publicKey"] = MyVpnConfiguration->getPublicKey();
    }

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = manager->post(request, data);

    connect(reply, &QNetworkReply::finished, [this, reply, manager, token, playlistUrl]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonObject obj = doc.object();

            // Support both direct and enveloped responses (NestJS GlobalInterceptor)
            while (obj.contains("data") && obj.value("data").isObject()) {
                obj = obj.value("data").toObject();
            }

            qDebug() << "Enrollment successful!";
            
            // 1. Save Provisioned Config
            MyConfiguration->setPlayerName(obj.value("deviceName").toString(MyConfiguration->getPlayerName()));
            // Prefer the server-assigned playlist URL; fall back to what the user typed
            QString serverPlaylistUrl = obj.value("playlistUrl").toString();
            MyConfiguration->setIndexUri(!serverPlaylistUrl.isEmpty() ? serverPlaylistUrl : playlistUrl);
            
            // Save management PIN if server sent one
            if (obj.contains("managementPin")) {
                MyConfiguration->setManagementPin(obj.value("managementPin").toString("0000"));
            }

            // 2. Setup VPN if returned
            if (!MyVpnConfiguration.isNull()) {
                MyVpnConfiguration->setEnrollmentToken(token);
                
                // Response is flattened (matches WireguardConfig::handleRegistrationResponse)
                QString clientIp  = obj.value("clientIp").toString();
                QString serverKey = obj.value("serverPublicKey").toString();
                QString endpoint  = obj.value("serverEndpoint").toString();

                if (!clientIp.isEmpty()) {
                    MyVpnConfiguration->setVirtualIp(clientIp);
                    if (!serverKey.isEmpty()) MyVpnConfiguration->setServerPublicKey(serverKey);
                    if (!endpoint.isEmpty())  MyVpnConfiguration->setServerEndpoint(endpoint);
                    
                    MyVpnConfiguration->setIsEnabled(true);
                    MyVpnConfiguration->setIsRegistered(true);
                }
                MyVpnConfiguration->save();
            }

            emit readyForPlaying();
        } else {
            QString error = reply->errorString();
            qDebug() << "Enrollment failed:" << error;
            emit initFailed("Registration Failed: " + error);
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}
