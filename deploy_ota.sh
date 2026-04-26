#!/bin/bash

# Configuration
VERSION_CODE=""
SKIP_BUILD=false

for arg in "$@"; do
    if [[ "$arg" == "--skip-build" ]]; then
        SKIP_BUILD=true
    elif [[ "$arg" =~ ^[0-9]+$ ]]; then
        VERSION_CODE="$arg"
    fi
done

# If no numeric version provided, extract from latest git tag (e.g., v1.0.1006 -> 1006)
if [ -z "$VERSION_CODE" ]; then
    LATEST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "v1.0.1000")
    VERSION_CODE=$(echo $LATEST_TAG | awk -F. '{print $NF}')
fi

BRANDING="LAPlayer"
VPC_IP="107.172.34.199"

echo "===================================================="
echo "    🚀 ONE-CLICK OTA DEPLOYMENT (Version: $VERSION_CODE)    "
if [ "$SKIP_BUILD" = true ]; then
    echo "    (SKIPPING BUILD STEP)                             "
fi
echo "===================================================="

# 1. Build the APK
if [ "$SKIP_BUILD" = false ]; then
    echo "[*] Step 1: Building Android APK..."
    export FORCE_VERSION_CODE=$VERSION_CODE
    export BRANDING=$BRANDING
    bash build_scripts/player/2.1_buildAndroid.sh
    if [ $? -ne 0 ]; then
        echo "[-] Error: Build script failed. Aborting deployment."
        exit 1
    fi
else
    echo "[*] Step 1: Skipping Build (Using existing APK)..."
fi

APK_FILE=$(find . -maxdepth 2 -name "la-player-android-*.$VERSION_CODE-debug.apk" | head -n 1)

if [ ! -f "$APK_FILE" ]; then
    echo "[-] Error: APK file for version $VERSION_CODE not found in root directory!"
    echo "    Wanted pattern: la-player-android-*.$VERSION_CODE-debug.apk"
    exit 1
fi

echo "[+] Build Successful: $APK_FILE"

# # 3. Upload to Cloudflare R2
# echo "[*] Step 2: Uploading APK to Cloudflare R2..."
# echo "[*] Using R2 Bridge: scratch/r2_upload.py"

# # Calculate the remote key (e.g., tenants/global/la-player-android-v1.0.1004-debug.apk)
# REMOTE_KEY="tenants/global/$(basename "$APK_FILE")"

# # Execute the upload using python
# python ../scratch/r2_upload.py "$APK_FILE" "$REMOTE_KEY"
# if [ $? -ne 0 ]; then
#     echo "[-] Error: Failed to upload APK to Cloudflare R2."
#     exit 1
# fi
# echo "[+] R2 Upload Successful."

# # 4. Update Database Metadata
# echo "[*] Step 3: Updating OTA Metadata in VPC Database..."
# echo "[*] Using SCP to run remote metadata sync..."
# # We still need SSH for the DB update part, but it's just a tiny 1KB script call now
# python ../scratch/fix_ota_metadata.py $VERSION_CODE

# if [ $? -ne 0 ]; then
#     echo "[-] Error: Failed to update database metadata."
#     exit 1
# fi

# echo "===================================================="
# echo "    ✨ DEPLOYMENT COMPLETE! VERSION $VERSION_CODE IS LIVE    "
# echo "    Check Swagger ota-check to confirm.             "
# echo "===================================================="
