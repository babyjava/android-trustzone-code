/*# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
*/
#include "platform_hal.h"
#include "qsee/QSEEComAPI.h"

#define QSEE_IN_LEN QSEECOM_ALIGN(IN_BUF_LEN)
#define QSEE_OUT_LEN QSEECOM_ALIGN(OUT_BUF_LEN)
#define QSEE_TA_PATH "/vendor/firmware_mnt/image"
#define QSEE_TA_NAME "sampleta"
#define QSEE_API_LIB "libQSEEComAPI.so"
static struct QSEECom_handle *g_handle;
static int (*PREFIX(QSEECom_start_app))(struct QSEECom_handle **, const char *, const char *, uint32_t);
static int (*PREFIX(QSEECom_shutdown_app))(struct QSEECom_handle **);
static int (*PREFIX(QSEECom_send_cmd))(struct QSEECom_handle *, void *, uint32_t, void *, uint32_t);

static int qsee_cmd(struct tee_client_device *dev)
{
    int status = PREFIX(QSEECom_send_cmd)(g_handle, &dev->in, QSEE_IN_LEN, &dev->out, QSEE_OUT_LEN);
    if_err(status != GENERIC_OK,  , "%d %d", dev->in.cmd, status);
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static int qsee_exit(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    PREFIX(QSEECom_shutdown_app)(&g_handle);
    dlclose(dev->handle);
    return GENERIC_OK;
}

static int qsee_init(void)
{
    int status = PREFIX(QSEECom_start_app)(&g_handle, QSEE_TA_PATH, QSEE_TA_NAME, (QSEE_IN_LEN + QSEE_OUT_LEN));
    ALOGD("%s", __func__);
    if_err(status != GENERIC_OK,  , "%s", QSEE_TA_PATH QSEE_TA_NAME);
    return status;
}

int qsee_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(QSEE_API_LIB, RTLD_LAZY);
    if_err(!dev->handle, return GENERIC_ERR;, QSEE_API_LIB);
    PREFIX(QSEECom_start_app) = (int(*)(struct QSEECom_handle **, const char *, const char *, uint32_t))DLSYM(QSEECom_start_app);
    PREFIX(QSEECom_send_cmd) = (int(*)(struct QSEECom_handle *, void *, uint32_t, void *, uint32_t))DLSYM(QSEECom_send_cmd);
    PREFIX(QSEECom_shutdown_app) = (int(*)(struct QSEECom_handle **))DLSYM(QSEECom_shutdown_app);

    dev->tee_init = qsee_init;
    dev->tee_cmd = qsee_cmd;
    dev->tee_exit = qsee_exit;
    return GENERIC_OK;
}