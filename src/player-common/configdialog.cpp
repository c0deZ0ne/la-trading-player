#include "configdialog.h"
#include "tools/main_configuration.hpp"
#include "lib_facade.h"
#include "vpn/wireguard_config.h"
#include <QDialog>
#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QVBoxLayout>
#include <QQmlProperty>
#include <QQmlEngine>
#include <QQuickItem>
#include <QUrl>
#include <QObject>
#include <QShowEvent>
#include <QColor>
#include <QDebug>
#include <QQmlError>
#include <QTimer>

ConfigDialog::ConfigDialog(QWidget *parent, MainConfiguration *Config, LibFacade *Lib) :  QDialog(parent)
{
    MyConfiguration = Config;
    MyLibFacade = Lib;
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    
    quickWidget = new QQuickWidget(this);
#if defined Q_OS_ANDROID
    quickWidget->setResizeMode(QQuickWidget::SizeViewToRootObject);
#else
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
#endif
    // Prevent the native white background from showing through the QML scene
    quickWidget->setClearColor(QColor("#000000"));
    quickWidget->setAttribute(Qt::WA_OpaquePaintEvent);
    
    // Set context properties BEFORE loading the source
    quickWidget->rootContext()->setContextProperty("MyConfig", MyConfiguration);
    if (MyLibFacade) {
        quickWidget->rootContext()->setContextProperty("LibFacade", MyLibFacade);
        quickWidget->rootContext()->setContextProperty("vpnConfig", MyLibFacade->getVpnConfig());
    }
    
    quickWidget->setSource(QUrl("qrc:/ConfigDialog.qml"));
    
    // Debug logging for QML load status
    if (quickWidget->status() == QQuickWidget::Error) {
        qDebug() << "ConfigDialog: QML Load Error!";
        for (const QQmlError &error : quickWidget->errors()) {
            qDebug() << "  Error:" << error.toString();
        }
    } else {
        qDebug() << "ConfigDialog: QML Load Status:" << quickWidget->status();
    }
    
    layout->addWidget(quickWidget);
    
    // Connect QML signals to C++ slots
    QObject *rootObject = quickWidget->rootObject();
    if (rootObject)
    {
        connect(rootObject, SIGNAL(accepted()), this, SLOT(onQmlAccepted()));
        connect(rootObject, SIGNAL(rejected()), this, SLOT(onQmlRejected()));
        
        // Initialize QML properties from C++
        rootObject->setProperty("playerName", MyConfiguration->getPlayerName());
        rootObject->setProperty("playlistUrl", MyConfiguration->getIndexUri());
        rootObject->setProperty("deviceId", MyConfiguration->getUuid());
        
        if (MyLibFacade) {
            connect(MyLibFacade, &LibFacade::initStarted, this, &ConfigDialog::onInitStarted);
            connect(MyLibFacade, &LibFacade::initFailed, this, &ConfigDialog::onInitFailed);
            connect(MyLibFacade, &LibFacade::readyForPlaying, this, &ConfigDialog::onReadyForPlaying);
        }
    }

#if !defined Q_OS_ANDROID
    setWindowFlags(Qt::WindowStaysOnTopHint);
    setMinimumSize(450, 600);
#else
    // Ensure full screen on Android
    showFullScreen();
#endif
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::showEvent(QShowEvent *showEvent)
{
    QDialog::showEvent(showEvent);
    activateWindow();
}

void ConfigDialog::onQmlAccepted()
{
    QObject *rootObject = quickWidget->rootObject();
    if (!rootObject) return;

    QString playerName = rootObject->property("playerName").toString();
    QString playlistUrl = rootObject->property("playlistUrl").toString();

    if (MyConfiguration->validateContentUrl(playlistUrl))
    {
        MyConfiguration->setPlayerName(playerName);
        MyConfiguration->determineIndexUri(MyConfiguration->getValidatedContentUrl());
        MyConfiguration->determineUserAgent();
        
        if (MyLibFacade) {
            MyLibFacade->initParser();
        } else {
            QDialog::accept();
        }
    }
    else
    {
        rootObject->setProperty("errorMessage", MyConfiguration->getErrorText());
        rootObject->setProperty("isConnecting", false);
    }
}

void ConfigDialog::onInitStarted()
{
    QObject *rootObject = quickWidget->rootObject();
    if (rootObject) {
        rootObject->setProperty("isConnecting", true);
        rootObject->setProperty("statusMessage", "Connecting to CMS...");
    }
}

void ConfigDialog::onInitFailed(QString reason)
{
    QObject *rootObject = quickWidget->rootObject();
    if (rootObject) {
        rootObject->setProperty("isConnecting", false);
        rootObject->setProperty("errorMessage", reason);
    }
}

void ConfigDialog::onReadyForPlaying()
{
    QObject *rootObject = quickWidget->rootObject();
    if (rootObject) {
        rootObject->setProperty("isConnecting", false);
        rootObject->setProperty("isSuccess", true);
        // Wait a bit before closing to show the success message
        QTimer::singleShot(1500, this, [this]() {
            QDialog::accept();
        });
    } else {
        QDialog::accept();
    }
}

void ConfigDialog::onQmlRejected()
{
    QDialog::reject();
}

void ConfigDialog::accept()
{
    onQmlAccepted();
}


