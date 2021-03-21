#include "platform_tee.h"

void hw_id(void)
{   int id_len = 2;
    g_dev->spi_rw(g_in->buf, g_in->buf_len, g_out->buf, id_len);
    if(g_out->status == GENERIC_OK){
    }
}
