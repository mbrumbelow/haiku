

#ifndef _FSSH_UUID_H
#define _FSSH_UUID_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

typedef unsigned char uuid_t[16];

#ifdef __cplusplus
extern "C"
{
#endif

    void uuid_generate(uuid_t out);
    void uuid_generate_random(uuid_t out);
    void uuid_generate_time(uuid_t out);

#ifdef __cplusplus
}
#endif

#endif