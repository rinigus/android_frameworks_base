package android.net;

import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.LinkProperties;

public class Helpers {
    private static final String TAG = "dbus_Helpers";
    public static NetworkInfo network_info = null;
    public static WifiInfo wifi_info = null;
    public static LinkProperties link_properties = null;
    public native static int getWifiInfo();
};

