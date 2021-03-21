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
#include "lib_hal.h"
#include "trusty/tipc.h"

static int g_handle = -1;
#define TRUSTY_DEVICE_NAME "/dev/trusty-ipc-dev0"
#define TADEMO_PORT "com.android.sampleta.demo"
#define TRUSTY_API_LIB "libtrusty.so"
static int (*PREFIX(tipc_connect))(const char *dev_name, const char *srv_name);
static int (*PREFIX(tipc_close))(int fd);

static int trusty_cmd(struct tee_client_device *dev, struct tee_in_buf *in, struct tee_out_buf *out)
{
    pthread_mutex_lock(&dev->mutex);
    int status = GENERIC_OK;
    ssize_t len = write(g_handle, in, IN_BUF_LEN);
    if_err(len != IN_BUF_LEN, status = GENERIC_ERR; goto end;, "%d %zd", in->cmd, len);
    len = read(g_handle, out, OUT_BUF_LEN);
    if_err(len != IN_BUF_LEN, status = GENERIC_ERR;, "%d %zd", in->cmd, len);
end:
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static void trusty_exit(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    if(g_handle > 0) PREFIX(tipc_close)(g_handle);
    if(dev->handle) dlclose(dev->handle);
}

static int trusty_init(void)
{
    ALOGD("%s", __func__);
    g_handle = PREFIX(tipc_connect)(TRUSTY_DEVICE_NAME, TADEMO_PORT);
    if_err(g_handle < 0, return GENERIC_ERR;, TRUSTY_DEVICE_NAME);
    return GENERIC_OK;
}

int trusty_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(TRUSTY_API_LIB, RTLD_LAZY);
    if_err(!dev->handle, return GENERIC_ERR;, TRUSTY_API_LIB);
    PREFIX(tipc_connect) = (int(*)(const char *, const char *))DLSYM(tipc_connect);
    PREFIX(tipc_close) = (int(*)(int))DLSYM(tipc_close);
    dev->tee_init = trusty_init;
    dev->tee_cmd = trusty_cmd;
    dev->tee_exit = trusty_exit;
    return GENERIC_OK;
}