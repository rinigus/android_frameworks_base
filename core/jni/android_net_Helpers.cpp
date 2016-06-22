#define LOG_TAG "dbus_Helpers"

#include "jni.h"
#include <android_runtime/AndroidRuntime.h>
#include "JNIHelp.h"

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>

namespace android{

void ip_string_to_bytes(char *value, unsigned char *address)
{
    sscanf(value, "%d.%d.%d.%d", (int*)&address[0], (int*)&address[1], (int*)&address[2], (int*)&address[3]);
}

void test_print(JNIEnv *env, jstring str)
{
    const char *nativeString = env->GetStringUTFChars(str, JNI_FALSE);

    ALOGE("krnlyng test: %s", nativeString);

    env->ReleaseStringUTFChars(str, nativeString);
}

void set_string_value2(JNIEnv *env, char *name, char *name2, char *value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);

    static unsigned char saved_ip[4];
    static char saved_interface[128];

    //ALOGW("name: %s:%s: val %s\n", name, name2, value);
    if(strcmp(name, "Ethernet") == 0)
    {
        if(strcmp(name2, "Address") == 0)
        {
            jclass WifiInfo = env->FindClass("android/net/wifi/WifiInfo");
            jfieldID mac_address = env->GetFieldID(WifiInfo, "mMacAddress", "Ljava/lang/String;");
            jstring arg = env->NewStringUTF(value);
            env->SetObjectField(wifi_info, mac_address, arg);
            env->DeleteLocalRef(arg);
        }
        else if(strcmp(name2, "Interface") == 0)
        {
            jclass LinkProperties = env->FindClass("android/net/LinkProperties");
            jfieldID mIfaceName = env->GetFieldID(LinkProperties, "mIfaceName", "Ljava/lang/String;");
            jstring arg = env->NewStringUTF(value);
            env->SetObjectField(link_properties, mIfaceName, arg);
            env->DeleteLocalRef(arg);
            strcpy(saved_interface, value);
        }
    }
    else if(strcmp(name, "IPv4") == 0)
    {
        if(strcmp(name2, "Address") == 0)
        {
            jclass WifiInfo = env->FindClass("android/net/wifi/WifiInfo");
            jclass InetAddress = env->FindClass("java/net/InetAddress");
            jmethodID getByAddress = env->GetStaticMethodID(InetAddress, "getByAddress", "([B)Ljava/net/InetAddress;");
            unsigned char address[4];
            ip_string_to_bytes(value, address);
            jbyteArray array = env->NewByteArray(4);
            env->SetByteArrayRegion(array, 0, 4, (jbyte*)address);
            jobject inet_address = env->CallStaticObjectMethod(InetAddress, getByAddress, array);
            env->DeleteLocalRef(array);
            jfieldID fmIpAddress = env->GetFieldID(WifiInfo, "mIpAddress", "Ljava/net/InetAddress;");
            env->SetObjectField(wifi_info, fmIpAddress, inet_address);
            env->DeleteLocalRef(inet_address);

            memcpy(saved_ip, address, sizeof(unsigned char) * 4);
        }
        else if(strcmp(name2, "Netmask") == 0)
        {
            int range = 0;
            unsigned char s_netmask[4];
            ip_string_to_bytes(value, s_netmask);
            for(int i=0;i<4;i++)
            {
                for(int j=0;j<8;j++)
                {
                    if((1 << j) & s_netmask[i])
                    {
                        range++;
                    }
                }
            }

            char address_string[64];
            sprintf(address_string, "%d.%d.%d.%d/%d", saved_ip[0], saved_ip[1], saved_ip[2], saved_ip[3], range);

            jclass LinkProperties = env->FindClass("android/net/LinkProperties");
            jclass LinkAddress = env->FindClass("android/net/LinkAddress");

            jmethodID LinkAddress_init = env->GetMethodID(LinkAddress, "<init>", "(Ljava/lang/String;)V");

            jstring la = env->NewStringUTF(address_string);
            jobject link_address = env->NewObject(LinkAddress, LinkAddress_init, la);
            env->DeleteLocalRef(la);

            jmethodID addLinkAddress = env->GetMethodID(LinkProperties, "addLinkAddress", "(Landroid/net/LinkAddress;)Z");

            jboolean b = env->CallBooleanMethod(link_properties, addLinkAddress, link_address);

            env->DeleteLocalRef(link_address);

        }
        else if(strcmp(name2, "Gateway") == 0)
        {
            jclass LinkProperties = env->FindClass("android/net/LinkProperties");
            jclass InetAddress = env->FindClass("java/net/InetAddress");
            jmethodID getByAddress = env->GetStaticMethodID(InetAddress, "getByAddress", "([B)Ljava/net/InetAddress;");
            unsigned char address[4];
            ip_string_to_bytes(value, address);
            jbyteArray array = env->NewByteArray(4);
            env->SetByteArrayRegion(array, 0, 4, (jbyte*)address);
            jobject inet_address = env->CallStaticObjectMethod(InetAddress, getByAddress, array);
            env->DeleteLocalRef(array);

            jclass RouteInfo = env->FindClass("android/net/RouteInfo");
            jmethodID RouteInfo_init = env->GetMethodID(RouteInfo, "<init>", "(Ljava/net/InetAddress;)V");
            jobject route_info = env->NewObject(RouteInfo, RouteInfo_init, inet_address);

            jmethodID addRoute = env->GetMethodID(LinkProperties, "addRoute", "(Landroid/net/RouteInfo;)Z");
            env->CallObjectMethod(link_properties, addRoute, route_info);

            env->DeleteLocalRef(inet_address);
            env->DeleteLocalRef(route_info);
        }
    }

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

void set_string_value(JNIEnv *env, char *name, char *value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);

    //ALOGW("name: %s val: %s\n", name, value);
    if(strcmp(name, "Name") == 0)
    {
        jclass WifiSsid;
        WifiSsid = env->FindClass("android/net/wifi/WifiSsid");
        jmethodID create = env->GetStaticMethodID(WifiSsid, "createFromAsciiEncoded", "(Ljava/lang/String;)Landroid/net/wifi/WifiSsid;");
        jstring arg = env->NewStringUTF(value);
        jobject wifissid = env->CallStaticObjectMethod(WifiSsid, create, arg);
        env->DeleteLocalRef(arg);
        jclass WifiInfo = env->FindClass("android/net/wifi/WifiInfo");
        jfieldID fmWifiSsid = env->GetFieldID(WifiInfo, "mWifiSsid", "Landroid/net/wifi/WifiSsid;");
        env->SetObjectField(wifi_info, fmWifiSsid, wifissid);
        env->DeleteLocalRef(wifissid);
    }
    else if(strcmp(name, "BSSID") == 0)
    {
        jclass WifiInfo = env->FindClass("android/net/wifi/WifiInfo");
        jfieldID fmBSSID = env->GetFieldID(WifiInfo, "mBSSID", "Ljava/lang/String;");
        jstring arg = env->NewStringUTF(value);
        env->SetObjectField(wifi_info, fmBSSID, arg);
        env->DeleteLocalRef(arg);
    }

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

void set_bool_value(JNIEnv *env, char *name, bool value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);

    //ALOGW("name: %s val: %s\n", name, value ? "true": "false");

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

void set_byte_value(JNIEnv *env, char *name, char value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);

    //ALOGW("name: %s val: %d\n", name, value);
    if(strcmp(name, "Strength") == 0)
    {
        jclass clazz;
        clazz = env->FindClass("android/net/wifi/WifiInfo");
        jfieldID fmrssi = env->GetFieldID(clazz, "mRssi", "I");
        env->SetIntField(wifi_info, fmrssi, value);
    }

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

void set_uint16_value2(JNIEnv *env, char *name, char *name2, uint16_t value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);

    //ALOGW("name: %s:%s val: %d\n", name, name2, value);
    if(strcmp(name, "Ethernet") == 0)
    {
        if(strcmp(name2, "MTU") == 0)
        {
            jclass LinkProperties = env->FindClass("android/net/LinkProperties");
            jfieldID mMtu = env->GetFieldID(LinkProperties, "mMtu", "I");
            env->SetIntField(link_properties, mMtu, value);
        }
    }

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

void set_string_value_array(JNIEnv *env, char *name, int index, char *value)
{
    jclass Helpers = env->FindClass("android/net/Helpers");
    jfieldID fwifi_info = env->GetStaticFieldID(Helpers, "wifi_info", "Landroid/net/wifi/WifiInfo;");
    jobject wifi_info = env->GetStaticObjectField(Helpers, fwifi_info);
    jfieldID fnetwork_info = env->GetStaticFieldID(Helpers, "network_info", "Landroid/net/NetworkInfo;");
    jobject network_info = env->GetStaticObjectField(Helpers, fnetwork_info);
    jfieldID flink_properties = env->GetStaticFieldID(Helpers, "link_properties", "Landroid/net/LinkProperties;");
    jobject link_properties = env->GetStaticObjectField(Helpers, flink_properties);
    //ALOGW("name: %s val[%d]: %s\n", name, index, value);


    if(strcmp(name, "Nameservers") == 0)
    {
        jclass LinkProperties = env->FindClass("android/net/LinkProperties");
        jclass InetAddress = env->FindClass("java/net/InetAddress");
        jmethodID getByAddress = env->GetStaticMethodID(InetAddress, "getByAddress", "([B)Ljava/net/InetAddress;");
        unsigned char address[4];
        ip_string_to_bytes(value, address);
        jbyteArray array = env->NewByteArray(4);
        env->SetByteArrayRegion(array, 0, 4, (jbyte*)address);
        jobject inet_address = env->CallStaticObjectMethod(InetAddress, getByAddress, array);
        env->DeleteLocalRef(array);

        jmethodID addDns = env->GetMethodID(LinkProperties, "addDnsServer", "(Ljava/net/InetAddress;)Z");

        env->CallVoidMethod(link_properties, addDns, inet_address);

        env->DeleteLocalRef(inet_address);
    }

    env->DeleteLocalRef(wifi_info);
    env->DeleteLocalRef(network_info);
    env->DeleteLocalRef(link_properties);
}

int get_values(JNIEnv *env, DBusMessageIter *struc)
{
    int value_array_type;
    DBusMessageIter value_array;
    dbus_message_iter_recurse(struc, &value_array);
    value_array_type = dbus_message_iter_get_arg_type(&value_array);
    int was_online = 0;
    while(value_array_type != DBUS_TYPE_INVALID)
    {
        if(value_array_type == DBUS_TYPE_DICT_ENTRY)
        {
            int dict_entry_type;
            DBusMessageIter dict_entry;
            dbus_message_iter_recurse(&value_array, &dict_entry);
            dict_entry_type = dbus_message_iter_get_arg_type(&dict_entry);
            char *name = NULL;
            while(dict_entry_type != DBUS_TYPE_INVALID)
            {
                if(dict_entry_type == DBUS_TYPE_STRING)
                {
                    dbus_message_iter_get_basic(&dict_entry, &name);
                }
                else if(dict_entry_type == DBUS_TYPE_VARIANT)
                {
                    int variant_type;
                    DBusMessageIter variant;
                    dbus_message_iter_recurse(&dict_entry, &variant);
                    variant_type = dbus_message_iter_get_arg_type(&variant);
                    while(variant_type != DBUS_TYPE_INVALID)
                    {
                        if(variant_type == DBUS_TYPE_STRING)
                        {
                            char *val;
                            dbus_message_iter_get_basic(&variant, &val);
                            if(strcmp(name, "State") == 0 && strcmp(val, "online") == 0)
                            {
                                was_online = 1;
                            }
                            set_string_value(env, name, val);
                        }
                        else if(variant_type == DBUS_TYPE_BOOLEAN)
                        {
                            bool val;
                            dbus_message_iter_get_basic(&variant, &val);
                            set_bool_value(env, name, val);
                        }
                        else if(variant_type == DBUS_TYPE_BYTE)
                        {
                            char val;
                            dbus_message_iter_get_basic(&variant, &val);
                            set_byte_value(env, name, val);
                        }
                        else if(variant_type == DBUS_TYPE_ARRAY)
                        {
                            int i = 0;
                            int inner_array_type;
                            DBusMessageIter inner_array;
                            dbus_message_iter_recurse(&variant, &inner_array);
                            inner_array_type = dbus_message_iter_get_arg_type(&inner_array);
                            while(inner_array_type != DBUS_TYPE_INVALID)
                            {
                                if(inner_array_type == DBUS_TYPE_STRING)
                                {
                                    char *inner_array_val;
                                    dbus_message_iter_get_basic(&inner_array, &inner_array_val);
                                    set_string_value_array(env, name, i, inner_array_val);
                                }
                                else if(inner_array_type == DBUS_TYPE_DICT_ENTRY)
                                {
                                    char *name2 = NULL;
                                    int inner_dict_entry_type;
                                    DBusMessageIter inner_dict_entry;
                                    dbus_message_iter_recurse(&inner_array, &inner_dict_entry);
                                    inner_dict_entry_type = dbus_message_iter_get_arg_type(&inner_dict_entry);
                                    while(inner_dict_entry_type != DBUS_TYPE_INVALID)
                                    {
                                        if(inner_dict_entry_type == DBUS_TYPE_STRING)
                                        {
                                            dbus_message_iter_get_basic(&inner_dict_entry, &name2);
                                        }
                                        else if(inner_dict_entry_type == DBUS_TYPE_VARIANT)
                                        {
                                            int inner_variant_type;
                                            DBusMessageIter inner_variant;
                                            dbus_message_iter_recurse(&inner_dict_entry, &inner_variant);
                                            inner_variant_type = dbus_message_iter_get_arg_type(&inner_variant);
                                            while(inner_variant_type != DBUS_TYPE_INVALID)
                                            {
                                                if(inner_variant_type == DBUS_TYPE_STRING)
                                                {
                                                    char *val;
                                                    dbus_message_iter_get_basic(&inner_variant, &val);
                                                    set_string_value2(env, name, name2, val);
                                                }
                                                else if(inner_variant_type == DBUS_TYPE_UINT16)
                                                {
                                                    uint16_t val;
                                                    dbus_message_iter_get_basic(&inner_variant, &val);
                                                    set_uint16_value2(env, name, name2, val);
                                                }
                                                else ALOGE("unhandled: name: %s:%s type: %c\n", name, name2, inner_variant_type);
                                                dbus_message_iter_next(&inner_variant);
                                                inner_variant_type = dbus_message_iter_get_arg_type(&inner_variant);
                                            }
                                        }
                                        else ALOGE("unhandled: name: %s type[%d]: %c\n", name, i, inner_dict_entry_type);
                                        dbus_message_iter_next(&inner_dict_entry);
                                        inner_dict_entry_type = dbus_message_iter_get_arg_type(&inner_dict_entry);
                                    }
                                }
                                else ALOGE("unhandled: name: %s type: %c\n", name, inner_array_type);
                                dbus_message_iter_next(&inner_array);
                                inner_array_type = dbus_message_iter_get_arg_type(&inner_array);
                                i++;
                            }
                        }
                        else ALOGE("unhandled: name: %s type: %c\n", name, variant_type);
                        dbus_message_iter_next(&variant);
                        variant_type = dbus_message_iter_get_arg_type(&variant);
                    }
                }

                dbus_message_iter_next(&dict_entry);
                dict_entry_type = dbus_message_iter_get_arg_type(&dict_entry);
            }
        }
        dbus_message_iter_next(&value_array);
        value_array_type = dbus_message_iter_get_arg_type(&value_array);
    }

    return was_online;
}

static DBusConnection *bus = NULL;

static jint sfdroid_dbus_Helpers_getWifiInfo(JNIEnv *env)
{
    int err = 0;
    int type = 0;
    int wifi = 0, cell = 0;
    DBusMessageIter args;
    DBusMessage *msg = NULL;
    DBusError error;
    DBusPendingCall *pending = NULL;

    dbus_error_init(&error);
    bus = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
    if(dbus_error_is_set(&error))
    {
        ALOGE("connection error: %s\n", error.message);
        dbus_error_free(&error);
        err = 1;
    }
    if(bus == NULL)
    {
        ALOGE("bus is NULL\n");
        err = 1;
        goto quit;
    }

    msg = dbus_message_new_method_call("net.connman", "/", "net.connman.Manager", "GetServices");
    if(msg == NULL)
    {
        ALOGE("new method call: msg is NULL\n");
        err = 1;
        goto quit;
    }

    if(!dbus_connection_send_with_reply(bus, msg, &pending, -1))
    {
        ALOGE("failed to send message\n");
        err = 1;
        goto quit;
    }
    if(pending == NULL)
    {
        ALOGE("pending is NULL\n");
        err = 1;
        goto quit;
    }

    dbus_connection_flush(bus);

    dbus_message_unref(msg);

    dbus_pending_call_block(pending);

    msg = dbus_pending_call_steal_reply(pending);
    if(msg == NULL)
    {
        ALOGE("failed to steal reply\n");
        err = 1;
        goto quit;
    }
    dbus_pending_call_unref(pending);

    if(!dbus_message_iter_init(msg, &args))
    {
        //ALOGW("message has no arguments\n");
        err = 1;
        goto quit;
    }

    type = dbus_message_iter_get_arg_type(&args);

    if(type == DBUS_TYPE_ARRAY)
    {
        int a_type;
        DBusMessageIter array;
        dbus_message_iter_recurse(&args, &array);
        a_type = dbus_message_iter_get_arg_type(&array);
        while(a_type != DBUS_TYPE_INVALID)
        {
            if(a_type == DBUS_TYPE_STRUCT)
            {
                int s_type;
                DBusMessageIter struc;
                dbus_message_iter_recurse(&array, &struc);
                s_type = dbus_message_iter_get_arg_type(&struc);
                while(s_type != DBUS_TYPE_INVALID)
                {
                    if(s_type == DBUS_TYPE_OBJECT_PATH)
                    {
                        char *val;
                        dbus_message_iter_get_basic(&struc, &val);
                        ////ALOGW("object path: %s\n", val);
                        cell = wifi = 0;
                        if(strstr(val, "cell") != NULL)
                        {
                            cell = 1;
                        }
                        else if(strstr(val, "wifi") != NULL)
                        {
                            wifi = 1;
                        }
                    }
                    else if(s_type == DBUS_TYPE_ARRAY)
                    {
                        if(get_values(env, &struc))
                        {
                            err = 0;
                            goto quit;
                        }
                    }
                    dbus_message_iter_next(&struc);
                    s_type = dbus_message_iter_get_arg_type(&struc);
                }
            }
            dbus_message_iter_next(&array);
            a_type = dbus_message_iter_get_arg_type(&array);
        }
    }

quit:
    if(msg) dbus_message_unref(msg);
    if(bus) dbus_connection_unref(bus);
    return err;
}


static JNINativeMethod gHelperMethods[] = {
    /* name, signature, funcPtr */
    { "getWifiInfo", "()I", (void*)sfdroid_dbus_Helpers_getWifiInfo },
};

int register_sfdroid_dbus_Helpers(JNIEnv* env) {
    return AndroidRuntime::registerNativeMethods(env,
            "android/net/Helpers", gHelperMethods, NELEM(gHelperMethods));
}
};

