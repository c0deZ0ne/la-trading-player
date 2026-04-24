package com.laplayer.player.java;

import android.content.Intent;
import android.net.VpnService;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;

import com.wireguard.android.backend.GoBackend;
import com.wireguard.android.backend.Statistics;
import com.wireguard.android.backend.Tunnel;
import com.wireguard.config.Config;
import com.wireguard.config.Interface;
import com.wireguard.config.Peer;
import com.wireguard.crypto.Key;
import com.wireguard.crypto.KeyPair;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class GarlicVpnService extends VpnService implements Tunnel {
    private static final String TAG = "GarlicVpnService";
    private GoBackend backend;
    private static GarlicVpnService instance;
    private ExecutorService executorService;

    public static int getVpnState() {
        if (instance != null && instance.backend != null) {
            try {
                Tunnel.State state = instance.backend.getState(instance);
                if (state == Tunnel.State.UP) return 2; // Connected
                if (state == Tunnel.State.DOWN) return 0; // Disconnected
            } catch (Exception e) {
                Log.e(TAG, "Error getting VPN state", e);
            }
        }
        return 0;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        executorService = Executors.newSingleThreadExecutor();
        try {
            backend = new GoBackend(this);
            Log.i(TAG, "VPN Service Created and GoBackend initialized successfully");
        } catch (Exception e) {
            Log.e(TAG, "FAILED to initialize GoBackend: " + e.getMessage());
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        startForeground(1, createNotification());
        if (intent != null) {
            String action = intent.getAction();
            if ("START".equals(action)) {
                startVpn(intent);
            } else if ("STOP".equals(action)) {
                stopVpn();
            }
        } else {
            // START_STICKY replay after OS killed the service — no config available,
            // do nothing and let the C++ layer re-initiate when it detects status=DOWN.
            Log.w(TAG, "onStartCommand: null intent (START_STICKY replay). Ignoring — C++ layer will retry.");
        }
        return START_STICKY;
    }

    @Override
    public void onDestroy() {
        if (executorService != null) {
            executorService.shutdown();
        }
        super.onDestroy();
    }

    private Notification createNotification() {
        String channelId = "vpn_channel";
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    channelId,
                    "VPN Service",
                    NotificationManager.IMPORTANCE_LOW);
            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
            return new Notification.Builder(this, channelId)
                    .setContentTitle("LA VPN")
                    .setContentText("VPN is securely connected and running")
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .build();
        } else {
            return new Notification.Builder(this)
                    .setContentTitle("LA VPN")
                    .setContentText("VPN is securely connected and running")
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .build();
        }
    }

    private void startVpn(final Intent intent) {
        Log.i(TAG, "VPN Start Command Received, queuing task to ExecutorService...");

        executorService.submit(new Runnable() {
            @Override
            public void run() {
                try {
                    // If already UP, notify UI immediately to ensure sync
                    if (backend != null && backend.getState(GarlicVpnService.this) == Tunnel.State.UP) {
                        Log.i(TAG, "startVpn: VPN already UP. Manually triggering state notification (2).");
                        GarlicActivity.notifyVpnStateChanged(2);
                        // We still continue to ensure any config updates are applied
                    }

                    String privateKey = intent.getStringExtra("privateKey");
                    String address = intent.getStringExtra("address");
                    String serverPubKey = intent.getStringExtra("serverPubKey");
                    String endpoint = intent.getStringExtra("endpoint");
                    String allowedIps = intent.getStringExtra("allowedIps");

                    Log.d(TAG, "Config: server=" + endpoint + ", addresses=" + address + ", allowed=" + allowedIps);

                    if (privateKey == null || privateKey.isEmpty())
                        throw new Exception("PrivateKey is NULL/Empty");
                    if (serverPubKey == null || serverPubKey.isEmpty())
                        throw new Exception("ServerPubKey is NULL/Empty");
                    if (address == null || address.isEmpty())
                        throw new Exception("Client Address is NULL/Empty");
                    if (endpoint == null || endpoint.isEmpty())
                        throw new Exception("Endpoint is NULL/Empty");

                    if (allowedIps == null || allowedIps.isEmpty()) {
                        Log.w(TAG, "No AllowedIPs provided, using 0.0.0.0/0");
                        allowedIps = "0.0.0.0/0";
                    }

                    Interface.Builder interfaceBuilder = new Interface.Builder();
                    try {
                        interfaceBuilder.setKeyPair(new KeyPair(Key.fromBase64(privateKey)));
                    } catch (Exception e) {
                        throw new Exception("Invalid Private Key format: " + e.getMessage());
                    }

                    String[] addressArray = address.split(",");
                    for (String addr : addressArray) {
                        String trimmedAddr = addr.trim();
                        if (!trimmedAddr.contains("/"))
                            trimmedAddr += "/32";
                        Log.d(TAG, "Parsing local address: " + trimmedAddr);
                        try {
                            interfaceBuilder.addAddress(com.wireguard.config.InetNetwork.parse(trimmedAddr));
                        } catch (Exception e) {
                            throw new Exception("Local Address parse error [" + trimmedAddr + "]: " + e.getMessage());
                        }
                    }

                    // Add DNS for full tunnel resolution
                    try {
                        Log.d(TAG, "Configuring DNS: 8.8.8.8");
                        interfaceBuilder.addDnsServer(java.net.InetAddress.getByName("8.8.8.8"));
                    } catch (Exception e) {
                        Log.w(TAG, "Failed to configure DNS: " + e.getMessage());
                    }

                    Peer.Builder peerBuilder = new Peer.Builder();
                    Key serverKey;
                    try {
                        peerBuilder.setEndpoint(com.wireguard.config.InetEndpoint.parse(endpoint));
                        serverKey = Key.fromBase64(serverPubKey);
                        peerBuilder.setPublicKey(serverKey);

                        // Persistent keepalive to keep server aware
                        peerBuilder.setPersistentKeepalive(25);

                    } catch (Exception e) {
                        throw new Exception("Endpoint or ServerPubKey error: " + e.getMessage());
                    }

                    String[] allowedIpsArray = allowedIps.split(",");
                    for (String allowed : allowedIpsArray) {
                        String trimmedAllowed = allowed.trim();
                        Log.d(TAG, "Parsing allowed IP: " + trimmedAllowed);
                        try {
                            peerBuilder.addAllowedIp(com.wireguard.config.InetNetwork.parse(trimmedAllowed));
                        } catch (Exception e) {
                            throw new Exception("Allowed IP parse error [" + trimmedAllowed + "]: " + e.getMessage());
                        }
                    }

                    Config config = new Config.Builder()
                            .setInterface(interfaceBuilder.build())
                            .addPeer(peerBuilder.build())
                            .build();

                    Log.i(TAG, "Applying WireGuard configuration via GoBackend...");
                    backend.setState(GarlicVpnService.this, Tunnel.State.UP, config);
                    Log.i(TAG, "GoBackend.setState(UP) completed successfully. Handshake will be verified via onStateChange.");

                } catch (final Exception e) {
                    final String msg = "VPN Failure: " + e.getMessage();
                    Log.e(TAG, msg, e);

                    // KEY FIX: If the OS hasn't committed the user's VPN permission yet
                    // (race condition between onActivityResult and GoBackend.establish),
                    // wait 3 seconds and retry ONE time before reporting failure to C++.
                    String errLower = e.getMessage() != null ? e.getMessage().toLowerCase() : "";
                    if (errLower.contains("prepare") || errLower.contains("establish") || errLower.contains("permission")) {
                        Log.w(TAG, "VPN permission not yet committed by OS — retrying in 3s...");
                        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                Log.i(TAG, "Retry attempt: re-queuing startVpn into executor...");
                                // Re-use the same intent data stored in the outer scope
                                final String retryKey    = intent.getStringExtra("privateKey");
                                final String retryAddr   = intent.getStringExtra("address");
                                final String retrySvKey  = intent.getStringExtra("serverPubKey");
                                final String retryEp     = intent.getStringExtra("endpoint");
                                final String retryAllowed= intent.getStringExtra("allowedIps");
                                if (retryKey != null && !retryKey.isEmpty()) {
                                    startVpn(intent);
                                } else {
                                    GarlicActivity.notifyVpnError(msg);
                                    GarlicActivity.notifyVpnStateChanged(0);
                                }
                            }
                        }, 3000);
                    } else {
                        GarlicActivity.notifyVpnError(msg);
                        GarlicActivity.notifyVpnStateChanged(0); // ERROR/DOWN
                    }
                }
            }
        });
    }

    private void stopVpn() {
        Log.i(TAG, "VPN Stop Command Received, queuing task to ExecutorService...");
        executorService.submit(new Runnable() {
            @Override
            public void run() {
                try {
                    backend.setState(GarlicVpnService.this, Tunnel.State.DOWN, null);
                    Log.i(TAG, "VPN Tunnel Stopped successfully");
                } catch (Exception e) {
                    Log.e(TAG, "Failed to stop VPN", e);
                }
            }
        });
    }

    @Override
    public String getName() {
        return "GarlicVPN";
    }

    private boolean m_isMonitoringHandshake = false;

    @Override
    public void onStateChange(State newState) {
        Log.i(TAG, "onStateChange: newState=" + newState);
        if (newState == State.DOWN) {
            Log.i(TAG, "VPN State changed to DOWN -> notifying UI (0)");
            GarlicActivity.notifyVpnStateChanged(0);
        } else if (newState == State.UP) {
            Log.i(TAG, "VPN State changed to UP -> triggering handshake verification...");
            verifyHandshakeAsync();
        }
    }

    private void verifyHandshakeAsync() {
        if (m_isMonitoringHandshake) return;
        
        executorService.submit(new Runnable() {
            @Override
            public void run() {
                m_isMonitoringHandshake = true;
                try {
                    Log.i(TAG, "Monitoring handshake...");
                    for (int i = 0; i < 15; i++) {
                        Thread.sleep(2000);
                        Statistics stats = backend.getStatistics(GarlicVpnService.this);
                        if (stats != null && stats.totalRx() > 0) {
                            Log.i(TAG, "Handshake verified via monitor loop.");
                            GarlicActivity.notifyVpnStateChanged(2); // Connected
                            m_isMonitoringHandshake = false;
                            return;
                        }
                    }
                    Log.w(TAG, "Monitor loop finished without handshake verification. Forcing DOWN.");
                    backend.setState(GarlicVpnService.this, Tunnel.State.DOWN, null);
                    GarlicActivity.notifyVpnStateChanged(0);
                } catch (Exception e) {
                    Log.e(TAG, "Error in verifyHandshakeAsync: " + e.getMessage());
                } finally {
                    m_isMonitoringHandshake = false;
                }
            }
        });
    }

    public static String[] generateKeyPair() {
        try {
            KeyPair keyPair = new KeyPair();
            return new String[] { keyPair.getPrivateKey().toBase64(), keyPair.getPublicKey().toBase64() };
        } catch (Exception e) {
            Log.e(TAG, "Key generation failed", e);
            return null;
        }
    }
}
