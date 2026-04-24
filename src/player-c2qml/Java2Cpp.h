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
#include <qglobal.h>
#if defined Q_OS_ANDROID

#ifndef JAVA2CPP_H
#define JAVA2CPP_H

#include <jni.h>
#include "lib_facade.h"


// C++ Linkage members
extern LibFacade *GlobalLibfacede;

inline void setGlobalLibFaceForJava(LibFacade *glf)
{
    GlobalLibfacede = glf;
}

#ifdef __cplusplus
extern "C" {
#endif

inline JNIEXPORT void JNICALL Java_com_sagiadinos_garlic_player_java_ConfigReceiver_getConfigPath(
        JNIEnv *env /*env*/,
        jobject /*this_obj*/,
        jstring path)
{
    const char *str = env->GetStringUTFChars(path, NULL);
    GlobalLibfacede->setConfigFromExternal(str);

}

inline JNIEXPORT void JNICALL Java_com_laplayer_player_java_ConfigReceiver_getConfigPath(
        JNIEnv *env /*env*/,
        jobject /*this_obj*/,
        jstring path)
{
    Java_com_sagiadinos_garlic_player_java_ConfigReceiver_getConfigPath(env, NULL, path);
}

// needed when you have local index on usb for
inline JNIEXPORT void JNICALL Java_com_sagiadinos_garlic_player_java_SmilIndexReceiver_getSmilIndexPath(
        JNIEnv *env /*env*/,
        jobject /*this_obj*/,
        jstring path)
{
    QString str(env->GetStringUTFChars(path, NULL));
    GlobalLibfacede->reloadWithNewIndex(str);
}

inline JNIEXPORT void JNICALL Java_com_laplayer_player_java_SmilIndexReceiver_getSmilIndexPath(
        JNIEnv *env /*env*/,
        jobject /*this_obj*/,
        jstring path)
{
    Java_com_sagiadinos_garlic_player_java_SmilIndexReceiver_getSmilIndexPath(env, NULL, path);
}

inline JNIEXPORT void JNICALL Java_com_laplayer_player_java_GarlicActivity_notifyOtaProgress(
        JNIEnv *env /*env*/,
        jobject /*this_obj*/,
        jlong received,
        jlong total)
{
    if (GlobalLibfacede != nullptr) {
        GlobalLibfacede->notifyOtaProgress((qint64)received, (qint64)total);
    }
}


#ifdef __cplusplus
}
#endif
#endif // JAVA2CPP_H
#endif
