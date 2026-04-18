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
#include "android_manager.h"
#include <jni.h>

AndroidManager* AndroidManager::m_instance = nullptr;

static void notifyVpnStateChanged(JNIEnv *env, jobject obj, jint state)
{
    Q_UNUSED(env);
    Q_UNUSED(obj);
    if (AndroidManager::instance()) {
        emit AndroidManager::instance()->vpnStatusChanged(state);
    }
}

static void notifyVpnError(JNIEnv *env, jobject obj, jstring message)
{
    Q_UNUSED(obj);
    const char *msgChars = env->GetStringUTFChars(message, nullptr);
    QString msg = QString::fromUtf8(msgChars);
    env->ReleaseStringUTFChars(message, msgChars);
    qWarning() << "[GarlicVPN] Error from Java layer:" << msg;
    if (AndroidManager::instance()) {
        emit AndroidManager::instance()->vpnError(msg);
    }
}

AndroidManager::AndroidManager()
{
    m_instance = this;
    MyActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    
    // Dynamic JNI Registration
    QAndroidJniEnvironment env;
    jclass clazz = env->FindClass(ANDROID_ACTIVITY_PATH);
    if (clazz) {
        JNINativeMethod methods[] = {
            {(char*)"notifyVpnStateChanged", (char*)"(I)V", (void *)&notifyVpnStateChanged},
            {(char*)"notifyVpnError", (char*)"(Ljava/lang/String;)V", (void *)&notifyVpnError}
        };
        if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
            qCritical() << "[JNI] FAILED to register native methods for Activity!";
        } else {
            qDebug() << "[JNI] Native methods registered successfully for Activity.";
        }
    } else {
        qCritical() << "[JNI] Could NOT find activity class for registration!";
    }

    MyActivity.callMethod<void>("registerBroadcastReceiver");
}

AndroidManager* AndroidManager::instance()
{
    return m_instance;
}

// No JNI bridge needed here. Moved above constructor.

bool AndroidManager::hasLauncher()
{
    bool is = MyActivity.callMethod<jboolean>("isLauncherInstalled");
    if (is)
    {
        QAndroidJniObject s = MyActivity.callObjectMethod<jstring>("getLauncherName");
        launcher_name       =  s.toString();
    }
    return is;
}

void AndroidManager::fetchDeviceInformation()
{
    MyActivity.callMethod<void>("fetchDeviceInformation");
}

bool AndroidManager::checkPermissiones()
{
    QStringList permissions = {
        "android.permission.READ_EXTERNAL_STORAGE",
        "android.permission.WRITE_EXTERNAL_STORAGE",
        "android.permission.CAMERA",
        "android.permission.RECORD_AUDIO",
        "android.permission.MODIFY_AUDIO_SETTINGS",
        "android.permission.ACCESS_FINE_LOCATION",
        "android.permission.ACCESS_COARSE_LOCATION"
    };

    bool allPermissionsGranted = true;
    for (const QString &permission : permissions)
    {
        auto result = QtAndroid::checkPermission(permission);
        if (result == QtAndroid::PermissionResult::Denied)
        {
            allPermissionsGranted = false;
            break; // break if a permission is missed
        }
    }
    if (allPermissionsGranted)
        return true;

    QtAndroid::PermissionResultMap resultHash = QtAndroid::requestPermissionsSync(permissions);

    for (const QString &permission : permissions)
    {
        if (resultHash[permission] == QtAndroid::PermissionResult::Denied)
        {
            qDebug() << "Permission denied:" << permission;
            return false;
        }
    }

    return true;
}

void AndroidManager::disableScreenSaver()
{
    // disable android screensaver https://stackoverflow.com/questions/44100627/how-to-disable-screensaver-on-qt-android-app
    // https://forum.qt.io/topic/57625/solved-keep-android-5-screen-on
    if (MyActivity.isValid())
    {
        QAndroidJniObject window = MyActivity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (window.isValid())
        {
            const int FLAG_KEEP_SCREEN_ON = 128;
            window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
        }
    }
    // not to crash in Android > 5.x Clear any possible pending exceptions.
    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

}

void AndroidManager::sendCloseCorrect()
{
    QAndroidJniObject::callStaticMethod<void>(ANDROID_ACTIVITY_PATH, "closePlayerCorrect");
}

QString AndroidManager::getLauncherVersion()
{
    QAndroidJniObject s = MyActivity.callObjectMethod<jstring>("getLauncherVersion");
    return launcher_name+"-"+s.toString();
}

QString AndroidManager::getLauncherName()
{
    return launcher_name;
}

QString AndroidManager::getSmilIndexFromLauncher()
{
    QAndroidJniObject s = MyActivity.callObjectMethod<jstring>("getContentUrlFromLauncher");
    return s.toString();
}

QString AndroidManager::getUUIDFromLauncher()
{
    QAndroidJniObject s = MyActivity.callObjectMethod<jstring>("getUUIDFromLauncher");
    return s.toString();
}

QStringList AndroidManager::generateVpnKeyPair()
{
    QStringList pair;
    QAndroidJniObject result = QAndroidJniObject::callStaticObjectMethod(
        ANDROID_VPN_SERVICE_PATH,
        "generateKeyPair",
        "()[Ljava/lang/String;"
    );

    if (result.isValid()) {
        jobjectArray array = result.object<jobjectArray>();
        QAndroidJniEnvironment env;
        int count = env->GetArrayLength(array);
        for (int i = 0; i < count; ++i) {
            jstring s = (jstring)env->GetObjectArrayElement(array, i);
            pair << QAndroidJniObject(s).toString();
            env->DeleteLocalRef(s);
        }
    }
    return pair;
}

void AndroidManager::startVpnTunnel(const QString &privateKey, const QString &address, const QString &serverPubKey, const QString &endpoint, const QString &allowedIps)
{
    qDebug() << "[Wireguard][CPP] AndroidManager::startVpnTunnel() called";
    QtAndroid::runOnAndroidThread([privateKey, address, serverPubKey, endpoint, allowedIps]() {
        QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", "()L" ANDROID_ACTIVITY_PATH ";");
        
        if (!MyActivity.isValid()) {
            qCritical() << "[Wireguard][CPP] FAILED to get Activity instance via getInstance()!";
            return;
        } else {
            qDebug() << "[Wireguard][CPP] Successfully obtained Activity instance.";
        }

        QAndroidJniObject jPrivateKey = QAndroidJniObject::fromString(privateKey);
        QAndroidJniObject jAddress = QAndroidJniObject::fromString(address);
        QAndroidJniObject jServerPubKey = QAndroidJniObject::fromString(serverPubKey);
        QAndroidJniObject jEndpoint = QAndroidJniObject::fromString(endpoint);
        QAndroidJniObject jAllowedIps = QAndroidJniObject::fromString(allowedIps);

        MyActivity.callMethod<void>("startVpn", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
                                   jPrivateKey.object<jstring>(),
                                   jAddress.object<jstring>(),
                                   jServerPubKey.object<jstring>(),
                                   jEndpoint.object<jstring>(),
                                   jAllowedIps.object<jstring>());
    });
}

void AndroidManager::stopVpnTunnel()
{
    qDebug() << "[Wireguard][CPP] AndroidManager::stopVpnTunnel() called";
    QtAndroid::runOnAndroidThread([]() {
        QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", "()L" ANDROID_ACTIVITY_PATH ";");
        if (MyActivity.isValid()) {
            MyActivity.callMethod<void>("stopVpn");
        }
    });
}

void AndroidManager::openNetworkSettings()
{
#if defined Q_OS_ANDROID
    QtAndroid::runOnAndroidThread([]() {
        QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", "()L" ANDROID_ACTIVITY_PATH ";");
        if (MyActivity.isValid()) {
            MyActivity.callMethod<void>("openNetworkSettings");
        }
    });
#endif
}

void AndroidManager::startKioskMode()
{
#if defined Q_OS_ANDROID
    QtAndroid::runOnAndroidThread([]() {
        QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", "()L" ANDROID_ACTIVITY_PATH ";");
        if (MyActivity.isValid()) {
            MyActivity.callMethod<void>("startKioskMode");
        }
    });
#endif
}

void AndroidManager::exitKioskMode()
{
#if defined Q_OS_ANDROID
    QtAndroid::runOnAndroidThread([]() {
        QAndroidJniObject MyActivity = QAndroidJniObject::callStaticObjectMethod(ANDROID_ACTIVITY_PATH, "getInstance", "()L" ANDROID_ACTIVITY_PATH ";");
        if (MyActivity.isValid()) {
            MyActivity.callMethod<void>("stopKioskMode");
        }
    });
#endif
}
