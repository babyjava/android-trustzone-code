#include "lib_hal.h"

static int g_handle = -1;
static uint8_t* g_mem;
extern struct hal_device g_device;
#define ISEE_DEVICE "/dev/teei_fp"
#define ISEE_MAGIC_NO _IO('T', 0x2)

static int isee_cmd(struct tee_client_device *dev, struct tee_in_buf *in, struct tee_out_buf *out){
    pthread_mutex_lock(&dev->mutex);
    int status = 0;
    memcpy(g_mem, in, IN_BUF_LEN);
    status = ioctl(g_handle, ISEE_MAGIC_NO, g_mem);
    if_err(status != GENERIC_OK, goto end;, "%d %d", in->cmd, status);
    memcpy(out, g_mem + IN_BUF_LEN, OUT_BUF_LEN);
end:
    pthread_mutex_unlock(&dev->mutex);
    return status;
}

static void isee_exit(struct tee_client_device *dev){
    ALOGD("%s", __func__);
    if(g_mem) free(g_mem);
    if(g_handle > 0) close(g_handle);
    if(dev->handle) dlclose(dev->handle);
}

static int isee_init(void)
{
    ALOGD("%s", __func__);
    g_handle = open(ISEE_DEVICE, O_RDWR);
    if_err(g_handle < GENERIC_OK, return GENERIC_ERR;, "%s", "open");

    g_mem = malloc(IN_BUF_LEN + OUT_BUF_LEN);
    if_err(!g_mem, return GENERIC_ERR;, "%s", "malloc");
    return GENERIC_OK;
}

int isee_client_open(struct tee_client_device *dev)
{
    dev->tee_init = isee_init;
    dev->tee_cmd = isee_cmd;
    dev->tee_exit = isee_exit;
    return GENERIC_OK;
}