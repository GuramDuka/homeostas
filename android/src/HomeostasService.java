package org.homeostas;
 
import android.content.Context;
import android.content.Intent;
//import android.widget.Toast;
import android.util.Log;
import org.qtproject.qt5.android.bindings.QtService;
 
public class HomeostasService extends QtService {
    private static final String TAG = "HomeostasService";

    public static void startHomeostasService(Context ctx) {
        //Log.i("Homeostas", "start service");
        ctx.startService(new Intent(ctx, HomeostasService.class));
    }

    public static void logFromService(String txt) {
        Log.i(TAG, txt);
    }

    //public void onCreate() {
    //    super.onCreate();
    //    Toast.makeText(this, "Homeostas service started", Toast.LENGTH_LONG).show();
    //    // do something when the service is created
    //}
}
