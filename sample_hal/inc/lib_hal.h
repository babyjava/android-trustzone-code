#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "pthread.h"
#include <errno.h>
// #include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <log/log.h>
#include <dlfcn.h>
#include "platform_common.h"

#define LIB_VERSION "version:1.0.0.0"
#define if_err(a, b, fmt, ...); if(a) { ALOGE("%s %d" fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); b}
#define PREFIX(s) f_##s
#define DLSYM(f) dlsym(dev->handle, #f); \
        if (dlerror() != NULL){ \
            ALOGE("dlsym err %s", #f); \
            return GENERIC_ERR; \
        }

struct tee_client_device
{
    int (*tee_init)(void);
    int (*tee_cmd)(struct tee_client_device *, struct tee_in_buf *, struct tee_out_buf *);
    void (*tee_exit)(struct tee_client_device *);
    pthread_mutex_t mutex;
    void *handle;
};

int gp_client_open(struct tee_client_device *dev);
int qsee_client_open(struct tee_client_device *dev);
int isee_client_open(struct tee_client_device *dev);
int trusty_client_open(struct tee_client_device *dev);
int kinibi_client_open(struct tee_client_device *dev);