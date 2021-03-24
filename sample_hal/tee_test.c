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

struct tee_client_device *g_device;
struct tee_in_buf *g_in;
struct tee_out_buf *g_out;
struct tee_performance_record *g_perf;

int (*g_open[TEE_COUNT])(struct tee_client_device *) = {
    qsee_client_open,
    isee_client_open,
    kinibi_client_open,
    trusty_client_open,
    gp_client_open,
};

static void init(int tee)
{
    g_device = (struct tee_client_device *)malloc(sizeof(struct tee_client_device));
    if_ab(!g_device, return);
    pthread_mutex_init(&g_device->mutex, NULL);
    g_open[tee](g_device);
    g_device->tee_init();
}

static void random_data(void)
{
    int len = 0;
    struct timeval tv;
    int *data = (int *)g_in->buf;
    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    g_in->spi_num = 0;
    g_in->i2c_num = 0x2e;
    g_in->cmd = (rand() % TEE_CMD_RELEASE);
    g_in->buf_len = (rand() % BUF_LEN);
    len = g_in->buf_len;
    while(len){
        gettimeofday(&tv,NULL);
        srand(tv.tv_usec);
        data[len--] = rand();
    }
}

static void collect_data(uint32_t cmd, uint32_t t)
{
    g_perf[cmd].cmd_run_times++;
    if(t > g_perf[cmd].cmd_cost_max_time) g_perf[cmd].cmd_cost_max_time = t;
    g_perf[cmd].cmd_cost_total_time += t;
}

static void print_data(void)
{
    int i = 0;
    uint32_t avg_t;
    for(i = 0;i < TEE_CMD_END; i++)
    {
        avg_t = g_perf[i].cmd_cost_total_time/g_perf[i].cmd_run_times;
        ALOGD("cmd = %d, run times = %d, max time = %d, avg time = %d\n", i, g_perf[i].cmd_run_times, g_perf[i].cmd_cost_max_time, avg_t);
    }
}

static void random_stability_performance_test(int times)
{
    struct timeval tv1;
    struct timeval tv2;
    uint32_t time_cost;
    while(times--){
        random_data();
        gettimeofday(&tv1,NULL);
        g_device->tee_cmd(g_device);
        gettimeofday(&tv2,NULL);
        time_cost = tv2.tv_usec - tv1.tv_usec;
        if_abc(g_out->status != GENERIC_OK, break, "%s %d", g_out->sys_err_line, g_out->sys_err);
        collect_data(g_in->cmd, time_cost);
    }
    print_data();
}

static void help(void)
{
    ALOGD("How to Run: ./tee_test Tee_platform Test_times\n"
        "Example 1: ./tee_test 0 10000\n"
        "Example 2: ./tee_test 1 20000\n"
        "Tee_platform: Qsee - 0, isee - 1, kinibi - 2, trusty - 3, gp - 4\n"
        "Test_times: > 0\n");
}

static void release(void)
{
    g_in->cmd = TEE_CMD_RELEASE;
    g_device->tee_cmd(g_device);
    g_device->tee_exit(g_device);
    pthread_mutex_destroy(&g_device->mutex);
}

int main(int argc, char **argv)
{
    int tee, times;
    if(argc < 2) goto end;
    tee = atoi(argv[1]);
    times = atoi(argv[2]);
    if(tee < 0 || tee >= TEE_COUNT || times < 1) goto end;

    init(tee);

    random_stability_performance_test(times);

    release();
    return 0;
end:
    help();
    return 0;
}
