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
#define TA_VERSION "1.0.0.0"
#define LOG_TAG "talog"
#define LOG_RECORD snprintf(g_out->sys_err_line, sizeof(g_out->sys_err_line), LOG_TAG "err %s:%d", __FILE__, __LINE__);g_out->status = GENERIC_ERR;
#define if_err(a, b); if(a) { LOG_RECORD b; }

typedef void (*V_FUNC)(void);

struct platform_device {
    V_FUNC km_decap;
    V_FUNC verify_token;
    V_FUNC i2c_rw;
    void (*km_encap)(uint8_t *msg, uint32_t msg_len, uint8_t *hmac, uint32_t *len);
    void (*get_hmac)(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac);
    void (*get_hmac_key)(uint8_t *key, uint32_t *key_len);
    void (*verify_hmac)(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac);
    void (*get_token)(uint64_t *token, uint64_t *t_time);
    void (*crypto)(void *in, void *out, uint32_t len);
    void (*decrypt)(void *in, void *out, uint32_t len);
    void (*sfs_read)(char *name, void *buf, size_t *buf_len);
    void (*sfs_write)(char *name, void *buf, size_t buf_len);
    void (*read)(char *name, void *buf, size_t *buf_len);
    void (*write)(char *name, void *buf, size_t buf_len);
    void (*sleep)(uint64_t us);
    void (*spi_rw)(void *in, uint32_t in_len, void *out, uint32_t out_len);
    void (*uptime)(uint64_t *t);
    void* (*malloc)(size_t s);
    void (*free)(void *p);
    uint8_t buf[IN_BUF_LEN + OUT_BUF_LEN];
    uint8_t iv[32];
};

void hw_id(void);

