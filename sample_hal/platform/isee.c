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

static int g_handle = -1;
static uint8_t* g_mem;
extern struct hal_device g_device;
#define ISEE_DEVICE "/dev/teei_fp"
#define ISEE_MAGIC_NO _IO('T', 0x2)

static int isee_cmd(struct tee_client_device *dev){
    pthread_mutex_lock(&dev->mutex);
    int status = 0;
    memcpy(g_mem, &dev->in, IN_BUF_LEN);
    status = ioctl(g_handle, ISEE_MAGIC_NO, g_mem);
    if_err(status != GENERIC_OK, goto end;, "%d %d", dev->in.cmd, status);
    memcpy(&dev->out, g_mem + IN_BUF_LEN, OUT_BUF_LEN);
end:
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static int isee_exit(struct tee_client_device *dev){
    ALOGD("%s", __func__);
    free(g_mem);
    close(g_handle);
    dlclose(dev->handle);
    return GENERIC_OK;
}

static int isee_init(void)
{
    ALOGD("%s", __func__);
    g_handle = open(ISEE_DEVICE, O_RDWR);
    if_err(g_handle < GENERIC_OK, return GENERIC_ERR;, "%s", "open");

    g_mem = malloc(IN_BUF_LEN + OUT_BUF_LEN);
    if_err(!g_mem, return GENERIC_ERR;, "%s", "malloc");
    return GENERIC_OK;
}

int isee_client_open(struct tee_client_device *dev)
{
    dev->tee_init = isee_init;
    dev->tee_cmd = isee_cmd;
    dev->tee_exit = isee_exit;
    return GENERIC_OK;
}