#include "platform_tee.h"

extern long ioctl(uint32_t fd, uint32_t req, void* buf);

static struct WRITE_THEN_READ_STR g_spi_config =
{
    .max_speed_hz = 4800000,
    .chip_select = 0,
    .mode = 0,
    .bits_per_word = 8,
    .number = 0,
    .debug = 0,
};

static void trusty_spi_rw(void *in, uint32_t in_len, void *out, uint32_t out_len)
{
    g_spi_config.txbuf = (uint32_t)in;
    g_spi_config.rxbuf = (uint32_t)out;
    g_spi_config.len = out_len;

    g_out->sys_err = ioctl(IO_DEVICE_FP, SPI_WRITE_AND_READ, &g_spi_config);
    if_err(g_out->sys_err != 0,  );
}

static void trusty_uptime(uint64_t *t)
{
    int64_t cur_time = 0;
    g_out->sys_err = gettime(0, 0, &cur_time);
    if_err(g_out->sys_err != 0, return);
    *t = cur_time/1000;
}

static void trusty_token(uint64_t *token, uint64_t *t_time)
{
    int len = 0;
    int try = 10;
    while(try > 0){
        len = trusty_rng_secure_rand((uint8_t*)token, sizeof(uint64_t));
        if(len == sizeof(uint64_t)){
            trusty_uptime(t_time);
            return;
        }
    }
    LOG_RECORD
}

static void trusty_get_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    unsigned int hmac_len = 0;
    hmac = HMAC(EVP_sha256(), key, key_len, msg, msg_len, hmac, &hmac_len);
    if_err(hmac == NULL,  );
}

static void trusty_verify_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    int i = 0;
    uint8_t hmac_t[HMAC_LEN];
    trusty_get_hmac(msg, msg_len, key, key_len, hmac_t);
    if_err(g_out->sys_err != 0, return);
    for(i = 0; i < HMAC_LEN; i++){
        if_err(hmac[i] ^ hmac_t[i], break)
    }
}

static void trusty_get_hmac_key(uint8_t *key, uint32_t *key_len)
{
    keymaster_session_t session;
    g_out->sys_err = keymaster_open();
    if_err(g_out->sys_err, return);

    session = (keymaster_session_t) g_out->sys_err;
    g_out->sys_err = keymaster_get_auth_token_key(session, &key, key_len);
    if_err(g_out->sys_err,  );

    keymaster_close(session);
}

static void trusty_sleep(uint64_t us)
{
    nanosleep(0, 0, us*1000);
}

static void trusty_write(char *name, void *buf, size_t buf_len)
{
    storage_session_t session;
    file_handle_t handle;
    g_out->sys_err = storage_open_session_async(&session, STORAGE_CLIENT_TD_PORT, 100);
    if_err(g_out->sys_err < 0, return);

    g_out->sys_err = storage_open_file(session, &handle, name, STORAGE_FILE_OPEN_CREATE | STORAGE_FILE_OPEN_TRUNCATE, 0);
    if_err(g_out->sys_err < 0, goto end);

    g_out->sys_err = storage_write(handle, 0, buf, buf_len, 0);
    if_err(g_out->sys_err < 0,  );

    storage_close_file(handle);
    storage_end_transaction(session, true);
end:
    storage_close_session(session);
}

static void trusty_read(char *name, void *buf, size_t *buf_len)
{
    storage_session_t session;
    file_handle_t handle;
    storage_off_t size = 0;
    g_out->sys_err = storage_open_session_async(&session, STORAGE_CLIENT_TD_PORT, 100);
    if_err(g_out->sys_err < 0, return);

    g_out->sys_err = storage_open_file(session, &handle, name, STORAGE_FILE_OPEN_CREATE, 0);
    if_err(g_out->sys_err < 0, goto close_session);

    g_out->sys_err = storage_get_file_size(handle, &size);
    if_err(g_out->sys_err < 0, goto end);

    if_err(size == 0, goto end);
    if_err(*buf_len < size, goto end);

    g_out->sys_err = storage_read(handle, 0, buf, size);
    if_err(g_out->sys_err < 0,  );

end:
    storage_close_file(handle);
    storage_end_transaction(session, true);
close_session:
    storage_close_session(session);
    *buf_len = size;
}


void platform_init(void)
{
    if (g_dev == NULL) {
        g_dev = (struct platform_device *)malloc(sizeof(struct platform_device));
        if (g_dev == NULL) {
            loge("%s, err malloc\n", __func__);
            return;
        }
    }
    g_in = (struct tee_in_buf *)g_dev->buf;
    g_out = (struct tee_out_buf *)(g_in + IN_BUF_LEN);
    g_dev->free = free,
    g_dev->malloc = malloc,
    g_dev->sleep = trusty_sleep,
    g_dev->uptime = trusty_uptime,
    g_dev->spi_rw = trusty_spi_rw,
    g_dev->sfs_read = trusty_read,
    g_dev->sfs_write = trusty_write,
    g_dev->get_token = trusty_token,
    g_dev->get_hmac = trusty_get_hmac,
    g_dev->verify_hmac = trusty_verify_hmac,
    g_dev->get_hmac_key = trusty_get_hmac_key;
};

void trusty_router(const uevent_t *ev, void* p_func)
{
    long rc = 0;
    ipc_msg_info_t in_buf;
    struct iovec iov = { g_dev->buf, in_buf.len };
    ipc_msg_t msg = {1, &iov, 0, NULL};

    if (!(ev->event & IPC_HANDLE_POLL_MSG)) {
        logd("%s, no event = 0x%x\n", __func__, ev->event);
        return;
    }
    rc = get_msg(ev->handle, &in_buf);
    if (rc != GENERIC_OK) {
        loge("%s, err get msg rc = %ld\n", __func__, rc);
        return;
    }
    rc = read_msg(ev->handle, in_buf.id, 0, &msg);
    if(rc != IN_BUF_LEN){
        loge("%s, err buf len = %ld - %d\n", __func__, rc, IN_BUF_LEN);
        return;
    }
    rc = put_msg(ev->handle, in_buf.id);
    if (rc != GENERIC_OK){
        loge("%s, err put_msg rc = %ld\n", __func__, rc);
        return;
    }
    ta_router();
    rc = send_msg(ev->handle, &msg);
    if (rc < 0) {
        loge("%s, err send_msg rc = %ld\n", __func__, rc);
        (void)p_func;
    }
}

void trusty_accept(const uevent_t *ev, void *p_func)
{
    uuid_t peer_uuid;
    long rc = 0;
    if (ev->event & IPC_HANDLE_POLL_READY) {
        rc = accept(ev->handle, &peer_uuid);
        if (rc < 0) {
            loge("%s err rc = %ld\n", __func__, rc);
        } else {
            logd("%s\n", __func__);
            (void)p_func;
            p_func = trusty_router;
        }
    }
    loge("%s, err event  = 0x%x\n", __func__, ev->event);
}
