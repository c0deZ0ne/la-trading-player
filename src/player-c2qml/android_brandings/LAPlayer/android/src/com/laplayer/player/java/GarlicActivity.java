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


public class GarlicActivity extends org.qtproject.qt5.android.bindings.QtActivity
{
    private static GarlicActivity m_instance;
    private boolean is_launcher     = false;
    private static LauncherInterface MyLauncherInterface = null;

    public GarlicActivity()
    {
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
    public void onCreate(Bundle savedInstanceState)
    {
        m_instance = this;
        super.onCreate(savedInstanceState);

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
                    PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_IMMUTABLE
                );
                
                AlarmManager mgr = (AlarmManager) m_instance.getBaseContext().getSystemService(Context.ALARM_SERVICE);
                mgr.set(AlarmManager.RTC, System.currentTimeMillis() + 2000, pendingIntent);
                
                System.exit(2);
            }
        });

        if (isGarlicLauncherInstalled())
        {
            is_launcher = true;
            MyLauncherInterface = new GarlicLauncher(this);
        }
        else if (isPhilipsLauncherInstalled())
        {
            is_launcher = true;
            MyLauncherInterface = new PhilipsLauncher(this);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            DevicePolicyManager dpm = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);
            ComponentName adminName = new ComponentName(this, AdminReceiver.class);

            Log.d("GarlicActivity", "Checking Device Owner status...");
            if (dpm.isDeviceOwnerApp(getPackageName())) {
                Log.d("GarlicActivity", "App IS Device Owner. Whitelisting package for Lock Task...");
                dpm.setLockTaskPackages(adminName, new String[]{getPackageName()});
                if (dpm.isLockTaskPermitted(getPackageName())) {
                    Log.d("GarlicActivity", "Lock Task IS permitted. Starting Lock Task...");
                    startLockTask();
                } else {
                    Log.d("GarlicActivity", "Lock Task IS NOT permitted for this package.");
                }
            } else {
                Log.d("GarlicActivity", "App IS NOT Device Owner.");
            }
        }

        hideSystemUI();
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
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
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

     public void registerBroadcastReceiver()
    {
        IntentFilter filter = new IntentFilter("com.laplayer.player.java.ConfigReceiver");
        ConfigReceiver MyConfigReceiver = new ConfigReceiver();
        registerReceiver(MyConfigReceiver, filter);

        // needed when USB Stick enters with local SMIL File
        IntentFilter filter2 = new IntentFilter("com.laplayer.player.java.SmilIndexReceiver");
        SmilIndexReceiver MySmilIndexReceiver = new SmilIndexReceiver();
        registerReceiver(MySmilIndexReceiver, filter2);
    }


    public void fetchDeviceInformation()
    {
        MyLauncherInterface.fetchDeviceInformation();
    }

    public boolean isLauncherInstalled()
    {
        return is_launcher;
    }

    public String getContentUrlFromLauncher()
    {
        return MyLauncherInterface.getContentUrlFromLauncher();
    }

    public String getUUIDFromLauncher()
    {
        return MyLauncherInterface.getUUIDFromLauncher();
    }

    public String getLauncherVersion()
    {
        return MyLauncherInterface.getLauncherVersion();
    }

    public String getLauncherName()
    {
        return MyLauncherInterface.getLauncherName();
    }

    public static void setScreenOff()
    {
        MyLauncherInterface.setScreenOff();
    }

    public static void setScreenOn()
    {
        MyLauncherInterface.setScreenOn();
    }

    public static void activateDeepStandBy(String seconds_to_wakeup)
    {
        MyLauncherInterface.activateDeepStandBy(seconds_to_wakeup);
    }

    public static void rebootOS(String task_id)
    {
        MyLauncherInterface.rebootOS(task_id);
    }

    public static void installSoftware(String apk_path)
    {
        MyLauncherInterface.installSoftware(apk_path);
    }

    public static void closePlayerCorrect()
    {
         Intent intent = new Intent("com.laplayer.launcher.receiver.PlayerClosedReceiver");
         intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
         m_instance.sendBroadcast(intent);
    }

    public static void applyConfig(String config_path)
    {
        Intent intent = new Intent("com.laplayer.launcher.receiver.ConfigXMLReceiver");
        intent.putExtra("config_path", config_path);
        m_instance.sendBroadcast(intent);
    }

    public static void startSecondApp(String package_name)
    {
         Intent intent = new Intent("com.laplayer.launcher.receiver.SecondAppReceiver");
         intent.putExtra("package_name", package_name);
         m_instance.sendBroadcast(intent);
    }

    private boolean isGarlicLauncherInstalled()
    {
        return isPackageInstalled("com.laplayer.launcher");
    }

    private boolean isPhilipsLauncherInstalled()
    {
        return isPackageInstalled("com.tpv.app.tpvlauncher");
    }

    private boolean isPackageInstalled(String targetPackage)
    {
        PackageManager pm = m_instance.getPackageManager();
        try
        {
            pm.getPackageInfo(targetPackage, PackageManager.GET_META_DATA);
        }
        catch (PackageManager.NameNotFoundException e)
        {
            return false;
        }
        return true;
    }

    private String m_vpnPrivateKey;
    private String m_vpnAddress;
    private String m_vpnServerPubKey;
    private String m_vpnEndpoint;
    private String m_vpnAllowedIps;

    public void startVpn(final String privateKey, final String address, final String serverPubKey, final String endpoint, final String allowedIps) {
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
        
        Log.d("GarlicActivity", "Intent Extras: pkg=" + getPackageName() + ", endpoint=" + m_vpnEndpoint + ", addresses=" + m_vpnAddress + ", allowed=" + m_vpnAllowedIps);
        
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
                try { notifyVpnError("VPN permission denied by user"); } catch (UnsatisfiedLinkError e) { Log.e("GarlicActivity", "notifyVpnError JNI not linked"); }
                try { notifyVpnStateChanged(0); } catch (UnsatisfiedLinkError e) { Log.e("GarlicActivity", "notifyVpnStateChanged JNI not linked"); }
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

    public static native void notifyVpnStateChanged(int state);
    public static native void notifyVpnError(String message);
}
