#ifndef WIREGUARDCONFIG_H
#define WIREGUARDCONFIG_H

#include <QObject>
#include <QString>
#include "tools/i_main_configuration.hpp"

class WireguardConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString privateKey READ getPrivateKey WRITE setPrivateKey NOTIFY privateKeyChanged)
    Q_PROPERTY(QString publicKey READ getPublicKey WRITE setPublicKey NOTIFY publicKeyChanged)
    Q_PROPERTY(QString serverPublicKey READ getServerPublicKey WRITE setServerPublicKey NOTIFY serverPublicKeyChanged)
    Q_PROPERTY(QString serverEndpoint READ getServerEndpoint WRITE setServerEndpoint NOTIFY serverEndpointChanged)
    Q_PROPERTY(QString virtualIp READ getVirtualIp WRITE setVirtualIp NOTIFY virtualIpChanged)
    Q_PROPERTY(QString allowedIps READ getAllowedIps WRITE setAllowedIps NOTIFY allowedIpsChanged)
    Q_PROPERTY(bool isEnabled READ getIsEnabled WRITE setIsEnabled NOTIFY isEnabledChanged)
    Q_PROPERTY(int status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ getErrorMessage WRITE setErrorMessage NOTIFY errorMessageChanged)

public:
    explicit WireguardConfig(IMainConfiguration *mainConfig, QObject *parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

    Q_INVOKABLE QString getPublicKey() const;
    Q_INVOKABLE void setPublicKey(const QString &value);

    Q_INVOKABLE QString getServerPublicKey() const;
    Q_INVOKABLE void setServerPublicKey(const QString &value);

    Q_INVOKABLE QString getServerEndpoint() const;
    Q_INVOKABLE void setServerEndpoint(const QString &value);

    Q_INVOKABLE QString getVirtualIp() const;
    Q_INVOKABLE void setVirtualIp(const QString &value);

    Q_INVOKABLE QString getAllowedIps() const;
    Q_INVOKABLE void setAllowedIps(const QString &value);

    Q_INVOKABLE bool getIsEnabled() const;
    Q_INVOKABLE void setIsEnabled(bool value);

    // Secure handling of private key (not a Q_PROPERTY)
    QString getPrivateKey() const;
    void setPrivateKey(const QString &value);

    int getStatus() const;
    void setStatus(int status);

    QString getErrorMessage() const;
    Q_INVOKABLE void setErrorMessage(const QString &value);

    Q_INVOKABLE void copyToClipboard(const QString &text);

public slots:
    void generateIdentity();
    void startVpn();
    void stopVpn();
    void setVpnError(const QString &message);

signals:
    void publicKeyChanged();
    void serverPublicKeyChanged();
    void serverEndpointChanged();
    void virtualIpChanged();
    void allowedIpsChanged();
    void isEnabledChanged();
    void statusChanged();
    void privateKeyChanged();
    void errorMessageChanged();

    // Requests to the main application (AndroidManager)
    void requestKeyGeneration();
    void requestVpnStart(QString privateKey, QString address, QString serverPubKey, QString endpoint, QString allowedIps);
    void requestVpnStop();
    void requestSystemReport();

private:
    IMainConfiguration *m_mainConfig;
    QString m_privateKey;
    QString m_publicKey;
    QString m_serverPublicKey;
    QString m_serverEndpoint;
    QString m_virtualIp;
    QString m_allowedIps;
    bool m_isEnabled;
    int m_status;
    QString m_errorMessage;
};

#endif // WIREGUARDCONFIG_H
