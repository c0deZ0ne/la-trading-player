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
#include "system_report.h"
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#endif

Reporting::CreateSystemReport::CreateSystemReport(MainConfiguration *config, ResourceMonitor *rm,
                           QObject *parent) : Reporting::CreateBase(config, parent)
{
    MyResourceMonitor = rm;
    MyConfiguration = config;
    MyNetwork.reset(new SystemInfos::Network(this));

}

void Reporting::CreateSystemReport::process()
{
    init();
    createSystemInfo();
    createGpsInfo();
    createNetwork();
    createVpnInfo();
    createConfiguration();
    createModelInfo();
    createFactoryDefault();
    createUserPref();
    createHardwareInfo();
}

void Reporting::CreateSystemReport::createNetwork()
{
    network = document.createElement("network");
    system_info.appendChild(network);
    
    // Pass 1: Physical Interfaces (WiFi/Ethernet) - prioritize these for lastKnownIp
    MyNetwork->resetInterface();
    for(int i = 0; i < MyNetwork->countInterfaces(); i++)
    {
        QString name = MyNetwork->getInterfaceId().toLower();
        // Match the logic used in getPhysicalIP()
        bool isPhysical = name.contains("wlan") || name.contains("eth") || name.contains("p2p") || 
                          (!MyNetwork->getMac().isEmpty() && !name.contains("tun") && !name.contains("wg"));

        if (isPhysical && MyNetwork->isRealInterface()) {
            appendNetworkChilds();
        }

        MyNetwork->nextInterface();
    }

    // Pass 2: VPN/Virtual Interfaces
    MyNetwork->resetInterface();
    for(int i = 0; i < MyNetwork->countInterfaces(); i++)
    {
        QString name = MyNetwork->getInterfaceId().toLower();
        // Match the logic used in getVpnIP()
        bool isVpn = name.contains("tun") || name.contains("wg") || name.contains("ppp") || 
                     (MyNetwork->getMac().isEmpty());

        if (isVpn && MyNetwork->isRealInterface()) {
            // Safety check: ensure we don't double-report physical interfaces
            bool wasPhysical = name.contains("wlan") || name.contains("eth") || name.contains("p2p");
            if (!wasPhysical) {
                appendNetworkChilds();
            }
        }

        MyNetwork->nextInterface();
    }
}

void Reporting::CreateSystemReport::createVpnInfo()
{
    QString vpn_ip = MyNetwork->getVpnIP();
    QString physical_ip = MyNetwork->getPhysicalIP();

    qDebug() << "[Wireguard][REPORT] Preparing System Report...";
    qDebug() << "[Wireguard][REPORT] Physical IP:" << (physical_ip.isEmpty() ? "none" : physical_ip);
    qDebug() << "[Wireguard][REPORT] VPN IP:" << (vpn_ip.isEmpty() ? "none" : vpn_ip);

    // Hijack the standard 'ipAddress' tag with the VPN IP
    // We leave 'vpnIp' as a property only to avoid 500 errors
    if (!vpn_ip.isEmpty()) {
        system_info.appendChild(createTagWithTextValue("ipAddress", vpn_ip));
    } else {
        system_info.appendChild(createTagWithTextValue("ipAddress", physical_ip));
    }
}

void Reporting::CreateSystemReport::createGpsInfo()
{
    if (MyResourceMonitor != nullptr) {
        system_info.appendChild(createTagWithTextValue("latitude", MyResourceMonitor->getLatitude()));
        system_info.appendChild(createTagWithTextValue("longitude", MyResourceMonitor->getLongitude()));
    } else {
        system_info.appendChild(createTagWithTextValue("latitude", "n/a"));
        system_info.appendChild(createTagWithTextValue("longitude", "n/a"));
    }
}

void Reporting::CreateSystemReport::createConfiguration()
{
    configuration = document.createElement("configuration");
    system_info.appendChild(configuration);
}

void Reporting::CreateSystemReport::createModelInfo()
{
    QString pcb_revision = "";
    QString manufacturer = "Sagiadinos";
    QString model_description = "";
    QString model_name = "";

#ifdef Q_OS_ANDROID
    pcb_revision = QAndroidJniObject::getStaticObjectField("android/os/Build", "HARDWARE", "Ljava/lang/String;").toString();
    manufacturer = QAndroidJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;").toString();
    model_name = QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();
#endif

    QDomElement model_info = document.createElement("modelInfo");
    configuration.appendChild(model_info);
    model_info.appendChild(createPropTag("PCB", MyConfiguration->getAppName()));
    model_info.appendChild(createPropTag("PCBRevision", pcb_revision));
    model_info.appendChild(createPropTag("operatingSystem", MyConfiguration->getOS()));
    model_info.appendChild(createPropTag("manufacturer", manufacturer));
    model_info.appendChild(createPropTag("manufacturerURL", "https://garlic-player.com"));
    model_info.appendChild(createPropTag("modelDescription", model_description));
    model_info.appendChild(createPropTag("modelName", model_name));
    model_info.appendChild(createPropTag("modelURL", ""));
    model_info.appendChild(createPropTag("option", ""));
}

void Reporting::CreateSystemReport::createFactoryDefault()
{
    QDomElement factory_default = document.createElement("factoryDefault");
    configuration.appendChild(factory_default);
    factory_default.appendChild(createTagWithTextValue("variant", ""));
}

void Reporting::CreateSystemReport::createUserPref()
{
    QDomElement user_pref = document.createElement("userPref");
    configuration.appendChild(user_pref);
    user_pref.appendChild(createPropTag("info.playerName", MyConfiguration->getPlayerName()));
    user_pref.appendChild(createPropTag("info.playGroup", ""));
    user_pref.appendChild(createPropTag("info.playGroupMaster", ""));
    user_pref.appendChild(createPropTag("system.locale", QLocale::system().name()));

    user_pref.appendChild(createPropTag("content.serverUrl", MyConfiguration->getIndexUri()));
}

void Reporting::CreateSystemReport::createHardwareInfo()
{
    QString hw_model_name = "";
    QString product_id = "";
    QString serial_number = "";
    QString vendor_id = "";

#ifdef Q_OS_ANDROID
    hw_model_name = QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;").toString();
    product_id = QAndroidJniObject::getStaticObjectField("android/os/Build", "PRODUCT", "Ljava/lang/String;").toString();
    serial_number = QAndroidJniObject::getStaticObjectField("android/os/Build", "SERIAL", "Ljava/lang/String;").toString();
    vendor_id = QAndroidJniObject::getStaticObjectField("android/os/Build", "BRAND", "Ljava/lang/String;").toString();
#endif

    hardware_info = document.createElement("hardwareInfo");
    player.appendChild(hardware_info);
    QDomElement hardware = document.createElement("hardware");
    hardware_info.appendChild(hardware);

    // TODO integrate TScreen Class into lib
    hardware.setAttribute("id", "display:0");
    hardware.appendChild(createPropTag("modelName", hw_model_name));
    hardware.appendChild(createPropTag("product_id", product_id));
    hardware.appendChild(createPropTag("serialNumber", serial_number));
    hardware.appendChild(createPropTag("vendorId", vendor_id));
}


void Reporting::CreateSystemReport::appendNetworkChilds()
{
    net_interface = document.createElement("interface");
    net_interface.setAttribute("id", MyNetwork->getInterfaceId());
    network.appendChild(net_interface);
    net_interface.appendChild(createTagWithTextValue("mac", MyNetwork->getMac()));
    net_interface.appendChild(createTagWithTextValue("type", MyNetwork->getType()));
    for(int i = 0; i < MyNetwork->countAddresses(); i++)
    {
        appendNetworkAddressChilds();
        MyNetwork->nextAddress();
    }
}

void Reporting::CreateSystemReport::appendNetworkAddressChilds()
{
    switch (MyNetwork->getProtocol())
    {
        case QAbstractSocket::IPv4Protocol:
            net_interface.appendChild(createTagWithTextValue("ip_v4", MyNetwork->getIP()));
            net_interface.appendChild(createTagWithTextValue("netmask_v4", MyNetwork->getNetMask()));
            net_interface.appendChild(createTagWithTextValue("broadcast_v4", MyNetwork->getBroadcast()));
            break;
        case QAbstractSocket::IPv6Protocol:
            net_interface.appendChild(createTagWithTextValue("ip_v6", MyNetwork->getIP()));
            net_interface.appendChild(createTagWithTextValue("netmask_v6", MyNetwork->getNetMask()));
            break;
        case QAbstractSocket::AnyIPProtocol:
            net_interface.appendChild(createTagWithTextValue("ip_both", MyNetwork->getIP()));
            net_interface.appendChild(createTagWithTextValue("netmask_both", MyNetwork->getNetMask()));
            net_interface.appendChild(createTagWithTextValue("broadcast_both", MyNetwork->getBroadcast()));
            break;
         default:
            break;

    }
}
