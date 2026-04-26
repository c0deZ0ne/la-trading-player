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

#include "main_configuration.hpp"
#include <QXmlStreamReader>
#include <QString>
#include <QCryptographicHash>
#include <QSysInfo>
#include <QUuid>
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QtAndroid>
#endif

// need to define static variable
QString MainConfiguration::log_directory = "";


/**
 * @brief TConfiguration::TConfiguration
 * @param MySettings
 */
MainConfiguration::MainConfiguration(ISettings *uc, QObject *parent) : QObject(parent)
{
    MySettings  = uc;
}

void MainConfiguration::init()
{
    uuid        = getUserConfigByKey("uuid");
    if (uuid.isEmpty()) {
        setUuid(getStaticHardwareId());
    }
    player_name = getUserConfigByKey("player_name");
    time_zone   = QTimeZone::systemTimeZoneId();

    determineOS();

    // ugly workaround from https://stackoverflow.com/questions/21976264/qt-isodate-formatted-date-time-including-timezone
    // cause otherwise we get no time zone in date string
    setStartTime(QDateTime::currentDateTime().toOffsetFromUtc(QDateTime::currentDateTime().offsetFromUtc()).toString(Qt::ISODate));
    
    determinePlayerName();
}

QString MainConfiguration::getBuildVersion()
{
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    if (!context.isValid()) return "";

    QAndroidJniObject packageName = context.callObjectMethod("getPackageName", "()Ljava/lang/String;");
    QAndroidJniObject packageManager = context.callObjectMethod("getPackageManager", "()Landroid/content/pm/PackageManager;");
    
    if (packageManager.isValid() && packageName.isValid()) {
        QAndroidJniObject packageInfo = packageManager.callObjectMethod(
            "getPackageInfo", 
            "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;", 
            packageName.object<jstring>(), 
            0
        );
        
        if (packageInfo.isValid()) {
            int versionCode = packageInfo.getField<jint>("versionCode");
            return QString::number(versionCode);
        }
    }
#endif
    return "";
}

void MainConfiguration::setAdditionalVersion(QString value)
{
    version = QString(version_from_git) + value;
}

void MainConfiguration::setUuid(const QString &value)
{
    uuid = value;
    setUserConfigByKey("uuid", value);
}

QString MainConfiguration::getLogDir()
{
    return MainConfiguration::log_directory;
}

QString MainConfiguration::getLastPlayedIndexPath()
{
    return getUserConfigByKey("last_played_index_path");
}

void MainConfiguration::setLastPlayedIndexPath(const QString &value)
{
    setUserConfigByKey("last_played_index_path", value);
}

QSettings *MainConfiguration::getUserConfig() {return MySettings->getOriginal();}

QString MainConfiguration::getUserConfigByKey(QString key)
{
    return MySettings->value(key);
}

void MainConfiguration::setUserConfigByKey(QString key, QString value)
{
    MySettings->setValue(key, value);
}

void MainConfiguration::setPlayerName(const QString &value)
{
    player_name = value;
    setUserConfigByKey("player_name", value);
}

void MainConfiguration::determinePlayerName()
{
    QString saved_name = getUserConfigByKey("player_name");
    if (!saved_name.isEmpty()) {
        setPlayerName(saved_name);
        return;
    }

    QString device_model = "";
#ifdef Q_OS_ANDROID
    QAndroidJniObject model = QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;");
    device_model = model.toString();
#else
    device_model = QSysInfo::machineHostName();
    if (device_model.isEmpty() || device_model == "localhost") {
        device_model = QSysInfo::prettyProductName();
    }
#endif

    QString full_id = getUuid();
    QString auto_name = device_model + "_" + full_id;
    
    setPlayerName(auto_name);
    // Note: We don't save to QSettings yet, so the user can still change it 
}

QString MainConfiguration::determineApiAccessToken(QString username, QString password)
{
    if (QDateTime::currentDateTime() < QDateTime::fromString(getApiAccessTokenExpire(), Qt::ISODate))
        return getApiAccessToken();

    QDateTime dt(QDateTime::currentDateTime());
    QString expire       = dt.addDays(1).toString(Qt::ISODate);
    QString hash_source  = username + password + expire;
    QString access_token = QString::fromUtf8(QCryptographicHash::hash(hash_source.toLocal8Bit(),QCryptographicHash::Md5).toHex());

    setUserConfigByKey("api_access_token_expire", expire);
    setUserConfigByKey("api_access_token", access_token.toUpper());

    return getApiAccessToken();
}

QString MainConfiguration::getApiAccessToken()
{
    return getUserConfigByKey("api_access_token");
}

QString MainConfiguration::getApiAccessTokenExpire()
{
    return getUserConfigByKey("api_access_token_expire");
}

QString MainConfiguration::getUuid() const {return uuid;}

QString MainConfiguration::getPlayerName() const {return player_name;}

void MainConfiguration::setLogDir(const QString &value){log_dir = value;}

void MainConfiguration::setUserAgent(const QString &value){user_agent = value;}

QString MainConfiguration::getUserAgent() const {return user_agent;}

QString MainConfiguration::getIndexUri(){return index_uri;}

QString MainConfiguration::getOS() const {return os;}

QString MainConfiguration::getIndexPath(){return index_path;}

QString MainConfiguration::getTimeZone() const {return time_zone;}

QString MainConfiguration::getBasePath() const{return base_path;}

QString MainConfiguration::getErrorText() const {return error_text;}

QString MainConfiguration::createUuid()
{
    QString hardware_id = "";
    
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    if (context.isValid()) {
        QAndroidJniObject content_resolver = context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        if (content_resolver.isValid()) {
            QAndroidJniObject android_id_string = QAndroidJniObject::fromString("android_id");
            QAndroidJniObject android_id = QAndroidJniObject::callStaticObjectMethod(
                "android/provider/Settings$Secure", 
                "getString", 
                "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", 
                content_resolver.object<jobject>(), 
                android_id_string.object<jstring>()
            );
            if (android_id.isValid()) {
                hardware_id = android_id.toString();
            }
        }
    }
#elif defined Q_OS_WIN32
    hardware_id = QString::fromLatin1(QSysInfo::machineUniqueId().toHex());
#endif

    if (hardware_id.isEmpty()) {
        QString id = QUuid::createUuid().toString();
        return id.mid(1, 36); 
    }
    
    return hardware_id;
}

QString MainConfiguration::getStaticHardwareId() const
{
    QString hardware_id = "";
    
#ifdef Q_OS_ANDROID
    QAndroidJniObject context = QtAndroid::androidContext();
    
    // 1. Try Hardware Serial (The most unique ID)
    QAndroidJniObject serial = QAndroidJniObject::getStaticObjectField("android/os/Build", "SERIAL", "Ljava/lang/String;");
    if (serial.isValid() && serial.toString() != "unknown" && !serial.toString().isEmpty()) {
        hardware_id = serial.toString();
    } 

    // 2. If Serial failed, try getting the PRIMARY IMEI (Slot 0)
    // IMPORTANT: Android 10+ (API 29+) restricts IMEI access to system apps or device owners.
    if (hardware_id.isEmpty() || hardware_id == "unknown") {
        int sdk = QAndroidJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
        
        if (sdk < 29) {
            QAndroidJniObject telephonyManager = context.callObjectMethod("getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", 
                                                                        QAndroidJniObject::fromString("phone").object<jstring>());
            if (telephonyManager.isValid()) {
                // Try Slot 0 first for multi-SIM stability
                QAndroidJniObject imei = telephonyManager.callObjectMethod("getImei", "(I)Ljava/lang/String;", 0);
                
                // JNI Exception check to prevent crashes on restricted devices
                QAndroidJniEnvironment env;
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    qWarning() << "[HardwareId] Caught SecurityException during getImei(0). Skipping...";
                } else {
                    if (imei.isValid() && !imei.toString().isEmpty()) {
                        hardware_id = imei.toString();
                    }
                }

                // Fallback to default getImei/getDeviceId if indexed call fails
                if (hardware_id.isEmpty()) {
                    imei = telephonyManager.callObjectMethod("getImei", "()Ljava/lang/String;");
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        qWarning() << "[HardwareId] Caught SecurityException during getImei(). Skipping...";
                    } else if (imei.isValid() && !imei.toString().isEmpty()) {
                        hardware_id = imei.toString();
                    }
                }
                
                if (hardware_id.isEmpty()) {
                    imei = telephonyManager.callObjectMethod("getDeviceId", "()Ljava/lang/String;");
                    if (env->ExceptionCheck()) {
                        env->ExceptionClear();
                        qWarning() << "[HardwareId] Caught SecurityException during getDeviceId(). Skipping...";
                    } else if (imei.isValid() && !imei.toString().isEmpty()) {
                        hardware_id = imei.toString();
                    }
                }
            }
        } else {
            qInfo() << "[HardwareId] API 29+ detected. Skipping IMEI access (restricted).";
        }
    }

    // 3. SECURE FALLBACK: Use Settings.Secure.ANDROID_ID if hardware IDs are restricted
    if (hardware_id.isEmpty() || hardware_id == "unknown") {
        qInfo() << "[HardwareId] Falling back to ANDROID_ID...";
        QAndroidJniObject content_resolver = context.callObjectMethod("getContentResolver", "()Landroid/content/ContentResolver;");
        if (content_resolver.isValid()) {
            QAndroidJniObject android_id_string = QAndroidJniObject::fromString("android_id");
            QAndroidJniObject android_id = QAndroidJniObject::callStaticObjectMethod(
                "android/provider/Settings$Secure", 
                "getString", 
                "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;", 
                content_resolver.object<jobject>(), 
                android_id_string.object<jstring>()
            );
            if (android_id.isValid() && !android_id.toString().isEmpty()) {
                hardware_id = android_id.toString();
            }
        }
    }

    // 4. LAST RESORT: If everything fails (highly unlikely), return a generic but non-empty ID
    if (hardware_id.isEmpty() || hardware_id == "unknown") {
        qWarning() << "[HardwareId] ALL hardware identifiers failed. Returning emergency ID.";
        return "GENERIC_ANDROID_ID";
    }
#elif defined Q_OS_WIN32
    hardware_id = QString::fromLatin1(QSysInfo::machineUniqueId().toHex());
#endif

    return hardware_id;
}

void MainConfiguration::setIndexUri(const QString &value)
{
    index_uri = value;
    setUserConfigByKey("index_uri", value);
}

void MainConfiguration::determineBasePath(QString absolute_path_to_bin)
{
    if (absolute_path_to_bin.contains("/bin"))
        setBasePath(absolute_path_to_bin.mid(0, absolute_path_to_bin.length()-3));
    else
        setBasePath(absolute_path_to_bin);

    if (base_path.mid(base_path.length()-1, 1) != "/")
        setBasePath(base_path+"/");
}


bool MainConfiguration::validateContentUrl(QString url_string)
{
    if (url_string.isEmpty()) // isEmpty or isValid do not work as expected
    {
        error_text += "A content-url is neccessary\n";
		return false;
    }

	QUrl    url(url_string, QUrl::StrictMode);
    error_text    = "";

	if (url.scheme() != "") // prevent thingsq like Http or httP
    {
        if (url.toString(QUrl::RemoveScheme).mid(0, 2) != "//")
        {
            error_text += "Url Scheme is no valid! Use "+ url.scheme() + "://domain.tld\n";
			return false;
        }

		if (url.scheme() != "file") // file:// will irritate garlic-lib TODO! Fix this later!
			setValidatedContentUrl(url.toString());
		else
			setValidatedContentUrl(url.toString(QUrl::RemoveScheme));
    }
    else
    {
        if (url_string.mid(0, 1) != "/")
        {
            error_text += "Relative path to SMIL-Index is not allowed! Use /path/to/index, http://domain.tld, https://domain.tld, or file://path/to/index\n";
			return false;
        }
        else
            setValidatedContentUrl(url.toString());
    }


    return true;
}

void MainConfiguration::determineIndexUri(const QString &value)
{
    if (!value.isEmpty())
    {
        setIndexUri(value);
    }
    else if (!getUserConfigByKey("index_uri").isEmpty()) // get from intern config
    {
        setIndexUri(getUserConfigByKey("index_uri"));
    }

    determineIndexPath();
}

void MainConfiguration::setIndexPath(const QString &value)
{
    index_path = value;
}

void MainConfiguration::setValidatedContentUrl(const QString &value)
{
    validated_content_url = value;
}

QString MainConfiguration::getValidatedContentUrl()
{
    return validated_content_url;
}

void MainConfiguration::setStandbyMode(const QString &value)
{
    setUserConfigByKey("standby_mode", value);
}

QString MainConfiguration::getStandbyMode()
{
    return getUserConfigByKey("standby_mode");
}

void MainConfiguration::setRebootDays(const QString &value)
{
    setUserConfigByKey("schedule/reboot_days", value);
}

QString MainConfiguration::getRebootDays()
{
    return getUserConfigByKey("schedule/reboot_days");
}

void MainConfiguration::setRebootTime(const QString &value)
{
    setUserConfigByKey("schedule/reboot_time", value);
}

QString MainConfiguration::getRebootTime()
{
    return getUserConfigByKey("schedule/reboot_time");
}

QString MainConfiguration::getStartTime() const
{
    return start_time;
}

void MainConfiguration::setStartTime(const QString &value)
{
    start_time = value;
}

void MainConfiguration::setBasePath(const QString &value)
{
    base_path = value;
}

/**
 * @brief TConfiguration::getPaths returns paths.
 *        Values can be config, var, media or base
 * @param path_name
 * @return
 */
QString MainConfiguration::getPaths(QString path_name)
{
    QString ret = base_path;
    if (path_name == "cache")
        ret = cache_dir;
    else if (path_name == "logs")
        ret = log_dir;
    else if (path_name == "scripts")
        ret += "scripts/";

    return ret;
}

void MainConfiguration::createDirectories()
{
#if defined Q_OS_WIN32  // QStandardPaths::CacheLocation in windows 7 is set to appdir (bin), which should not be writable after installation in Program Files
    cache_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache/";
    log_dir   = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs/";
#elif defined  Q_OS_ANDROID
    // Use AppDataLocation for OS-managed storage (deleted automatically on uninstall)
    // This typically resolves to /storage/emulated/0/Android/data/<package>/files/
    QString managed_root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!managed_root.isEmpty()) {
        cache_dir = managed_root + "/cache/";
        log_dir   = managed_root + "/logs/";
        
        // Try creating these directories. If it fails, fallback to internal storage
        if (!createDirectoryIfNotExist(cache_dir) || !createDirectoryIfNotExist(log_dir)) {
            qWarning(SmilParser) << "OS-managed external storage not writable, falling back to internal storage.";
            cache_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache/";
            log_dir   = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs/";
        }
    } else {
        qWarning(SmilParser) << "No managed data location found, falling back to internal storage.";
        cache_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache/";
        log_dir   = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs/";
    }
#else
    cache_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+ "/";
    log_dir   = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs/";
#endif
    createDirectoryIfNotExist(cache_dir);
    createDirectoryIfNotExist(log_dir);
    MainConfiguration::log_directory = log_dir;
}


void MainConfiguration::determineUserAgent()
{
    user_agent = "GAPI/1.0 (UUID:"+getUuid()+"; NAME:"+getPlayerName()+") garlic-"+getOS()+"/"+getVersion()+" (MODEL:Garlic)";
}


// ========================  private ================================================

/**
 * @brief TConfiguration::determineIndexPath
 * determine the clean index path
 * e.g only the scheme://domain.tld/path_to/
 */
void MainConfiguration::determineIndexPath()
{
    if (index_uri.mid(0, 4) == "http" || index_uri.mid(0, 3) == "ftp")
    {
        QUrl url(index_uri);
        setIndexPath(url.adjusted(QUrl::RemoveFilename |
                                  QUrl::RemoveQuery |
                                  QUrl::RemoveFragment).toString());
        if (index_path.mid(index_path.length()-1, 1) != "/")
            setIndexPath(index_path+"/");

    }
    else
    {
        QFileInfo fi(index_uri);
        setIndexPath(fi.path()+"/");
    }
}

bool MainConfiguration::createDirectoryIfNotExist(QString path)
{
    QDir dir;
    dir.setPath(path);
    if (!dir.exists() && !dir.mkpath("."))
    {
        qCritical(SmilParser) << "Failed to create " << dir.path() << "\r";
        return false;
    }
    return true;
}

void MainConfiguration::determineOS()
{
#if defined  Q_OS_ANDROID
    os = OS_ANDROID;
#elif defined Q_OS_FREEBSD
    os = OS_BSD;
#elif defined Q_OS_HURD
    os = OS_HURD;
#elif defined Q_OS_IOS
    os = OS_IOS;
#elif defined Q_OS_LINUX
    os = OS_LINUX;
#elif defined Q_OS_NETBSD
    os = OS_BSD;
#elif defined Q_OS_OPENBSD
    os = OS_BSD;
#elif defined Q_OS_OSX
    os = OS_OSX;
#elif defined Q_OS_WIN32
    os = OS_WINDOWS;
#elif defined Q_OS_WINRT
    os = OS_WINRT;
#else
    os = OS_UNKNOWN;
#endif
}

QJsonObject MainConfiguration::getSystemMetadata() const
{
    QJsonObject meta;
    meta["deviceId"] = getUuid();
    meta["serialNumber"] = getStaticHardwareId();
    meta["deviceName"] = getPlayerName();
    meta["appVersion"] = version;
    meta["playlistUrl"] = index_uri;
    meta["osVersion"] = QSysInfo::prettyProductName();

#ifdef Q_OS_ANDROID
    QAndroidJniObject model = QAndroidJniObject::getStaticObjectField("android/os/Build", "MODEL", "Ljava/lang/String;");
    QAndroidJniObject brand = QAndroidJniObject::getStaticObjectField("android/os/Build", "BRAND", "Ljava/lang/String;");
    QAndroidJniObject manufacturer = QAndroidJniObject::getStaticObjectField("android/os/Build", "MANUFACTURER", "Ljava/lang/String;");
    QAndroidJniObject release = QAndroidJniObject::getStaticObjectField("android/os/Build$VERSION", "RELEASE", "Ljava/lang/String;");
    int sdk = QAndroidJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");

    meta["model"] = model.toString();
    meta["brand"] = brand.toString();
    meta["manufacturer"] = manufacturer.toString();
    meta["osVersion"] = "Android " + release.toString() + " (API " + QString::number(sdk) + ")";
#else
    meta["model"] = QSysInfo::machineHostName();
    meta["brand"] = QSysInfo::productType();
    meta["manufacturer"] = QSysInfo::prettyProductName();
#endif

    return meta;
}
