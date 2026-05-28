#include "remote_management_manager.h"
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QCoreApplication>
#include "lib_facade.h"
#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif

RemoteManagementManager::RemoteManagementManager(QObject *parent) : QObject(parent)
{
    m_server = new QTcpServer(this);
}

void RemoteManagementManager::start(quint16 port)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "[RemoteMgmt] Listener started on port" << port;
    } else {
        qCritical() << "[RemoteMgmt] FAILED to start listener:" << m_server->errorString();
    }
    connect(m_server, &QTcpServer::newConnection, this, &RemoteManagementManager::onNewConnection);
}

void RemoteManagementManager::onNewConnection()
{
    QTcpSocket *socket = m_server->nextPendingConnection();
    qDebug() << "[RemoteMgmt] New connection from" << socket->peerAddress().toString();
    connect(socket, &QTcpSocket::readyRead, this, &RemoteManagementManager::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &RemoteManagementManager::onDisconnected);
}

void RemoteManagementManager::onReadyRead()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        socket->write("{\"error\": \"Invalid JSON\"}");
        return;
    }

    handleCommand(socket, doc.object());
}

void RemoteManagementManager::handleCommand(QTcpSocket *socket, const QJsonObject &command)
{
    QString type = command["type"].toString();
    qDebug() << "[RemoteMgmt] Processing command:" << type;

    QJsonObject resp;
    if (type == "PING") {
        resp["status"] = "PONG";
        sendResponse(socket, resp);
    } else if (type == "SHELL_EXEC") {
        // Support both "cmd" and "command" keys
        QString cmd = command.contains("command") ? command["command"].toString() : command["cmd"].toString();
        handleShellExec(socket, cmd);
    } else if (type == "FILE_LS") {
        handleFileLs(socket, command["path"].toString());
    } else if (type == "OTA_UPDATE") {
        handleOtaUpdate(socket, command);
    } else if (type == "SET_CONFIG") {
        handleSetConfig(socket, command["config"].toObject());
    } else if (type == "REBOOT") {
        handleReboot(socket);
    } else if (type == "RESTART_APP" || type == "APP_RESTART") {
        handleRestartApp(socket);
    } else {
        resp["error"] = "Unknown command type";
        sendResponse(socket, resp);
    }
}

void RemoteManagementManager::sendResponse(QTcpSocket *socket, const QJsonObject &response)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray data = QJsonDocument(response).toJson(QJsonDocument::Compact);
    socket->write(data);
    socket->flush();
    socket->disconnectFromHost();
    qDebug() << "[RemoteMgmt] Response sent and socket disconnecting...";
}

void RemoteManagementManager::handleShellExec(QTcpSocket *socket, const QString &cmd)
{
    qDebug() << "[RemoteMgmt] Executing shell:" << cmd;
    QProcess process;
    process.start("sh", QStringList() << "-c" << cmd);
    process.waitForFinished(10000); // Increased to 10s for heavy commands

    QJsonObject resp;
    resp["status"] = "OK";
    resp["stdout"] = QString::fromUtf8(process.readAllStandardOutput());
    resp["stderr"] = QString::fromUtf8(process.readAllStandardError());
    resp["exitCode"] = process.exitCode();
    sendResponse(socket, resp);
}

void RemoteManagementManager::handleFileLs(QTcpSocket *socket, const QString &path)
{
    QDir dir(path);
    QJsonObject resp;
    if (!dir.exists()) {
        resp["error"] = "Directory not found";
    } else {
        QJsonArray files;
        for (const QFileInfo &info : dir.entryInfoList()) {
            QJsonObject f;
            f["name"] = info.fileName();
            f["size"] = info.size();
            f["isDir"] = info.isDir();
            files.append(f);
        }
        resp["files"] = files;
    }
    sendResponse(socket, resp);
}

void RemoteManagementManager::handleSetConfig(QTcpSocket *socket, const QJsonObject &config)
{
    qDebug() << "[RemoteMgmt] Setting new config:" << config;
    
    bool changed = false;
    LibFacade *facade = qobject_cast<LibFacade*>(parent());

    if (config.contains("playlistUrl") && facade) {
        facade->reloadWithNewIndex(config["playlistUrl"].toString());
        changed = true;
    }

    if (config.contains("managementPin") && facade) {
        QString newPin = config["managementPin"].toString();
        if (!newPin.isEmpty() && facade->getConfiguration()) {
            facade->getConfiguration()->setManagementPin(newPin);
            changed = true;
        }
    }

    QJsonObject resp;
    resp["status"] = changed ? "OK" : "ERROR";
    if (!changed) resp["error"] = "Failed to update config or LibFacade missing";
    sendResponse(socket, resp);
}

void RemoteManagementManager::handleOtaUpdate(QTcpSocket *socket, const QJsonObject &command)
{
    QString url = command["url"].toString();
    if (url.isEmpty()) url = command["apkUrl"].toString(); // Support both keys
    
    QString sha = command["sha256"].toString();
    if (sha.isEmpty()) sha = command["sha256Hash"].toString(); 
    if (sha.isEmpty()) sha = command["hash"].toString(); // Support backend 'hash' key

    qInfo() << "[RemoteMgmt][OTA] Remote Push received. URL:" << url << "SHA:" << sha;

    QJsonObject resp;
    if (url.isEmpty()) {
        resp["status"] = "ERROR";
        resp["message"] = "No download URL provided";
        sendResponse(socket, resp);
        return;
    }

    // Loop Prevention: Only update if the pushed version is NEWER
    // Backend sends "version" key; support "versionCode" as alias
    int pushedVersion = command.contains("versionCode") ? command["versionCode"].toInt() : command["version"].toInt();
    int currentVersion = GlobalLibfacede->getConfiguration()->getBuildVersion().toInt();
    
    if (pushedVersion > 0 && pushedVersion <= currentVersion) {
        qInfo() << "[RemoteMgmt][OTA] Skipping update. Pushed version" << pushedVersion 
                << "is not newer than current" << currentVersion;
        resp["status"] = "SKIPPED";
        resp["message"] = "Already on version " + QString::number(currentVersion);
        sendResponse(socket, resp);
        return;
    }

    // Delegate to the robust Java-based downloader (Fixes OOM crashes)
    #if defined Q_OS_ANDROID
    // Note: Since we need to respond to the socket, we trigger the download 
    // but the actual progress will be visible on the device UI.
    // We send a success response to the management API to indicate the command was accepted.
    QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", QString("()L" + QString(ANDROID_ACTIVITY_PATH) + ";").toLocal8Bit().data());
    if (MyActivity.isValid()) {
        QAndroidJniObject jUrl = QAndroidJniObject::fromString(url);
        QAndroidJniObject jSha = QAndroidJniObject::fromString(sha);
        MyActivity.callMethod<void>("downloadAndInstall", "(Ljava/lang/String;Ljava/lang/String;I)V",
                                   jUrl.object<jstring>(),
                                   jSha.object<jstring>(),
                                   (jint)pushedVersion);
        
        resp["status"] = "OTA_STARTED";
        resp["message"] = "Download initiated on device";
        sendResponse(socket, resp);
    } else {
        resp["status"] = "ERROR";
        resp["message"] = "GarlicActivity instance not found";
        sendResponse(socket, resp);
    }
    #else
    resp["status"] = "ERROR";
    resp["message"] = "OTA push only supported on Android";
    sendResponse(socket, resp);
    #endif
}

void RemoteManagementManager::handleReboot(QTcpSocket *socket)
{
    qInfo() << "[RemoteMgmt] REBOOT command received";
    QJsonObject resp;
    resp["status"] = "OK";
    resp["message"] = "Rebooting device...";
    sendResponse(socket, resp);
    // Delay to allow the response to flush before the OS shuts down
    QTimer::singleShot(600, this, []() {
        if (GlobalLibfacede) {
            GlobalLibfacede->reboot("remote_reboot");
        }
    });
}

void RemoteManagementManager::handleRestartApp(QTcpSocket *socket)
{
    qInfo() << "[RemoteMgmt] RESTART_APP command received";
    QJsonObject resp;
    resp["status"] = "OK";
    resp["message"] = "Restarting application...";
    sendResponse(socket, resp);
    QTimer::singleShot(600, this, []() {
        QCoreApplication::quit();
    });
}

void RemoteManagementManager::onDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) socket->deleteLater();
}
