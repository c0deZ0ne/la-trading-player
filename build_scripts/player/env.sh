#!/bin/bash

set -e
SCRIPTDIR=$(dirname "$0")

export QT_BASE_PATH="C:/Qt"						# path to your qt base directory
export QT_VERSION=5.15.2										# The Qt Version 5.7, 5.8, 5.9.2 etc
export CONFIG_DEBUG_RELEASE=debug
export DEV_JOBS=1			# determine how many cores can be used
export ANDROID_API_VERSION=android-31

if [ -z "QT_BASE_PATH" ]; then
	echo Error: Set the correct paths in QT_BASE_PATH 
	exit 1;
fi
##################################################################

# create directory name
export SHADOW_BUILD_DIR=build-$QT_VERSION-$CONFIG_DEBUG_RELEASE


# check if called from jenkins to set correct paths
if [ -z "$BUILD_NUMBER" ]; then
	export GARLIC_DIR="c:/Users/Training/Desktop/garlic-player/"
	source $SCRIPTDIR/writeVersionFromGithub.sh 
else
	export GARLIC_DIR=$PWD
	source $SCRIPTDIR/writeVersionFromGithub.sh
fi

export DEPLOY_SUFFIX=$GARLIC_VERSION

# create diretory for deployed files
export PACKAGE_DIR=../packages
mkdir -p packages
