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

AndroidManager::AndroidManager()
{
    m_instance = this;
    MyActivity = QAndroidJniObject::callStaticObjectMethod("org/qtproject/qt5/android/QtNative", "activity", "()Landroid/app/Activity;");
    MyActivity.callMethod<void>("registerBroadcastReceiver");
}

AndroidManager* AndroidManager::instance()
{
    return m_instance;
}

// JNI Bridge
extern "C" JNIEXPORT void JNICALL
Java_com_sagiadinos_garlic_player_java_GarlicActivity_notifyVpnStateChanged(JNIEnv *env, jclass clazz, jint state)
{
    Q_UNUSED(env);
    Q_UNUSED(clazz);
    if (AndroidManager::instance()) {
        emit AndroidManager::instance()->vpnStatusChanged(state);
    }
}

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
    QAndroidJniObject::callStaticMethod<void>("com/sagiadinos/garlic/player/java/GarlicActivity", "closePlayerCorrect");
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
        "com/sagiadinos/garlic/player/java/GarlicVpnService",
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

void AndroidManager::startVpnTunnel(const QString &privateKey, const QString &address, const QString &serverPubKey, const QString &endpoint)
{
    if (!MyActivity.isValid()) return;

    QAndroidJniObject intent("android/content/Intent", "()V");
    QAndroidJniObject action = QAndroidJniObject::fromString("START");
    QAndroidJniObject serviceClass = QAndroidJniObject::fromString("com.sagiadinos.garlic.player.java.GarlicVpnService");
    
    // Set component
    QAndroidJniObject context = MyActivity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    QAndroidJniObject packageName = context.callObjectMethod("getPackageName", "()Ljava/lang/String;");
    intent.callObjectMethod("setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            packageName.object<jstring>(), serviceClass.object<jstring>());
    
    intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", action.object<jstring>());
    
    // Add extras
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            QAndroidJniObject::fromString("privateKey").object<jstring>(), QAndroidJniObject::fromString(privateKey).object<jstring>());
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            QAndroidJniObject::fromString("address").object<jstring>(), QAndroidJniObject::fromString(address).object<jstring>());
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            QAndroidJniObject::fromString("serverPubKey").object<jstring>(), QAndroidJniObject::fromString(serverPubKey).object<jstring>());
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            QAndroidJniObject::fromString("endpoint").object<jstring>(), QAndroidJniObject::fromString(endpoint).object<jstring>());

    MyActivity.callObjectMethod("startService", "(Landroid/content/Intent;)Landroid/content/ComponentName;", intent.object<jobject>());
}

void AndroidManager::stopVpnTunnel()
{
    if (!MyActivity.isValid()) return;

    QAndroidJniObject intent("android/content/Intent", "()V");
    QAndroidJniObject action = QAndroidJniObject::fromString("STOP");
    QAndroidJniObject serviceClass = QAndroidJniObject::fromString("com.sagiadinos.garlic.player.java.GarlicVpnService");
    
    QAndroidJniObject context = MyActivity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    QAndroidJniObject packageName = context.callObjectMethod("getPackageName", "()Ljava/lang/String;");
    intent.callObjectMethod("setClassName", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", 
                            packageName.object<jstring>(), serviceClass.object<jstring>());
    
    intent.callObjectMethod("setAction", "(Ljava/lang/String;)Landroid/content/Intent;", action.object<jstring>());

    MyActivity.callObjectMethod("startService", "(Landroid/content/Intent;)Landroid/content/ComponentName;", intent.object<jobject>());
}
