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

void hw_id(void)
{
    int len = 2;
    uint8_t buf = {0xaa, 0xbb};
    g_dev->spi_rw(buf, len, g_out->buf, len);
}

void hw_sleep(void)
{
    int len = 2;
    uint8_t buf = {0xaa, 0xbb};
    g_dev->spi_rw(buf, len, NULL, 0);
}

void hw_wakeup(void)
{
    int len = 2;
    uint8_t buf = {0xaa, 0xbb};
    g_dev->spi_rw(buf, len, NULL, 0);
}