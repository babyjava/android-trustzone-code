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
extern struct hal_device g_device;
#define ISEE_DEVICE "/dev/teei_fp"
#define ISEE_MAGIC_NO _IO('T', 0x2)

struct ut_pf_fp_cmd_header {
    uint32_t reserved_header_1;
    uint32_t reserved_header_2;
    uint32_t reserved_header_3;
    uint32_t data_length;
    void *buf;
};

static struct ut_pf_fp_cmd_header g_cmd;
static int isee_cmd(struct tee_client_device *dev, struct tee_in_buf *in, struct tee_out_buf *out){
    pthread_mutex_lock(&dev->mutex);
    int status = 0;
    g_cmd.buf = in;
    g_cmd.data_length = IN_BUF_LEN + OUT_BUF_LEN;
    status = ioctl(g_handle, ISEE_MAGIC_NO, &g_cmd);
    if_abc(status, (void)out, "%d %d", in->cmd, status);
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static void isee_exit(void){
    ALOGD("%s", __func__);
    close(g_handle);
}

static int isee_init(void)
{
    ALOGD("%s", __func__);
    g_handle = open(ISEE_DEVICE, O_RDWR);
    if_ab(g_handle < GENERIC_OK, return GENERIC_ERR);
    return GENERIC_OK;
}

int isee_client_open(struct tee_client_device *dev)
{
    dev->tee_init = isee_init;
    dev->tee_cmd = isee_cmd;
    dev->tee_exit = isee_exit;
    return GENERIC_OK;
}