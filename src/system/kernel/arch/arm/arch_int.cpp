/*
 * Copyright 2003-2022, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		François Revol <revol@free.fr>
 *              Ithamar R. Adema <ithamar@upgrade-android.com>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <int.h>

#include <arch_cpu_defs.h>
#include <arch/smp.h>
#include <boot/kernel_args.h>
#include <device_manager.h>
#include <kscheduler.h>
#include <interrupt_controller.h>
#include <ksyscalls.h>
#include <smp.h>
#include <syscall_numbers.h>
#include <thread.h>
#include <timer.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <algorithm>
#include <string.h>

#include <drivers/bus/FDT.h>
#include "arch_int_gicv2.h"
#include "soc.h"

#include "soc_pxa.h"
#include "soc_omap3.h"
#include "soc_sun4i.h"

#define TRACE_ARCH_INT
#ifdef TRACE_ARCH_INT
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define VECTORPAGE_SIZE		64
#define USER_VECTOR_ADDR_LOW	0x00000000
#define USER_VECTOR_ADDR_HIGH	0xffff0000

extern int _vectors_start;
extern int _vectors_end;

static area_id sVectorPageArea;
static void *sVectorPageAddress;
static area_id sUserVectorPageArea;
static void *sUserVectorPageAddress;
//static fdt_module_info *sFdtModule;

// An iframe stack used in the early boot process when we don't have
// threads yet.
struct iframe_stack gBootFrameStack;


void
arch_int_enable_io_interrupt(int irq)
{
	TRACE(("arch_int_enable_io_interrupt(%d)\n", irq));
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->EnableInterrupt(irq);
}


void
arch_int_disable_io_interrupt(int irq)
{
	TRACE(("arch_int_disable_io_interrupt(%d)\n", irq));
	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->DisableInterrupt(irq);
}


/* arch_int_*_interrupts() and friends are in arch_asm.S */

int32
arch_int_assign_to_cpu(int32 irq, int32 cpu)
{
	// Not yet supported.
	return 0;
}


static void
print_iframe(const char *event, struct iframe *frame)
{
	if (event)
		dprintf("Exception: %s\n", event);

	dprintf("R00=%08lx R01=%08lx R02=%08lx R03=%08lx\n"
		"R04=%08lx R05=%08lx R06=%08lx R07=%08lx\n",
		frame->r0, frame->r1, frame->r2, frame->r3,
		frame->r4, frame->r5, frame->r6, frame->r7);
	dprintf("R08=%08lx R09=%08lx R10=%08lx R11=%08lx\n"
		"R12=%08lx SP=%08lx LR=%08lx  PC=%08lx CPSR=%08lx\n",
		frame->r8, frame->r9, frame->r10, frame->r11,
		frame->r12, frame->svc_sp, frame->svc_lr, frame->pc, frame->spsr);
}


status_t
arch_int_init(kernel_args *args)
{
	return B_OK;
}


extern "C" void arm_vector_init(void);


status_t
arch_int_init_post_vm(kernel_args *args)
{
	// create a read/write kernel area
	sVectorPageArea = create_area("vectorpage", (void **)&sVectorPageAddress,
		B_ANY_ADDRESS, VECTORPAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_WRITE_AREA | B_KERNEL_READ_AREA);
	if (sVectorPageArea < 0)
		panic("vector page could not be created!");

	// clone it at a fixed address with user read/only permissions
	sUserVectorPageAddress = (addr_t*)USER_VECTOR_ADDR_HIGH;
	sUserVectorPageArea = clone_area("user_vectorpage",
		(void **)&sUserVectorPageAddress, B_EXACT_ADDRESS,
		B_READ_AREA | B_EXECUTE_AREA, sVectorPageArea);

	if (sUserVectorPageArea < 0)
		panic("user vector page @ %p could not be created (%lx)!",
			sVectorPageAddress, sUserVectorPageArea);

	// copy vectors into the newly created area
	memcpy(sVectorPageAddress, &_vectors_start, VECTORPAGE_SIZE);

	arm_vector_init();

	// see if high vectors are enabled
	if ((mmu_read_c1() & (1 << 13)) != 0)
		dprintf("High vectors already enabled\n");
	else {
		mmu_write_c1(mmu_read_c1() | (1 << 13));

		if ((mmu_read_c1() & (1 << 13)) == 0)
			dprintf("Unable to enable high vectors!\n");
		else
			dprintf("Enabled high vectors\n");
	}

	if (strncmp(args->arch_args.interrupt_controller.kind, INTC_KIND_GICV2,
		sizeof(args->arch_args.interrupt_controller.kind)) == 0) {
		InterruptController *ic = new(std::nothrow) GICv2InterruptController(
			args->arch_args.interrupt_controller.regs1.start,
			args->arch_args.interrupt_controller.regs2.start);
		if (ic == NULL)
			return B_NO_MEMORY;
	} else if (strncmp(args->arch_args.interrupt_controller.kind, INTC_KIND_OMAP3,
		sizeof(args->arch_args.interrupt_controller.kind)) == 0) {
		InterruptController *ic = new(std::nothrow) OMAP3InterruptController(
			args->arch_args.interrupt_controller.regs1.start);
		if (ic == NULL)
			return B_NO_MEMORY;
	} else if (strncmp(args->arch_args.interrupt_controller.kind, INTC_KIND_PXA,
		sizeof(args->arch_args.interrupt_controller.kind)) == 0) {
		InterruptController *ic = new(std::nothrow) PXAInterruptController(
			args->arch_args.interrupt_controller.regs1.start);
		if (ic == NULL)
			return B_NO_MEMORY;
	} else if (strncmp(args->arch_args.interrupt_controller.kind, INTC_KIND_SUN4I,
		sizeof(args->arch_args.interrupt_controller.kind)) == 0) {
		InterruptController *ic = new(std::nothrow) Sun4iInterruptController(
			args->arch_args.interrupt_controller.regs1.start);
		if (ic == NULL)
			return B_NO_MEMORY;
	} else {
		panic("No interrupt controllers found!\n");
	}

	return B_OK;
}


status_t
arch_int_init_io(kernel_args* args)
{
	return B_OK;
}


status_t
arch_int_init_post_device_manager(struct kernel_args *args)
{
	return B_ENTRY_NOT_FOUND;
}


// Little helper class for handling the
// iframe stack as used by KDL.
class IFrameScope {
public:
	IFrameScope(struct iframe *iframe) {
		fThread = thread_get_current_thread();
		if (fThread)
			arm_push_iframe(&fThread->arch_info.iframes, iframe);
		else
			arm_push_iframe(&gBootFrameStack, iframe);
	}

	virtual ~IFrameScope() {
		// pop iframe
		if (fThread)
			arm_pop_iframe(&fThread->arch_info.iframes);
		else
			arm_pop_iframe(&gBootFrameStack);
	}
private:
	Thread* fThread;
};


extern "C" void
arch_arm_undefined(struct iframe *iframe)
{
	print_iframe("Undefined Instruction", iframe);
	IFrameScope scope(iframe); // push/pop iframe

	panic("not handled!");
}


extern "C" void
arch_arm_syscall(struct iframe *iframe)
{
#ifdef TRACE_ARCH_INT
	print_iframe("Software interrupt", iframe);
#endif

	uint32_t syscall = *(uint32_t *)(iframe->pc-4) & 0x00ffffff;
	TRACE(("syscall number: %d\n", syscall));

	uint32_t args[20];
	if (syscall < kSyscallCount) {
		TRACE(("syscall(%s,%d)\n",
			kExtendedSyscallInfos[syscall].name,
			kExtendedSyscallInfos[syscall].parameter_count));

		int argSize = kSyscallInfos[syscall].parameter_size;
		memcpy(args, &iframe->r0, std::min<int>(argSize, 4 * sizeof(uint32)));
		if (argSize > 4 * sizeof(uint32)) {
			status_t res = user_memcpy(&args[4], (void *)iframe->usr_sp,
				(argSize - 4 * sizeof(uint32)));
			if (res < B_OK) {
				dprintf("can't read syscall arguments on user stack\n");
				iframe->r0 = res;
				return;
			}
		}
	}

	enable_interrupts();

	uint64 returnValue = 0;
	syscall_dispatcher(syscall, (void*)args, &returnValue);

	TRACE(("returning %" B_PRId64 "\n", returnValue));
	iframe->r0 = returnValue;
}


extern "C" void
arch_arm_data_abort(struct iframe *frame)
{
	Thread *thread = thread_get_current_thread();
	bool isUser = (frame->spsr & CPSR_MODE_MASK) == CPSR_MODE_USR;
	int32 fsr = arm_get_fsr();
	addr_t far = arm_get_far();
	bool isWrite = (fsr & FSR_WNR) == FSR_WNR;
	addr_t newip = 0;

#ifdef TRACE_ARCH_INT
	print_iframe("Data Abort", frame);
	dprintf("FAR: %08lx, isWrite: %d, thread: %s\n", far, isWrite, thread->name);
#endif

	IFrameScope scope(frame);

	if (debug_debugger_running()) {
		// If this CPU or this thread has a fault handler, we're allowed to be
		// here.
		if (thread != NULL) {
			cpu_ent* cpu = &gCPU[smp_get_current_cpu()];

			if (cpu->fault_handler != 0) {
				debug_set_page_fault_info(far, frame->pc,
					isWrite ? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->svc_sp = cpu->fault_handler_stack_pointer;
				frame->pc = cpu->fault_handler;
				return;
			}

			if (thread->fault_handler != 0) {
				kprintf("ERROR: thread::fault_handler used in kernel "
					"debugger!\n");
				debug_set_page_fault_info(far, frame->pc,
						isWrite ? DEBUG_PAGE_FAULT_WRITE : 0);
				frame->pc = reinterpret_cast<uintptr_t>(thread->fault_handler);
				return;
			}
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Touching "
			"address %p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	} else if ((frame->spsr & (1 << 7)) != 0) {
		// interrupts disabled

		// If a page fault handler is installed, we're allowed to be here.
		// TODO: Now we are generally allowing user_memcpy() with interrupts
		// disabled, which in most cases is a bug. We should add some thread
		// flag allowing to explicitly indicate that this handling is desired.
		uintptr_t handler = reinterpret_cast<uintptr_t>(thread->fault_handler);
		if (thread && thread->fault_handler != 0) {
			if (frame->pc != handler) {
				frame->pc = handler;
				return;
			}

			// The fault happened at the fault handler address. This is a
			// certain infinite loop.
			panic("page fault, interrupts disabled, fault handler loop. "
				"Touching address %p from pc %p\n", (void*)far,
				(void*)frame->pc);
		}

		// If we are not running the kernel startup the page fault was not
		// allowed to happen and we must panic.
		panic("page fault, but interrupts were disabled. Touching address "
			"%p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	} else if (thread != NULL && thread->page_faults_allowed < 1) {
		panic("page fault not allowed at this place. Touching address "
			"%p from pc %p\n", (void *)far, (void *)frame->pc);
		return;
	}

	enable_interrupts();

	vm_page_fault(far, frame->pc, isWrite, false, isUser, &newip);

	if (newip != 0) {
		// the page fault handler wants us to modify the iframe to set the
		// IP the cpu will return to to be this ip
		frame->pc = newip;
	}
}


extern "C" void
arch_arm_prefetch_abort(struct iframe *frame)
{
	Thread *thread = thread_get_current_thread();
	bool isUser = (frame->spsr & CPSR_MODE_MASK) == CPSR_MODE_USR;
	addr_t newip = 0;

#ifdef TRACE_ARCH_INT
	print_iframe("Prefetch Abort", frame);
	dprintf("thread: %s\n", thread->name);
#endif

	IFrameScope scope(frame);

	if (debug_debugger_running()) {
		// If this CPU or this thread has a fault handler, we're allowed to be
		// here.
		if (thread != NULL) {
			cpu_ent* cpu = &gCPU[smp_get_current_cpu()];

			if (cpu->fault_handler != 0) {
				debug_set_page_fault_info(frame->pc, frame->pc, 0);
				frame->svc_sp = cpu->fault_handler_stack_pointer;
				frame->pc = cpu->fault_handler;
				return;
			}

			if (thread->fault_handler != 0) {
				kprintf("ERROR: thread::fault_handler used in kernel "
					"debugger!\n");
				debug_set_page_fault_info(frame->pc, frame->pc, 0);
				frame->pc = reinterpret_cast<uintptr_t>(thread->fault_handler);
				return;
			}
		}

		// otherwise, not really
		panic("page fault in debugger without fault handler! Prefetch abort at %p\n",
			(void *)frame->pc);
		return;
	} else if ((frame->spsr & (1 << 7)) != 0) {
		// interrupts disabled

		// If a page fault handler is installed, we're allowed to be here.
		uintptr_t handler = reinterpret_cast<uintptr_t>(thread->fault_handler);
		if (thread && thread->fault_handler != 0) {
			if (frame->pc != handler) {
				frame->pc = handler;
				return;
			}

			// The fault happened at the fault handler address. This is a
			// certain infinite loop.
			panic("page fault, interrupts disabled, fault handler loop. "
				"Prefetch abort at %p\n", (void*)frame->pc);
		}

		// If we are not running the kernel startup the page fault was not
		// allowed to happen and we must panic.
		panic("page fault, but interrupts were disabled. Prefetch abort at %p\n",
			(void *)frame->pc);
		return;
	} else if (thread != NULL && thread->page_faults_allowed < 1) {
		panic("page fault not allowed at this place. Prefetch abort at %p\n",
			(void *)frame->pc);
		return;
	}

	enable_interrupts();

	vm_page_fault(frame->pc, frame->pc, false, true, isUser, &newip);

	if (newip != 0) {
		frame->pc = newip;
	}
}


extern "C" void
arch_arm_irq(struct iframe *iframe)
{
	IFrameScope scope(iframe);

	InterruptController *ic = InterruptController::Get();
	if (ic != NULL)
		ic->HandleInterrupt();

	Thread* thread = thread_get_current_thread();
	cpu_status state = disable_interrupts();
	if (thread->cpu->invoke_scheduler) {
		SpinLocker schedulerLocker(thread->scheduler_lock);
		scheduler_reschedule(B_THREAD_READY);
		schedulerLocker.Unlock();
		restore_interrupts(state);
	} else if (thread->post_interrupt_callback != NULL) {
		void (*callback)(void*) = thread->post_interrupt_callback;
		void* data = thread->post_interrupt_data;

		thread->post_interrupt_callback = NULL;
		thread->post_interrupt_data = NULL;

		restore_interrupts(state);

		callback(data);
	}
}


extern "C" void
arch_arm_fiq(struct iframe *iframe)
{
	IFrameScope scope(iframe);

	panic("FIQ not implemented yet!");
}
