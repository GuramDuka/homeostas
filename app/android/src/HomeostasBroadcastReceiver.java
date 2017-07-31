package org.homeostas;
 
import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.util.Log;

public class HomeostasBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        //Log.i("HomeostasBroadcastReceiver", "start service");

        if( intent.getAction().equalsIgnoreCase(Intent.ACTION_BOOT_COMPLETED) ) {
            Intent i = new Intent(context, HomeostasService.class);
            //i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            //context.startActivity(i);
            context.startService(i);
        }
    }
}
