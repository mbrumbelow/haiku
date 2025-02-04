/******************************************************************************
 *
 * Module Name: oshaiku - Haiku OSL interfaces
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/


#include <sys/cdefs.h>

#include <OS.h>

#ifdef _KERNEL_MODE
#	include <KernelExport.h>

#	include <dpc.h>
#	include <PCI.h>

#	include <boot_item.h>
#	include <kernel.h>
#	include <vm/vm.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

__BEGIN_DECLS
#include <uacpi/acpi.h>
#include <uacpi/status.h>
#include <uacpi/kernel_api.h>
__END_DECLS

#include "arch_init.h"


#define _COMPONENT ACPI_OS_SERVICES

// verbosity level 0 = off, 1 = normal, 2 = all
#define DEBUG_OSHAIKU 0

#if DEBUG_OSHAIKU <= 0
// No debugging, do nothing
#	define DEBUG_FUNCTION()
#	define DEBUG_FUNCTION_F(x, y...)
#	define DEBUG_FUNCTION_V()
#	define DEBUG_FUNCTION_VF(x, y...)
#else
#ifdef _KERNEL_MODE
#define printf kprintf
#endif
#	define DEBUG_FUNCTION() \
		printf("acpi[%" B_PRId32 "]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#	define DEBUG_FUNCTION_F(x, y...) \
		printf("acpi[%" B_PRId32 "]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);
#	if DEBUG_OSHAIKU == 1
// No verbose debugging, do nothing
#		define DEBUG_FUNCTION_V()
#		define DEBUG_FUNCTION_VF(x, y...)
#	else
// Full debugging
#		define DEBUG_FUNCTION_V() \
			printf("acpi[%" B_PRId32 "]: %s\n", find_thread(NULL), __PRETTY_FUNCTION__);
#		define DEBUG_FUNCTION_VF(x, y...) \
			printf("acpi[%" B_PRId32 "]: %s(" x ")\n", find_thread(NULL), __PRETTY_FUNCTION__, y);
#	endif
#endif


#ifdef _KERNEL_MODE
extern pci_module_info *gPCIManager;
extern dpc_module_info *gDPC;
extern void *gDPCHandle;
#endif

static phys_addr_t sACPIRoot = 0;
static void *sInterruptHandlerData[32];

/******************************************************************************
 *
 * FUNCTION:    uacpi_kernel_initialize, uacpi_kernel_deinitialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Init and terminate.  Nothing to do.
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_initialize(uacpi_init_level current_init_lvl)
{
	DEBUG_FUNCTION();
	return UACPI_STATUS_OK;
}


void
uacpi_kernel_deinitialize()
{
	DEBUG_FUNCTION();
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetRootPointer
 *
 * PARAMETERS:  None
 *
 * RETURN:      RSDP physical address
 *
 * DESCRIPTION: Gets the root pointer (RSDP)
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_get_rsdp(uacpi_phys_addr* out_rdsp_address)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	if (sACPIRoot == 0) {
		phys_addr_t* acpiRootPointer = (phys_addr_t*)get_boot_item("ACPI_ROOT_POINTER", NULL);
		if (acpiRootPointer != NULL)
			sACPIRoot = *acpiRootPointer;
	}
	*out_rdsp_address = sACPIRoot;
#else
	// TODO
	*out_rdsp_address = 0;
#endif
	return UACPI_STATUS_OK;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsVprintf
 *
 * PARAMETERS:  fmt                 Standard printf format
 *              args                Argument list
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output with argument list pointer
 *
 *****************************************************************************/
void
uacpi_kernel_vlog(uacpi_log_level level, const char *fmt, va_list args)
{
#ifndef _KERNEL_MODE
	vfprintf(stderr, fmt, args);
#else
	// Buffer the output until we have a complete line to send to syslog, this avoids added
	// "KERN:" entries in the middle of the line, and mixing up of the ACPI output with other
	// messages from other CPUs
	static char outputBuffer[1024];
	
	// Append the new text to the buffer
	size_t len = strlen(outputBuffer);
	size_t printed = vsnprintf(outputBuffer + len, 1024 - len, fmt, args);
	if (printed >= 1024 - len) {
		// There was no space to fit the printed string in the outputBuffer. Remove what we added
		// there, fush the buffer, and print the long string directly
		outputBuffer[len] = '\0';
		dprintf("%s\n", outputBuffer);
		outputBuffer[0] = '\0';
		dvprintf(fmt, args);
		return;
	}

	// See if we have a complete line
	char* eol = strchr(outputBuffer + len, '\n');
	while (eol != nullptr) {
		// Print the completed line, then remove it from the buffer
		*eol = 0;
		dprintf("uacpi: %s\n", outputBuffer);
		memmove(outputBuffer, eol + 1, strlen(eol + 1) + 1);
		// See if there is another line to print still in the buffer (in case ACPICA would call
		// this function with a single string containing multiple newlines)
		eol = strchr(outputBuffer, '\n');
	}
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsPrintf
 *
 * PARAMETERS:  fmt, ...            Standard printf format
 *
 * RETURN:      None
 *
 * DESCRIPTION: Formatted output
 *
 *****************************************************************************/
void
uacpi_kernel_log(uacpi_log_level level, const char *fmt, ...)
{
	va_list args;

	DEBUG_FUNCTION();
	va_start(args, fmt);
	uacpi_kernel_vlog(level, fmt, args);
	va_end(args);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsMapMemory
 *
 * PARAMETERS:  where               Physical address of memory to be mapped
 *              length              How much memory to map
 *
 * RETURN:      Pointer to mapped memory.  Null on error.
 *
 * DESCRIPTION: Map physical memory into caller's address space
 *
 *****************************************************************************/
void *
uacpi_kernel_map(phys_addr_t where, size_t length)
{
#ifdef _KERNEL_MODE
	// map_physical_memory() defaults to uncached memory if no type is specified.
	// But ACPICA handles flushing caches itself, so we don't need it uncached,
	// and on some architectures (e.g. ARM) uncached memory does not support
	// unaligned accesses.
	//
	// However, if ACPICA maps (or re-maps) memory that's also used by some other
	// module (e.g. PCI configuration space), then we'll end up with the same
	// physical memory mapped twice with different attributes. On many arches,
	// this is invalid. So we stick with the default type where possible.
#if defined(__HAIKU_ARCH_ARM) || defined(__HAIKU_ARCH_ARM64)
	const uint32 memoryType = B_WRITE_BACK_MEMORY;
#else
	const uint32 memoryType = 0;
#endif

	void *there;
	area_id area = map_physical_memory("acpi_physical_mem_area", (phys_addr_t)where, length,
		B_ANY_KERNEL_ADDRESS | memoryType, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		&there);

	DEBUG_FUNCTION_F("addr: 0x%08lx; length: %lu; mapped: %p; area: %" B_PRId32,
		(addr_t)where, (size_t)length, there, area);
	if (area < 0) {
		dprintf("ACPI: cannot map memory at 0x%" B_PRIu64 ", length %"
			B_PRIu64 "\n", (uint64)where, (uint64)length);
		return NULL;
	}
	return there;
#else
	return NULL;
#endif

	// return ACPI_TO_POINTER((ACPI_SIZE) where);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsUnmapMemory
 *
 * PARAMETERS:  where               Logical address of memory to be unmapped
 *              length              How much memory to unmap
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete a previously created mapping.  Where and Length must
 *              correspond to a previous mapping exactly.
 *
 *****************************************************************************/
void
uacpi_kernel_unmap(void *where, size_t length)
{
	DEBUG_FUNCTION_F("unmapped: %p; length: %lu", where, length);
	delete_area(area_for(where));
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsAllocate
 *
 * PARAMETERS:  size                Amount to allocate, in bytes
 *
 * RETURN:      Pointer to the new allocation.  Null on error.
 *
 * DESCRIPTION: Allocate memory.  Algorithm is dependent on the OS.
 *
 *****************************************************************************/
void *
uacpi_kernel_alloc(size_t size)
{
	void *mem = (void *) malloc(size);
	DEBUG_FUNCTION_VF("result: %p", mem);
	return mem;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsFree
 *
 * PARAMETERS:  mem                 Pointer to previously allocated memory
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free memory allocated via AcpiOsAllocate
 *
 *****************************************************************************/
void
uacpi_kernel_free(void *mem)
{
	DEBUG_FUNCTION_VF("mem: %p", mem);
	free(mem);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsCreateSemaphore
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an OS semaphore
 *
 *****************************************************************************/
uacpi_handle
uacpi_kernel_create_event()
{
	DEBUG_FUNCTION();
	return (uacpi_handle)(uintptr_t)create_sem(0, "acpi_sem");
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsDeleteSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete an OS semaphore
 *
 *****************************************************************************/
void
uacpi_kernel_free_event(uacpi_handle handle)
{
	DEBUG_FUNCTION_F("sem: %" B_PRId32, (sem_id)(uintptr_t)handle);
	delete_sem((sem_id)(uintptr_t)handle);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *              units               - How many units to wait for
 *              timeout             - How long to wait
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Wait for units
 *
 *****************************************************************************/
#define ACPI_WAIT_FOREVER 0xFFFF
#define ACPI_DO_NOT_WAIT 0x0000

uacpi_bool
uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout)
{
	uacpi_bool result = UACPI_FALSE;
	DEBUG_FUNCTION_VF("sem: %d; timeout: %u", (sem_id)(uintptr_t)handle, timeout);

	if (timeout == ACPI_WAIT_FOREVER) {
		result = acquire_sem_etc((sem_id)(uintptr_t)handle, 1, 0, 0)
			== B_OK ? UACPI_TRUE : UACPI_FALSE;
	} else {
		switch (acquire_sem_etc((sem_id)(uintptr_t)handle, 1, B_RELATIVE_TIMEOUT,
			(bigtime_t)timeout * 1000)) {
			case B_OK:
				result = UACPI_TRUE;
				break;
			case B_INTERRUPTED:
			case B_TIMED_OUT:
			case B_WOULD_BLOCK:
				result = UACPI_FALSE;
				break;
			case B_BAD_VALUE:
			default:
				result = UACPI_FALSE;
				break;
		}
	}
	DEBUG_FUNCTION_VF("sem: %d; timeout: %u result: %" B_PRIu32,
		(sem_id)(uintptr_t)handle, timeout, (uint32)result);
	return result;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignalSemaphore
 *
 * PARAMETERS:  handle              - Handle returned by AcpiOsCreateSemaphore
 *              units               - Number of units to send
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Send units
 *
 *****************************************************************************/
void
uacpi_kernel_signal_event(uacpi_handle handle)
{
	DEBUG_FUNCTION_VF("sem: %d", (sem_id)(uintptr_t)handle);
	// We can be called from interrupt handler, so don't reschedule
	release_sem_etc((sem_id)(uintptr_t)handle, 1, B_DO_NOT_RESCHEDULE);
}


void
uacpi_kernel_reset_event(uacpi_handle handle)
{
	release_sem_etc((sem_id)(uintptr_t)handle, 1, B_DO_NOT_RESCHEDULE | B_RELEASE_ALL);
}
/******************************************************************************
 *
 * FUNCTION:    Spinlock interfaces
 *
 * DESCRIPTION: Map these interfaces to semaphore interfaces
 *
 *****************************************************************************/
uacpi_handle
uacpi_kernel_create_spinlock()
{
#ifdef _KERNEL_MODE
	uacpi_handle* outHandle = (uacpi_handle*) malloc(sizeof(spinlock));
	DEBUG_FUNCTION_F("result: %p", outHandle);
	if (outHandle == NULL)
		return NULL;

	B_INITIALIZE_SPINLOCK((spinlock*)outHandle);
	return outHandle;
#else
	// TODO
	return NULL;
#endif
}


void
uacpi_kernel_free_spinlock(uacpi_handle handle)
{
	DEBUG_FUNCTION();
	free((void*)handle);
}


uacpi_cpu_flags
uacpi_kernel_lock_spinlock(uacpi_handle handle)
{
#ifdef _KERNEL_MODE
	cpu_status cpu;
	DEBUG_FUNCTION_F("spinlock: %p", handle);
	cpu = disable_interrupts();
	acquire_spinlock((spinlock*)handle);
	return cpu;
#else
	// TODO
	return 0;
#endif
}


void
uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags)
{
#ifdef _KERNEL_MODE
	release_spinlock((spinlock*)handle);
	restore_interrupts(flags);
	DEBUG_FUNCTION_F("spinlock: %p", handle);
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsInstallInterruptHandler
 *
 * PARAMETERS:  interruptNumber     Level handler should respond to.
 *              Isr                 Address of the ACPI interrupt handler
 *              ExceptPtr           Where status is returned
 *
 * RETURN:      Handle to the newly installed handler.
 *
 * DESCRIPTION: Install an interrupt handler.  Used to install the ACPI
 *              OS-independent handler.
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_install_interrupt_handler(uacpi_u32 interruptNumber,
	uacpi_interrupt_handler serviceRoutine, uacpi_handle context, uacpi_handle* outHandle)
{
	DEBUG_FUNCTION_F("vector: %" B_PRIu32 "; handler: %p context %p",
		interruptNumber, serviceRoutine, context);

#ifdef _KERNEL_MODE
	status_t result;

	// It so happens that the Haiku and ACPI-CA interrupt handler routines
	// return the same values with the same meanings
	sInterruptHandlerData[interruptNumber] = context;
	result = install_io_interrupt_handler(interruptNumber,
		(interrupt_handler)serviceRoutine, context, 0);

	DEBUG_FUNCTION_F("vector: %" B_PRIu32 "; handler: %p context %p returned %" B_PRId32,
		interruptNumber, serviceRoutine, context, (uint32)result);

	*outHandle = (uacpi_handle)(uintptr_t)interruptNumber;
	return result == B_OK ? UACPI_STATUS_OK : UACPI_STATUS_INVALID_ARGUMENT;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsRemoveInterruptHandler
 *
 * PARAMETERS:  Handle              Returned when handler was installed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Uninstalls an interrupt handler.
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_uninstall_interrupt_handler(uacpi_interrupt_handler serviceRoutine, uacpi_handle handle)
{
	DEBUG_FUNCTION_F("vector: %" B_PRIu32 "; handler: %p", *(uint32*)handle, serviceRoutine);
#ifdef _KERNEL_MODE
	int32 interruptNumber = *(uint32*)handle;

	return remove_io_interrupt_handler(interruptNumber, (interrupt_handler)serviceRoutine,
		sInterruptHandlerData[interruptNumber]) == B_OK ? UACPI_STATUS_OK : UACPI_STATUS_INVALID_ARGUMENT;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsExecute
 *
 * PARAMETERS:  type            - Type of execution
 *              function        - Address of the function to execute
 *              context         - Passed as a parameter to the function
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Execute a new thread
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_schedule_work(uacpi_work_type type, uacpi_work_handler function,
		uacpi_handle context)
{
	DEBUG_FUNCTION();
/* TODO: Prioritize urgent?
	switch (type) {
		case OSL_GLOBAL_LOCK_HANDLER:
		case OSL_NOTIFY_HANDLER:
		case OSL_GPE_HANDLER:
		case OSL_DEBUGGER_THREAD:
		case OSL_EC_POLL_HANDLER:
		case OSL_EC_BURST_HANDLER:
			break;
	}
*/

#if KERNEL_MODE
	if (gDPC->queue_dpc(gDPCHandle, function, context) != B_OK) {
		DEBUG_FUNCTION_F("Serious failure in AcpiOsExecute! function: %p",
			function);
		return UACPI_STATUS_INVALID_ARGUMENT;
	}
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsStall
 *
 * PARAMETERS:  microseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at microsecond granularity
 *
 *****************************************************************************/
void
uacpi_kernel_stall(uacpi_u8 microseconds)
{
#if KERNEL_MODE
	DEBUG_FUNCTION_F("microseconds: %" B_PRIu32, (uint32)microseconds);
	if (microseconds)
		spin(microseconds);
#else
	// TODO
	// usleep(microseconds);
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSleep
 *
 * PARAMETERS:  milliseconds        To sleep
 *
 * RETURN:      Blocks until sleep is completed.
 *
 * DESCRIPTION: Sleep at millisecond granularity
 *
 *****************************************************************************/
void
uacpi_kernel_sleep(uacpi_u64 milliseconds)
{
	DEBUG_FUNCTION_F("milliseconds: %" B_PRIu32, (uint32)milliseconds);
#if KERNEL_MODE
	if (gKernelStartup)
		spin(milliseconds * 1000);
	else
		snooze(milliseconds * 1000);
#else
	// TODO
	// usleep(milliseconds * 1000);
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetTimer
 *
 * PARAMETERS:  None
 *
 * RETURN:      Current time in nanosecond units
 *
 * DESCRIPTION: Get the current system time
 *
 *****************************************************************************/
uint64_t
uacpi_kernel_get_nanoseconds_since_boot()
{
	DEBUG_FUNCTION();
	return system_time_nsecs();
}


// No specific open/close operations needed, so we just use the physical address as a handle.
uacpi_status
uacpi_kernel_pci_device_open(uacpi_pci_address address, uacpi_handle* out_handle)
{
	// FIXME this can fail if out of memory, add error handling
	*out_handle = new uacpi_pci_address(address);
	return UACPI_STATUS_OK;
}


void
uacpi_kernel_pci_device_close(uacpi_handle handle)
{
	delete (uacpi_pci_address*)handle;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPciConfiguration
 *
 * PARAMETERS:  pciId               Seg/Bus/Dev
 *              reg                 Device Register
 *              value               Buffer where value is placed
 *              width               Number of bits
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from PCI configuration space
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_pci_read8(uacpi_handle handle, uacpi_size reg, uacpi_u8* value)
{
#ifdef _KERNEL_MODE
	uacpi_pci_address* pciId = (uacpi_pci_address*)handle;
	*value = gPCIManager->read_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 1);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_pci_read16(uacpi_handle handle, uacpi_size reg, uacpi_u16* value)
{
#ifdef _KERNEL_MODE
	uacpi_pci_address* pciId = (uacpi_pci_address*)handle;
	*value = gPCIManager->read_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 2);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_pci_read32(uacpi_handle handle, uacpi_size reg, uacpi_u32* value)
{
#ifdef _KERNEL_MODE
	uacpi_pci_address* pciId = (uacpi_pci_address*)handle;
	*value = gPCIManager->read_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 4);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}



/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePciConfiguration
 *
 * PARAMETERS:  pciId               Seg/Bus/Dev
 *              reg                 Device Register
 *              value               Value to be written
 *              width               Number of bits
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Write data to PCI configuration space
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size reg, uacpi_u8 value)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	uacpi_pci_address* pciId = (uacpi_pci_address*)device;
	gPCIManager->write_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 1, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size reg, uacpi_u16 value)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	uacpi_pci_address* pciId = (uacpi_pci_address*)device;
	gPCIManager->write_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 2, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size reg, uacpi_u32 value)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION();
	uacpi_pci_address* pciId = (uacpi_pci_address*)device;
	gPCIManager->write_pci_config(
		pciId->bus, pciId->device, pciId->function, reg, 4, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


uacpi_status
uacpi_kernel_io_map(
	uacpi_io_addr base, uacpi_size len, uacpi_handle* out_handle)
{
	*out_handle = (uacpi_handle)base;
	return UACPI_STATUS_OK;
}


void
uacpi_kernel_io_unmap(uacpi_handle)
{
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsReadPort
 *
 * PARAMETERS:  address             Address of I/O port/register to read
 *              Value               Where value is placed
 *              width               Number of bits
 *
 * RETURN:      Value read from port
 *
 * DESCRIPTION: Read data from an I/O port or register
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_io_read8(uacpi_handle base, uacpi_size offset, uacpi_u8 *value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; width: 8", address);
	*value = gPCIManager->read_io_8(address);

	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_io_read16(uacpi_handle base, uacpi_size offset, uacpi_u16 *value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; width: 16", (addr_t)address);
	*value = gPCIManager->read_io_16(address);

	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}

uacpi_status
uacpi_kernel_io_read32(uacpi_handle base, uacpi_size offset, uacpi_u32 *value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; width: 32", (addr_t)address);
	*value = gPCIManager->read_io_32(address);

	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWritePort
 *
 * PARAMETERS:  address             Address of I/O port/register to write
 *              value               Value to write
 *              width               Number of bits
 *
 * RETURN:      None
 *
 * DESCRIPTION: Write data to an I/O port or register
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_io_write8(uacpi_handle base, uacpi_size offset, uint8 value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; value: %" B_PRIu32,
		(addr_t)address, (uint32)value);
	gPCIManager->write_io_8(address, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


uacpi_status
uacpi_kernel_io_write16(uacpi_handle base, uacpi_size offset, uint16 value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; value: %" B_PRIu32,
		(addr_t)address, (uint32)value);
	gPCIManager->write_io_16(address, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


uacpi_status
uacpi_kernel_io_write32(uacpi_handle base, uacpi_size offset, uint32 value)
{
#ifdef _KERNEL_MODE
	addr_t address = (uacpi_io_addr)base + offset;
	DEBUG_FUNCTION_F("addr: 0x%08lx; value: %" B_PRIu32,
		(addr_t)address, (uint32)value);
	gPCIManager->write_io_32(address, value);
	return UACPI_STATUS_OK;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsGetThreadId
 *
 * PARAMETERS:  None
 *
 * RETURN:      Id of the running thread
 *
 * DESCRIPTION: Get the Id of the current (running) thread
 *
 * NOTE:        The environment header should contain this line:
 *                  #define ACPI_THREAD_ID pthread_t
 *
 *****************************************************************************/
uacpi_thread_id
uacpi_kernel_get_thread_id()
{
	thread_id thread = find_thread(NULL);
	// TODO: We arn't allowed threads with id 0, handle this case.
	// ACPICA treats a 0 return as an error,
	// but we are thread 0 in early boot
	return (uacpi_thread_id)(uintptr_t)thread;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsSignal
 *
 * PARAMETERS:  function            ACPI CA signal function code
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Miscellaneous functions. Example implementation only.
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_handle_firmware_request(uacpi_firmware_request* request)
{
	DEBUG_FUNCTION();

	switch (request->type) {
		case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
#ifdef _KERNEL_MODE
			panic("ACPI FATAL event: %d %d %ld", request->fatal.type, request->fatal.code, request->fatal.arg);
			break;
#endif
		case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
			if (request->breakpoint.ctx != NULL)
				uacpi_kernel_log(UACPI_LOG_INFO, "AcpiOsBreakpoint: %s ****\n", (const char*)request->breakpoint.ctx);
			else
				uacpi_kernel_log(UACPI_LOG_INFO, "At AcpiOsBreakpoint ****\n");
			break;
	}

	return UACPI_STATUS_OK;
}


uacpi_handle
uacpi_kernel_create_mutex()
{
#ifdef _KERNEL_MODE
	mutex* outHandle = (mutex*) malloc(sizeof(mutex));
	DEBUG_FUNCTION_F("result: %p", outHandle);
	if (outHandle == NULL)
		return NULL;

	mutex_init(outHandle, "acpi mutex");
	return outHandle;
#else
	// TODO
	return NULL;
#endif
}


void
uacpi_kernel_free_mutex(uacpi_handle handle)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("mutex: %ld", (addr_t)handle);
	mutex_destroy((mutex*)handle);
	free((void*)handle);
#else
	// TODO
#endif
}


uacpi_status
uacpi_kernel_acquire_mutex(uacpi_handle handle, uint16 timeout)
{
#ifdef _KERNEL_MODE
	uacpi_status result = UACPI_STATUS_OK;
	DEBUG_FUNCTION_VF("mutex: %p; timeout: %u", handle, timeout);

	if (timeout == ACPI_WAIT_FOREVER) {
		result = (mutex_lock((mutex*)handle) == B_OK) ? UACPI_STATUS_OK : UACPI_STATUS_INVALID_ARGUMENT;
	} else if (timeout == ACPI_DO_NOT_WAIT) {
		result = (mutex_trylock((mutex*)handle) == B_OK) ? UACPI_STATUS_OK : UACPI_STATUS_TIMEOUT;
	} else {
		switch (mutex_lock_with_timeout((mutex*)handle, B_RELATIVE_TIMEOUT,
			(bigtime_t)timeout * 1000)) {
			case B_OK:
				result = UACPI_STATUS_OK;
				break;
			case B_INTERRUPTED:
			case B_TIMED_OUT:
			case B_WOULD_BLOCK:
				result = UACPI_STATUS_TIMEOUT;
				break;
			case B_BAD_VALUE:
			default:
				result = UACPI_STATUS_INVALID_ARGUMENT;
				break;
		}
	}
	DEBUG_FUNCTION_VF("mutex: %p; timeout: %u result: %" B_PRIu32,
		handle, timeout, (uint32)result);
	return result;
#else
	return UACPI_STATUS_UNIMPLEMENTED;
#endif
}


void
uacpi_kernel_release_mutex(uacpi_handle handle)
{
#ifdef _KERNEL_MODE
	DEBUG_FUNCTION_F("mutex: %p", handle);
	mutex_unlock((mutex*)handle);
#else
	// TODO
#endif
}


/******************************************************************************
 *
 * FUNCTION:    AcpiOsWaitEventsComplete
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Wait for all asynchronous events to complete. This
 *              implementation does nothing.
 *
 *****************************************************************************/
uacpi_status
uacpi_kernel_wait_for_work_completion(void)
{
    //TODO: FreeBSD See description.
	return UACPI_STATUS_UNIMPLEMENTED;
}
