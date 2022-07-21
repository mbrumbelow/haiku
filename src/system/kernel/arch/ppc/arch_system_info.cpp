/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <OS.h>

#include <arch_cpu.h>
#include <arch/system_info.h>
#include <boot/kernel_args.h>


char sCPUVendor[32];
uint32 sPVR;

static uint64 sCPUClockFrequency;
static uint64 sBusClockFrequency;

struct cpu_model {
	uint16			version;
	char vendor[32];
};

// mapping of CPU versions to vendors
struct cpu_model kCPUModels[] = {
	{ MPC601,		"Motorola" },
	{ MPC603,		"Motorola" },
	{ MPC604,		"Motorola" },
	{ MPC602,		"Motorola" },
	{ MPC603e,		"Motorola" },
	{ MPC603ev,		"Motorola" },
	{ MPC750,		"Motorola" },
	{ MPC604ev,		"Motorola" },
	{ MPC7400,		"Motorola" },
	{ MPC620,		"Motorola" },
	{ IBM403,		"IBM" },
	{ IBM401A1,		"IBM" },
	{ IBM401B2,		"IBM" },
	{ IBM401C2,		"IBM" },
	{ IBM401D2,		"IBM" },
	{ IBM401E2,		"IBM" },
	{ IBM401F2,		"IBM" },
	{ IBM401G2,		"IBM" },
	{ IBMPOWER3,	"IBM" },
	{ MPC860,		"Motorola" },
	{ MPC8240,		"Motorola" },
	{ IBM405GP,		"IBM" },
	{ IBM405L,		"IBM" },
	{ IBM750FX,		"IBM" },
	{ MPC7450,		"Motorola" },
	{ MPC7455,		"Motorola" },
	{ MPC7457,		"Motorola" },
	{ MPC7447A,		"Motorola" },
	{ MPC7448,		"Motorola" },
	{ MPC7410,		"Motorola" },
	{ MPC8245,		"Motorola" },
	{ 0,			"Unknown" }
};


void
arch_fill_topology_node(cpu_topology_node_info* node, int32 cpu)
{
	switch (node->type) {
		case B_TOPOLOGY_ROOT:
#if  __powerpc64__
			node->data.root.platform = B_CPU_PPC_64;
#else
			node->data.root.platform = B_CPU_PPC;
#endif
			break;

		case B_TOPOLOGY_PACKAGE:
			node->data.package.vendor = sCPUVendor;
			node->data.package.cache_line_size = CACHE_LINE_SIZE;
			break;

		case B_TOPOLOGY_CORE:
			node->data.core.model = sPVR;
			node->data.core.default_frequency = sCPUClockFrequency;
			break;

		default:
			break;
	}
}


status_t
arch_system_info_init(struct kernel_args *args)
{
	int i;

	sCPUClockFrequency = args->arch_args.cpu_frequency;
	sBusClockFrequency = args->arch_args.bus_frequency;

	// The PVR (processor version register) contains processor version and
	// revision.
	sPVR = get_pvr();
	uint16 model = (uint16)(sPVR >> 16);
	//sCPURevision = (uint16)(pvr & 0xffff);

	// Populate vendor
	for (i = 0; strcmp(kCPUModels[i].vendor, "Unknown"); i++) {
		if (model == kCPUModels[i].version) {
			sCPUVendor = kCPUModels[i].vendor;
			break;
		}
	}

	return B_OK;
}


status_t
arch_get_frequency(uint64 *frequency, int32 cpu)
{
	*frequency = sCPUClockFrequency;
	return B_OK;
}
