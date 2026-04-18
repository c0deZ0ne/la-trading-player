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
#include <QQmlApplicationEngine>
#include "wrapper_settings.hpp"
#include <QTimer>
#include "vpn/wireguard_config.h"
#include "qdialog.h"
#include "tools/logger.h"
#include "../player-common/cmdparser.h"
#include "../player-common/screen.h"
#include "../player-common/player_configuration.h"

#if defined  Q_OS_ANDROID
    #include <android/log.h>
    #include "Java2Cpp.h"
    #include "android_manager.h"
#endif

#include <QtWebView>
#include "mainwindow.h"
#include "rest_api/httpd.h"

void handleMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Logger& MyLogger = Logger::getInstance();
    MyLogger.dispatchMessages(type, context, msg);

#if defined Q_OS_ANDROID
    android_LogPriority priority = ANDROID_LOG_DEBUG;
    switch (type) {
        case QtDebugMsg: priority = ANDROID_LOG_DEBUG; break;
        case QtInfoMsg: priority = ANDROID_LOG_INFO; break;
        case QtWarningMsg: priority = ANDROID_LOG_WARN; break;
        case QtCriticalMsg: priority = ANDROID_LOG_ERROR; break;
        case QtFatalMsg: priority = ANDROID_LOG_FATAL; break;
    }
    __android_log_print(priority, "GarlicPlayer", "%s", msg.toLocal8Bit().constData());
#endif
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // Raspberry and POT needs this http://thebugfreeblog.blogspot.de/2018/01/pot-570-with-qt-5100-built-for-armv8.html

    QApplication app(argc, argv);
    QtWebView::initialize();

// must be checked and before app create directories
#if defined Q_OS_ANDROID
    AndroidManager *MyAndroidManager = new AndroidManager();
    if (!MyAndroidManager->checkPermissiones())
    {
        MyAndroidManager->sendCloseCorrect();
        app.quit();
        return -1;
    }
    MyAndroidManager->disableScreenSaver();
#endif

    MainConfiguration *MyMainConfiguration   = new MainConfiguration(new WrapperSettings());
    MyMainConfiguration->init();
    MyMainConfiguration->createDirectories();

	qInstallMessageHandler(handleMessages); // must set after createDiretories

    PlayerConfiguration  *MyPlayerConfiguration = new PlayerConfiguration(MyMainConfiguration);

    LibFacade  *MyLibFacade = new LibFacade();

#if defined Q_OS_ANDROID
    if (MyAndroidManager->hasLauncher())
    {
        MyAndroidManager->fetchDeviceInformation();
        // needed for launcher stuff to init or fetch
        QTime dieTime= QTime::currentTime().addSecs(5);
        while (MyAndroidManager->getUUIDFromLauncher().isEmpty() && QTime::currentTime() < dieTime)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
        MyLibFacade->toggleLauncher(MyAndroidManager->hasLauncher());
        MyPlayerConfiguration->setHasLauncher(MyAndroidManager->hasLauncher());

        MyPlayerConfiguration->setUuidFromLauncher(MyAndroidManager->getUUIDFromLauncher());

        MyPlayerConfiguration->setSmilIndexUriFromLauncher(MyAndroidManager->getSmilIndexFromLauncher());

        dieTime = QTime::currentTime().addSecs(5);
        while (MyAndroidManager->getLauncherVersion().isEmpty() && QTime::currentTime() < dieTime)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
        MyPlayerConfiguration->setVersionFromLauncher(MyAndroidManager->getLauncherVersion());
    }

    setGlobalLibFaceForJava(MyLibFacade);
#endif

    // This must can only be be done after Launcher inits
    MyPlayerConfiguration->determineInitConfigValues();

    MyLibFacade->init(MyMainConfiguration);

#if defined Q_OS_ANDROID
    // Connect VPN Signals to AndroidManager
    WireguardConfig *vpnConfig = MyLibFacade->getVpnConfig();
    QObject::connect(vpnConfig, &WireguardConfig::requestKeyGeneration, [vpnConfig, MyAndroidManager]() {
        QStringList keys = MyAndroidManager->generateVpnKeyPair();
        if (keys.size() >= 2) {
            vpnConfig->setPrivateKey(keys[0]);
            vpnConfig->setPublicKey(keys[1]);
        }
    });

    QObject::connect(vpnConfig, &WireguardConfig::requestVpnStart, [MyAndroidManager](QString priv, QString addr, QString pub, QString endp, QString allowed) {
        qDebug() << "[Wireguard][CPP] main.cpp: requestVpnStart intercepted";
        MyAndroidManager->startVpnTunnel(priv, addr, pub, endp, allowed);
    });

    QObject::connect(vpnConfig, &WireguardConfig::requestVpnStop, [MyAndroidManager]() {
        MyAndroidManager->stopVpnTunnel();
    });

    QObject::connect(MyAndroidManager, &AndroidManager::vpnStatusChanged, vpnConfig, [vpnConfig](int status) {
        vpnConfig->setStatus(status);
    });

    QObject::connect(MyAndroidManager, &AndroidManager::vpnError, vpnConfig, &WireguardConfig::setVpnError);

    QObject::connect(vpnConfig, &WireguardConfig::requestSystemReport, [MyLibFacade]() {
        qDebug() << "[Wireguard][REPORT] VPN Connected. Triggering immediate system report...";
        MyLibFacade->forceSystemReport();
    });
    
    // Auto-start VPN if enabled (Non-blocking)
    if (vpnConfig->getIsEnabled()) {
        qDebug() << "[Wireguard][AUTOSTART] VPN is enabled, scheduled to start in 2s...";
        QTimer::singleShot(2000, vpnConfig, [vpnConfig]() {
            qDebug() << "[Wireguard][AUTOSTART] Triggering delayed VPN start...";
            vpnConfig->startVpn();
        });
    }
#endif

    MyPlayerConfiguration->printVersionInformation();

    qmlRegisterType<LibFacade>("com.garlic.LibFacade", 1, 0, "LibFacade");
    qmlRegisterUncreatableType<WireguardConfig>("com.garlic.vpn", 1, 0, "WireguardConfig", "Accessed via LibFacade");

    TCmdParser MyParser(MyMainConfiguration);
    MyParser.addOptions();

    if (!MyParser.parse(MyLibFacade))
        return 1;
#ifdef QT_DEBUG
    QLoggingCategory::setFilterRules("*.debug=true\nqt.*=false");
#else
    QLoggingCategory::setFilterRules("*.debug=false");
#endif

    TScreen    MyScreen(Q_NULLPTR);
    MyScreen.selectCurrentScreen(MyParser.getScreenSelect());
    // ToDo if init webserver
    QScopedPointer<RestApi::Httpd>             MyHttp;
    MyHttp.reset(new RestApi::Httpd(MyLibFacade));
    MyHttp.data()->init(&app);

    MainWindow w(&MyScreen, MyLibFacade, MyPlayerConfiguration);

    QQmlEngine::setObjectOwnership(&w, QQmlEngine::CppOwnership);

    // Show config dialog if:
    // 1. Index URI is empty OR
    // 2. No previous successful playback happened (fresh install) OR
    // 3. Index URI is invalid
    bool isFreshStart = MyMainConfiguration->getLastPlayedIndexPath().isEmpty();
    bool isInvalidUri = MyMainConfiguration->getIndexUri().isEmpty() || !MyMainConfiguration->validateContentUrl(MyMainConfiguration->getIndexUri());

    if ((isFreshStart || isInvalidUri) && w.openConfigDialog() == QDialog::Rejected)
    {
        return 0;
    }
    w.init();

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    w.showFullScreen();
#else

    w.show();

    QString val = MyParser.getWindowMode();
    if (val == "fullscreen")
        w.resizeAsNormalFullScreen();
    else if (val == "bigscreen")
        w.resizeAsBigFullScreen();
    else if (val == "windowed")
    {
        w.setMainWindowSize(MyParser.getWindowSize());
        w.resizeAsWindow();
    }
#endif

    return app.exec();
}
