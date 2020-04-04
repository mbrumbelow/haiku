/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>
#include <i2c.h>
#include <bus/PCI.h>
#include <PCI_x86.h>


extern "C" {
#	include "acpi.h"
}

#include "pch_i2c.h"


//#define TRACE_PCH_I2C
#ifdef TRACE_PCH_I2C
#	define TRACE(x...) dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define PCH_I2C_PCI_DEVICE_MODULE_NAME "busses/i2c/pch_i2c/pci/driver_v1"
#define PCH_I2C_SIM_MODULE_NAME "busses/i2c/pch_i2c/device/v1"

#define write32(address, data) \
	(*((volatile uint32*)(address)) = (data))
#define read32(address) \
	(*((volatile uint32*)(address)))

typedef enum {
	PCH_I2C_IRQ_LEGACY,
	PCH_I2C_IRQ_MSI,
	PCH_I2C_IRQ_MSI_X_SHARED
} pch_i2c_irq_type;


typedef struct {
	pci_device_module_info* pci;
	pci_device* device;
	phys_addr_t base_addr;
	uint8 irq;
	pch_i2c_irq_type irq_type;
	i2c_bus sim;

	device_node* node;
	pci_info info;

	area_id registersArea;
	addr_t registers;
	uint32 capabilities;

	uint16 ss_hcnt;
	uint16 ss_lcnt;
	uint16 fs_hcnt;
	uint16 fs_lcnt;
	uint16 hs_hcnt;
	uint16 hs_lcnt;
	uint32 sda_hold_time;

	uint8 tx_fifo_depth;
	uint8 rx_fifo_depth;

	uint32 masterConfig;

	// transfer
	bool	busy;
	bool	readwait;
	bool	writewait;
	i2c_op	op;
	void*	buffer;
	size_t	length;
	uint32	flags;
	int32	error;
} pch_i2c_pci_sim_info;


struct pch_i2c_crs {
	uint16	i2c_addr;
	uint8	irq;
    uint8	irq_triggering;
	uint8	irq_polarity;
	uint8	irq_sharable;

	uint32	addr_min;
	uint32	addr_bas;
	uint32	addr_len;
	uint16	gpio_int_pin;
	uint16	gpio_int_flags;
};


device_manager_info* gDeviceManager;
i2c_for_controller_interface* gI2c;
acpi_module_info* gACPI;
static pci_x86_module_info* sPCIx86Module;


static void
enable_device(pch_i2c_pci_sim_info* bus, bool enable)
{
	uint32 status = enable ? 1 : 0;
	for (int tries = 100; tries >= 0; tries--) {
		write32(bus->registers + PCH_IC_ENABLE, status);
		if ((read32(bus->registers + PCH_IC_ENABLE_STATUS) & 1) == status)
			return;
		snooze(25);
	}

	ERROR("enable_device failed\n");
}


static int32
pch_i2c_interrupt_handler(pch_i2c_pci_sim_info* bus)
{
	int32 handled = B_HANDLED_INTERRUPT;

	// Check if this interrupt is ours
	uint32 enable = read32(bus->registers + PCH_IC_ENABLE);
	if (enable == 0)
		return B_UNHANDLED_INTERRUPT;

	uint32 status = read32(bus->registers + PCH_IC_INTR_STAT);
	if ((status & PCH_IC_INTR_STAT_RX_UNDER) != 0)
		write32(bus->registers + PCH_IC_CLR_RX_UNDER, 0);
	if ((status & PCH_IC_INTR_STAT_RX_OVER) != 0)
		write32(bus->registers + PCH_IC_CLR_RX_OVER, 0);
	if ((status & PCH_IC_INTR_STAT_TX_OVER) != 0)
		write32(bus->registers + PCH_IC_CLR_TX_OVER, 0);
	if ((status & PCH_IC_INTR_STAT_RD_REQ) != 0)
		write32(bus->registers + PCH_IC_CLR_RD_REQ, 0);
	if ((status & PCH_IC_INTR_STAT_TX_ABRT) != 0)
		write32(bus->registers + PCH_IC_CLR_TX_ABRT, 0);
	if ((status & PCH_IC_INTR_STAT_RX_DONE) != 0)
		write32(bus->registers + PCH_IC_CLR_RX_DONE, 0);
	if ((status & PCH_IC_INTR_STAT_ACTIVITY) != 0)
		write32(bus->registers + PCH_IC_CLR_ACTIVITY, 0);
	if ((status & PCH_IC_INTR_STAT_STOP_DET) != 0)
		write32(bus->registers + PCH_IC_CLR_STOP_DET, 0);
	if ((status & PCH_IC_INTR_STAT_START_DET) != 0)
		write32(bus->registers + PCH_IC_CLR_START_DET, 0);
	if ((status & PCH_IC_INTR_STAT_GEN_CALL) != 0)
		write32(bus->registers + PCH_IC_CLR_GEN_CALL, 0);

	TRACE("pch_i2c_interrupt_handler %" B_PRIx32 "\n", status);

	if ((status & ~PCH_IC_INTR_STAT_ACTIVITY) == 0)
		return handled;
	/*if ((status & PCH_IC_INTR_STAT_TX_ABRT) != 0)
		tx error */
	if ((status & PCH_IC_INTR_STAT_RX_FULL) != 0)
		ConditionVariable::NotifyAll(&bus->readwait, B_OK);
	if ((status & PCH_IC_INTR_STAT_TX_EMPTY) != 0)
		ConditionVariable::NotifyAll(&bus->writewait, B_OK);
	if ((status & PCH_IC_INTR_STAT_STOP_DET) != 0) {
		bus->busy = false;
		ConditionVariable::NotifyAll(&bus->busy, B_OK);
	}

	return handled;
}


//	#pragma mark -


static void
set_sim(i2c_bus_cookie cookie, i2c_bus sim)
{
	CALLED();
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)cookie;
	bus->sim = sim;
}


static status_t
exec_command(i2c_bus_cookie cookie, i2c_op op, i2c_addr slaveAddress,
	const void *cmdBuffer, size_t cmdLength, void* dataBuffer,
	size_t dataLength)
{
	CALLED();
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)cookie;

	if (bus->busy)
		return B_BUSY;
	bus->busy = true;

	TRACE("exec_command: acquired busy flag\n");

	uint32 status = 0;
	for (int tries = 100; tries >= 0; tries--) {
		status = read32(bus->registers + PCH_IC_STATUS);
		if ((status & PCH_IC_STATUS_ACTIVITY) == 0)
			break;
		snooze(1000);
	}

	if ((status & PCH_IC_STATUS_ACTIVITY) != 0) {
		bus->busy = false;
		return B_BUSY;
	}

	TRACE("exec_command: write slave address\n");

	enable_device(bus, false);
	write32(bus->registers + PCH_IC_CON,
		read32(bus->registers + PCH_IC_CON) & ~PCH_IC_CON_10BIT_ADDR_MASTER);
	write32(bus->registers + PCH_IC_TAR, slaveAddress);

	write32(bus->registers + PCH_IC_INTR_MASK, 0);
	read32(bus->registers + PCH_IC_CLR_INTR);

	enable_device(bus, true);

	read32(bus->registers + PCH_IC_CLR_INTR);
	write32(bus->registers + PCH_IC_INTR_MASK, PCH_IC_INTR_STAT_TX_EMPTY);

	// wait for write
	// wait_lock

	if (cmdLength > 0) {
		TRACE("exec_command: write command buffer\n");
		uint16 txLimit = bus->tx_fifo_depth
			- read32(bus->registers + PCH_IC_TXFLR);
		if (cmdLength > txLimit) {
			ERROR("exec_command can't write, cmd too long %" B_PRIuSIZE
				" (max %d)\n", cmdLength, txLimit);
			bus->busy = false;
			return B_BAD_VALUE;
		}

		uint8* buffer = (uint8*)cmdBuffer;
		for (size_t i = 0; i < cmdLength; i++) {
			uint32 cmd = buffer[i];
			if (i == cmdLength - 1 && dataLength == 0 && IS_STOP_OP(op))
				cmd |= PCH_IC_DATA_CMD_STOP;
			write32(bus->registers + PCH_IC_DATA_CMD, cmd);
		}
	}

	TRACE("exec_command: processing buffer %" B_PRIuSIZE " bytes\n",
		dataLength);
	uint16 txLimit = bus->tx_fifo_depth
		- read32(bus->registers + PCH_IC_TXFLR);
	uint8* buffer = (uint8*)dataBuffer;
	size_t readPos = 0;
	size_t i = 0;
	while (i < dataLength) {
		uint32 cmd = PCH_IC_DATA_CMD_READ;
		if (IS_WRITE_OP(op))
			cmd = buffer[i];

		if (i == 0 && cmdLength > 0 && IS_READ_OP(op))
			cmd |= PCH_IC_DATA_CMD_RESTART;

		if (i == (dataLength - 1) && IS_STOP_OP(op))
			cmd |= PCH_IC_DATA_CMD_STOP;

		write32(bus->registers + PCH_IC_DATA_CMD, cmd);

		if (IS_READ_OP(op) && IS_BLOCK_OP(op) && readPos == 0)
			txLimit = 1;
		txLimit--;
		i++;

		// here read the data if needed
		while (IS_READ_OP(op) && (txLimit == 0 || i == dataLength)) {
			write32(bus->registers + PCH_IC_INTR_MASK,
				PCH_IC_INTR_STAT_RX_FULL);

			// sleep until wake up by intr handler
			struct ConditionVariable condition;
			condition.Publish(&bus->readwait, "pch_i2c");
			ConditionVariableEntry variableEntry;
			status_t status = variableEntry.Wait(&bus->readwait,
				B_RELATIVE_TIMEOUT, 500000L);
			condition.Unpublish();
			if (status != B_OK)
				ERROR("exec_command timed out waiting for read\n");
			uint32 rxBytes = read32(bus->registers + PCH_IC_RXFLR);
			if (rxBytes == 0) {
				ERROR("exec_command timed out reading %" B_PRIuSIZE " bytes\n",
					dataLength - readPos);
				bus->busy = false;
				return B_ERROR;
			}
			for (; rxBytes > 0; rxBytes--) {
				uint32 read = read32(bus->registers + PCH_IC_DATA_CMD);
				if (readPos < dataLength)
					buffer[readPos++] = read;
			}

			if (IS_BLOCK_OP(op) && readPos > 0 && dataLength > buffer[0])
				dataLength = buffer[0] + 1;
			if (readPos >= dataLength)
				break;

			TRACE("exec_command %" B_PRIuSIZE" bytes to be read\n",
				dataLength - readPos);
			txLimit = bus->tx_fifo_depth
				- read32(bus->registers + PCH_IC_TXFLR);
		}
	}

	status_t err = B_OK;
	if (IS_STOP_OP(op) && IS_WRITE_OP(op)) {
		TRACE("exec_command: waiting busy condition\n");
		while (bus->busy) {
			write32(bus->registers + PCH_IC_INTR_MASK,
				PCH_IC_INTR_STAT_STOP_DET);

			// sleep until wake up by intr handler
			struct ConditionVariable condition;
			condition.Publish(&bus->busy, "pch_i2c");
			ConditionVariableEntry variableEntry;
			err = variableEntry.Wait(&bus->busy, B_RELATIVE_TIMEOUT,
				500000L);
			condition.Unpublish();
			if (err != B_OK)
				ERROR("exec_command timed out waiting for busy\n");
		}
	}
	TRACE("exec_command: processing done\n");

	bus->busy = false;

	return err;
}


static acpi_status
pch_i2c_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct pch_i2c_crs* crs = (struct pch_i2c_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_SERIAL_BUS &&
	    res->Data.CommonSerialBus.Type == ACPI_RESOURCE_SERIAL_TYPE_I2C) {
		crs->i2c_addr = B_LENDIAN_TO_HOST_INT16(
			res->Data.I2cSerialBus.SlaveAddress);
		return AE_CTRL_TERMINATE;
	} else if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
		crs->irq = res->Data.Irq.Interrupts[0];
		crs->irq_triggering = res->Data.Irq.Triggering;
		crs->irq_polarity = res->Data.Irq.Polarity;
		crs->irq_sharable = res->Data.Irq.Sharable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		crs->irq = res->Data.ExtendedIrq.Interrupts[0];
		crs->irq_triggering = res->Data.ExtendedIrq.Triggering;
		crs->irq_polarity = res->Data.ExtendedIrq.Polarity;
		crs->irq_sharable = res->Data.ExtendedIrq.Sharable;
	}

	return B_OK;
}


static status_t
acpi_GetInteger(acpi_handle acpiCookie,
	const char* path, int64* number)
{
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);

	// Assume that what we've been pointed at is an Integer object, or
	// a method that will return an Integer.
	status_t status = gACPI->evaluate_method(acpiCookie, path, NULL, &buf);
	if (status == B_OK) {
		if (object.object_type == ACPI_TYPE_INTEGER)
			*number = object.integer.integer;
		else
			status = B_BAD_VALUE;
	}
	return status;
}


static acpi_status
pch_i2c_scan_bus_callback(acpi_handle object, uint32 nestingLevel,
	void *context, void** returnValue)
{
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)context;
	TRACE("pch_i2c_scan_bus_callback %p\n", object);

	// skip absent devices
	int64 sta;
	status_t status = acpi_GetInteger(object, "_STA", &sta);
	if (status == B_OK && (sta & ACPI_STA_DEVICE_PRESENT) == 0)
		return B_OK;

	// Attach devices for I2C resources
	struct pch_i2c_crs crs;
	status = gACPI->walk_resources(object, (ACPI_STRING)"_CRS",
		pch_i2c_scan_parse_callback, &crs);
	if (status != B_OK) {
		ERROR("Error while getting I2C devices\n");
		return status;
	}

	TRACE("pch_i2c_scan_bus_callback deviceAddress %x\n", crs.i2c_addr);

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status = gACPI->ns_handle_to_pathname(object, &buffer);
	if (status != B_OK) {
		ERROR("pch_i2c_scan_bus_callback ns_handle_to_pathname failed\n");
		return status;
	}

	char* hid = NULL;
	char* cidList[8] = { NULL };
	status = gACPI->get_device_info((const char*)buffer.pointer, &hid,
		(char**)&cidList, 8);
	if (status != B_OK) {
		ERROR("pch_i2c_scan_bus_callback get_device_info failed\n");
		return status;
	}

	device_node* deviceNode;
	status = gI2c->register_device(bus->sim, crs.i2c_addr, hid, cidList,
		object);
	free(hid);
	for (int i = 0; cidList[i] != NULL; i++)
		free(cidList[i]);
	free(buffer.pointer);

	TRACE("pch_i2c_scan_bus_callback registered device: %s\n", strerror(status));

	return status;
}


static status_t
scan_bus(i2c_bus_cookie cookie)
{
	CALLED();
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)cookie;
	device_node *acpiNode = NULL;

	pci_info *pciInfo = &bus->info;

	// search ACPI I2C nodes for this device
	{
		device_node* deviceRoot = gDeviceManager->get_root_node();
		uint32 addr = (pciInfo->device << 16) | pciInfo->function;
		device_attr acpiAttrs[] = {
			{ B_DEVICE_BUS, B_STRING_TYPE, { string: "acpi" }},
			{ ACPI_DEVICE_ADDR_ITEM, B_UINT32_TYPE, {ui32: addr}},
			{ NULL }
		};
		if (addr != 0 && gDeviceManager->find_child_node(deviceRoot, acpiAttrs,
				&acpiNode) != B_OK) {
			ERROR("init_bus() acpi device not found\n");
			return B_DEV_CONFIGURATION_ERROR;
		}
	}

	TRACE("init_bus() find_child_node() found %x %x %p\n",
		pciInfo->device, pciInfo->function, acpiNode);
	// TODO eventually check timings on acpi
	acpi_device_module_info *acpi;
	acpi_device	acpiDevice;
	if (gDeviceManager->get_driver(acpiNode, (driver_module_info **)&acpi,
		(void **)&acpiDevice) == B_OK) {
		// find out I2C device nodes
		acpi->walk_namespace(acpiDevice, ACPI_TYPE_DEVICE, 1,
			pch_i2c_scan_bus_callback, NULL, bus, NULL);
	}

	return B_OK;
}


//	#pragma mark -


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	status_t status = B_OK;

	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)calloc(1,
		sizeof(pch_i2c_pci_sim_info));
	if (bus == NULL) {
		return B_NO_MEMORY;
	}

	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* parent = gDeviceManager->get_parent_node(node);
		device_node* pciParent = gDeviceManager->get_parent_node(parent);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
			(void**)&device);
		gDeviceManager->put_node(pciParent);
		gDeviceManager->put_node(parent);
	}

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&sPCIx86Module)
			!= B_OK) {
		sPCIx86Module = NULL;
	}

	bus->node = node;
	bus->pci = pci;
	bus->device = device;

	pci_info *pciInfo = &bus->info;
	pci->get_pci_info(device, pciInfo);

	bus->base_addr = pciInfo->u.h0.base_registers[0];
	bus->base_addr &= PCI_address_memory_32_mask;
	if ((pciInfo->u.h0.base_register_flags[0] & 0xc) == PCI_address_type_64)
		bus->base_addr += (phys_addr_t)pciInfo->u.h0.base_registers[1] << 32;
	size_t mapSize = pciInfo->u.h0.base_register_sizes[0];

	// enable power
	pci->set_powerstate(device, PCI_pm_state_d0);

	// enable bus master and memory
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	if (sPCIx86Module != NULL) {
		// try MSI-X
		uint8 msixCount = sPCIx86Module->get_msix_count(
			pciInfo->bus, pciInfo->device, pciInfo->function);
		if (msixCount >= 1) {
			uint8 vector;
			if (sPCIx86Module->configure_msix(pciInfo->bus, pciInfo->device,
					pciInfo->function, 1, &vector) == B_OK
				&& sPCIx86Module->enable_msix(pciInfo->bus, pciInfo->device,
					pciInfo->function) == B_OK) {
				TRACE_ALWAYS("using MSI-X vector %u\n", vector);
				bus->irq = vector;
				bus->irq_type = PCH_I2C_IRQ_MSI_X_SHARED;
			} else {
				ERROR("couldn't use MSI-X SHARED\n");
			}
		} else if (sPCIx86Module->get_msi_count(
			pciInfo->bus, pciInfo->device, pciInfo->function) >= 1) {
			// try MSI
			uint8 vector;
			if (sPCIx86Module->configure_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function, 1, &vector) == B_OK
				&& sPCIx86Module->enable_msi(pciInfo->bus, pciInfo->device,
					pciInfo->function) == B_OK) {
				TRACE_ALWAYS("using MSI vector %u\n", vector);
				bus->irq = vector;
				bus->irq_type = PCH_I2C_IRQ_MSI;
			} else {
				ERROR("couldn't use MSI\n");
			}
		}
	}
	if (bus->irq_type == PCH_I2C_IRQ_LEGACY) {
		bus->irq = pciInfo->u.h0.interrupt_line;
		TRACE_ALWAYS("using legacy interrupt %u\n", bus->irq);
	}
	if (bus->irq == 0 || bus->irq == 0xff) {
		ERROR("PCI IRQ not assigned\n");
		status = B_ERROR;
		goto err;
	}

	bus->registersArea = map_physical_memory("PCHI2C memory mapped registers",
		bus->base_addr, mapSize, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&bus->registers);
	// init bus
	bus->capabilities = read32(bus->registers + PCH_SUP_CAPABLITIES);
	TRACE("init_bus() 0x%x (0x%" B_PRIx32 ")\n",
		(bus->capabilities >> PCH_SUP_CAPABLITIES_TYPE_SHIFT)
			& PCH_SUP_CAPABLITIES_TYPE_MASK,
		bus->capabilities);
	if (((bus->capabilities >> PCH_SUP_CAPABLITIES_TYPE_SHIFT)
			& PCH_SUP_CAPABLITIES_TYPE_MASK) != 0) {
		status = B_ERROR;
		ERROR("init_bus() device type not supported\n");
		goto err;
	}

	write32(bus->registers + PCH_SUP_RESETS, 0);
	write32(bus->registers + PCH_SUP_RESETS,
		PCH_SUP_RESETS_FUNC | PCH_SUP_RESETS_IDMA);

	if (bus->ss_hcnt == 0)
		bus->ss_hcnt = read32(bus->registers + PCH_IC_SS_SCL_HCNT);
	if (bus->ss_lcnt == 0)
		bus->ss_lcnt = read32(bus->registers + PCH_IC_SS_SCL_LCNT);
	if (bus->fs_hcnt == 0)
		bus->fs_hcnt = read32(bus->registers + PCH_IC_FS_SCL_HCNT);
	if (bus->fs_lcnt == 0)
		bus->fs_lcnt = read32(bus->registers + PCH_IC_FS_SCL_LCNT);
	if (bus->sda_hold_time == 0)
		bus->sda_hold_time = read32(bus->registers + PCH_IC_SDA_HOLD);
	TRACE("init_bus() 0x%04" B_PRIx16 " 0x%04" B_PRIx16 " 0x%04" B_PRIx16
		" 0x%04" B_PRIx16 " 0x%08" B_PRIx32 "\n", bus->ss_hcnt, bus->ss_lcnt,
		bus->fs_hcnt, bus->fs_lcnt, bus->sda_hold_time);

	enable_device(bus, false);

	write32(bus->registers + PCH_IC_SS_SCL_HCNT, bus->ss_hcnt);
	write32(bus->registers + PCH_IC_SS_SCL_LCNT, bus->ss_lcnt);
	write32(bus->registers + PCH_IC_FS_SCL_HCNT, bus->fs_hcnt);
	write32(bus->registers + PCH_IC_FS_SCL_LCNT, bus->fs_lcnt);
	if (bus->hs_hcnt > 0)
		write32(bus->registers + PCH_IC_HS_SCL_HCNT, bus->hs_hcnt);
	if (bus->hs_lcnt > 0)
		write32(bus->registers + PCH_IC_HS_SCL_LCNT, bus->hs_lcnt);
	{
		uint32 reg = read32(bus->registers + PCH_IC_COMP_VERSION);
		if (reg >= PCH_IC_COMP_VERSION_MIN)
			write32(bus->registers + PCH_IC_SDA_HOLD, bus->sda_hold_time);
	}

	{
		bus->tx_fifo_depth = 32;
		bus->rx_fifo_depth = 32;
		uint32 reg = read32(bus->registers + PCH_IC_COMP_PARAM1);
		uint8 rx_fifo_depth = PCH_IC_COMP_PARAM1_RX(reg);
		uint8 tx_fifo_depth = PCH_IC_COMP_PARAM1_TX(reg);
		if (rx_fifo_depth > 1 && rx_fifo_depth < bus->rx_fifo_depth)
			bus->rx_fifo_depth = rx_fifo_depth;
		if (tx_fifo_depth > 1 && tx_fifo_depth < bus->tx_fifo_depth)
			bus->tx_fifo_depth = tx_fifo_depth;
		write32(bus->registers + PCH_IC_RX_TL, 0);
		write32(bus->registers + PCH_IC_TX_TL, bus->tx_fifo_depth / 2);
	}

	bus->masterConfig = PCH_IC_CON_MASTER | PCH_IC_CON_SLAVE_DISABLE |
	    PCH_IC_CON_RESTART_EN | PCH_IC_CON_SPEED_FAST;
	write32(bus->registers + PCH_IC_CON, bus->masterConfig);

	write32(bus->registers + PCH_IC_INTR_MASK, 0);
	read32(bus->registers + PCH_IC_CLR_INTR);

	status = install_io_interrupt_handler(bus->irq,
		(interrupt_handler)pch_i2c_interrupt_handler, bus, 0);
	if (status != B_OK)
		goto err;

	*bus_cookie = bus;
	return status;

err:
	if (bus->registersArea >= 0)
		delete_area(bus->registersArea);
	if (sPCIx86Module != NULL) {
		put_module(B_PCI_X86_MODULE_NAME);
		sPCIx86Module = NULL;
	}
	delete bus;
	return status;
}


static void
uninit_bus(void* bus_cookie)
{
	pch_i2c_pci_sim_info* bus = (pch_i2c_pci_sim_info*)bus_cookie;

	remove_io_interrupt_handler(bus->irq,
		(interrupt_handler)pch_i2c_interrupt_handler, bus);

	if (bus->irq_type != PCH_I2C_IRQ_LEGACY) {
		if (sPCIx86Module != NULL) {
			sPCIx86Module->disable_msi(bus->info.bus,
				bus->info.device, bus->info.function);
			sPCIx86Module->unconfigure_msi(bus->info.bus,
				bus->info.device, bus->info.function);
		}
	}
	if (sPCIx86Module != NULL) {
		put_module(B_PCI_X86_MODULE_NAME);
		sPCIx86Module = NULL;
	}
	delete bus;
}


//	#pragma mark -


static status_t
register_child_devices(void* cookie)
{
	CALLED();
	device_node* node = (device_node*)cookie;
	device_node* parent = gDeviceManager->get_parent_node(node);
	pci_device_module_info* pci;
	pci_device* device;
	gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
		(void**)&device);

	char prettyName[25];
	sprintf(prettyName, "PCH I2C Device %" B_PRIu16, 0);

	device_attr attrs[] = {
		// properties of this controller for i2c bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ string: I2C_FOR_CONTROLLER_MODULE_NAME }},

		// private data to identify the device
		{ NULL }
	};

	return gDeviceManager->register_node(node, PCH_I2C_SIM_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	*device_cookie = node;
	return B_OK;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCH I2C PCI"}},
		{}
	};

	return gDeviceManager->register_node(parent,
		PCH_I2C_PCI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 vendorID, deviceID;

	// make sure parent is a PCH I2C PCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK || gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID,
				&vendorID, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID,
				false) < B_OK) {
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (vendorID == 0x8086) {
		switch (deviceID) {
			case 0x02c5:
			case 0x02c6:
			case 0x02e8:
			case 0x02e9:
			case 0x02ea:
			case 0x02eb:
			case 0x06e8:
			case 0x06e9:
			case 0x06ea:
			case 0x06eb:
			case 0x0aac:
			case 0x0aae:
			case 0x0ab0:
			case 0x0ab2:
			case 0x0ab4:
			case 0x0ab6:
			case 0x0ab8:
			case 0x0aba:
			case 0x1aac:
			case 0x1aae:

			case 0x1ab0:
			case 0x1ab2:
			case 0x1ab4:
			case 0x1ab6:
			case 0x1ab8:
			case 0x1aba:

			case 0x31ac:
			case 0x31ae:
			case 0x31b0:
			case 0x31b2:
			case 0x31b4:
			case 0x31b6:
			case 0x31b8:
			case 0x31ba:

			case 0x34c5:
			case 0x34c6:
			case 0x34e8:
			case 0x34e9:
			case 0x34ea:
			case 0x34eb:

			case 0x4b44:
			case 0x4b45:
			case 0x4b4b:
			case 0x4b4c:
			case 0x4b78:
			case 0x4b79:
			case 0x4b7a:
			case 0x4b7b:

			case 0x4dc5:
			case 0x4dc6:
			case 0x4de8:
			case 0x4de9:
			case 0x4dea:
			case 0x4deb:

			case 0x5aac:
			case 0x5aae:
			case 0x5ab0:
			case 0x5ab2:
			case 0x5ab4:
			case 0x5ab6:
			case 0x5ab8:
			case 0x5aba:

			case 0x9d60:
			case 0x9d61:
			case 0x9d62:
			case 0x9d63:
			case 0x9d64:
			case 0x9d65:

			case 0x9dc5:
			case 0x9dc6:
			case 0x9de8:
			case 0x9de9:
			case 0x9dea:
			case 0x9deb:

			case 0xa0c5:
			case 0xa0c6:
			case 0xa0d8:
			case 0xa0d9:
			case 0xa0e8:
			case 0xa0e9:
			case 0xa0ea:
			case 0xa0eb:

			case 0xa160:
			case 0xa161:
			case 0xa162:

			case 0xa2e0:
			case 0xa2e1:
			case 0xa2e2:
			case 0xa2e3:

			case 0xa368:
			case 0xa369:
			case 0xa36a:
			case 0xa36b:
				break;
			default:
				return 0.0f;
		}
		pci_device_module_info* pci;
		pci_device* device;
		gDeviceManager->get_driver(parent, (driver_module_info**)&pci,
			(void**)&device);
		uint8 pciSubDeviceId = pci->read_pci_config(device, PCI_revision,
			1);

		TRACE("PCH I2C device found! vendor 0x%04x, device 0x%04x\n", vendorID,
			deviceID);
		return 0.8f;
	}

	return 0.0f;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info**)&gACPI },
	{ I2C_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gI2c },
	{}
};


static i2c_sim_interface sPchI2cDeviceModule = {
	{
		{
			PCH_I2C_SIM_MODULE_NAME,
			0,
			NULL
		},

		NULL,	// supports device
		NULL,	// register device
		init_bus,
		uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		NULL, 	// device removed
	},

	set_sim,
	exec_command,
	scan_bus,
};


static driver_module_info sPchI2cPciDevice = {
	{
		PCH_I2C_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	NULL,	// uninit
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


module_info* modules[] = {
	(module_info* )&sPchI2cPciDevice,
	(module_info* )&sPchI2cDeviceModule,
	NULL
};
