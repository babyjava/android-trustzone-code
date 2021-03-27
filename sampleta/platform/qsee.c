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

static const qsee_spi_config_t g_spi_config = {
    .spi_bits_per_word = 8,
    .spi_clk_always_on = QSEE_SPI_CLK_NORMAL,
    .spi_clk_polarity = QSEE_SPI_CLK_IDLE_LOW,
    .spi_cs_mode = QSEE_SPI_CS_KEEP_ASSERTED,
    .spi_cs_polarity = QSEE_SPI_CS_ACTIVE_LOW,
    .spi_shift_mode = QSEE_SPI_INPUT_FIRST_MODE,
    .max_freq = 19200000,
    .hs_mode = 0,
    .loopback = 0,
};

static void qsee_spi_rw(void *in, uint32_t in_len, void *out, uint32_t out_len){
    qsee_spi_transaction_info_t spi_w = {
        .buf_addr = in,
        .buf_len = in_len,
    };
    qsee_spi_transaction_info_t spi_r = {
        .buf_addr = out,
        .buf_len = out_len,
    };

    g_out->sys_err = qsee_spi_open(g_in->spi_num);
    if_err(g_out->sys_err, return);

    g_out->sys_err = qsee_spi_full_duplex(g_in->spi_num, &g_spi_config, &spi_w, &spi_r);
    if_err(g_out->sys_err, );

    qsee_spi_close(g_in->spi_num);
}

static void qsee_i2c_rw(void)
{
    qsee_i2c_transaction_info_t readInfo;
    qsee_i2c_config_t i2c_config;
    qsee_i2c_bus_config_t busConfig;
    qsee_i2c_slave_device_config_t deviceConfig;
    i2c_config.p_bus_config = &busConfig;
    i2c_config.p_slave_config = &deviceConfig;
    readInfo.start_addr = (uint32_t)g_in->buf;
    readInfo.p_buf = g_out->buf;
    readInfo.buf_len = g_out->buf_len;
    readInfo.total_bytes = 0;
    g_out->sys_err = qsee_i2c_open(g_in->i2c_num);
    if_err(g_out->sys_err, return);

    g_out->sys_err = qsee_i2c_read(g_in->i2c_num, &i2c_config, &readInfo);
    if_err(g_out->sys_err, );

    g_out->sys_err = qsee_i2c_close(g_in->i2c_num);
    if_err(g_out->sys_err, );
}

static void qsee_get_token(uint64_t *token, uint64_t *t_time){
    int len = 0;
    int try = 10;
    while(try--){
        len = qsee_prng_getdata((uint8_t *)token, sizeof(uint64_t));
        if(len == sizeof(uint64_t)){
            *t_time = qsee_get_uptime();
            return;
        }
    }
    LOG_RECORD
}

static void qsee_get_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac){
    g_out->sys_err = qsee_hmac(QSEE_HMAC_SHA256, msg, msg_len, key, key_len, hmac);
    if_err(g_out->sys_err,  );
}

static void qsee_verify_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac){
    uint32_t i = 0;
    uint8_t hmac_t[HMAC_LEN];
    qsee_get_hmac(msg, msg_len, key, key_len, hmac_t);
    if_err(g_out->sys_err != 0, return);
    for(i = 0; i < HMAC_LEN; i++){
        if_err(hmac[i] ^ hmac_t[i], break);
    }
}

static void qsee_get_hmac_key(uint8_t *key, uint32_t *key_len){
    char name[256] = {0};
    g_out->sys_err = qsee_decapsulate_inter_app_message(name, g_in->buf, g_in->buf_len, key, *key_len);
    if_err(g_out->sys_err,  );
}

static void qsee_km_encap(uint8_t *msg, uint32_t msg_len, uint8_t *hmac, uint32_t *len){
    char *name = "keymaster64";
    g_out->sys_err = qsee_encapsulate_inter_app_message(name, msg, msg_len, hmac, len);
    if_err(g_out->sys_err,  );
}

static void qsee_uptime(uint64_t *t){
    *t = qsee_get_uptime();
}

static void qsee_crypto(void *in, void *out, uint32_t len)
{
    if_err(!in || !out || !len , return);
    memcpy(out, in, len);
    g_out->sys_err = qsee_fts_encrypt(out, len, g_dev->iv, QSEE_FTS_CRYPTO_IV_SIZE);
    if_err(g_out->sys_err,  );
}


static void qsee_decrypt(void *in, void *out, uint32_t len)
{
    if_err(!in || !out || !len , return);
    memcpy(out, in, len);
    g_out->sys_err = qsee_fts_decrypt(out, len, g_dev->iv, QSEE_FTS_CRYPTO_IV_SIZE);
    if_err(g_out->sys_err,  );
}

static void qsee_write(char *name, void *buf, size_t buf_len)
{
    int fd = 0;
    if_err(!name || !buf || !buf_len , return);
    g_out->sys_err = qsee_sfs_open(name, O_CREAT | O_WRONLY);
    if_err(g_out->sys_err < 0, return);

    fd = g_out->sys_err;
    g_out->sys_err = qsee_sfs_write(fd, buf, buf_len);
    if_err(g_out->sys_err < 0,  )
    qsee_sfs_close(fd);
}

static void qsee_read(char *name, void *buf, size_t *buf_len)
{
    int fd = 0;
    uint32_t size = 0;
    if_err(!name || !buf || !buf_len , return);
    g_out->sys_err = qsee_sfs_open(name, O_CREAT | O_RDONLY);
    if_err(g_out->sys_err < 0, return);

    fd = g_out->sys_err;
    g_out->sys_err = qsee_sfs_getSize(fd, &size);
    if_err(g_out->sys_err < 0, goto end);

    if_err(size == 0, goto end);
    if_err(*buf_len < size, goto end);

    g_out->sys_err = qsee_sfs_read(fd, buf, size);
    if_err(g_out->sys_err != size,  );
end:
    qsee_sfs_close(fd);
    *buf_len = size;
}


void platform_init(struct platform_device *dev){
    if(dev == NULL) {
        g_dev = (struct platform_device *)qsee_malloc(sizeof(struct platform_device));
        if (g_dev == NULL) {
            loge("%s, err malloc\n", __func__);
            return;
        }
    }
    g_in = (struct tee_in_buf *)g_dev->io_buf;
    g_dev->free = qsee_free,
    g_dev->sfs_read = qsee_read,
    g_dev->sfs_write = qsee_write,
    g_dev->spi_rw = qsee_spi_rw,
    g_dev->i2c_rw = qsee_i2c_rw,
    g_dev->malloc = qsee_malloc,
    g_dev->uptime = qsee_uptime,
    g_dev->crypto = qsee_crypto,
    g_dev->decrypt = qsee_decrypt,
    g_dev->get_hmac = qsee_get_hmac,
    g_dev->km_encap = qsee_km_encap,
    g_dev->get_token = qsee_get_token,
    g_dev->verify_hmac = qsee_verify_hmac,
    g_dev->get_hmac_key = qsee_get_hmac_key;
};

