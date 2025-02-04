/*
 * Copyright 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */


#include <ACPI.h>
#include <apic.h>

extern "C" {
#include "acpi.h"
#include "uacpi/status.h"
#include "uacpi/uacpi.h"
}

#include "arch_init.h"


#define PIC_MODE 0
#define APIC_MODE 1

#if 0
// TODO uacpi does not provide this, we need to reimplement it ourselves
-ACPI_STATUS ACPI_INIT_FUNCTION
-AcpiFindRootPointer (
-    ACPI_PHYSICAL_ADDRESS   *TableAddress)
-{
-    UINT8                   *TablePtr;
-    UINT8                   *MemRover;
-    UINT32                  PhysicalAddress;
-    UINT32                  EbdaWindowSize;
-
-
-    ACPI_FUNCTION_TRACE (AcpiFindRootPointer);
-
-
-    /* 1a) Get the location of the Extended BIOS Data Area (EBDA) */
-
-    TablePtr = AcpiOsMapMemory (
-        (ACPI_PHYSICAL_ADDRESS) ACPI_EBDA_PTR_LOCATION,
-        ACPI_EBDA_PTR_LENGTH);
-    if (!TablePtr)
-    {
-        ACPI_ERROR ((AE_INFO,
-            "Could not map memory at 0x%8.8X for length %u",
-            ACPI_EBDA_PTR_LOCATION, ACPI_EBDA_PTR_LENGTH));
-
-        return_ACPI_STATUS (AE_NO_MEMORY);
-    }
-
-    ACPI_MOVE_16_TO_32 (&PhysicalAddress, TablePtr);
-
-    /* Convert segment part to physical address */
-
-    PhysicalAddress <<= 4;
-    AcpiOsUnmapMemory (TablePtr, ACPI_EBDA_PTR_LENGTH);
-
-    /* EBDA present? */
-
-    /*
-     * Check that the EBDA pointer from memory is sane and does not point
-     * above valid low memory
-     */
-    if (PhysicalAddress > 0x400 &&
-        PhysicalAddress < 0xA0000)
-    {
-        /*
-         * Calculate the scan window size
-         * The EBDA is not guaranteed to be larger than a KiB and in case
-         * that it is smaller, the scanning function would leave the low
-         * memory and continue to the VGA range.
-         */
-        EbdaWindowSize = ACPI_MIN(ACPI_EBDA_WINDOW_SIZE,
-            0xA0000 - PhysicalAddress);
-
-        /*
-         * 1b) Search EBDA paragraphs
-         */
-        TablePtr = AcpiOsMapMemory (
-            (ACPI_PHYSICAL_ADDRESS) PhysicalAddress,
-            EbdaWindowSize);
-        if (!TablePtr)
-        {
-            ACPI_ERROR ((AE_INFO,
-                "Could not map memory at 0x%8.8X for length %u",
-                PhysicalAddress, EbdaWindowSize));
-
-            return_ACPI_STATUS (AE_NO_MEMORY);
-        }
-
-        MemRover = AcpiTbScanMemoryForRsdp (
-            TablePtr, EbdaWindowSize);
-        AcpiOsUnmapMemory (TablePtr, EbdaWindowSize);
-
-        if (MemRover)
-        {
-            /* Return the physical address */
-
-            PhysicalAddress +=
-                (UINT32) ACPI_PTR_DIFF (MemRover, TablePtr);
-
-            *TableAddress = (ACPI_PHYSICAL_ADDRESS) PhysicalAddress;
-            return_ACPI_STATUS (AE_OK);
-        }
-    }
-
-    /*
-     * 2) Search upper memory: 16-byte boundaries in E0000h-FFFFFh
-     */
-    TablePtr = AcpiOsMapMemory (
-        (ACPI_PHYSICAL_ADDRESS) ACPI_HI_RSDP_WINDOW_BASE,
-        ACPI_HI_RSDP_WINDOW_SIZE);
-
-    if (!TablePtr)
-    {
-        ACPI_ERROR ((AE_INFO,
-            "Could not map memory at 0x%8.8X for length %u",
-            ACPI_HI_RSDP_WINDOW_BASE, ACPI_HI_RSDP_WINDOW_SIZE));
-
-        return_ACPI_STATUS (AE_NO_MEMORY);
-    }
-
-    MemRover = AcpiTbScanMemoryForRsdp (
-        TablePtr, ACPI_HI_RSDP_WINDOW_SIZE);
-    AcpiOsUnmapMemory (TablePtr, ACPI_HI_RSDP_WINDOW_SIZE);
-
-    if (MemRover)
-    {
-        /* Return the physical address */
-
-        PhysicalAddress = (UINT32)
-            (ACPI_HI_RSDP_WINDOW_BASE + ACPI_PTR_DIFF (MemRover, TablePtr));
-
-        *TableAddress = (ACPI_PHYSICAL_ADDRESS) PhysicalAddress;
-        return_ACPI_STATUS (AE_OK);
-    }
-
-    /* A valid RSDP was not found */
-
-    ACPI_BIOS_ERROR ((AE_INFO, "A valid RSDP was not found"));
-    return_ACPI_STATUS (AE_NOT_FOUND);
-}
#endif

phys_addr_t
arch_init_find_root_pointer()
{
	phys_addr_t address;
	uacpi_status status = AcpiFindRootPointer(&address);
	if (status == UACPI_STATUS_OK)
		return address;

	return 0;
}


void
arch_init_interrupt_controller()
{
	uacpi_object* arg;
	uacpi_object_array parameter;

	arg = uacpi_object_create_integer(apic_available() ? APIC_MODE : PIC_MODE);
	parameter.count = 1;
	parameter.objects = &arg;

	uacpi_execute(NULL, "\\_PIC", &parameter);
}
