#include "platform_tee.h"

struct platform_device *g_dev;
struct tee_in_buf *g_in;
struct tee_out_buf *g_out;

static void spi_test(void)
{
    g_dev->spi_rw(g_in->buf, g_in->buf_len, g_out->buf, g_out->buf_len);
}

static void i2c_test(void)
{
    g_dev->i2c_rw();
}

static void write_test(void)
{
    g_dev->sfs_write(g_in->name, g_in->buf, g_in->buf_len);
}

static void read_rest(void)
{
    g_dev->sfs_read(g_in->name, g_out->buf, &g_out->buf_len);
}

static void init(void)
{
    hw_id();
}

static void release(void)
{
    g_dev->free(g_dev);
}
static void (*ta_cmd[])(void) = {
    spi_test,
    i2c_test,
    write_test,
    read_rest,
    init,
    release,
};

void ta_router(void)
{
    g_out->status = GENERIC_OK;
    if(g_in->cmd < TEE_CMD_END){
        ta_cmd[g_in->cmd]();
    } else {
        LOG_RECORD
    }
}