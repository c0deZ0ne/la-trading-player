package com.sagiadinos.garlic.player.java;

import android.content.Intent;
import android.net.VpnService;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import com.wireguard.android.backend.GoBackend;
import com.wireguard.android.backend.Tunnel;
import com.wireguard.config.Config;
import com.wireguard.config.Interface;
import com.wireguard.config.Peer;
import com.wireguard.crypto.Key;

import java.net.InetAddress;
import java.util.Collections;

public class GarlicVpnService extends VpnService implements Tunnel {
    private static final String TAG = "GarlicVpnService";
    private static GoBackend backend;
    private static GarlicVpnService instance;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        if (backend == null) {
            backend = new GoBackend(this);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if ("START".equals(action)) {
                startVpn(intent);
            } else if ("STOP".equals(action)) {
                stopVpn();
            }
        }
        return START_STICKY;
    }

    private void startVpn(Intent intent) {
        try {
            String privateKey = intent.getStringExtra("privateKey");
            String address = intent.getStringExtra("address"); // e.g. 10.8.0.2/32
            String serverPubKey = intent.getStringExtra("serverPubKey");
            String endpoint = intent.getStringExtra("endpoint"); // e.g. 1.2.3.4:51820

            Config config = new Config.Builder()
                .setInterface(new Interface.Builder()
                    .parseAddresses(address)
                    .parsePrivateKey(privateKey)
                    .build())
                .addPeer(new Peer.Builder()
                    .parseAllowedIPs("0.0.0.0/0")
                    .parseEndpoint(endpoint)
                    .parsePublicKey(serverPubKey)
                    .build())
                .build();

            backend.setState(this, Tunnel.State.UP, config);
            Log.i(TAG, "VPN Tunnel Started successfully");
        } catch (Exception e) {
            Log.e(TAG, "Failed to start VPN", e);
        }
    }

    private void stopVpn() {
        try {
            backend.setState(this, Tunnel.State.DOWN, null);
            Log.i(TAG, "VPN Tunnel Stopped");
        } catch (Exception e) {
            Log.e(TAG, "Failed to stop VPN", e);
        }
    }

    // --- WireGuard Tunnel Interface Methods ---
    @Override
    public String getName() {
        return "GarlicVPN";
    }

    @Override
    public void onStateChange(State newState) {
        Log.d(TAG, "VPN State changed to: " + newState);
        // We could notify C++ here via JNI if needed
    }

    // --- Helper Methods for JNI ---
    
    /**
     * Generates a new Base64 Private/Public key pair.
     * Returns a string array [PrivateKey, PublicKey]
     */
    public static String[] generateKeyPair() {
        try {
            com.wireguard.crypto.KeyPair keyPair = new com.wireguard.crypto.KeyPair();
            return new String[]{keyPair.getPrivateKey().toBase64(), keyPair.getPublicKey().toBase64()};
        } catch (Exception e) {
            Log.e(TAG, "Key generation failed", e);
            return null;
        }
    }
}
