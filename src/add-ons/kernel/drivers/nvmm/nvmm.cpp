/*
 * Copyright (c) 2018-2021 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __HAIKU__
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/kernel.h>
#include <sys/mman.h>
#endif

#include "nvmm.h"
#include "nvmm_internal.h"
#include "nvmm_ioctl.h"

static struct nvmm_machine machines[NVMM_MAX_MACHINES];
volatile unsigned int nmachines __cacheline_aligned;

static const struct nvmm_impl *nvmm_impl_list[] = {
#if defined(__x86_64__)
#ifndef __HAIKU__
	&nvmm_x86_svm,	/* x86 AMD SVM */
#endif
	&nvmm_x86_vmx	/* x86 Intel VMX */
#endif
};

const struct nvmm_impl *nvmm_impl __read_mostly = NULL;

struct nvmm_owner nvmm_root_owner;

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

const struct nvmm_impl *
nvmm_ident(void)
{
	size_t i;

	for (i = 0; i < __arraycount(nvmm_impl_list); i++) {
		if ((*nvmm_impl_list[i]->ident)())
			return nvmm_impl_list[i];
	}

	return NULL;
}

int
nvmm_init(void)
{
	size_t i, n;

	nvmm_impl = nvmm_ident();
	if (nvmm_impl == NULL)
		return ENOTSUP;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		machines[i].machid = i;
		os_rwl_init(&machines[i].lock);
		for (n = 0; n < NVMM_MAX_VCPUS; n++) {
			machines[i].cpus[n].present = false;
			machines[i].cpus[n].cpuid = n;
			os_mtx_init(&machines[i].cpus[n].lock);
		}
	}

	(*nvmm_impl->init)();

	return 0;
}

void
nvmm_fini(void)
{
	size_t i, n;

	for (i = 0; i < NVMM_MAX_MACHINES; i++) {
		os_rwl_destroy(&machines[i].lock);
		for (n = 0; n < NVMM_MAX_VCPUS; n++) {
			os_mtx_destroy(&machines[i].cpus[n].lock);
		}
	}

	(*nvmm_impl->fini)();
	nvmm_impl = NULL;
}

/* -------------------------------------------------------------------------- */
