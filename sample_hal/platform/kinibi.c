#include "lib_hal.h"
#include "kinibi/MobiCoreDriverApi.h"

#define KINIBI_API_LIB "libMcTrusty.so"
#define TA_PATH "/system/app/mcRegistry/88866600000000000000000000000000.tlbin"
static uint8_t *g_mem;
static void *g_ta;
static mcSessionHandle_t g_session;

static mcResult_t (*PREFIX(mcOpenDevice))(uint32_t);
static mcResult_t (*PREFIX(mcCloseDevice))(uint32_t);
static mcResult_t (*PREFIX(mcCloseSession))(mcSessionHandle_t *);
static mcResult_t (*PREFIX(mcNotify))(mcSessionHandle_t *);
static mcResult_t (*PREFIX(mcWaitNotification))(mcSessionHandle_t *, int32_t);
static mcResult_t (*PREFIX(mcOpenTrustlet))(mcSessionHandle_t *, mcSpid_t, uint8_t *, uint32_t, uint8_t *, uint32_t);

static int kinibi_cmd(struct tee_client_device *dev, struct tee_in_buf *in, struct tee_out_buf *out)
{
    pthread_mutex_lock(&dev->mutex);
    int status = 0;
    memcpy(g_mem, in, IN_BUF_LEN);
    status = PREFIX(mcNotify)(&g_session);
    if_err(status != GENERIC_OK, goto end;, "%d %d", in->cmd, status);
    status = PREFIX(mcWaitNotification)(&g_session, 1000);
    if_err(status != GENERIC_OK,  , "%d %d", in->cmd, status);
end:
    memcpy(out, g_mem + IN_BUF_LEN, OUT_BUF_LEN);
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static void kinibi_exit(struct tee_client_device *dev)
{
    ALOGE("%s", __func__);
    PREFIX(mcCloseSession)(&g_session);
    PREFIX(mcCloseDevice)(MC_DEVICE_ID_DEFAULT);
    if(g_ta) free(g_ta);
    if(dev->handle) dlclose(dev->handle);
}

static int kinibi_init(void)
{   int fd = 0;
    int len = 0;
    int status = PREFIX(mcOpenDevice)(MC_DEVICE_ID_DEFAULT);
    ALOGD("%s", __func__);
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%d", status);

    fd = open(TA_PATH, O_RDONLY);
    if_err(fd < GENERIC_OK, return GENERIC_ERR;, "%d", fd);

    len = lseek(fd, 0, SEEK_END);
    if_err(len < GENERIC_OK, return GENERIC_ERR;, "%d", len);

    g_ta = malloc(len);
    if_err(!g_ta, return GENERIC_ERR;, "%s", "malloc");

    status = read(fd, g_ta, len);
    if_err(status != len, return GENERIC_ERR;, "%d", status);

    g_mem = malloc(IN_BUF_LEN + OUT_BUF_LEN);
    if_err(!g_mem, return GENERIC_ERR;, "%s", "malloc");

    g_session.deviceId = MC_DEVICE_ID_DEFAULT;
    status = PREFIX(mcOpenTrustlet)(&g_session, MC_SPID_SYSTEM, g_ta, len, g_mem, (IN_BUF_LEN + OUT_BUF_LEN));
    if_err(status != GENERIC_OK, return GENERIC_ERR;, "%s", "mcOpenTrustlet");
    return GENERIC_OK;
}

int kinibi_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(KINIBI_API_LIB, RTLD_LAZY);
    if_err(!dev->handle, return GENERIC_ERR;, "%s", KINIBI_API_LIB);
    PREFIX(mcOpenDevice) = (mcResult_t(*)(uint32_t))DLSYM(mcOpenDevice);
    PREFIX(mcCloseDevice) = (mcResult_t(*)(uint32_t))DLSYM(mcCloseDevice);
    PREFIX(mcCloseSession) = (mcResult_t(*)(mcSessionHandle_t *))DLSYM(mcCloseSession);
    PREFIX(mcNotify) = (mcResult_t(*)(mcSessionHandle_t *))DLSYM(mcNotify);
    PREFIX(mcWaitNotification) = (mcResult_t(*)(mcSessionHandle_t *, int32_t))DLSYM(mcWaitNotification);
    PREFIX(mcOpenTrustlet) = (mcResult_t(*)(mcSessionHandle_t *, mcSpid_t, uint8_t *, uint32_t, uint8_t *, uint32_t))DLSYM(mcOpenTrustlet);
    dev->tee_init = kinibi_init;
    dev->tee_cmd = kinibi_cmd;
    dev->tee_exit = kinibi_exit;
    return GENERIC_OK;
}