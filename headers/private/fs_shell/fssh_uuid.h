

#ifndef _FSSH_UUID_H
#define _FSSH_UUID_H


typedef unsigned char uuid_t[16];

#ifdef __cplusplus
extern "C"
{
#endif

    void uuid_generate(uuid_t out);

#ifdef __cplusplus
}
#endif

#endif