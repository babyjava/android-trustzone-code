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
#define GENERIC_OK 0
#define GENERIC_ERR -1
#define HMAC_LEN 32
#define GP_CMD 0xbeef
#define BUF_LEN 1024*32
#define IN_BUF_LEN sizeof(struct tee_in_buf)
#define OUT_BUF_LEN sizeof(struct tee_out_buf)

struct tee_in_buf
{
    uint32_t cmd;
    char name[32];
    uint32_t spi_num;
    uint32_t i2c_num;
    uint32_t buf_len;
    uint8_t  buf[BUF_LEN];
};

struct tee_out_buf
{
    uint32_t status;
    int32_t sys_err;
    char err_line[32];
    uint32_t hw_id;
    uint32_t ta_ver;
    uint32_t buf_len;
    uint8_t  buf[BUF_LEN];
};

enum tee_cmd
{
    TEE_CMD_SPI_TEST,
    TEE_CMD_I2C_TEST,
    TEE_CMD_WRITE_TEST,
    TEE_CMD_READ_TEST,
    TEE_CMD_READ_ID,
    TEE_CMD_RELEASE,
    TEE_CMD_END,
};
