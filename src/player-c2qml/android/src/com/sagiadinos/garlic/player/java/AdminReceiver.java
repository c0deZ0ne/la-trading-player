package com.sagiadinos.garlic.player.java;

import android.app.admin.DeviceAdminReceiver;
import android.content.Context;
import android.content.Intent;
import android.widget.Toast;

import androidx.annotation.NonNull;

public class AdminReceiver extends DeviceAdminReceiver {
    @Override
    public void onEnabled(@NonNull Context context, @NonNull Intent intent) {
        Toast.makeText(context, "Garlic Player Admin: Enabled", Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onDisabled(@NonNull Context context, @NonNull Intent intent) {
        Toast.makeText(context, "Garlic Player Admin: Disabled", Toast.LENGTH_SHORT).show();
    }
}
