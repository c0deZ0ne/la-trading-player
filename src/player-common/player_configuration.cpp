#include "player_configuration.h"
#include "app_defaults.h"

PlayerConfiguration::PlayerConfiguration(MainConfiguration *mc, QObject *parent) : QObject(parent)
{
    MyMainConfiguration = mc;
}

void PlayerConfiguration::determineInitConfigValues()
{
    determineUuid();
    MyMainConfiguration->determinePlayerName();
    if (!launcher_version.isEmpty())
    {
        // L is needed temporary by SmilControl-CMS to determine correct build number for
        // releasing screen time functionality Nov 2021
        MyMainConfiguration->setAdditionalVersion("L" + launcher_version);
        qInfo(System) << "Launcher version: " << launcher_version;
    }
    determineSmilIndexUri();
    MyMainConfiguration->determineUserAgent();

    QApplication::setApplicationName(MyMainConfiguration->getAppName());
    QApplication::setApplicationVersion(MyMainConfiguration->getVersion());
    QApplication::setApplicationDisplayName(MyMainConfiguration->getAppName());

    QDir dir(".");
    MyMainConfiguration->determineBasePath(dir.absolutePath());
}

void PlayerConfiguration::setHasLauncher(bool value)
{
    has_launcher = value;
}

void PlayerConfiguration::setUuidFromLauncher(QString value)
{
    launcher_uuid = value;
}

void PlayerConfiguration::setVersionFromLauncher(QString value)
{
    launcher_version = value;
}

void PlayerConfiguration::printVersionInformation()
{
    if (!launcher_version.isEmpty())
    {
        qInfo() << "Launcher version: " << launcher_version;
    }
    qInfo() << MyMainConfiguration->getAppName()+ ": " + MyMainConfiguration->getVersion() << " Operating System:" << MyMainConfiguration->getOS();
}

QString PlayerConfiguration::determineDefaultContentUrlName()
{
    QString ret = "SmilControl";
    QString tmp = "";

#ifdef DEFAULT_CONTENT_URL_NAME
    tmp = STRINGIFY(DEFAULT_CONTENT_URL_NAME);
    if (tmp != "")
        ret = tmp;
#endif

    return ret;
}

QString PlayerConfiguration::determineDefaultContentUrl()
{
#ifndef MANAGEMENT_URL
#define MANAGEMENT_URL PROD_MANAGEMENT_URL
#endif
    return QString("%1/api/v1/device-playlist/%2/xml/").arg(MANAGEMENT_URL).arg(MyMainConfiguration->getStaticHardwareId());
}


void PlayerConfiguration::setSmilIndexUriFromLauncher(QString value)
{
    launcher_smil_index_uri = value;
    if (value != MyMainConfiguration->getIndexUri())
    {
        MyMainConfiguration->setIndexUri(value);
    }
}

void PlayerConfiguration::determineUuid()
{
    // if launcher has an uuid use it (Launcher identity takes precedence if present)
    if (has_launcher && !launcher_uuid.isEmpty() && launcher_uuid != MyMainConfiguration->getUuid())
    {
        MyMainConfiguration->setUuid(launcher_uuid);
        return;
    }

    // Otherwise, always use the physical hardware ID as the source of truth
    MyMainConfiguration->setUuid(MyMainConfiguration->getStaticHardwareId());
}

void PlayerConfiguration::determineSmilIndexUri()
{
    if (has_launcher && launcher_smil_index_uri != MyMainConfiguration->getIndexUri())
    {
        MyMainConfiguration->setIndexUri(launcher_smil_index_uri);
    }
    
    if (MyMainConfiguration->getIndexUri().isEmpty())
    {
        MyMainConfiguration->setIndexUri(determineDefaultContentUrl());
    }
}

void PlayerConfiguration::determinePlayerName()
{

}

