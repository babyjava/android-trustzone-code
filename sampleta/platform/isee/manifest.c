#include <manifest.h>

TA_CONFIG_BEGIN

uuid : { 0xf8888888, 0x17d3, 0x6688, { 0x9b, 0x01, 0xc9, 0x92, 0xd5, 0x6d, 0x78, 0xbf } },
ta_flags : TA_FLAGS_INSTANCE_KEEP_ALIVE|TA_FLAGS_CAP_KM|TA_FLAGS_CAP_SPI,
log_tag : "sampleta",
ipc_buf_size: MAX_IPC_BUF_SIZE,

TA_CONFIG_END
