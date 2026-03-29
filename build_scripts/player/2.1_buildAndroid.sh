#!/bin/bash

# set -e

# SCRIPTDIR=$(dirname "$0")
# source $SCRIPTDIR/env.sh

# # =====================================================

# export ANDROID_ARCH="NEED TO CUSTOMIZED"							# Qt >= 5.14 supports android multi-ABI builds have a look in pro-files
# export ANDROID_SDK_ROOT="NEED TO CUSTOMIZED" 					 	# Your full basic path to your Android SDK tools do not use ~/ 
# export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT=/ndk/22.1.7171670	  		# path to Google NDK use this NDK
# export QT_MKSPEC=android-clang  								    # android-clang or android-g++

# #
# export JDK_PATH=/usr/lib/jvm/java-17-openjdk-amd64					# you can use openjdk 11, too
# export JAVA_HOME=$JDK_PATH
# export PATH=$JAVA_HOME/bin:$PATH

# # =====================================================

# QT_PATH_RUNTIME=$QT_BASE_PATH/$QT_VERSION/$ANDROID_ARCH
# BUILD_DIR=$ANDROID_ARCH-$SHADOW_BUILD_DIR
# BUILD_TARGET=android-build

set -e

SCRIPTDIR=$(dirname "$0")
source $SCRIPTDIR/env.sh

# =====================================================

export ANDROID_ARCH="android"							# Qt >= 5.14 supports android multi-ABI builds have a look in pro-files
export ANDROID_SDK_ROOT="C:/Users/Training/AppData/Local/Android/Sdk" 					 	# Your full basic path to your Android SDK tools do not use ~/ 
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk-bundle	  		# path to Google NDK use this NDK
export QT_MKSPEC=android-clang  								    # android-clang or android-g++

#
export JDK_PATH="C:/Program Files/Eclipse Adoptium/jdk-11.0.28.6-hotspot"					# you can use openjdk 11, too
export JAVA_HOME=$JDK_PATH
export PATH=$JAVA_HOME/bin:$PATH

# =====================================================

QT_PATH_RUNTIME=$QT_BASE_PATH/$QT_VERSION/$ANDROID_ARCH
BUILD_DIR=$ANDROID_ARCH-$SHADOW_BUILD_DIR
BUILD_TARGET=android-build

echo 
echo ========== enter shadowbuild dir 
echo 
mkdir -p $BUILD_DIR
cd $BUILD_DIR

echo 
echo ========== prepare build 
echo 
$QT_PATH_RUNTIME/bin/qmake -r -spec $QT_MKSPEC  $GARLIC_DIR/src/complete_c2qml.pro CONFIG+=$CONFIG_DEBUG_RELEASE CONFIG+=qml_$CONFIG_DEBUG_RELEASE

echo 
echo ========== build 
echo 
make -j $DEV_JOBS --silent

echo 
echo ========== pack apk with gradle 
echo 
rm -rf $BUILD_TARGET
make INSTALL_ROOT=$BUILD_TARGET install || true

# this is for every ABI. If you want to use less uncomment the mathcin lines
# $QT_PATH_RUNTIME/bin/qmake -install qinstall -exe bin/libgarlic-player_arm64-v8a.so ./player-c2qml/$BUILD_TARGET/libs/arm64-v8a/libgarlic-player_arm64-v8a.so

# $QT_PATH_RUNTIME/bin/qmake -install qinstall -exe bin/libgarlic-player_armeabi-v7a.so ./player-c2qml/$BUILD_TARGET/libs/armeabi-v7a/libgarlic-player_armeabi-v7a.so

# $QT_PATH_RUNTIME/bin/qmake -install qinstall -exe bin/libgarlic-player_x86_64.so ./player-c2qml/$BUILD_TARGET/libs/x86_64/libgarlic-player_x86_64.so

# $QT_PATH_RUNTIME/bin/qmake -install qinstall -exe bin/libgarlic-player_x86.so ./player-c2qml/$BUILD_TARGET/libs/x86/libgarlic-player_x86.so

# for signing
KEYFILE="your key file path"
STORE_PASSWORD="your store password"
KEY_ALIAS="your alias"
KEY_PASSWORD="your key password"

$QT_PATH_RUNTIME/bin/androiddeployqt \
	--input ./player-c2qml/android-la-player-deployment-settings.json \
	--output ./player-c2qml/$BUILD_TARGET \
	--deployment bundled \
	--android-platform $ANDROID_API_VERSION \
	--jdk "$JDK_PATH"  \
	--no-gradle \
	--$CONFIG_DEBUG_RELEASE \
	--verbose || true
# Update Gradle version for compatibility with AGP 7.0.4
sed -i 's/gradle-5.6.4-bin.zip/gradle-7.0.2-bin.zip/' ./player-c2qml/$BUILD_TARGET/gradle/wrapper/gradle-wrapper.properties

# Run Gradle manually
cd ./player-c2qml/$BUILD_TARGET
./gradlew assemble${CONFIG_DEBUG_RELEASE^}
cd ../..

mv ./player-c2qml/$BUILD_TARGET/build/outputs/apk/$CONFIG_DEBUG_RELEASE/*.apk la-player-$ANDROID_ARCH-$DEPLOY_SUFFIX-$CONFIG_DEBUG_RELEASE.apk

