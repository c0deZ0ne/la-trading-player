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
    private GoBackend backend;
    private static GarlicVpnService instance;

    @Override
    public void onCreate() {
        super.onCreate();
        instance = this;
        backend = new GoBackend(this);
        Log.i(TAG, "VPN Service Created and GoBackend initialized");
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
            Log.i(TAG, "VPN Start Command Received");
            String privateKey = intent.getStringExtra("privateKey");
            String address = intent.getStringExtra("address");
            String serverPubKey = intent.getStringExtra("serverPubKey");
            String endpoint = intent.getStringExtra("endpoint");
            String allowedIps = intent.getStringExtra("allowedIps");

            Log.d(TAG, "Config: server=" + endpoint + ", addresses=" + address + ", allowed=" + allowedIps);

            if (privateKey == null || privateKey.isEmpty()) throw new Exception("PrivateKey is NULL/Empty");
            if (serverPubKey == null || serverPubKey.isEmpty()) throw new Exception("ServerPubKey is NULL/Empty");
            if (address == null || address.isEmpty()) throw new Exception("Client Address is NULL/Empty");
            if (endpoint == null || endpoint.isEmpty()) throw new Exception("Endpoint is NULL/Empty");

            if (allowedIps == null || allowedIps.isEmpty()) {
                Log.w(TAG, "No AllowedIPs provided, using 0.0.0.0/0");
                allowedIps = "0.0.0.0/0";
            }

            Interface.Builder interfaceBuilder = new Interface.Builder();
            try {
                interfaceBuilder.setPrivateKey(Key.fromBase64(privateKey));
            } catch (Exception e) {
                throw new Exception("Invalid Private Key format: " + e.getMessage());
            }
            
            String[] addressArray = address.split(",");
            for (String addr : addressArray) {
                String trimmedAddr = addr.trim();
                if (!trimmedAddr.contains("/")) trimmedAddr += "/32";
                Log.d(TAG, "Parsing local address: " + trimmedAddr);
                try {
                    interfaceBuilder.addAddress(com.wireguard.config.InetNetwork.parse(trimmedAddr));
                } catch (Exception e) {
                    throw new Exception("Local Address parse error [" + trimmedAddr + "]: " + e.getMessage());
                }
            }

            Peer.Builder peerBuilder = new Peer.Builder();
            try {
                peerBuilder.setEndpoint(com.wireguard.config.InetEndpoint.parse(endpoint));
                peerBuilder.setPublicKey(Key.fromBase64(serverPubKey));
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
            backend.setState(this, Tunnel.State.UP, config);
            Log.i(TAG, "GoBackend.setState(UP) called successfully");

        } catch (Exception e) {
            String msg = "VPN Failure: " + e.getMessage();
            Log.e(TAG, msg);
            GarlicActivity.notifyVpnError(msg);
            GarlicActivity.notifyVpnStateChanged(0);
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
        // Status codes matching QML expectations:
        // 0 = Disconnected/DOWN
        // 1 = Connecting/TOGGLING
        // 2 = Connected/UP
        // 3 = Error (sent via notifyVpnError)
        int status;
        switch (newState) {
            case UP:
                status = 2; // Connected
                break;
            case TOGGLING:
                status = 1; // Connecting
                break;
            case DOWN:
            default:
                status = 0; // Disconnected
                break;
        }
        Log.i(TAG, "VPN State changed to: " + newState + " -> status code: " + status);
        GarlicActivity.notifyVpnStateChanged(status);
    }

    // --- Helper Methods for JNI ---
    
    /**
     * Generates a new Base64 Private/Public key pair.
     * Returns a string array [PrivateKey, PublicKey]
     */
    public static String[] generateKeyPair() {
        try {
            Key privateKey = Key.generatePrivate();
            Key publicKey = privateKey.getPublicKey();
            return new String[]{privateKey.toBase64(), publicKey.toBase64()};
        } catch (Exception e) {
            Log.e(TAG, "Key generation failed", e);
            return null;
        }
    }
}
