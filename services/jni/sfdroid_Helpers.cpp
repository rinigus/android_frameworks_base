/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "sfdroid_Helpers"

#include "JNIHelp.h"
#include "jni.h"
#include <sys/socket.h>
#include <sys/un.h>

#include <android_runtime/AndroidRuntime.h>

#define SFDROID_ROOT "/tmp/sfdroid"
#define APP_HELPERS_HANDLE_FILE (SFDROID_ROOT "/app_helpers_handle")

namespace android {
    static int sfdroid_fd = -1;

    int connect_to_sfdroid()
    {
        struct sockaddr_un addr;
        int fd;

        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(fd < 0)
        {
            ALOGE("error creating socket stream");
            return -1;
        }

        // don't crash if we disconnect
        //int set = 1;
        //setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
        signal(SIGPIPE, SIG_IGN);

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, APP_HELPERS_HANDLE_FILE, sizeof(addr.sun_path)-1);

        if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            ALOGE("error connecting to renderer: %s", strerror(errno));
            close(fd);
            return -1;
        }

        return fd;
    }

    void sfdroid_Helpers_notify_of_app_start(JNIEnv *env, jobject thiz, jstring component)
    {
        if(sfdroid_fd < 0)
        {
            sfdroid_fd = connect_to_sfdroid();
        }

        if(sfdroid_fd >= 0)
        {
            int err = 0;
            if(!component) ALOGE("component is NULL");
            const char *cmp = env->GetStringUTFChars(component, NULL);

            char buf[2];
            int16_t len = strlen(cmp) + 1;

            memcpy(buf, &len, 2);

            if(send(sfdroid_fd, buf, 2, MSG_WAITALL) < 0)
            {
                ALOGE("failed to send application info length: %s", strerror(errno));
                err = 1;
                goto exit;
            }

            if(send(sfdroid_fd, cmp, len, MSG_WAITALL) < 0)
            {
                ALOGE("failed to send application info %s", strerror(errno));
                err = 1;
                goto exit;
            }

        exit:
            env->ReleaseStringUTFChars(component, cmp);
            if(err)
            {
                close(sfdroid_fd);
                sfdroid_fd = -1;
            }
            return;
        }
    }

    void sfdroid_Helpers_notify_of_app_close(JNIEnv *env, jobject thiz, jstring component)
    {
        if(sfdroid_fd < 0)
        {
            sfdroid_fd = connect_to_sfdroid();
        }

        if(sfdroid_fd >= 0)
        {
            int err = 0;
            if(!component) ALOGE("component is NULL");
            const char *cmp = env->GetStringUTFChars(component, NULL);

            char buf[2];
            char buffclose[5120];
            int16_t len = strlen("close:") + strlen(cmp) + 1;

            memcpy(buf, &len, 2);

            if(send(sfdroid_fd, buf, 2, MSG_WAITALL) < 0)
            {
                ALOGE("failed to send application info length: %s", strerror(errno));
                err = 1;
                goto exit;
            }

            snprintf(buffclose, 5120, "close:%s", cmp);

            if(send(sfdroid_fd, buffclose, len, MSG_WAITALL) < 0)
            {
                ALOGE("failed to send application info %s", strerror(errno));
                err = 1;
                goto exit;
            }

        exit:
            env->ReleaseStringUTFChars(component, cmp);
            if(err)
            {
                close(sfdroid_fd);
                sfdroid_fd = -1;
            }
            return;
        }
    }

    static JNINativeMethod gsfdroidHelpersMethods[] = {
        { "notify_of_app_start", "(Ljava/lang/String;)V", (void*)sfdroid_Helpers_notify_of_app_start },
        { "notify_of_app_close", "(Ljava/lang/String;)V", (void*)sfdroid_Helpers_notify_of_app_close },
    };

    int register_sfdroid_Helpers(JNIEnv *env)
    {
        int res = jniRegisterNativeMethods(env, "sfdroid/Helpers", gsfdroidHelpersMethods, NELEM(gsfdroidHelpersMethods));
        return res;
    }
};

