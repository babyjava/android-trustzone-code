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

#ifdef TEE_QSEE
void tz_app_init(void)
{
   qsee_log_set_mask(QSEE_LOG_MSG_FATAL | QSEE_LOG_MSG_ERROR | QSEE_LOG_MSG_DEBUG);
   platform_init();
   logd("%s",__func__);
}

void tz_app_cmd_handler(void* cmd, uint32 cmdlen, void* rsp, uint32 rsplen)
{
    if(cmdlen < IN_BUF_LEN || rsplen < OUT_BUF_LEN){
        loge("%s err cmdlen: %d - %d, rsplen: %d - %d", __func__, cmdlen, IN_BUF_LEN, rsplen, OUT_BUF_LEN);
        return;
    }
    if ((!cmd) || (!rsp)){
        loge("%s err buf ", __func__);
        return;
    }
    memcpy(g_in, cmd, IN_BUF_LEN);
    g_out = rsp;
    ta_router();
}

void tz_app_shutdown(void)
{
    logd("%s",__func__);
}
#endif

#ifdef TEE_KINIBI
DECLARE_TRUSTLET_MAIN_STACK(4096)
_TLAPI_ENTRY void tlMain(const addr_t tciBuffer, const uint32_t tciBufferLen)
{
    if((NULL == tciBuffer) || ((IN_BUF_LEN + OUT_BUF_LEN) > tciBufferLen)) {
        loge("%s, err\n", __func__);
        tlApiExit(EXIT_ERROR);
    }
    platform_init();
    for(;;){
        tlApiWaitNotification(TLAPI_INFINITE_TIMEOUT);
        memcpy(g_in, tciBuffer, IN_BUF_LEN);
        g_out = (tciBuffer + IN_BUF_LEN);
        ta_router();
        tlApiNotify();
    }
}
#endif

#ifdef TEE_GP
#include <tee_internal_api.h>
TEE_Result TA_EXPORT TA_CreateEntryPoint(void)
{
    loge("%s %s", LOG_TAG, __func__);
    platform_init();
    return TEE_SUCCESS;
}

TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes, TEE_Param params[4], void** sessionContext)
{
    (void)paramTypes; (void)params; (void)sessionContext;
    loge("%s %s", LOG_TAG, __func__);
    return TEE_SUCCESS;
}

TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void* sessionContext, uint32_t commandID, uint32_t paramTypes, TEE_Param params[4])
{
    (void)sessionContext;
    if((IN_BUF_LEN != params[0].memref.size) || (OUT_BUF_LEN != params[1].memref.size)){
        loge("err cmdlen = %d - %d, rsplen = %d - %d", params[0].memref.size, IN_BUF_LEN, params[1].memref.size, OUT_BUF_LEN);
        return TEE_ERROR_GENERIC;
    }
    if (paramTypes != TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT, TEE_PARAM_TYPE_MEMREF_OUTPUT, TEE_PARAM_TYPE_NONE, TEE_PARAM_TYPE_NONE)) {
        loge("%s err paramTypes = %d", __func__, paramTypes);
        return TEE_ERROR_GENERIC;
    }
    if (GP_CMD != commandID) {
        loge("%s err commandID = %d", __func__, commandID);
        return TEE_ERROR_GENERIC;
    }
    memcpy(g_in, params[0].memref.buffer, IN_BUF_LEN);
    g_out = params[1].memref.buffer;
    ta_router();
    return TEE_SUCCESS;
}

void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext)
{
    (void)sessionContext;
    loge("%s %s", LOG_TAG, __func__);
}

void TA_EXPORT TA_DestroyEntryPoint(void)
{
    loge("%s %s", LOG_TAG, __func__);
}
#endif

#ifdef TEE_ISEE
void ut_pf_fp_init(void)
{
    logd("%s",__func__);
    platform_init();
}

ut_int32_t ut_pf_fp_invoke_command(ut_pf_fp_cmd_header_t *header, void *data, ut_uint32_t param_length)
{
    if(param_length != (IN_BUF_LEN + OUT_BUF_LEN)){
        loge("%s err len = %d - %d - %d", __func__, param_length, IN_BUF_LEN, OUT_BUF_LEN);
        return TEE_ERROR_GENERIC;
    }
    memcpy(g_in, data, IN_BUF_LEN);
    g_out = (data + IN_BUF_LEN);
    ta_router();
    return TEE_SUCCESS;
}
#endif

#ifdef TEE_TRUSTY
int main(void)
{
    uevent_t ue;
    p_func_t p_func;
    long rc = port_create(TADEMO_PORT, 1, TADEMO_MAX_BUFFER_LENGTH, IPC_PORT_ALLOW_NS_CONNECT);
    if (rc < 0) {
        loge("%s err port_create %s, rc = %ld\n", __func__, TADEMO_PORT, rc);
        return rc;
    }
    platform_init();
    p_func = trusty_accept;
    while (true) {
        ue.handle = INVALID_IPC_HANDLE;
        ue.event = 0;
        ue.cookie = NULL;
        rc = wait_any(&ue, INFINITE_TIME);
        if (rc == GENERIC_OK) {
            p_func(&ue, (void *)p_func);
        } else {
            loge("%s err rc = %ld\n", __func__, rc);
            break;
        }
    }
    return 0;
}
#endif