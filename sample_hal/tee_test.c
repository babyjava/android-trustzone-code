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

static struct tee_client_device g_device;
typedef int (*tee_open_t)(struct tee_client_device *);

static tee_open_t g_open[] = {
    qsee_client_open,
    isee_client_open,
    kinibi_client_open,
    trusty_client_open,
    gp_client_open,
};

static void random_data(struct tee_in_buf *in)
{
    struct timeval tv;
    int len = IN_BUF_LEN/sizeof(int);
    int *data = (int *)in;
    while(len--){
        gettimeofday(&tv,NULL);
        srand(tv.tv_usec);
        data[len] = rand();
    }
    in->cmd = (in->cmd % TEE_CMD_END);
}

static void random_stability_performance_test(int times)
{
    struct tee_in_buf in;
    struct tee_out_buf out;
    while(times--){
        random_data(&in);
        g_device.tee_cmd(&g_device, &in, &out);
        if_err(out.status != GENERIC_OK, break;, "%s %d", out.sys_err_line, out.sys_err);
    }
}

static void help(void)
{
    ALOGD("How to Run: ./tee_test Tee_platform Test_times\n"
        "Example 1: ./tee_test 0 10000\n"
        "Example 2: ./tee_test 1 20000\n"
        "Tee_platform: Qsee - 0, isee - 1, kinibi - 2, trusty - 3, gp - 4\n"
        "Test_times: > 0\n");
}

int main(int argc, char **argv)
{
    int tee, times;
    if(argc < 2) goto end;
    tee = atoi(argv[1]);
    times = atoi(argv[2]);
    if(tee < 0 || tee > 5 || times < 1) goto end;

    pthread_mutex_init(&g_device.mutex, NULL);
    g_open[tee](&g_device);
    g_device.tee_init();

    random_stability_performance_test(times);

    g_device.tee_exit(&g_device);
    pthread_mutex_destroy(&g_device.mutex);
    return 0;
end:
    help();
    return 0;
}
