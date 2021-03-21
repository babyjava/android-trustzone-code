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
#include <stdio.h>
#include <inttypes.h>
#include "platform_common.h"
#include "platform_device.h"


//microtrust
#ifdef TEE_ISEE
#include "ut_pf_fp_ta.h"
#include "ut_pf_cp.h"
#include "ut_pf_ts.h"
#include "ut_pf_spi.h"
#include "ut_pf_km.h"
#include "ut_pf_time.h"
#include "ut_sys_misc.h"
#include "ut_pf_log.h"
#include "isee/spi.h"
#include <tee_internal_api.h>
#define loge ut_pf_log_err
#define logd ut_pf_log_dbg
#endif

//qualcomm
#ifdef TEE_QSEE
#include <comdef.h>
#include "qsee_hmac.h"
#include "qsee_prng.h"
#include "qsee_heap.h"
#include "qsee_timer.h"
#include "qsee_spi.h"
#include "qsee_i2c.h"
#include "qsee_fs.h"
#include "qsee_sfs.h"
#include "qsee_message.h"
#include "qsee_log.h"
#include <gpPersistObjCrypto.h>
#include <gpPersistObjFileIO.h>
#define loge(fmt, ...) qsee_log(QSEE_LOG_MSG_FATAL, fmt, __VA_ARGS__)
#define logd(fmt, ...) qsee_log(QSEE_LOG_MSG_DEBUG, fmt, __VA_ARGS__)
#endif

//trustonic
#ifdef TEE_KINIBI
#include "tlStd.h"
#include "tee_internal_api.h"
#include "TlApi/TlApi.h"
#include "TlApi/TlApiError.h"
#include "spi.h"
#include "drtlspi_api.h"
#define logd tlApiLogvPrintf
#define loge tlApiLogPrintf
#define EXIT_ERROR  ((uint32_t)(-1))
#endif

//sprd
#ifdef TEE_TRUSTY
#include <stdlib.h>
#include <string.h>
#include <trusty_std.h>
#include <trusty_ipc.h>
#include <sprd_pal_fp_default.h>
#include <io_device_def.h>
#include <lib/storage/storage.h>
#include "lib/rng/trusty_rng.h"
#include <lib/keymaster/keymaster.h>
#include <openssl/hmac.h>

#define TADEMO_PORT "com.android.sampleta.demo"
#define TADEMO_MAX_BUFFER_LENGTH 1024*32
#define loge(fmt, ...) fprintf(stderr, "%s: %d: " fmt, LOG_TAG, __LINE__,  ## __VA_ARGS__)
#define logd(fmt, ...) fprintf(stderr, "%s: %d: " fmt, LOG_TAG, __LINE__,  ## __VA_ARGS__)
typedef void (*p_func_t)(const uevent_t *ev, void* p_func);
void trusty_router(const uevent_t *ev, void* p_func);
void trusty_accept(const uevent_t *ev, void* p_func);
#endif

void ta_router(void);
void platform_init(void);

extern struct platform_device *g_dev;
extern struct tee_in_buf *g_in;
extern struct tee_out_buf *g_out;
