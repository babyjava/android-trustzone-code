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
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "pthread.h"
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <log/log.h>
#include <dlfcn.h>
#include "platform_common.h"

#define TEE_COUNT 5
#define if_ab(a, b); if(a) { ALOGE("%s %d\n", __FILE__, __LINE__); b;}
#define if_abc(a, b, fmt, ...); if(a) { ALOGE("%s %d" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); b;}
#define PREFIX(s) f_##s
#define DLSYM(f) dlsym(dev->handle, #f); \
        if (dlerror() != NULL){ \
            ALOGE("dlsym err %s", #f); \
            return GENERIC_ERR; \
        }

struct tee_performance
{
    uint32_t cmd_run_times;
    uint32_t cmd_cost_max_time;
    uint64_t cmd_cost_total_time;
};

struct tee_client_device
{
    int (*tee_init)(void);
    int (*tee_cmd)(struct tee_client_device *, struct tee_in_buf *in, struct tee_out_buf *out);
    void (*tee_exit)(void);
    uint8_t buf[IN_BUF_LEN + OUT_BUF_LEN];
    struct tee_performance perf[TEE_CMD_RELEASE];
    pthread_mutex_t mutex;
    void *handle;
};

int gp_client_open(struct tee_client_device *dev);
int qsee_client_open(struct tee_client_device *dev);
int isee_client_open(struct tee_client_device *dev);
int trusty_client_open(struct tee_client_device *dev);
int kinibi_client_open(struct tee_client_device *dev);