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
#include <manifest.h>

TA_CONFIG_BEGIN

uuid : { 0xf8888888, 0x17d3, 0x6688, { 0x9b, 0x01, 0xc9, 0x92, 0xd5, 0x6d, 0x78, 0xbf } },
ta_flags : TA_FLAGS_INSTANCE_KEEP_ALIVE|TA_FLAGS_CAP_KM|TA_FLAGS_CAP_SPI,
log_tag : "sampleta",
ipc_buf_size: MAX_IPC_BUF_SIZE,

TA_CONFIG_END
