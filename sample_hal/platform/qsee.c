#include "lib_hal.h"
#include "qsee/QSEEComAPI.h"

#define QSEE_IN_LEN QSEECOM_ALIGN(IN_BUF_LEN)
#define QSEE_OUT_LEN QSEECOM_ALIGN(OUT_BUF_LEN)
#define QSEE_TA_PATH "/vendor/firmware_mnt/image"
#define QSEE_TA_NAME "sampleta"
#define QSEE_API_LIB "libQSEEComAPI.so"
static struct QSEECom_handle *g_handle;
static int (*PREFIX(QSEECom_start_app))(struct QSEECom_handle **, const char *, const char *, uint32_t);
static int (*PREFIX(QSEECom_shutdown_app))(struct QSEECom_handle **);
static int (*PREFIX(QSEECom_send_cmd))(struct QSEECom_handle *, void *, uint32_t, void *, uint32_t);

static int qsee_cmd(struct tee_client_device *dev, struct tee_in_buf *in, struct tee_out_buf *out)
{
    int status = PREFIX(QSEECom_send_cmd)(g_handle, in, QSEE_IN_LEN, out, QSEE_OUT_LEN);
    if_err(status != GENERIC_OK,  , "%d %d", in->cmd, status);
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static void qsee_exit(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    if(g_handle) PREFIX(QSEECom_shutdown_app)(&g_handle);
    if(dev->handle) dlclose(dev->handle);
}

static int qsee_init(void)
{
    int status = PREFIX(QSEECom_start_app)(&g_handle, QSEE_TA_PATH, QSEE_TA_NAME, (QSEE_IN_LEN + QSEE_OUT_LEN));
    ALOGD("%s", __func__);
    if_err(status != GENERIC_OK,  , "%s", QSEE_TA_PATH QSEE_TA_NAME);
    return status;
}

int qsee_client_open(struct tee_client_device *dev)
{
    ALOGD("%s", __func__);
    dev->handle = dlopen(QSEE_API_LIB, RTLD_LAZY);
    if_err(!dev->handle, return GENERIC_ERR;, QSEE_API_LIB);
    PREFIX(QSEECom_start_app) = (int(*)(struct QSEECom_handle **, const char *, const char *, uint32_t))DLSYM(QSEECom_start_app);
    PREFIX(QSEECom_send_cmd) = (int(*)(struct QSEECom_handle *, void *, uint32_t, void *, uint32_t))DLSYM(QSEECom_send_cmd);
    PREFIX(QSEECom_shutdown_app) = (int(*)(struct QSEECom_handle **))DLSYM(QSEECom_shutdown_app);

    dev->tee_init = qsee_init;
    dev->tee_cmd = qsee_cmd;
    dev->tee_exit = qsee_exit;
    return GENERIC_OK;
}