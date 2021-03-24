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
#include "kinibi/MobiCoreDriverApi.h"

#define KINIBI_API_LIB "libMcTrusty.so"
#define TA_PATH "/system/app/mcRegistry/88866600000000000000000000000000.tlbin"
static void *g_mem;
static void *g_ta;
static mcSessionHandle_t g_session;

static mcResult_t (*PREFIX(mcOpenDevice))(uint32_t);
static mcResult_t (*PREFIX(mcCloseDevice))(uint32_t);
static mcResult_t (*PREFIX(mcCloseSession))(mcSessionHandle_t *);
static mcResult_t (*PREFIX(mcNotify))(mcSessionHandle_t *);
static mcResult_t (*PREFIX(mcWaitNotification))(mcSessionHandle_t *, int32_t);
static mcResult_t (*PREFIX(mcOpenTrustlet))(mcSessionHandle_t *, mcSpid_t, uint8_t *, uint32_t, uint8_t *, uint32_t);

static int kinibi_cmd(struct tee_client_device *dev)
{
    pthread_mutex_lock(&dev->mutex);
    int status = 0;
    memcpy(g_mem, &dev->in, IN_BUF_LEN);
    status = PREFIX(mcNotify)(&g_session);
    if_abc(status, goto end, "%d %d", dev->in.cmd, status);
    status = PREFIX(mcWaitNotification)(&g_session, 1000);
    if_abc(status, goto end, "%d %d", dev->in.cmd, status);
    memcpy(&dev->out, (uint8_t *)g_mem + IN_BUF_LEN, OUT_BUF_LEN);
end:
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static int kinibi_exit(struct tee_client_device *dev)
{
    ALOGE("%s", __func__);
    free(g_ta);
    PREFIX(mcCloseSession)(&g_session);
    PREFIX(mcCloseDevice)(MC_DEVICE_ID_DEFAULT);
    dlclose(dev->handle);
    return GENERIC_OK;
}

static int kinibi_init(void)
{   int fd = 0;
    int len = 0;
    int status = PREFIX(mcOpenDevice)(MC_DEVICE_ID_DEFAULT);
    ALOGD("%s", __func__);
    if_ab(status, return GENERIC_ERR);

    fd = open(TA_PATH, O_RDONLY);
    if_ab(fd < GENERIC_OK, return GENERIC_ERR);

    len = lseek(fd, 0, SEEK_END);
    if_ab(len < GENERIC_OK, return GENERIC_ERR);

    g_ta = malloc(len);
    if_ab(!g_ta, return GENERIC_ERR);

    status = read(fd, g_ta, len);
    if_ab(status != len, return GENERIC_ERR);

    g_mem = malloc(IN_BUF_LEN + OUT_BUF_LEN);
    if_ab(!g_mem, return GENERIC_ERR);

    g_session.deviceId = MC_DEVICE_ID_DEFAULT;
    status = PREFIX(mcOpenTrustlet)(&g_session, MC_SPID_SYSTEM, g_ta, len, g_mem, (IN_BUF_LEN + OUT_BUF_LEN));
    if_ab(status != GENERIC_OK, return GENERIC_ERR);
    return GENERIC_OK;
}

int kinibi_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(KINIBI_API_LIB, RTLD_LAZY);
    if_abc(!dev->handle, return GENERIC_ERR, "%s", KINIBI_API_LIB);
    PREFIX(mcOpenDevice) = (mcResult_t(*)(uint32_t))DLSYM(mcOpenDevice);
    PREFIX(mcCloseDevice) = (mcResult_t(*)(uint32_t))DLSYM(mcCloseDevice);
    PREFIX(mcCloseSession) = (mcResult_t(*)(mcSessionHandle_t *))DLSYM(mcCloseSession);
    PREFIX(mcNotify) = (mcResult_t(*)(mcSessionHandle_t *))DLSYM(mcNotify);
    PREFIX(mcWaitNotification) = (mcResult_t(*)(mcSessionHandle_t *, int32_t))DLSYM(mcWaitNotification);
    PREFIX(mcOpenTrustlet) = (mcResult_t(*)(mcSessionHandle_t *, mcSpid_t, uint8_t *, uint32_t, uint8_t *, uint32_t))DLSYM(mcOpenTrustlet);
    dev->tee_init = kinibi_init;
    dev->tee_cmd = kinibi_cmd;
    dev->tee_exit = kinibi_exit;
    return GENERIC_OK;
}