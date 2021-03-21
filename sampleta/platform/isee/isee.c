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
#include "platform_tee.h"

static struct mt_chip_conf g_spi_config = {
    .setuptime = 15,
    .holdtime = 15,
    .high_time = 15,
    .low_time = 15,
    .cs_idletime = 20,
    .ulthgh_thrsh = 0,
    .cpol = SPI_CPOL_0,
    .cpha = SPI_CPHA_0,
    .rx_mlsb = SPI_MSB,
    .tx_mlsb = SPI_MSB,
    .tx_endian = SPI_LENDIAN,
    .rx_endian = SPI_LENDIAN,
    .com_mod = DMA_TRANSFER, //DMA_TRANSFER;//FIFO_TRANSFER;
    .pause = PAUSE_MODE_DISABLE,
    .finish_intr = FINISH_INTR_DIS,
    .deassert = DEASSERT_DISABLE,
    .ulthigh = ULTRA_HIGH_DISABLE,
    .tckdly = TICK_DLY0,
};

static void isee_spi_rw(void *in, uint32_t in_len, void *out, uint32_t out_len)
{
    g_out->sys_err = ut_pf_spi_send_and_receive(in, out, in_len, out_len, &g_spi_config, 1);
    if_err(g_out->sys_err != 0, );
}

static void isee_uptime(uint64_t *t)
{
    ut_uint32_t seconds = 0;
    ut_uint32_t million_seconds = 0;
    g_out->sys_err = ut_pf_time_get_system_time(&seconds, &million_seconds);
    if_err(g_out->sys_err != 0, return);
    *t = seconds*1000 + million_seconds;
}

static void isee_token(uint64_t *token, uint64_t *t_time)
{
    int len = 0;
    int try = 10;
    while(try > 0){
        len = ut_pf_cp_rd_random(NULL, (uint8_t*)token, sizeof(uint64_t));
        if(len == sizeof(uint64_t)){
            isee_uptime(t_time);
            return;
        }
    }
    LOG_RECORD
}

static void isee_get_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    uint32_t i, n, l;
    ut_pf_cp_context_t ctx = NULL;
    ut_uint32_t dstlen = HMAC_LEN;
    g_out->sys_err = ut_pf_cp_open(&ctx, UT_PF_CP_CLS_MC, UT_PF_CP_ACT_MC_HMAC_SHA256);
    if_err(g_out->sys_err != 0, return);

    n = msg_len / dstlen;
    l = msg_len % dstlen;
    g_out->sys_err = ut_pf_cp_mc_start(ctx, key, key_len, NULL, 0);
    if_err(g_out->sys_err != 0, goto end);

    for (i = 0; i < n; i++) {
        g_out->sys_err = ut_pf_cp_mc_update(ctx, msg, dstlen);
        if_err(g_out->sys_err != 0, goto end);
        msg += dstlen;
    }
    if (l > 0) {
        g_out->sys_err = ut_pf_cp_mc_update(ctx, msg, l);
        if_err(g_out->sys_err != 0, goto end);
    }
    g_out->sys_err = ut_pf_cp_mc_finish(ctx, hmac, &dstlen);
    if_err(g_out->sys_err != 0,  );
end:
    ut_pf_cp_close(ctx);
}

static void isee_verify_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    uint8_t hmac_t[HMAC_LEN];
    int i = 0;
    isee_get_hmac(msg, msg_len, key, key_len, hmac_t);
    if_err(g_out->sys_err != 0, return);
    for(i = 0; i < HMAC_LEN; i++){
        if_err(hmac[i] ^ hmac_t[i], break);
    }
}

static void isee_get_hmac_key(uint8_t *key, uint32_t *key_len)
{
    g_out->sys_err = ut_pf_km_get_hmac_key(key, key_len);
    if_err(g_out->sys_err != 0,  );
}

static void isee_km_encap(uint8_t *msg, uint32_t msg_len, uint8_t *hmac, uint32_t *len)
{
    g_out->sys_err = ut_pf_km_enc_authtoken(msg, msg_len, hmac, len);
    if_err(g_out->sys_err != 0,  );
}

static void isee_write(char *name, void *buf, size_t buf_len)
{
    int fd = 0;
    g_out->sys_err = ut_pf_ts_cp_open(name, UT_PF_TS_O_CREAT | UT_PF_TS_O_WRONLY | UT_PF_TS_O_SYNC);
    if_err(g_out->sys_err != 0, return);

    fd = g_out->sys_err;
    g_out->sys_err = ut_pf_ts_cp_write(fd, buf, buf_len);
    if_err(g_out->sys_err < 0,  );

    ut_pf_ts_cp_close(fd);
}

static void isee_read(char *name, void *buf, size_t *buf_len)
{
    int fd = 0;
    size_t size = 0;
    g_out->sys_err = ut_pf_ts_cp_open(name, UT_PF_TS_O_CREAT | UT_PF_TS_O_RDONLY);
    if_err(g_out->sys_err != 0, return);

    fd = g_out->sys_err;
    g_out->sys_err = ut_pf_ts_cp_size(fd);
    if_err(g_out->sys_err < 0, goto end);

    size = g_out->sys_err;
    if_err(size == 0, goto end);
    if_err(*buf_len < size, goto end);

    g_out->sys_err = ut_pf_ts_cp_read(fd, buf, size);
    if_err(g_out->sys_err < 0,  );
end:
    ut_pf_ts_cp_close(fd);
    *buf_len = size;
}

void platform_init(void)
{
    if(g_dev == NULL) {
        g_dev = (struct platform_device *)malloc(sizeof(struct platform_device));
        if (g_dev == NULL) {
            loge("%s, err malloc\n", __func__);
            return;
        }
    }
    g_in = (struct tee_in_buf *)g_dev->buf;
    g_out = (struct tee_out_buf *)(g_in + IN_BUF_LEN);
    g_dev->free = free,
    g_dev->malloc = malloc,
    g_dev->spi_rw = isee_spi_rw,
    g_dev->uptime = isee_uptime,
    g_dev->get_token = isee_token,
    g_dev->km_encap = isee_km_encap,
    g_dev->get_hmac = isee_get_hmac,
    g_dev->sfs_read = isee_read,
    g_dev->sfs_write = isee_write,
    g_dev->verify_hmac = isee_verify_hmac,
    g_dev->get_hmac_key = isee_get_hmac_key;
};