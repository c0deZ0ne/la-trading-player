#include "wireguard_config.h"
#include <QDebug>
#include <QGuiApplication>
#include <QClipboard>

WireguardConfig::WireguardConfig(IMainConfiguration *mainConfig, QObject *parent)
    : QObject(parent)
    , m_mainConfig(mainConfig)
    , m_publicKey("")
    , m_serverPublicKey("")
    , m_serverEndpoint("")
    , m_virtualIp("")
    , m_allowedIps("0.0.0.0/0")
    , m_isEnabled(false)
    , m_status(0)
    , m_errorMessage("")
{
}

void WireguardConfig::load()
{
    if (!m_mainConfig) return;

    m_privateKey = m_mainConfig->getUserConfigByKey("vpn_private_key");
    m_publicKey = m_mainConfig->getUserConfigByKey("vpn_public_key");
    
    m_serverPublicKey = m_mainConfig->getUserConfigByKey("vpn_server_public_key");
    if (m_serverPublicKey.isEmpty()) m_serverPublicKey = ""; // Placeholder-free

    m_serverEndpoint = m_mainConfig->getUserConfigByKey("vpn_server_endpoint");
    if (m_serverEndpoint.isEmpty()) m_serverEndpoint = "vpn.example.com:51820";

    m_virtualIp = m_mainConfig->getUserConfigByKey("vpn_virtual_ip");
    if (m_virtualIp.isEmpty()) m_virtualIp = "10.8.0.2/32";

    m_allowedIps = m_mainConfig->getUserConfigByKey("vpn_allowed_ips");
    if (m_allowedIps.isEmpty()) m_allowedIps = "0.0.0.0/0";
    
    m_isEnabled = (m_mainConfig->getUserConfigByKey("vpn_enabled") == "true");

    emit privateKeyChanged();
    emit publicKeyChanged();
    emit serverPublicKeyChanged();
    emit serverEndpointChanged();
    emit virtualIpChanged();
    emit allowedIpsChanged();
    emit isEnabledChanged();
}

void WireguardConfig::save()
{
    if (!m_mainConfig) return;

    m_mainConfig->setUserConfigByKey("vpn_private_key", m_privateKey);
    m_mainConfig->setUserConfigByKey("vpn_public_key", m_publicKey);
    m_mainConfig->setUserConfigByKey("vpn_server_public_key", m_serverPublicKey);
    m_mainConfig->setUserConfigByKey("vpn_server_endpoint", m_serverEndpoint);
    m_mainConfig->setUserConfigByKey("vpn_virtual_ip", m_virtualIp);
    m_mainConfig->setUserConfigByKey("vpn_allowed_ips", m_allowedIps);
    m_mainConfig->setUserConfigByKey("vpn_enabled", m_isEnabled ? "true" : "false");
}

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

void WireguardConfig::generateIdentity()
{
    emit requestKeyGeneration();
}

void WireguardConfig::startVpn()
{
    setErrorMessage("");
    setStatus(1); // Connecting
    emit requestVpnStart(m_privateKey, m_virtualIp, m_serverPublicKey, m_serverEndpoint, m_allowedIps);
}

void WireguardConfig::stopVpn()
{
    setStatus(0); // Disconnected
    emit requestVpnStop();
}

void WireguardConfig::setVpnError(const QString &message)
{
    qWarning() << "[WireguardConfig] VPN error received:" << message;
    setErrorMessage(message);
    setStatus(3); // Error
}

void WireguardConfig::copyToClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
    }
}

int WireguardConfig::getStatus() const { return m_status; }

void WireguardConfig::setStatus(int status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
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
