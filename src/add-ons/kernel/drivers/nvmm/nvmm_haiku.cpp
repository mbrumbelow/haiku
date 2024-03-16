/*
 * Copyright 2024 Daniel Martin, dalmemail@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <Drivers.h>

#include "nvmm.h"
#include "nvmm_os.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

status_t
init_hardware(void)
{
	if (vmx_ident()) {
		TRACE("nvmm: VMX supported\n");
		return B_OK;
	}
	return B_ERROR;
}


const char **
publish_devices(void)
{
	TRACE("nvmm: publish_devices\n");
	return NULL;
}


device_hooks *
find_device(const char *name)
{
	TRACE("nvmm: find_device\n");
	return NULL;
}


status_t
init_driver(void)
{
	TRACE("nvmm: init_driver\n");
	return B_OK;
}


void
uninit_driver(void)
{
	TRACE("nvmm: uninit_driver\n");
}
