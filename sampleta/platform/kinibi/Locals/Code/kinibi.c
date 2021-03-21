#include "platform_tee.h"

static struct mt_chip_conf g_spi_cfg = {
    .setuptime = 15,
    .holdtime = 15,
    .high_time = 15,
    .low_time = 15,
    .cs_idletime = 20,
    .ulthgh_thrsh = 0,
    .cpol = SPI_CPOL_0,
    .cpha = SPI_CPHA_0,
    .rx_mlsb = SPI_MSB,
    .tx_mlsb = SPI_MSB,
    .tx_endian = SPI_LENDIAN,
    .rx_endian = SPI_LENDIAN,
    .com_mod = DMA_TRANSFER, //DMA_TRANSFER;//FIFO_TRANSFER;
    .pause = PAUSE_MODE_DISABLE,
    .finish_intr = FINISH_INTR_DIS,
    .deassert = DEASSERT_DISABLE,
    .ulthigh = ULTRA_HIGH_DISABLE,
    .tckdly = TICK_DLY0,
};

_TLAPI_EXTERN_C tlApiResult_t drSpiSend(const void *tx_buf, void *rx_buf, uint32_t len, struct mt_chip_conf *chip_conf, uint32_t flag)
{
    (void)tx_buf;
    (void)rx_buf;
    (void)len;
    (void)chip_conf;
    (void)flag;
    return 0;
}

static void kinibi_spi_rw(void *in, uint32_t in_len, void *out, uint32_t out_len)
{
    (void)in_len;
    g_out->sys_err = drSpiSend(in, out, out_len, &g_spi_cfg, 1);
    if_err(g_out->sys_err != 0,  );
}

static void kinibi_uptime(uint64_t *t)
{
    tlApiGetSecureTimestamp(t);
    *t = (*t / 1000);
}

static void kinibi_token(uint64_t *token, uint64_t *t_time)
{
    size_t len = 0;
    int try = 10;
    while(try--){
        len = tlApiRandomGenerateData(TLAPI_ALG_SECURE_RANDOM, (uint8_t *)token, &len);
        if(len == sizeof(uint64_t) && *token != 0){
            kinibi_uptime(t_time);
            return;
        }
    }
    LOG_RECORD
}

static void kinibi_get_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    size_t hmac_len = 0;
    tlApiCrSession_t session = CR_SID_INVALID;
    tlApiKey_t api_key = {0};
    tlApiSymKey_t sym_key = {key, key_len};
    api_key.symKey = &sym_key;
    g_out->sys_err = tlApiSignatureInit(&session, &api_key, TLAPI_MODE_SIGN, TLAPI_ALG_HMAC_SHA_256);
    if_err(g_out->sys_err != 0, return);

    g_out->sys_err = tlApiSignatureSign(session, msg, msg_len, hmac, &hmac_len);
    if_err(g_out->sys_err != 0,  );
}

static void kinibi_verify_hmac(uint8_t *msg, uint32_t msg_len, uint8_t *key, uint32_t key_len, uint8_t *hmac)
{
    int i = 0;
    uint8_t hmac_t[HMAC_LEN];
    kinibi_get_hmac(msg, msg_len, key, key_len, hmac_t);
    if_err(g_out->sys_err != 0, return);
    for(i = 0; i < HMAC_LEN; i++){
        if_err(hmac[i] ^ hmac_t[i], break);
    }
}

static void kinibi_get_hmac_key(uint8_t *key, uint32_t *key_len){
    uint8_t salt[] = {'k','e','y','m','a','s','t','e','r','1', 0, 0};
    g_out->sys_err = tlApiDeriveKey(salt, sizeof(salt), key, *key_len, MC_SO_CONTEXT_SP, /* MC_SPID_SYSTEM */ MC_SO_LIFETIME_POWERCYCLE);
    if_err(g_out->sys_err != 0,  );
}

static void kinibi_crypto(void *in, void *out, uint32_t len)
{
    size_t destLen = 0;
    g_out->sys_err = tlApiWrapObjectExt(in, 0, len, out, &destLen,
      MC_SO_CONTEXT_TLT, MC_SO_LIFETIME_PERMANENT, NULL, TLAPI_WRAP_DEFAULT);
    if_err(g_out->sys_err != 0,  );
}

static void kinibi_decrypt(void *in, void *out, uint32_t len)
{
    size_t destLen = 0;
    g_out->sys_err = tlApiUnwrapObjectExt(in, len, out, &destLen, TLAPI_UNWRAP_DEFAULT);
    if_err(g_out->sys_err != 0,  );
}

static void kinibi_read(char *name, void *buf, size_t *buf_len)
{
    TEE_ObjectHandle handle;
    uint32_t count;
    g_out->sys_err = TEE_CreatePersistentObject(
        TEE_STORAGE_PRIVATE,
        (void*)name, strlen(name),
        TEE_DATA_FLAG_ACCESS_READ|
        TEE_DATA_FLAG_ACCESS_WRITE|
        TEE_DATA_FLAG_ACCESS_WRITE_META,
        TEE_HANDLE_NULL, /* no attributes */
        NULL, 0, /* no initial data */
        &handle);
    if_err(g_out->sys_err != 0,  return);
    TEE_ReadObjectData(handle, buf, *buf_len, &count);
    if_err(g_out->sys_err != 0, );
    TEE_CloseObject(handle);
}

static void kinibi_write(char *name, void *buf, size_t buf_len)
{
    TEE_ObjectHandle handle;
    g_out->sys_err = TEE_CreatePersistentObject(
        TEE_STORAGE_PRIVATE,
        (void*)name, strlen(name),
        TEE_DATA_FLAG_ACCESS_READ|
        TEE_DATA_FLAG_ACCESS_WRITE|
        TEE_DATA_FLAG_ACCESS_WRITE_META,
        TEE_HANDLE_NULL, /* no attributes */
        NULL, 0, /* no initial data */
        &handle);
    if_err(g_out->sys_err != 0,  return);
    TEE_WriteObjectData(handle, buf, buf_len);
    if_err(g_out->sys_err != 0, );
    TEE_CloseObject(handle);
}

static void* kinibi_malloc(size_t size)
{
    return tlApiMalloc(size, 0);
}

void platform_init(){
    if(g_dev == NULL) {
        g_dev = (struct platform_device *)tlApiMalloc(sizeof(struct platform_device), 0);
        if (g_dev == NULL) {
            loge("%s, err malloc\n", __func__);
            return;
        }
    }
    g_in = (struct tee_in_buf *)g_dev->buf;
    g_out = (struct tee_out_buf *)(g_in + IN_BUF_LEN);
    g_dev->free = tlApiFree,
    g_dev->read = kinibi_read,
    g_dev->write = kinibi_write,
    g_dev->spi_rw = kinibi_spi_rw,
    g_dev->malloc = kinibi_malloc,
    g_dev->uptime = kinibi_uptime,
    g_dev->crypto = kinibi_crypto,
    g_dev->decrypt = kinibi_decrypt,
    g_dev->get_hmac = kinibi_get_hmac,
    g_dev->get_token = kinibi_token,
    g_dev->verify_hmac = kinibi_verify_hmac,
    g_dev->get_hmac_key = kinibi_get_hmac_key;
};