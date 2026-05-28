#!/bin/bash

echo 
echo ========== detect version information from github
echo
# check if called from jenkins
if [ -z "$BUILD_NUMBER" ]; then
	export GIT_DIR=$GARLIC_DIR
else
	export GIT_DIR=$WORKSPACE/
fi
VERSION_NAME=`git --git-dir="$GIT_DIR/.git" describe --tags $(git --git-dir="$GIT_DIR/.git" rev-list --tags --max-count=1) 2>/dev/null || echo "v1.0"`
BASE_VERSION=$(echo $VERSION_NAME | sed -E 's/\.[0-9]+$//')

# Use FORCE_VERSION_CODE if provided, otherwise count git commits
if [ ! -z "$FORCE_VERSION_CODE" ]; then
    COMMIT_NUMBER=$FORCE_VERSION_CODE
    echo "Forcing version code to: $COMMIT_NUMBER"
    # Always construct from tag base + forced code — never reuse the existing tag as-is,
    # because a suffix match like "1013" ending in "3" would silently swallow the forced value.
    export GARLIC_VERSION="$BASE_VERSION.$COMMIT_NUMBER"
else
    COMMIT_NUMBER=`git --git-dir="$GIT_DIR/.git" rev-list --all --count`
    # If the tag's last component already equals the commit count, use the tag as-is
    TAG_LAST=$(echo $VERSION_NAME | awk -F. '{print $NF}')
    if [[ "$TAG_LAST" == "$COMMIT_NUMBER" ]]; then
        export GARLIC_VERSION=$VERSION_NAME
    else
        export GARLIC_VERSION="$BASE_VERSION.$COMMIT_NUMBER"
    fi
fi

echo 
echo ========== write header file and version information to Androidmanifest
echo 

echo "#define version_from_git \"$GARLIC_VERSION\"" > $GIT_DIR/src/garlic-lib/version.h

# Determine the correct manifest path based on the branding (defaulting to LAPlayer)
MANIFEST_PATH="$GIT_DIR/src/player-c2qml/android_brandings/${BRANDING:-LAPlayer}/android/AndroidManifest.xml"

if [ -f "$MANIFEST_PATH" ]; then
    echo "Updating Manifest: $MANIFEST_PATH"
    # Update versionName and versionCode using sed (Standard on Git Bash)
    sed -i "s/android:versionName=\"[^\"]*\"/android:versionName=\"$GARLIC_VERSION\"/" "$MANIFEST_PATH"
    sed -i "s/android:versionCode=\"[^\"]*\"/android:versionCode=\"$COMMIT_NUMBER\"/" "$MANIFEST_PATH"
else
    echo "Warning: Manifest not found at $MANIFEST_PATH"
fi
