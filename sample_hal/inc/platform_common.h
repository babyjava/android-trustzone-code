#define GENERIC_OK 0
#define GENERIC_ERR -1
#define HMAC_LEN 32
#define GP_CMD 0xaebecede
#define BUF_LEN 256
#define IN_BUF_LEN sizeof(struct tee_in_buf)
#define OUT_BUF_LEN sizeof(struct tee_out_buf)

struct tee_in_buf
{
    uint32_t cmd;
    char name[16];
    uint32_t buf_len;
    uint8_t  buf[BUF_LEN];
    uint32_t spi_num;
    uint32_t i2c_num;
};

struct tee_out_buf
{
    uint32_t status;
    int32_t sys_err;
    uint32_t hw_id;
    uint32_t ta_ver;
    uint32_t buf_len;
    uint8_t  buf[BUF_LEN];
    char sys_err_line[32];
};

enum tee_cmd
{
    TEE_CMD_SPI_TEST,
    TEE_CMD_I2C_TEST,
    TEE_CMD_WRITE_TEST,
    TEE_CMD_READ_TEST,
    TEE_CMD_INIT,
    TEE_CMD_EXIT,
    TEE_CMD_END,
};
