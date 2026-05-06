#include "wireguard_config.h"
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include "../version.h"

// ─── Default management API base (port 3000 = NestJS backend) ────────────────
// Override at runtime via setManagementBaseUrl() if your deployment differs.
static const QString DEFAULT_MANAGEMENT_URL = QStringLiteral("http://178.128.46.45:3000");

// ─── Default enrollment token ─────────────────────────────────────────────────
// This matches the token stored in the backend for the initial fleet tenant.
// Devices that have already registered will skip the handshake automatically.
static const QString DEFAULT_ENROLLMENT_TOKEN = QStringLiteral("Enter your enrollment token");

WireguardConfig::WireguardConfig(IMainConfiguration *mainConfig, QObject *parent)
    : QObject(parent)
    , m_mainConfig(mainConfig)
    , m_publicKey("")
    , m_serverPublicKey("")  // Fetched dynamically via handshake
    , m_serverEndpoint("178.128.46.45:51820")
    , m_virtualIp("")
    , m_allowedIps("0.0.0.0/0")
    , m_isEnabled(false)
    , m_status(Disconnected)
    , m_errorMessage("")
    , m_enrollmentToken(DEFAULT_ENROLLMENT_TOKEN)
    , m_managementBaseUrl(DEFAULT_MANAGEMENT_URL)
    , m_isRegistered(false)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_otaTimer(nullptr)
    , m_reconnectTimer(new QTimer(this))
{
    m_reconnectTimer->setInterval(15000); // 15s watchdog
    connect(m_reconnectTimer, &QTimer::timeout, this, &WireguardConfig::handleReconnect);
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void WireguardConfig::load()
{
    if (!m_mainConfig) return;

    m_privateKey      = m_mainConfig->getUserConfigByKey("vpn_private_key");
    m_publicKey       = m_mainConfig->getUserConfigByKey("vpn_public_key");
    m_serverPublicKey = m_mainConfig->getUserConfigByKey("vpn_server_public_key");

    m_serverEndpoint = m_mainConfig->getUserConfigByKey("vpn_server_endpoint");
    if (m_serverEndpoint.isEmpty()) m_serverEndpoint = "178.128.46.45:51820";

    m_virtualIp  = m_mainConfig->getUserConfigByKey("vpn_virtual_ip");   // empty = not yet registered
    m_allowedIps = m_mainConfig->getUserConfigByKey("vpn_allowed_ips");
    if (m_allowedIps.isEmpty()) m_allowedIps = "0.0.0.0/0";

    m_isEnabled    = (m_mainConfig->getUserConfigByKey("vpn_enabled") == "true");
    m_isRegistered = (m_mainConfig->getUserConfigByKey("vpn_registered") == "true");
    m_tenantId     = m_mainConfig->getUserConfigByKey("vpn_tenant_id");

    // Restore saved enrollment token if one was persisted
    QString savedToken = m_mainConfig->getUserConfigByKey("vpn_enrollment_token");
    if (!savedToken.isEmpty()) m_enrollmentToken = savedToken;

    // ZERO-TOUCH: If no identity exists (first boot or wiped), auto-generate it now.
    if (m_publicKey.isEmpty()) {
        qDebug() << "[Wireguard] No identity found. Auto-generating secure keypair...";
        // If we lost our keys, our previous registration is invalid. Force a new one.
        m_isRegistered = false;
        m_virtualIp = "";
        generateIdentity();
    }

    emit privateKeyChanged();
    emit publicKeyChanged();
    emit serverPublicKeyChanged();
    emit serverEndpointChanged();
    emit virtualIpChanged();
    emit allowedIpsChanged();
    emit isEnabledChanged();

    // Start watchdog if enabled on boot
    if (m_isEnabled && m_status == Disconnected) {
        qDebug() << "[Wireguard] Watchdog active on boot. Interval: 15s";
        m_reconnectTimer->start();
    }

    emit playerNameChanged();
    emit enrollmentTokenChanged();
}

void WireguardConfig::save()
{
    if (!m_mainConfig) return;

    m_mainConfig->setUserConfigByKey("vpn_private_key",    m_privateKey);
    m_mainConfig->setUserConfigByKey("vpn_public_key",     m_publicKey);
    m_mainConfig->setUserConfigByKey("vpn_server_public_key", m_serverPublicKey);
    m_mainConfig->setUserConfigByKey("vpn_server_endpoint", m_serverEndpoint);
    m_mainConfig->setUserConfigByKey("vpn_virtual_ip",     m_virtualIp);
    m_mainConfig->setUserConfigByKey("vpn_allowed_ips",    m_allowedIps);
    m_mainConfig->setUserConfigByKey("vpn_enabled",        m_isEnabled ? "true" : "false");
    m_mainConfig->setUserConfigByKey("vpn_registered",     m_isRegistered ? "true" : "false");
    m_mainConfig->setUserConfigByKey("vpn_tenant_id",      m_tenantId);
}

// ─── Getters / Setters ────────────────────────────────────────────────────────

QString WireguardConfig::getPublicKey() const { return m_publicKey; }
void WireguardConfig::setPublicKey(const QString &value)
{
    if (m_publicKey != value) {
        m_publicKey = value;
        emit publicKeyChanged();
        save();
    }
}

QString WireguardConfig::getServerPublicKey() const { return m_serverPublicKey; }
void WireguardConfig::setServerPublicKey(const QString &value)
{
    if (m_serverPublicKey != value) {
        m_serverPublicKey = value;
        emit serverPublicKeyChanged();
        save();
    }
}

QString WireguardConfig::getServerEndpoint() const { return m_serverEndpoint; }
void WireguardConfig::setServerEndpoint(const QString &value)
{
    if (m_serverEndpoint != value) {
        m_serverEndpoint = value;
        emit serverEndpointChanged();
        save();
    }
}

QString WireguardConfig::getVirtualIp() const { return m_virtualIp; }
void WireguardConfig::setVirtualIp(const QString &value)
{
    if (m_virtualIp != value) {
        m_virtualIp = value;
        emit virtualIpChanged();
        save();
    }
}

QString WireguardConfig::getAllowedIps() const { return m_allowedIps; }
void WireguardConfig::setAllowedIps(const QString &value)
{
    if (m_allowedIps != value) {
        m_allowedIps = value;
        emit allowedIpsChanged();
        save();
    }
}

bool WireguardConfig::getIsEnabled() const { return m_isEnabled; }
void WireguardConfig::setIsEnabled(bool value)
{
    if (m_isEnabled != value) {
        m_isEnabled = value;
        emit isEnabledChanged();
        save();
        
        if (m_isEnabled && m_status == Disconnected) {
            m_reconnectTimer->start();
        } else if (!m_isEnabled) {
            m_reconnectTimer->stop();
        }
    }
}

QString WireguardConfig::getPrivateKey() const { return m_privateKey; }
void WireguardConfig::setPrivateKey(const QString &value)
{
    if (m_privateKey != value) {
        m_privateKey = value;
        emit privateKeyChanged();
        save();
    }
}

void WireguardConfig::setEnrollmentToken(const QString &token)
{
    if (m_enrollmentToken != token) {
        m_enrollmentToken = token;
        if (m_mainConfig)
            m_mainConfig->setUserConfigByKey("vpn_enrollment_token", token);
        emit enrollmentTokenChanged();
    }
}

QString WireguardConfig::getEnrollmentToken() const { return m_enrollmentToken; }

QString WireguardConfig::getPlayerName() const
{
    return m_mainConfig ? m_mainConfig->getPlayerName() : "UNKNOWN_DEVICE";
}

void WireguardConfig::setManagementBaseUrl(const QString &url)
{
    m_managementBaseUrl = url;
}

bool WireguardConfig::getIsRegistered() const { return m_isRegistered; }
void WireguardConfig::setIsRegistered(bool value)
{
    if (m_isRegistered != value) {
        m_isRegistered = value;
        save();
    }
}

// ─── VPN Lifecycle ────────────────────────────────────────────────────────────

bool WireguardConfig::isConfigComplete() const
{
    return !m_privateKey.isEmpty()
        && !m_publicKey.isEmpty()
        && !m_serverPublicKey.isEmpty()
        && !m_serverEndpoint.isEmpty()
        && !m_virtualIp.isEmpty()
        && m_isRegistered;
}

void WireguardConfig::startVpn()
{
    // DEADLOCK FIX: Don't block 'Registering' if the config is now complete.
    // If we are 'Connecting', we are already talking to the Android VpnService.
    if (m_status == Connecting) {
        qInfo() << "[Wireguard] Connection already in progress. Ignoring request.";
        return;
    }

    qCritical() << "[Wireguard] startVpn() called."
                << "Registered:" << m_isRegistered
                << "VirtualIp:" << m_virtualIp;
    setErrorMessage("");

    // If the device is not yet registered with the backend, run the handshake first.
    // startVpn() will be called again automatically by handleRegistrationResponse().
    if (!isConfigComplete()) {
        qInfo() << "[Wireguard] Config incomplete — initiating auto-registration handshake.";
        performHandshake();
        return;
    }

    setIsEnabled(true);
    setStatus(Connecting);
    emit requestVpnStart(m_privateKey, m_virtualIp + "/32", m_serverPublicKey, m_serverEndpoint, m_allowedIps);
}

void WireguardConfig::stopVpn()
{
    if (m_otaTimer) {
        m_otaTimer->stop();
    }
    setIsEnabled(false);
    setStatus(Disconnected);
    emit requestVpnStop();
}

void WireguardConfig::setVpnError(const QString &message)
{
    qWarning() << "[WireguardConfig] VPN error:" << message;
    setErrorMessage(message);
    setStatus(Error);
}

// ─── Zero-Touch Auto-Registration ────────────────────────────────────────────

void WireguardConfig::performHandshake()
{
    if (m_publicKey.isEmpty()) {
        setVpnError("No public key generated. Please generate an identity first.");
        return;
    }
    if (m_enrollmentToken.isEmpty()) {
        setVpnError("No enrollment token configured.");
        return;
    }

    setStatus(Registering);
    setErrorMessage("Registering device identity with management server...");

    // 'deviceId' is now federated with the global PlayerName (Device Name)
    QString deviceId = getPlayerName();

    // Build JSON body matching DeviceRegistrationDto exactly
    QJsonObject payload = m_mainConfig->getSystemMetadata();
    payload["publicKey"]       = m_publicKey;
    payload["enrollmentToken"] = m_enrollmentToken;  // required field in DTO

    QByteArray body = QJsonDocument(payload).toJson(QJsonDocument::Compact);

    QUrl url(m_managementBaseUrl + "/api/v1/devices/vpn-register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    qInfo() << "[Wireguard] POST" << url.toString() << "deviceId:" << deviceId;
    QNetworkReply *reply = m_networkManager->post(request, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleRegistrationResponse(reply);
    });
}

void WireguardConfig::handleRegistrationResponse(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = QString("Registration failed: %1").arg(reply->errorString());
        qWarning() << "[Wireguard]" << errMsg;
        setVpnError(errMsg);
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        setVpnError("Registration failed: invalid JSON response from server.");
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject dataObj = root;

    // Peel away "data" layers until we reach the actual payload
    while (dataObj.contains("data") && dataObj.value("data").isObject()) {
        dataObj = dataObj.value("data").toObject();
    }

    // Response fields match DeviceRegistrationResponseDto: clientIp, serverPublicKey, serverEndpoint
    QString clientIp   = dataObj.value("clientIp").toString();
    QString serverKey  = dataObj.value("serverPublicKey").toString();
    QString endpoint   = dataObj.value("serverEndpoint").toString();

    if (clientIp.isEmpty()) {
        setVpnError("Registration failed: server returned no virtual IP.");
        return;
    }

    qInfo() << "[Wireguard] Registration succeeded. VirtualIP:" << clientIp
            << "ServerKey:" << serverKey;

    if (!serverKey.isEmpty()) m_serverPublicKey = serverKey;
    if (!endpoint.isEmpty())  m_serverEndpoint  = endpoint;
    m_virtualIp    = clientIp;   // e.g. "100.64.0.2" — /32 appended in startVpn()
    m_tenantId     = dataObj.value("tenantId").toString();
    m_isRegistered = true;
    save();

    emit serverPublicKeyChanged();
    emit serverEndpointChanged();
    emit virtualIpChanged();
    setErrorMessage("Identity confirmed. Establishing tunnel...");

    // Now that we have a valid config, initiate the actual VPN tunnel
    startVpn();
}

void WireguardConfig::checkOtaUpdate()
{
    qDebug() << "[Wireguard][OTA] Checking for newer APK version...";
    
    // Extract version code from "v1.0.1004"
    QString vStr = QString(version_from_git);
    int currentVersion = vStr.section('.', -1).toInt();
    
    QUrl url(m_managementBaseUrl + "/api/v1/devices/ota-check");
    QUrlQuery query;
    query.addQueryItem("versionCode", QString::number(currentVersion));
    if (!m_tenantId.isEmpty()) {
        query.addQueryItem("tenantId", m_tenantId);
    }
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("x-device-id", getPlayerName().toUtf8());
    request.setRawHeader("x-enrollment-token", m_enrollmentToken.toUtf8());
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleOtaResponse(reply);
    });
}

void WireguardConfig::handleOtaResponse(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[Wireguard][OTA] Poll failed:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonObject dataObj = root;

    // Support both direct and enveloped responses for robustness
    while (dataObj.contains("data") && dataObj.value("data").isObject()) {
        dataObj = dataObj.value("data").toObject();
    }

    if (dataObj.value("updateAvailable").toBool()) {
        QString downloadUrl = dataObj.value("downloadUrl").toString();
        QString sha256 = dataObj.value("sha256Hash").toString();
        qInfo() << "[Wireguard][OTA] UPDATE AVAILABLE! URL:" << downloadUrl << "SHA:" << sha256;
        emit requestOtaDownload(downloadUrl, sha256);
    } else {
        qDebug() << "[Wireguard][OTA] Device is up to date.";
    }
}

void WireguardConfig::resetRegistration()
{
    qInfo() << "[Wireguard] Resetting registration state.";

    // 1. Notify Backend to free IP
    if (m_isRegistered && !m_tenantId.isEmpty()) {
        qInfo() << "[Wireguard] Notifying VPC to release Virtual IP...";
        QNetworkRequest request(QUrl(m_managementBaseUrl + "/api/v1/devices/vpn-deregister"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject payload;
        payload["deviceId"] = m_mainConfig->getUuid();
        payload["tenantId"] = m_tenantId;

        m_networkManager->post(request, QJsonDocument(payload).toJson());
    }

    // 2. Destroy Tunnel (Defuse the Time Bomb)
    qInfo() << "[Wireguard] Killing active tunnel before wipe...";
    stopVpn();

    // 3. Wipe Memory & Save
    m_isRegistered = false;
    m_virtualIp    = "";
    m_tenantId     = "";
    m_serverPublicKey = ""; // Force re-fetch
    m_serverEndpoint = "178.128.46.45:51820";

    save();
    emit virtualIpChanged();
    emit serverPublicKeyChanged();
    emit serverEndpointChanged();
    setStatus(Disconnected);
}

// ─── Identity ─────────────────────────────────────────────────────────────────

void WireguardConfig::generateIdentity()
{
    emit requestKeyGeneration();
}

// ─── Utilities ────────────────────────────────────────────────────────────────

void WireguardConfig::copyToClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
    }
}

WireguardConfig::VpnStatus WireguardConfig::getStatus() const { return m_status; }

void WireguardConfig::setStatus(VpnStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();

        // Manage Reconnect Watchdog
        if (m_status == Connected) {
            m_reconnectTimer->stop();
            
            save();
            emit requestSystemReport();
            
            // Start OTA Polling Timer (every 15 minutes)
            if (!m_otaTimer) {
                m_otaTimer = new QTimer(this);
                connect(m_otaTimer, &QTimer::timeout, this, &WireguardConfig::checkOtaUpdate);
            }
            m_otaTimer->start(15 * 60 * 1000); // 15 mins
            
            // Trigger an immediate check on connection
            QTimer::singleShot(5000, this, &WireguardConfig::checkOtaUpdate);
        } 
        else if (m_status == Disconnected || m_status == Error) {
            if (m_isEnabled) {
                qInfo() << "[Wireguard] Disconnected while enabled. Starting reconnect watchdog...";
                m_reconnectTimer->start();
            }
        }
    }
}

void WireguardConfig::handleReconnect()
{
    if (m_isEnabled && (m_status == Disconnected || m_status == Error)) {
        qInfo() << "[Wireguard][Watchdog] Re-triggering VPN start...";
        startVpn();
    } else {
        m_reconnectTimer->stop();
    }
}

QString WireguardConfig::getErrorMessage() const { return m_errorMessage; }
void WireguardConfig::setErrorMessage(const QString &value)
{
    if (m_errorMessage != value) {
        m_errorMessage = value;
        emit errorMessageChanged();
    }
}
