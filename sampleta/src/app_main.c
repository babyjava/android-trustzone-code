/*
  @file app_main.c
  @brief App main entry point.

*/
/*===========================================================================
  Copyright (c) 2020-2021 by XXX Technologies, Incorporated.  All Rights Reserved.
  ===========================================================================*/

/*=============================
==============================================

  EDIT HISTORY FOR FILE
  $Header: talibv1/app_main.c
  $Author: sheldon $

  # when       who     what, where, why
  # --------   ---     ---------------------------------------------------------
  # 20/02/2020   sheldon     init
  ===========================================================================*/
#include "platform_tee.h"

#ifdef TEE_QSEE
void tz_app_init(void)
{
   qsee_log_set_mask(QSEE_LOG_MSG_FATAL | QSEE_LOG_MSG_ERROR | QSEE_LOG_MSG_DEBUG);
   logd("%s",__func__);
   platform_init();
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
    ta_router();
    memcpy(rsp, g_out, OUT_BUF_LEN);
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
        ta_router();
        memcpy((tciBuffer + IN_BUF_LEN ), g_out, OUT_BUF_LEN);
        tlApiNotify();
    }
}
#endif

#ifdef TEE_GP
#include <tee_internal_api.h>
TEE_Result TA_EXPORT TA_CreateEntryPoint(void){
   loge("%s %s", LOG_TAG, __func__);
    return TEE_SUCCESS;
}
TEE_Result TA_EXPORT TA_OpenSessionEntryPoint(uint32_t paramTypes, TEE_Param params[4], void** sessionContext){
    (void)paramTypes; (void)params; (void)sessionContext;
    loge("%s %s", LOG_TAG, __func__);
    return TEE_SUCCESS;
}
TEE_Result TA_EXPORT TA_InvokeCommandEntryPoint(void* sessionContext, uint32_t commandID, uint32_t paramTypes, TEE_Param params[4]){
    (void)sessionContext;
    (void)paramTypes;
    struct fp_transfer_buf *in = (struct fp_transfer_buf *)params[0].memref.buffer;
    struct fp_receive_buf *out  = (struct fp_receive_buf *)params[1].memref.buffer;
    if((FP_TRANSFER_LEN != params[0].memref.size) || (FP_RECEIVE_LEN != params[1].memref.size)){
        loge("err cmdlen = %d - %d, rsplen = %d - %d", params[0].memref.size, FP_TRANSFER_LEN, params[1].memref.size, FP_RECEIVE_LEN);
        return TEE_ERROR_GENERIC;
    }
    if ((!in) || (!out) || (FP_GP_CMD != commandID)){
        loge("%s %s", LOG_TAG, __func__);
        return TEE_ERROR_GENERIC;
    }
    ta_router(in, out);
    return TEE_SUCCESS;
}
void TA_EXPORT TA_CloseSessionEntryPoint(void *sessionContext){
    (void)sessionContext;
    loge("%s %s", LOG_TAG, __func__);
}
void TA_EXPORT TA_DestroyEntryPoint(void){
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
    ta_router();
    memcpy((data + IN_BUF_LEN ), g_out, OUT_BUF_LEN);
    header->data_length = param_length;
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