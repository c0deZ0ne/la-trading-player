/*
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
*/

package com.laplayer.player.java;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.graphics.Color;
import android.os.Environment;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.VpnService;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.PowerManager;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.util.Log;
import java.util.concurrent.ExecutionException;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.pm.PackageInstaller;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import android.content.BroadcastReceiver;
import java.security.MessageDigest;
import java.math.BigInteger;

public class GarlicActivity extends org.qtproject.qt5.android.bindings.QtActivity {
    private static GarlicActivity m_instance;
    private boolean is_launcher = false;
    private static LauncherInterface MyLauncherInterface = null;

    public GarlicActivity() {
        Log.d("GarlicActivity", "Constructor called");
        m_instance = this;
    }

    public static GarlicActivity getInstance() {
        if (m_instance == null) {
            Log.e("GarlicActivity", "getInstance() called but m_instance is NULL!");
        }
        return m_instance;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        m_instance = this;
        super.onCreate(savedInstanceState);

        handleIntent(getIntent());

        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable throwable) {
                Log.e("GarlicActivity", "Uncaught exception: ", throwable);

                Intent intent = new Intent(m_instance, GarlicActivity.class);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

                PendingIntent pendingIntent = PendingIntent.getActivity(
                        m_instance.getBaseContext(),
                        0,
                        intent,
                        PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_IMMUTABLE);

                AlarmManager mgr = (AlarmManager) m_instance.getBaseContext().getSystemService(Context.ALARM_SERVICE);
                mgr.set(AlarmManager.RTC, System.currentTimeMillis() + 2000, pendingIntent);

                System.exit(2);
            }
        });

        if (isGarlicLauncherInstalled()) {
            is_launcher = true;
            MyLauncherInterface = new GarlicLauncher(this);
        } else if (isPhilipsLauncherInstalled()) {
            is_launcher = true;
            MyLauncherInterface = new PhilipsLauncher(this);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            DevicePolicyManager dpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
            ComponentName adminName = new ComponentName(this, AdminReceiver.class);

            Log.d("GarlicActivity", "Checking Device Owner status...");
            if (dpm.isDeviceOwnerApp(getPackageName())) {
                Log.d("GarlicActivity", "App IS Device Owner. Whitelisting package for Lock Task...");
                dpm.setLockTaskPackages(adminName, new String[] { getPackageName() });
                if (dpm.isLockTaskPermitted(getPackageName())) {
                    Log.d("GarlicActivity", "Lock Task IS permitted. Starting Lock Task...");
                    startLockTask();
                } else {
                    Log.d("GarlicActivity", "Lock Task IS NOT permitted for this package.");
                }

                // Zero-Touch VPN: Silently grant VPN permissions for this package
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                    try {
                        Log.d("GarlicActivity", "Granting Zero-Touch VPN permission...");
                        dpm.setAlwaysOnVpnPackage(adminName, getPackageName(), false);
                    } catch (Exception e) {
                        Log.e("GarlicActivity", "Failed to set Always-On VPN: " + e.getMessage());
                    }
                }
            } else {
                Log.d("GarlicActivity", "App IS NOT Device Owner.");
            }
        }

        hideSystemUI();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleIntent(intent);
    }

    private void handleIntent(Intent intent) {
        if (intent == null)
            return;
        String updateUrl = intent.getStringExtra("updateUrl");
        String sha256 = intent.getStringExtra("sha256");
        int versionCode = intent.getIntExtra("versionCode", 0);
        if (updateUrl != null && !updateUrl.isEmpty()) {
            Log.i("GarlicActivity", "Internal Intent Triggered OTA: " + updateUrl + " versionCode=" + versionCode);
            downloadAndInstall(updateUrl, sha256 != null ? sha256 : "", versionCode);
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemUI();
            startKioskMode(); // Auto-resume kiosk mode when regaining focus from Settings
        }
    }

    private void hideSystemUI() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            window.setStatusBarColor(Color.TRANSPARENT);
            window.setNavigationBarColor(Color.TRANSPARENT);
            window.addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams layoutParams = getWindow().getAttributes();
            layoutParams.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(layoutParams);
        }
    }

    public void registerBroadcastReceiver() {
        IntentFilter filter = new IntentFilter("com.laplayer.player.java.ConfigReceiver");
        ConfigReceiver MyConfigReceiver = new ConfigReceiver();
        registerReceiver(MyConfigReceiver, filter);

        // needed when USB Stick enters with local SMIL File
        IntentFilter filter2 = new IntentFilter("com.laplayer.player.java.SmilIndexReceiver");
        SmilIndexReceiver MySmilIndexReceiver = new SmilIndexReceiver();
        registerReceiver(MySmilIndexReceiver, filter2);
    }

    public void fetchDeviceInformation() {
        MyLauncherInterface.fetchDeviceInformation();
    }

    public boolean isLauncherInstalled() {
        return is_launcher;
    }

    public String getContentUrlFromLauncher() {
        return MyLauncherInterface.getContentUrlFromLauncher();
    }

    public String getUUIDFromLauncher() {
        return MyLauncherInterface.getUUIDFromLauncher();
    }

    public String getLauncherVersion() {
        return MyLauncherInterface.getLauncherVersion();
    }

    public String getLauncherName() {
        return MyLauncherInterface.getLauncherName();
    }

    public static void setScreenOff() {
        MyLauncherInterface.setScreenOff();
    }

    public static void setScreenOn() {
        MyLauncherInterface.setScreenOn();
    }

    public static void activateDeepStandBy(String seconds_to_wakeup) {
        MyLauncherInterface.activateDeepStandBy(seconds_to_wakeup);
    }

    public static void rebootOS(String task_id) {
        MyLauncherInterface.rebootOS(task_id);
    }

    public static void installSoftware(String apk_path) {
        MyLauncherInterface.installSoftware(apk_path);
    }

    public static void closePlayerCorrect() {
        Intent intent = new Intent("com.laplayer.launcher.receiver.PlayerClosedReceiver");
        intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        m_instance.sendBroadcast(intent);
    }

    public static void applyConfig(String config_path) {
        Intent intent = new Intent("com.laplayer.launcher.receiver.ConfigXMLReceiver");
        intent.putExtra("config_path", config_path);
        m_instance.sendBroadcast(intent);
    }

    public static void startSecondApp(String package_name) {
        Intent intent = new Intent("com.laplayer.launcher.receiver.SecondAppReceiver");
        intent.putExtra("package_name", package_name);
        m_instance.sendBroadcast(intent);
    }

    private boolean isGarlicLauncherInstalled() {
        return isPackageInstalled("com.laplayer.launcher");
    }

    private boolean isPhilipsLauncherInstalled() {
        return isPackageInstalled("com.tpv.app.tpvlauncher");
    }

    private boolean isPackageInstalled(String targetPackage) {
        PackageManager pm = m_instance.getPackageManager();
        try {
            pm.getPackageInfo(targetPackage, PackageManager.GET_META_DATA);
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
        return true;
    }

    private String m_vpnPrivateKey;
    private String m_vpnAddress;
    private String m_vpnServerPubKey;
    private String m_vpnEndpoint;
    private String m_vpnAllowedIps;

    public void startVpn(final String privateKey, final String address, final String serverPubKey,
            final String endpoint, final String allowedIps) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                showToast("VPN START SIGNAL RECEIVED");
                Log.i("GarlicActivity", "===> startVpn() reached. endpoint=" + endpoint + " address=" + address);

                m_vpnPrivateKey = privateKey;
                m_vpnAddress = address;
                m_vpnServerPubKey = serverPubKey;
                m_vpnEndpoint = endpoint;
                m_vpnAllowedIps = allowedIps;

                // Notify C++ layer that we are now Connecting (guarded against JNI link errors)
                try {
                    Log.i("GarlicActivity", "Calling notifyVpnStateChanged(1)...");
                    notifyVpnStateChanged(1);
                    Log.i("GarlicActivity", "notifyVpnStateChanged(1) succeeded.");
                } catch (UnsatisfiedLinkError e) {
                    Log.e("GarlicActivity", "CRITICAL: notifyVpnStateChanged JNI not linked: " + e.getMessage());
                } catch (Exception e) {
                    Log.e("GarlicActivity", "CRITICAL: notifyVpnStateChanged threw: " + e.getMessage());
                }

                Log.i("GarlicActivity", "Calling VpnService.prepare()...");
                Intent intent = VpnService.prepare(GarlicActivity.this);
                if (intent != null) {
                    Log.i("GarlicActivity", "VPN not prepared, starting activity for result (permission dialog)");
                    startActivityForResult(intent, 1024);
                } else {
                    Log.i("GarlicActivity", "VPN already prepared, calling startVpnInternal()");
                    startVpnInternal();
                }
            }
        });
    }

    private void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                android.widget.Toast.makeText(GarlicActivity.this, message, android.widget.Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void startVpnInternal() {
        Log.d("GarlicActivity", "startVpnInternal: Prepared Intent for service. Action=START");
        Intent intent = new Intent(this, GarlicVpnService.class);
        intent.setAction("START");
        intent.putExtra("privateKey", m_vpnPrivateKey);
        intent.putExtra("address", m_vpnAddress);
        intent.putExtra("serverPubKey", m_vpnServerPubKey);
        intent.putExtra("endpoint", m_vpnEndpoint);
        intent.putExtra("allowedIps", m_vpnAllowedIps);

        Log.d("GarlicActivity", "Intent Extras: pkg=" + getPackageName() + ", endpoint=" + m_vpnEndpoint
                + ", addresses=" + m_vpnAddress + ", allowed=" + m_vpnAllowedIps);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1024) {
            if (resultCode == RESULT_OK) {
                Log.i("GarlicActivity", "VPN permission GRANTED by user — waiting 600ms for OS to commit...");
                // KEY FIX: Delay the service start by 600ms so Android has time
                // to fully commit the VPN permission grant before GoBackend.establish()
                // is called. Without this, GoBackend races against the system and fails.
                new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        Log.i("GarlicActivity", "Delay complete — calling startVpnInternal()");
                        startVpnInternal();
                    }
                }, 600);
            } else {
                Log.e("GarlicActivity", "VPN permission DENIED by user");
                try {
                    notifyVpnError("VPN permission denied by user");
                } catch (UnsatisfiedLinkError e) {
                    Log.e("GarlicActivity", "notifyVpnError JNI not linked");
                }
                try {
                    notifyVpnStateChanged(0);
                } catch (UnsatisfiedLinkError e) {
                    Log.e("GarlicActivity", "notifyVpnStateChanged JNI not linked");
                }
            }
        }
    }

    public void stopVpn() {
        Log.i("GarlicActivity", "Requesting VPN Stop");
        showToast("VPN STOP SIGNAL RECEIVED");
        Intent intent = new Intent(this, GarlicVpnService.class);
        intent.setAction("STOP");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(intent);
        } else {
            startService(intent);
        }
    }

    public void stopKioskMode() {
        Log.i("GarlicActivity", "Stopping Kiosk Mode (Lock Task)...");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            try {
                stopLockTask();
            } catch (Exception e) {
                Log.e("GarlicActivity", "Failed to stop lock task", e);
            }
        }
    }

    public void startKioskMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            try {
                DevicePolicyManager dpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
                if (dpm != null && dpm.isLockTaskPermitted(getPackageName())) {
                    startLockTask();
                }
            } catch (Exception e) {
                Log.e("GarlicActivity", "Failed to start lock task", e);
            }
        }
    }

    public void openNetworkSettings() {
        Log.i("GarlicActivity", "Opening Network Settings - Suspending Kiosk Mode");
        stopKioskMode();
        Intent intent = new Intent(android.provider.Settings.ACTION_WIFI_SETTINGS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
    }

    // Called from C++ JNI — versionCode=0 means "unknown, skip version check"
    public void downloadAndInstall(final String urlString, final String expectedSha256) {
        downloadAndInstall(urlString, expectedSha256, 0);
    }

    public void downloadAndInstall(final String urlString, final String expectedSha256, final int requestedVersionCode) {
        // --- CLIENT-SIDE VERSION GUARD ---
        // Refuse to download if the pushed version is not strictly newer than what is installed.
        if (requestedVersionCode > 0) {
            try {
                int installedVersionCode = getPackageManager()
                        .getPackageInfo(getPackageName(), 0).versionCode;
                if (requestedVersionCode <= installedVersionCode) {
                    Log.i("GarlicActivity", "OTA SKIPPED: pushed version " + requestedVersionCode
                            + " is not newer than installed " + installedVersionCode);
                    return;
                }
                Log.i("GarlicActivity", "OTA ACCEPTED: updating from " + installedVersionCode
                        + " to " + requestedVersionCode);
            } catch (Exception e) {
                Log.w("GarlicActivity", "Could not read installed versionCode: " + e.getMessage());
            }
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.i("GarlicActivity", "Starting Resumable OTA Download: " + urlString);
                    File outputFile = new File(getExternalFilesDir(null), "update.apk");
                    long existingSize = 0;
                    if (outputFile.exists()) {
                        existingSize = outputFile.length();
                        Log.i("GarlicActivity", "Partial download found: " + existingSize + " bytes");
                    }

                    URL url = new URL(urlString);
                    HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                    connection.setConnectTimeout(15000); // 15s to establish connection
                    connection.setReadTimeout(60000);    // 60s to wait for data (handles stalls)

                    // Set Range header if we have partial file
                    if (existingSize > 0) {
                        connection.setRequestProperty("Range", "bytes=" + existingSize + "-");
                    }

                    connection.connect();

                    int responseCode = connection.getResponseCode();
                    Log.i("GarlicActivity", "HTTP Response Code: " + responseCode);

                    boolean isResume = (responseCode == HttpURLConnection.HTTP_PARTIAL);
                    boolean isNew = (responseCode == HttpURLConnection.HTTP_OK);

                    if (!isResume && !isNew) {
                        Log.e("GarlicActivity", "Server does not support resume or returned error: " + responseCode);
                        // If 416 (Requested Range Not Satisfiable), just delete and restart
                        if (responseCode == 416) {
                            outputFile.delete();
                            downloadAndInstall(urlString, expectedSha256);
                            return;
                        }
                        return;
                    }

                    long totalToDownload = connection.getContentLength();
                    Log.i("GarlicActivity", "Content-Length to download: " + totalToDownload);

                    long totalRead = isResume ? existingSize : 0;
                    long finalTotal = isResume ? (totalToDownload + existingSize) : totalToDownload;

                    try (InputStream input = connection.getInputStream();
                            OutputStream output = new FileOutputStream(outputFile, isResume)) {
                        byte[] data = new byte[131072]; // 128KB buffer for better throughput
                        int count;
                        long lastReportTime = 0;
                        
                        while ((count = input.read(data)) != -1) {
                            output.write(data, 0, count);
                            totalRead += count;
                            
                            // Log and notify UI at most every 500ms or 512KB
                            long now = System.currentTimeMillis();
                            if (now - lastReportTime > 500 || totalRead == finalTotal) {
                                lastReportTime = now;
                                Log.i("GarlicActivity", "Download progress: " + (totalRead / 1024) + " KB");
                                try {
                                    notifyOtaProgress(totalRead, finalTotal);
                                } catch (UnsatisfiedLinkError e) {
                                    // JNI not linked yet, ignore
                                }
                            }
                        }
                        output.flush();
                        Log.i("GarlicActivity", "Download finished. Total size on disk: " + outputFile.length());
                    }

                    // PROFESSIONAL VERIFICATION: Match exact byte count from Content-Length header
                    // If finalTotal is -1 (Chunked Encoding), we skip size check but still proceed to SHA check
                    if (finalTotal == -1 || (finalTotal > 0 && outputFile.length() == finalTotal)) {
                        Log.i("GarlicActivity", "Download complete (Size verified or Chunked). Checking SHA-256...");
                        
                        if (expectedSha256 != null && !expectedSha256.isEmpty()) {
                            String actualSha = calculateFileSha256(outputFile);
                            if (actualSha.equalsIgnoreCase(expectedSha256)) {
                                Log.i("GarlicActivity", "SHA-256 VERIFIED. Starting install...");
                                performSilentInstall(outputFile.getAbsolutePath());
                            } else {
                                Log.e("GarlicActivity", "SHA-256 MISMATCH! Expected: " + expectedSha256 + " Actual: " + actualSha);
                                outputFile.delete(); // Delete bad file
                            }
                        } else {
                            Log.w("GarlicActivity", "No SHA-256 provided. Installing based on successful stream completion...");
                            performSilentInstall(outputFile.getAbsolutePath());
                        }
                    } else {
                        Log.e("GarlicActivity", "Download verification FAILED. Size on disk: " + outputFile.length()
                                + " Expected from server: " + finalTotal);
                    }

                } catch (Exception e) {
                    Log.e("GarlicActivity", "OTA Download error: " + e.getMessage());
                }
            }
        }).start();
    }

    private String calculateFileSha256(File file) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            InputStream is = new FileInputStream(file);
            byte[] buffer = new byte[65536];
            int read;
            while ((read = is.read(buffer)) > 0) {
                digest.update(buffer, 0, read);
            }
            is.close();
            byte[] hash = digest.digest();
            return String.format("%064x", new BigInteger(1, hash));
        } catch (Exception e) {
            Log.e("GarlicActivity", "Failed to calculate SHA-256", e);
            return "";
        }
    }

    public void installApk(final String apkPath) {
        Log.i("GarlicActivity", "installApk() called from C++: " + apkPath);
        performSilentInstall(apkPath);
    }

    private void performSilentInstall(final String apkPath) {
        // Strategy 1: DevicePolicyManager silent install (works if app is Device Owner)
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
            try {
                DevicePolicyManager dpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
                if (dpm != null && dpm.isDeviceOwnerApp(getPackageName())) {
                    Log.i("GarlicActivity", "Device Owner detected — using DPM silent install");

                    // Use application context for PackageInstaller to survive process death
                    PackageInstaller packageInstaller = getApplicationContext()
                            .getPackageManager().getPackageInstaller();

                    PackageInstaller.SessionParams params = new PackageInstaller.SessionParams(
                            PackageInstaller.SessionParams.MODE_FULL_INSTALL);
                    // Tell the system this is a non-incremental install with a known size
                    File apkFile = new File(apkPath);
                    long apkSize = apkFile.length();
                    params.setSize(apkSize);

                    int sessionId = packageInstaller.createSession(params);
                    Log.i("GarlicActivity", "PackageInstaller session created: " + sessionId);

                    try (PackageInstaller.Session session = packageInstaller.openSession(sessionId)) {
                        try (OutputStream out = session.openWrite("OTA_UPDATE", 0, apkSize);
                                java.io.InputStream in = new FileInputStream(apkFile)) {
                            byte[] buffer = new byte[65536];
                            int c;
                            long written = 0;
                            while ((c = in.read(buffer)) != -1) {
                                out.write(buffer, 0, c);
                                written += c;
                            }
                            session.fsync(out);
                            Log.i("GarlicActivity", "APK written to session: " + written + " bytes");
                        }

                        Intent broadcastIntent = new Intent(getApplicationContext(), InstallationReceiver.class);
                        broadcastIntent.setFlags(Intent.FLAG_RECEIVER_FOREGROUND);
                        PendingIntent pi = PendingIntent.getBroadcast(
                                getApplicationContext(), sessionId, broadcastIntent,
                                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);

                        session.commit(pi.getIntentSender());
                        Log.i("GarlicActivity", "DPM install session committed. Waiting for system to apply...");
                    }
                    return;
                } else {
                    Log.w("GarlicActivity", "Not a Device Owner — falling back to install intent");
                }
            } catch (Exception e) {
                Log.e("GarlicActivity", "DPM install failed, falling back to intent: " + e.getMessage(), e);
            }
        }

        // Strategy 2: ACTION_VIEW intent — temporarily suspend kiosk so installer UI
        // can surface
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.i("GarlicActivity", "Suspending Kiosk for OTA install prompt...");
                    stopKioskMode();

                    // Using FileProvider required for Android 7+ (API 24+) to avoid
                    // FileUriExposedException
                    android.net.Uri apkUri = androidx.core.content.FileProvider.getUriForFile(
                            getApplicationContext(),
                            getPackageName() + ".fileprovider",
                            new File(apkPath));

                    Intent installIntent = new Intent(Intent.ACTION_VIEW);
                    installIntent.setDataAndType(apkUri, "application/vnd.android.package-archive");
                    installIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    installIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                    startActivity(installIntent);

                    Log.i("GarlicActivity", "Install intent launched. Kiosk will resume after install.");
                } catch (Exception e) {
                    Log.e("GarlicActivity", "Install intent failed: " + e.getMessage());
                    // Re-enable kiosk if intent failed
                    startKioskMode();
                }
            }
        });
    }

    public static class InstallationReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int status = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
            String message = intent.getStringExtra(PackageInstaller.EXTRA_STATUS_MESSAGE);
            Log.i("GarlicActivity", "InstallationReceiver: status=" + status + " message=" + message);

            if (status == PackageInstaller.STATUS_SUCCESS) {
                Log.i("GarlicActivity", "UPDATE SUCCESSFUL. OS will restart the app.");
            } else {
                Log.e("GarlicActivity", "UPDATE FAILED: " + message);
            }
        }
    }

    public static native void notifyVpnStateChanged(int state);

    public static native void notifyVpnError(String message);

    public static int getVpnState() {
        return GarlicVpnService.getVpnState();
    }

    public static native void notifyOtaProgress(long received, long total);
}
