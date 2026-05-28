#ifndef WIREGUARDCONFIG_H
#define WIREGUARDCONFIG_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
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
    Q_PROPERTY(VpnStatus status READ getStatus NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ getErrorMessage WRITE setErrorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString playerName READ getPlayerName NOTIFY playerNameChanged)
    Q_PROPERTY(QString enrollmentToken READ getEnrollmentToken WRITE setEnrollmentToken NOTIFY enrollmentTokenChanged)
    Q_PROPERTY(QString managementBaseUrl READ getManagementBaseUrl WRITE setManagementBaseUrl NOTIFY managementBaseUrlChanged)

public:
    // VPN Status codes
    enum VpnStatus {
        Disconnected = 0,
        Connecting   = 1,
        Connected    = 2,
        Error        = 3,
        Registering  = 4  // NEW: performing backend handshake
    };
    Q_ENUM(VpnStatus)

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

    VpnStatus getStatus() const;
    void setStatus(VpnStatus status);

    bool getIsRegistered() const;
    void setIsRegistered(bool value);

    QString getErrorMessage() const;
    Q_INVOKABLE void setErrorMessage(const QString &value);

    Q_INVOKABLE void copyToClipboard(const QString &text);

    // Enrollment token — set once during provisioning or pre-baked into the build
    void setEnrollmentToken(const QString &token);
    QString getEnrollmentToken() const;

    // Federated Identity from Main Config
    QString getPlayerName() const;

    // Management API base URL (host of the NestJS backend, not the VPN port)
    Q_INVOKABLE void setManagementBaseUrl(const QString &url);
    Q_INVOKABLE QString getManagementBaseUrl() const;

public slots:
    void generateIdentity();
    void startVpn();
    void stopVpn();
    void setVpnError(const QString &message);

    // Zero-touch auto-registration slots
    void performHandshake();
    void resetRegistration();
    void checkOtaUpdate();

    // OTA Result Reporting — called from AndroidManager signal
    void reportOtaStatus(bool success, const QString &status, const QString &message);

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
    void playerNameChanged();
    void enrollmentTokenChanged();
    void managementBaseUrlChanged();

    // Requests to the main application (AndroidManager)
    void requestKeyGeneration();
    void requestVpnStart(QString privateKey, QString address, QString serverPubKey, QString endpoint, QString allowedIps);
    void requestVpnStop();
    void requestSystemReport();
    void requestOtaDownload(QString url, QString sha256, int versionCode);

private slots:
    void handleRegistrationResponse(QNetworkReply *reply);

    void handleReconnect();

private:
    IMainConfiguration *m_mainConfig;
    QString m_privateKey;
    QString m_publicKey;
    QString m_serverPublicKey;
    QString m_serverEndpoint;
    QString m_virtualIp;
    QString m_allowedIps;
    bool m_isEnabled;
    VpnStatus m_status;
    QString m_errorMessage;

    // Auto-registration state
    QString m_enrollmentToken;
    QString m_managementBaseUrl;
    QString m_tenantId;
    bool m_isRegistered;
    QNetworkAccessManager *m_networkManager;

    bool isConfigComplete() const;
    QTimer *m_otaTimer;
    QTimer *m_reconnectTimer;
    void handleOtaResponse(QNetworkReply *reply);
    int m_pendingOtaVersionCode = 0; // version code of the in-flight OTA update
};

#endif // WIREGUARDCONFIG_H
