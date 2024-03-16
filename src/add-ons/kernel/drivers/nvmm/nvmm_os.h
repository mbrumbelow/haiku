// Assume there would be netBSD and DragonFlyBSD specific code here too
// includes
#ifdef __HAIKU__
#include <arch/x86/arch_cpu.h>
#include <cpu.h>
#include <SupportDefs.h>
#endif

#ifdef __HAIKU__
#define TRACE(x) dprintf(x)
#endif
// functions
#ifdef __HAIKU__
#define rdmsr		x86_read_msr
#define os_printf	TRACE
#define x86_get_cr0	x86_read_cr0
#define x86_get_cr4	x86_read_cr4
#endif

// constants
#ifdef __HAIKU__
#define CR0_PG		CR0_PAGING_ENABLE
#define CR0_PE		CR0_PROTECTED_MODE_ENABLE
#define CR4_VMXE	IA32_CR4_VMXE
#endif
