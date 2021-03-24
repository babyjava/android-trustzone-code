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
#include "gp/tee_client_api.h"

#define GP_API_LIB "libGP.so"
static TEEC_Context g_context;
static TEEC_Session g_session;
static TEEC_Operation g_operation;
static TEEC_SharedMemory g_in_mem, g_out_mem;
static TEEC_Result (*PREFIX(TEEC_InitializeContext))(const char *, TEEC_Context *);
static TEEC_Result (*PREFIX(TEEC_OpenSession))(TEEC_Context *context,
                 TEEC_Session *session, const TEEC_UUID *destination,
                 uint32_t connectionMethod, const void *connectionData,
                 TEEC_Operation *operation, uint32_t *returnOrigin);
static TEEC_Result (*PREFIX(TEEC_RegisterSharedMemory))(TEEC_Context *context,
                      TEEC_SharedMemory *sharedMem);
static TEEC_Result (*PREFIX(TEEC_AllocateSharedMemory))(TEEC_Context *context,
                      TEEC_SharedMemory *sharedMem);
static TEEC_Result (*PREFIX(TEEC_InvokeCommand))(TEEC_Session *session,
                   uint32_t commandID, TEEC_Operation *operation, uint32_t *returnOrigin);
static void (*PREFIX(TEEC_CloseSession))(TEEC_Session *session);
static void (*PREFIX(TEEC_FinalizeContext))(TEEC_Context *context);
static void (*PREFIX(TEEC_ReleaseSharedMemory))(TEEC_SharedMemory *sharedMemory);

static int gp_cmd(struct tee_client_device *dev)
{
    pthread_mutex_lock(&dev->mutex);
    memcpy(g_in_mem.buffer, &dev->in, IN_BUF_LEN);
    int status = PREFIX(TEEC_InvokeCommand)(&g_session, GP_CMD, &g_operation, NULL);
    if_err(status != GENERIC_OK, goto end;, "%d %d", dev->in.cmd, status);
    memcpy(&dev->out, g_out_mem.buffer, OUT_BUF_LEN);
end:
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static int gp_exit(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    PREFIX(TEEC_ReleaseSharedMemory)(&g_in_mem);
    PREFIX(TEEC_ReleaseSharedMemory)(&g_out_mem);
    PREFIX(TEEC_CloseSession)(&g_session);
    PREFIX(TEEC_FinalizeContext)(&g_context);
    dlclose(dev->handle);
    return GENERIC_OK;
}

static int gp_init(void)
{
    int status = 0;
    char host_name[] = "bta_loader";
    TEEC_UUID uuid = { 0xf832e4d8, 0x17d3, 0x6688, { 0x9b, 0x01, 0xc9, 0x92, 0xd5, 0x6d, 0x78, 0xbf } };
    ALOGD("%s", __func__);
    memset(&g_operation, 0, sizeof(TEEC_Operation));
    g_operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT, TEEC_MEMREF_PARTIAL_OUTPUT, TEEC_NONE , TEEC_NONE);
    g_operation.started = 1;
    g_operation.params[0].memref.parent = &g_in_mem;
    g_operation.params[0].memref.size = IN_BUF_LEN;
    g_operation.params[1].memref.parent = &g_out_mem;
    g_operation.params[1].memref.size = OUT_BUF_LEN;
    g_in_mem.size  = IN_BUF_LEN;
    g_in_mem.flags = TEEC_MEM_INPUT;
    g_out_mem.size  = OUT_BUF_LEN;
    g_out_mem.flags = TEEC_MEM_OUTPUT;
    status = PREFIX(TEEC_InitializeContext)(host_name, &g_context);
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%s", host_name);
    status = PREFIX(TEEC_OpenSession)(&g_context, &g_session, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL, NULL);
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%s", "OpenSession");
    status = PREFIX(TEEC_AllocateSharedMemory)(&g_context, &g_in_mem);
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%s", "Memory in");
    status = PREFIX(TEEC_AllocateSharedMemory)(&g_context, &g_out_mem);
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%s", "Memory out");
    return GENERIC_OK;
}

int gp_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(GP_API_LIB, RTLD_LAZY);
    if_err(!dev->handle, return GENERIC_ERR;, "%s", GP_API_LIB);
    PREFIX(TEEC_InitializeContext) = (TEEC_Result(*)(const char *, TEEC_Context *))DLSYM(TEEC_InitializeContext);
    PREFIX(TEEC_FinalizeContext) = (void(*)(TEEC_Context *))DLSYM(TEEC_FinalizeContext);
    PREFIX(TEEC_OpenSession) = (TEEC_Result(*)(TEEC_Context *, TEEC_Session *, const TEEC_UUID *, uint32_t ,
                                   const void *, TEEC_Operation *, uint32_t *))DLSYM(TEEC_OpenSession);
    PREFIX(TEEC_CloseSession) = (void(*)(TEEC_Session *))DLSYM(TEEC_CloseSession);
    PREFIX(TEEC_InvokeCommand) = (TEEC_Result(*)(TEEC_Session *, uint32_t, TEEC_Operation *, uint32_t *))DLSYM(TEEC_InvokeCommand);
    PREFIX(TEEC_RegisterSharedMemory) = (TEEC_Result(*)(TEEC_Context *, TEEC_SharedMemory *))DLSYM(TEEC_RegisterSharedMemory);
    PREFIX(TEEC_AllocateSharedMemory) = (TEEC_Result(*)(TEEC_Context *, TEEC_SharedMemory *))DLSYM(TEEC_AllocateSharedMemory);
    PREFIX(TEEC_ReleaseSharedMemory) = (void(*)(TEEC_SharedMemory *))DLSYM(TEEC_ReleaseSharedMemory);

    dev->tee_init = gp_init;
    dev->tee_cmd = gp_cmd;
    dev->tee_exit = gp_exit;
    return GENERIC_OK;
}