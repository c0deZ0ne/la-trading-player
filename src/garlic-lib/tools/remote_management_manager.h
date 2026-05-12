#ifndef REMOTE_MANAGEMENT_MANAGER_H
#define REMOTE_MANAGEMENT_MANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QProcess>

class RemoteManagementManager : public QObject
{
    Q_OBJECT
public:
    explicit RemoteManagementManager(QObject *parent = nullptr);
    void start(quint16 port = 3006);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer *m_server;
    void handleCommand(QTcpSocket *socket, const QJsonObject &command);
    void sendResponse(QTcpSocket *socket, const QJsonObject &response);
    
    // Command Handlers
    void handleShellExec(QTcpSocket *socket, const QString &cmd);
    void handleFileLs(QTcpSocket *socket, const QString &path);
    void handleOtaUpdate(QTcpSocket *socket, const QJsonObject &command);
    void handleSetConfig(QTcpSocket *socket, const QJsonObject &config);
    void handleReboot(QTcpSocket *socket, const QString &taskId);
    void handleAppRestart(QTcpSocket *socket);
};

#endif // REMOTE_MANAGEMENT_MANAGER_H
