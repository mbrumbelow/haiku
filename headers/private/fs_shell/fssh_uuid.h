/*
 * Copyright 2020 Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
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