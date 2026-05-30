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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include <QSettings>
#include <QTimeZone>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QUuid>
#include <QString>
#include <QCryptographicHash>

#include "i_settings.hpp"
#include "i_main_configuration.hpp"
#include "logger.h"
#include "version.h"

/**
 * @brief The MainConfiguration class
 */
class MainConfiguration  : public QObject, public IMainConfiguration
{
    Q_OBJECT
    Q_PROPERTY(QString managementPin READ getManagementPin NOTIFY managementPinChanged)
    public:
        const     QString        OS_ANDROID  = "android";
        const     QString        OS_DARWIN   = "darwin";
        const     QString        OS_HURD     = "hurd";
        const     QString        OS_IOS      = "iOS";
        const     QString        OS_LINUX    = "linux";
        const     QString        OS_NETBSD   = "BSD";
        const     QString        OS_OSX      = "macOS";
        const     QString        OS_WINDOWS  = "windows";
        const     QString        OS_UNKNOWN  = "unknown";

        const     QString        STANDBY_MODE_NONE       = "no_standby";
        const     QString        STANDBY_MODE_PARTIALLY  = "partially";
        const     QString        STANDBY_MODE_DEEP       = "deep";


        explicit        MainConfiguration(ISettings *uc, QObject *parent = Q_NULLPTR);
        void            init() override;
        QString         getVersion() override {return version;}
        QString         getBuildVersion() override;
        void            setAdditionalVersion(QString value) override;
        void            setAppName(QString value) override {app_name = value;}
        QString         getAppName() override {return app_name;}
        QString         getDescription() override {return "SMIL Player for Digital Signage";}

        static QString  log_directory;
        static QString  getLogDir();

        void            setLastPlayedIndexPath(const QString &value) override;
        QSettings      *getUserConfig() override;
        QString         getUserConfigByKey(QString key) override;
        void            setUserConfigByKey(QString key, QString value) override;
        QString         createUuid() override;
        QString         getStaticHardwareId() const override;
        void            setUuid(const QString &value) override;
        void            setPlayerName(const QString &value) override;
        void            determinePlayerName() override;
        QString         determineApiAccessToken(QString username, QString password) override;
        QString         getApiAccessToken() override;
        QString         getApiAccessTokenExpire() override;

        Q_INVOKABLE QString         getUuid() const override;
        Q_INVOKABLE QString         getPlayerName() const override;
        void            setLogDir(const QString &value) override;
        void            setUserAgent(const QString &value) override;
        QString         getUserAgent() const override;
        Q_INVOKABLE QString         getIndexUri() override;
        QString         getOS() const override;
        QString         getIndexPath() override;
        QString         getTimeZone() const override;
        QString         getBasePath() const override;
        QString         getErrorText() const override;

        void            setValidatedContentUrl(const QString &value) override;
        QString         getValidatedContentUrl() override;
        void            setStandbyMode(const QString &value) override;
        QString         getStandbyMode() override;
        void            setRebootDays(const QString &value) override;
        QString         getRebootDays() override;
        void            setRebootTime(const QString &value) override;
        QString         getRebootTime() override;
        QString         getLastPlayedIndexPath() override;
        QString         getStartTime() const override;
        void            setStartTime(const QString &value) override;
        QJsonObject     getSystemMetadata() const override;
        QString         getPaths(QString path_name) override;
        Q_INVOKABLE void setIndexUri(const QString &value) override;
        void            setIndexPath(const QString &value) override;
//        void            setNetworkInterface(const QString &value);
//        QString         getNetworkInterface();
        void            setBasePath(const QString &value) override;
        void            determineBasePath(QString absolute_path_to_bin) override;
        void            determineIndexUri(const QString &value) override;
        void            createDirectories() override;
        bool            validateContentUrl(QString url_string) override;
        void            determineUserAgent() override;

        QString         getManagementPin() override;
        void            setManagementPin(const QString &value) override;

signals:
        void            managementPinChanged();

private:
        ISettings      *MySettings;
        QString         uuid = "";
        QString         player_name = "";
        QString         version = version_from_git;
        QString         user_agent = "";
        QString         os = "";
        QString         base_path = "";
        QString         index_uri = "";
        QString         index_path = "";
        QString         validated_content_url = "";
        QString         start_time = "";
        QString         time_zone = "";
        QString         cache_dir = "";
        QString         log_dir = "";
        QString         app_name = "La Player";
        QString         error_text = "";
        QString         management_pin = "0000";
        bool            createDirectoryIfNotExist(QString path);
        void            determineIndexPath();
        void            determineOS();
};

#endif // CONFIGURATION_H
