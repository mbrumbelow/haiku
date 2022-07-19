/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <arch/cpu.h>
#include <boot/kernel_args.h>
#include <vm/VMAddressSpace.h>
#include <commpage.h>
#include <elf.h>
#include <Htif.h>
#include <platform/sbi/sbi_syscalls.h>
#include <arch_cpu_defs.h>

extern "C" {
#include <libfdt.h>
#include <libfdt_env.h>
};


extern "C" void SVec();

extern uint32 gPlatform;
extern void* gFDT;


static status_t
detect_cpu(int curr_cpu)
{
	cpu_ent* cpu = &gCPU[curr_cpu];

	// if we have an FDT...
	if (gFDT != NULL) {
		int node = fdt_path_offset(gFDT, "/");
		int len;

		// first lets look for the board name
		const char* platform = (const char *)fdt_getprop(gFDT, node,
			"compatible", &len);

		if (platform != NULL) {
			if (!strcmp(platform, "riscv-virtio")) {
				strcpy(cpu->arch.vendor, "QEMU");
				strcpy(cpu->arch.model_name, "virtio");
			} else if (!strncmp(platform, "sifive,", 7)) {
				// TODO: maybe just split on ,?
				strcpy(cpu->arch.vendor, "SiFive");
				strncpy(cpu->arch.model_name, platform + 7,
					MAX((size_t)len - 7, sizeof(cpu->arch.model_name)));
			} else {
				strcpy(cpu->arch.vendor, "RISC-V");
				strcpy(cpu->arch.model_name, "Unknown");
			}
		}
	}

	dprintf("CPU: %s %s\n", cpu->arch.vendor, cpu->arch.model_name);

	// Detect cpu extensions
	switch (gPlatform) {
		case kPlatformSbi: {
			// TODO: get via /cpus/cpu@0 riscv,isa
			strcpy(cpu->arch.isa, "rv64gc");
			break;
		}
		case kPlatformMNative:
		default:
			// TODO: decode
			uint32 misa = Misa() & STANDARD_EXT_MASK;
			snprintf(cpu->arch.isa, sizeof(cpu->arch.isa), "rv64,0x%X", misa);
			break;
	}

	return B_OK;
}


status_t
arch_cpu_preboot_init_percpu(kernel_args *args, int curr_cpu)
{
	// dprintf("arch_cpu_preboot_init_percpu(%" B_PRId32 ")\n", curr_cpu);
	return B_OK;
}


status_t
arch_cpu_init_percpu(kernel_args *args, int curr_cpu)
{
	detect_cpu(curr_cpu);

	SetStvec((uint64)SVec);
	SstatusReg sstatus(Sstatus());
	sstatus.ie = 0;
	sstatus.fs = extStatusInitial; // enable FPU
	sstatus.xs = extStatusOff;
	SetSstatus(sstatus.val);
	SetSie(Sie() | (1 << sTimerInt) | (1 << sSoftInt) | (1 << sExternInt));

	return B_OK;
}


status_t
arch_cpu_init(kernel_args *args)
{
	for (uint32 curCpu = 0; curCpu < args->num_cpus; curCpu++) {
		cpu_ent* cpu = &gCPU[curCpu];

		cpu->arch.hartId = args->arch_args.hartIds[curCpu];

		cpu->topology_id[CPU_TOPOLOGY_PACKAGE] = 0;
		cpu->topology_id[CPU_TOPOLOGY_CORE] = curCpu;
		cpu->topology_id[CPU_TOPOLOGY_SMT] = 0;

		for (unsigned int i = 0; i < CPU_MAX_CACHE_LEVEL; i++)
			cpu->cache_id[i] = -1;
	}

	uint64 conversionFactor
		= (1LL << 32) * 1000000LL / args->arch_args.timerFrequency;

	__riscv64_setup_system_time(conversionFactor);

	return B_OK;
}


status_t
arch_cpu_init_post_vm(kernel_args *args)
{
	// Set address space ownership to currently running threads
	for (uint32 i = 0; i < args->num_cpus; i++) {
		VMAddressSpace::Kernel()->Get();
	}

	return B_OK;
}


status_t
arch_cpu_init_post_modules(kernel_args *args)
{
	return B_OK;
}


void
arch_cpu_sync_icache(void *address, size_t len)
{
}


void
arch_cpu_memory_read_barrier(void)
{
}


void
arch_cpu_memory_write_barrier(void)
{
}


void
arch_cpu_invalidate_TLB_range(addr_t start, addr_t end)
{
	int32 numPages = end / B_PAGE_SIZE - start / B_PAGE_SIZE;
	while (numPages-- >= 0) {
		FlushTlbPage(start);
		start += B_PAGE_SIZE;
	}
}


void
arch_cpu_invalidate_TLB_list(addr_t pages[], int num_pages)
{
	for (int i = 0; i < num_pages; i++)
		FlushTlbPage(pages[i]);
}


void
arch_cpu_global_TLB_invalidate(void)
{
	FlushTlbAll();
}


void
arch_cpu_user_TLB_invalidate(void)
{
	FlushTlbAll();
}


status_t
arch_cpu_shutdown(bool reboot)
{
	if (gPlatform == kPlatformSbi) {
		sbi_system_reset(
			reboot ? SBI_RESET_TYPE_COLD_REBOOT : SBI_RESET_TYPE_SHUTDOWN,
			SBI_RESET_REASON_NONE);
	}

	HtifShutdown();
	return B_ERROR;
}
