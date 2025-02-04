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
#include "uacpi/utilities.h"
}

#include "arch_init.h"


#define PIC_MODE 0
#define APIC_MODE 1

void
arch_init_interrupt_controller()
{
	uacpi_set_interrupt_model(apic_available()
		? UACPI_INTERRUPT_MODEL_IOAPIC : UACPI_INTERRUPT_MODEL_PIC);
}
