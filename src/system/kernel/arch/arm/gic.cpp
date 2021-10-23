#include <int.h>
#include <interrupt_controller.h>
#include <kernel.h>
#include <vm/vm.h>

#include "gic.h"
#include "gic_regs.h"

#define GIC_SPI_IRQ_START	32


static volatile uint32_t *gicd_regs = NULL;
static volatile uint32_t *gicc_regs = NULL;


uint32_t
gicd_read(uint32_t reg)
{
	return gicd_regs[reg];
}


void
gicd_write(uint32_t reg, uint32_t value)
{
	//dprintf("GICD[%04x] <-- 0x%08x\n", reg*4, value);
	gicd_regs[reg] = value;
}


uint32_t
gicc_read(uint32_t reg)
{
	return gicc_regs[reg];
}


void
gicc_write(uint32_t reg, uint32_t value)
{
	//dprintf("GICC[%04x] <-- 0x%08x\n", reg*4, value);
	gicc_regs[reg] = value;
}


uint32_t
gic_get_iar(void)
{
	return gicc_regs[GICC_REG_IAR];
}


uint32_t
gic_get_irq(uint32_t iar)
{
	return iar & 0x3FF;
}


void
gic_eoi(uint32_t iar)
{
	gicc_regs[GICC_REG_EOIR] = iar;
}


bool
gic_is_spurious_interrupt(uint32_t irq)
{
	return (irq == 1022) || (irq == 1023);
}


GICv2InterruptController::GICv2InterruptController()
: InterruptController()
{
	area_id gicd_area = vm_map_physical_memory(B_SYSTEM_TEAM, "gicd", (void**)&gicd_regs,
		B_ANY_KERNEL_ADDRESS, GICD_REG_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		GICD_REG_START, false);
	if (gicd_area < 0) {
		panic("not able to map the memory area for gicd\n");
	}

	area_id gicc_area = vm_map_physical_memory(B_SYSTEM_TEAM, "gicc", (void**)&gicc_regs,
		B_ANY_KERNEL_ADDRESS, GICC_REG_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		GICC_REG_START, false);
	if (gicc_area < 0) {
		panic("not able to map the memory area for gicc\n");
	}

	// disable GICD
	gicd_write(GICD_REG_CTLR, 0);

	// disable GICC
	gicc_write(GICC_REG_CTLR, 0);

	// TODO: disable all interrupts
	gicd_write(GICD_REG_ICENABLER, 0xffffffff);
	gicd_write(GICD_REG_ICENABLER+1, 0xffffffff);

	// set PMR and BPR
	gicc_write(GICC_REG_PMR, 0xff);
	gicc_write(GICC_REG_BPR, 0x07);

	// enable GICC
	gicc_write(GICC_REG_CTLR, 0x03);

	// enable GICD
	gicd_write(GICD_REG_CTLR, 0x03);
}


void GICv2InterruptController::EnableInterrupt(int irq)
{
	irq += GIC_SPI_IRQ_START;

	uint32_t ena_reg = GICD_REG_ISENABLER + irq / 32;
	uint32_t ena_val = 1 << (irq % 32);
	gicd_write(ena_reg, ena_val);

	uint32_t prio_reg = GICD_REG_IPRIORITYR + irq / 4;
	uint32_t prio_val = gicd_read(prio_reg);
	prio_val |= 0x80 << (irq % 4 * 8);
	gicd_write(prio_reg, prio_val);
}


void GICv2InterruptController::DisableInterrupt(int irq)
{
	irq += GIC_SPI_IRQ_START;
	gicd_regs[GICD_REG_ICENABLER + irq / 32] = 1 << (irq % 32);
}


void GICv2InterruptController::HandleInterrupt()
{
	uint32_t iar = gic_get_iar();
	uint32_t irqnr = gic_get_irq(iar);
	if (gic_is_spurious_interrupt(irqnr)) {
		dprintf("spurious interrupt\n");
	} else {
		//dprintf("========== irq[%d] ==========\n", irqnr);
		int_io_interrupt_handler(irqnr-GIC_SPI_IRQ_START, true);
		//dprintf("====== end of irq [%d] ======\n", irqnr);
	}

	gic_eoi(iar);
}
