/*
 * Copyright 2024, Enrique Medina Gremaldos, quique@necos.es.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>
#include <bus/PCI.h>


#include "dw_i2c.h"


device_manager_info* gDeviceManager;
i2c_for_controller_interface* gI2c;
acpi_module_info* gACPI;


static void
enable_device(dw_i2c_sim_info* bus, bool enable)
{
	uint32 status = enable ? 1 : 0;
	for (int tries = 100; tries >= 0; tries--) {
		write32(bus->registers + DW_IC_ENABLE, status);
		if ((read32(bus->registers + DW_IC_ENABLE_STATUS) & 1) == status)
			return;
		snooze(25);
	}

	ERROR("enable_device failed\n");
}


static int32
dw_i2c_interrupt_handler(dw_i2c_sim_info* bus)
{
	int32 handled = B_HANDLED_INTERRUPT;
	
	// Check if this interrupt is ours
	uint32 enable = read32(bus->registers + DW_IC_ENABLE);
	if (enable == 0)
		return B_UNHANDLED_INTERRUPT;
	
	uint32 status = read32(bus->registers + DW_IC_INTR_STAT);
	
	if ((status & DW_IC_INTR_STAT_RX_UNDER) != 0)
		write32(bus->registers + DW_IC_CLR_RX_UNDER, 0);
	if ((status & DW_IC_INTR_STAT_RX_OVER) != 0)
		write32(bus->registers + DW_IC_CLR_RX_OVER, 0);
	if ((status & DW_IC_INTR_STAT_TX_OVER) != 0)
		write32(bus->registers + DW_IC_CLR_TX_OVER, 0);
	if ((status & DW_IC_INTR_STAT_RD_REQ) != 0)
		write32(bus->registers + DW_IC_CLR_RD_REQ, 0);
	if ((status & DW_IC_INTR_STAT_TX_ABRT) != 0)
		write32(bus->registers + DW_IC_CLR_TX_ABRT, 0);
	if ((status & DW_IC_INTR_STAT_RX_DONE) != 0)
		write32(bus->registers + DW_IC_CLR_RX_DONE, 0);
	if ((status & DW_IC_INTR_STAT_ACTIVITY) != 0)
		write32(bus->registers + DW_IC_CLR_ACTIVITY, 0);
	if ((status & DW_IC_INTR_STAT_STOP_DET) != 0)
		write32(bus->registers + DW_IC_CLR_STOP_DET, 0);
	if ((status & DW_IC_INTR_STAT_START_DET) != 0)
		write32(bus->registers + DW_IC_CLR_START_DET, 0);
	if ((status & DW_IC_INTR_STAT_GEN_CALL) != 0)
		write32(bus->registers + DW_IC_CLR_GEN_CALL, 0);
	
	TRACE("interrupt status %x\n",status);
	
	if ((status & ~DW_IC_INTR_STAT_ACTIVITY) == 0)
		return handled;
	
	if ((status & DW_IC_INTR_STAT_RX_FULL) != 0)
		ConditionVariable::NotifyAll(&bus->readwait, B_OK);
	if ((status & DW_IC_INTR_STAT_TX_EMPTY) != 0)
		ConditionVariable::NotifyAll(&bus->writewait, B_OK);
	if ((status & DW_IC_INTR_STAT_STOP_DET) != 0) {
		bus->busy = 0;
		ConditionVariable::NotifyAll(&bus->busy, B_OK);
	}
	
	return handled;
}


//	#pragma mark -


static void
set_sim(i2c_bus_cookie cookie, i2c_bus sim)
{
	CALLED();
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)cookie;
	bus->sim = sim;
}


static status_t
exec_command(i2c_bus_cookie cookie, i2c_op op, i2c_addr slaveAddress,
	const void *cmdBuffer, size_t cmdLength, void* dataBuffer,
	size_t dataLength)
{
	CALLED();
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)cookie;

	if (atomic_test_and_set(&bus->busy, 1, 0) != 0)
		return B_BUSY;

	TRACE("exec_command: acquired busy flag\n");

	uint32 status = 0;
	for (int tries = 100; tries >= 0; tries--) {
		status = read32(bus->registers + DW_IC_STATUS);
		if ((status & DW_IC_STATUS_MASTER_ACTIVITY) == 0)
			break;
		snooze(1000);
	}

	if ((status & DW_IC_STATUS_MASTER_ACTIVITY) != 0) {
		bus->busy = 0;
		return B_BUSY;
	}

	TRACE("exec_command: write slave address\n");
	
	enable_device(bus, false);
	write32(bus->registers + DW_IC_CON,
		read32(bus->registers + DW_IC_CON) & ~DW_IC_CON_10BITADDR_MASTER);
	write32(bus->registers + DW_IC_TAR, slaveAddress);
	
	write32(bus->registers + DW_IC_INTR_MASK, 0);
	read32(bus->registers + DW_IC_CLR_INTR);

	enable_device(bus, true);

	read32(bus->registers + DW_IC_CLR_INTR);
	write32(bus->registers + DW_IC_INTR_MASK, DW_IC_INTR_STAT_TX_EMPTY);

	if (cmdLength > 0) {
		TRACE("exec_command: write command buffer\n");
		uint16 txLimit = bus->tx_fifo_depth
			- read32(bus->registers + DW_IC_TXFLR);
		if (cmdLength > txLimit) {
			ERROR("exec_command can't write, cmd too long %" B_PRIuSIZE
				" (max %d)\n", cmdLength, txLimit);
			bus->busy = 0;
			return B_BAD_VALUE;
		}

		uint8* buffer = (uint8*)cmdBuffer;
		for (size_t i = 0; i < cmdLength; i++) {
			uint32 cmd = buffer[i];
			if (i == cmdLength - 1 && dataLength == 0 && IS_STOP_OP(op))
				cmd |= DW_IC_DATA_CMD_STOP;
			write32(bus->registers + DW_IC_DATA_CMD, cmd);
		}
	}
	
	TRACE("exec_command: processing buffer %" B_PRIuSIZE " bytes\n",
		dataLength);
	uint16 txLimit = bus->tx_fifo_depth
		- read32(bus->registers + DW_IC_TXFLR);
	uint8* buffer = (uint8*)dataBuffer;
	size_t readPos = 0;
	size_t i = 0;
	
	while (i < dataLength) {
		uint32 cmd = DW_IC_DATA_CMD_READ;
		if (IS_WRITE_OP(op))
			cmd = buffer[i];

		if (i == 0 && cmdLength > 0 && IS_READ_OP(op))
			cmd |= DW_IC_DATA_CMD_RESTART;

		if (i == (dataLength - 1) && IS_STOP_OP(op))
			cmd |= DW_IC_DATA_CMD_STOP;

		write32(bus->registers + DW_IC_DATA_CMD, cmd);

		if (IS_READ_OP(op) && IS_BLOCK_OP(op) && readPos == 0)
			txLimit = 1;
		txLimit--;
		i++;

		// here read the data if needed
		while (IS_READ_OP(op) && (txLimit == 0 || i == dataLength)) {
			write32(bus->registers + DW_IC_INTR_MASK,
				DW_IC_INTR_STAT_RX_FULL);

			// sleep until wake up by intr handler
			struct ConditionVariable condition;
			condition.Publish(&bus->readwait, "dw_i2c");
			ConditionVariableEntry variableEntry;
			status_t status = variableEntry.Wait(&bus->readwait,
				B_RELATIVE_TIMEOUT, 500000L);
			condition.Unpublish();
			if (status != B_OK)
				ERROR("exec_command timed out waiting for read\n");
			uint32 rxBytes = read32(bus->registers + DW_IC_RXFLR);
			if (rxBytes == 0) {
				ERROR("exec_command timed out reading %" B_PRIuSIZE " bytes\n",
					dataLength - readPos);
				bus->busy = 0;
				return B_ERROR;
			}
			for (; rxBytes > 0; rxBytes--) {
				uint32 read = read32(bus->registers + DW_IC_DATA_CMD);
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
				- read32(bus->registers + DW_IC_TXFLR);
		}
	}
	
	status_t err = B_OK;
	if (IS_STOP_OP(op) && IS_WRITE_OP(op)) {
		TRACE("exec_command: waiting busy condition\n");
		while (bus->busy == 1) {
			write32(bus->registers + DW_IC_INTR_MASK,
				DW_IC_INTR_STAT_STOP_DET);

			// sleep until wake up by intr handler
			struct ConditionVariable condition;
			condition.Publish(&bus->busy, "dw_i2c");
			ConditionVariableEntry variableEntry;
			err = variableEntry.Wait(&bus->busy, B_RELATIVE_TIMEOUT,
				500000L);
			condition.Unpublish();
			if (err != B_OK)
				ERROR("exec_command timed out waiting for busy\n");
		}
	}
	TRACE("exec_command: processing done\n");

	bus->busy = 0;
	
	return err;
}


static acpi_status
dw_i2c_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct dw_i2c_crs* crs = (struct dw_i2c_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_SERIAL_BUS &&
		res->Data.CommonSerialBus.Type == ACPI_RESOURCE_SERIAL_TYPE_I2C) {
		crs->i2c_addr = B_LENDIAN_TO_HOST_INT16(
			res->Data.I2cSerialBus.SlaveAddress);
		return AE_CTRL_TERMINATE;
	} else if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
		crs->irq = res->Data.Irq.Interrupts[0];
		crs->irq_triggering = res->Data.Irq.Triggering;
		crs->irq_polarity = res->Data.Irq.Polarity;
		crs->irq_shareable = res->Data.Irq.Shareable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		crs->irq = res->Data.ExtendedIrq.Interrupts[0];
		crs->irq_triggering = res->Data.ExtendedIrq.Triggering;
		crs->irq_polarity = res->Data.ExtendedIrq.Polarity;
		crs->irq_shareable = res->Data.ExtendedIrq.Shareable;
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


acpi_status
dw_i2c_scan_bus_callback(acpi_handle object, uint32 nestingLevel,
	void *context, void** returnValue)
{
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)context;
	TRACE("dw_i2c_scan_bus_callback %p\n", object);

	// skip absent devices
	int64 sta;
	status_t status = acpi_GetInteger(object, "_STA", &sta);
	if (status == B_OK && (sta & ACPI_STA_DEVICE_PRESENT) == 0)
		return B_OK;

	// Attach devices for I2C resources
	struct dw_i2c_crs crs;
	status = gACPI->walk_resources(object, (ACPI_STRING)"_CRS",
		dw_i2c_scan_parse_callback, &crs);
	if (status != B_OK) {
		ERROR("Error while getting I2C devices\n");
		return status;
	}

	TRACE("dw_i2c_scan_bus_callback deviceAddress %x\n", crs.i2c_addr);

	acpi_data buffer;
	buffer.pointer = NULL;
	buffer.length = ACPI_ALLOCATE_BUFFER;
	status = gACPI->ns_handle_to_pathname(object, &buffer);
	if (status != B_OK) {
		ERROR("dw_i2c_scan_bus_callback ns_handle_to_pathname failed\n");
		return status;
	}

	char* hid = NULL;
	char* cidList[8] = { NULL };
	status = gACPI->get_device_info((const char*)buffer.pointer, &hid,
		(char**)&cidList, 8, NULL, NULL);
	if (status != B_OK) {
		ERROR("dw_i2c_scan_bus_callback get_device_info failed\n");
		return status;
	}

	status = gI2c->register_device(bus->sim, crs.i2c_addr, hid, cidList,
		object);
	free(hid);
	for (int i = 0; cidList[i] != NULL; i++)
		free(cidList[i]);
	free(buffer.pointer);

	TRACE("dw_i2c_scan_bus_callback registered device: %s\n", strerror(status));

	return status;
}


static status_t
scan_bus(i2c_bus_cookie cookie)
{
	CALLED();
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)cookie;
	if (bus->scan_bus != NULL)
		return bus->scan_bus(bus);
	return B_OK;
}


static status_t
acquire_bus(i2c_bus_cookie cookie)
{
	CALLED();
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)cookie;
	return mutex_lock(&bus->lock);
}


static void
release_bus(i2c_bus_cookie cookie)
{
	CALLED();
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)cookie;
	mutex_unlock(&bus->lock);
}


//	#pragma mark -


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();
	uint32 bus_status = 0;
	status_t status = B_OK;

	driver_module_info* driver;
	dw_i2c_sim_info* bus;
	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, &driver, (void**)&bus);
	gDeviceManager->put_node(parent);

	TRACE_ALWAYS("init_bus() addr 0x%" B_PRIxPHYSADDR " size 0x%" B_PRIx64
		" irq 0x%" B_PRIx32 "\n", bus->base_addr, bus->map_size, bus->irq);

	bus->registersArea = map_physical_memory("DWI2C memory mapped registers",
		bus->base_addr, bus->map_size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&bus->registers);
	
	
	if (bus->ss_hcnt == 0)
		bus->ss_hcnt = read32(bus->registers + DW_IC_SS_SCL_HCNT);
	if (bus->ss_lcnt == 0)
		bus->ss_lcnt = read32(bus->registers + DW_IC_SS_SCL_LCNT);
	if (bus->fs_hcnt == 0)
		bus->fs_hcnt = read32(bus->registers + DW_IC_FS_SCL_HCNT);
	if (bus->fs_lcnt == 0)
		bus->fs_lcnt = read32(bus->registers + DW_IC_FS_SCL_LCNT);
	if (bus->sda_hold_time == 0)
		bus->sda_hold_time = read32(bus->registers + DW_IC_SDA_HOLD);
	TRACE_ALWAYS("init_bus() 0x%04" B_PRIx16 " 0x%04" B_PRIx16 " 0x%04" B_PRIx16
		" 0x%04" B_PRIx16 " 0x%08" B_PRIx32 "\n", bus->ss_hcnt, bus->ss_lcnt,
		bus->fs_hcnt, bus->fs_lcnt, bus->sda_hold_time);
	
	enable_device(bus, false);

	write32(bus->registers + DW_IC_SS_SCL_HCNT, bus->ss_hcnt);
	write32(bus->registers + DW_IC_SS_SCL_LCNT, bus->ss_lcnt);
	write32(bus->registers + DW_IC_FS_SCL_HCNT, bus->fs_hcnt);
	write32(bus->registers + DW_IC_FS_SCL_LCNT, bus->fs_lcnt);
	if (bus->hs_hcnt > 0)
		write32(bus->registers + DW_IC_HS_SCL_HCNT, bus->hs_hcnt);
	if (bus->hs_lcnt > 0)
		write32(bus->registers + DW_IC_HS_SCL_LCNT, bus->hs_lcnt);
	{
		uint32 reg = read32(bus->registers + DW_IC_COMP_VERSION);
		TRACE_ALWAYS("version: 0x%08" B_PRIx32 "\n",reg);
		//if (reg >= PCH_IC_COMP_VERSION_MIN)
			//write32(bus->registers + DW_IC_SDA_HOLD, bus->sda_hold_time);
	}

	{
		bus->tx_fifo_depth = 32;
		bus->rx_fifo_depth = 32;
		uint32 reg = read32(bus->registers + DW_IC_COMP_PARAM1);
		uint32 rx_fifo_depth = DW_IC_COMP_PARAM1_RX(reg);
		uint32 tx_fifo_depth = DW_IC_COMP_PARAM1_TX(reg);
		
		TRACE_ALWAYS("comp1: 0x%08" B_PRIx32 "\n",reg);
		TRACE_ALWAYS("rx fifo depth: 0x%04" B_PRIx16 "\n",rx_fifo_depth);
		TRACE_ALWAYS("tx fifo depth: 0x%04" B_PRIx16 "\n",tx_fifo_depth);
		
		if (rx_fifo_depth > 1 && rx_fifo_depth < bus->rx_fifo_depth)
			bus->rx_fifo_depth = rx_fifo_depth;
		if (tx_fifo_depth > 1 && tx_fifo_depth < bus->tx_fifo_depth)
			bus->tx_fifo_depth = tx_fifo_depth;
			
		write32(bus->registers + DW_IC_RX_TL, 0);
		write32(bus->registers + DW_IC_TX_TL, bus->tx_fifo_depth / 2);
	}

	bus->masterConfig = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
	    DW_IC_CON_RESTART_EN | DW_IC_CON_SPEED_FAST;
	write32(bus->registers + DW_IC_CON, bus->masterConfig);
	
	write32(bus->registers + DW_IC_INTR_MASK, 0);
	read32(bus->registers + DW_IC_CLR_INTR);
	
	status = install_io_interrupt_handler(bus->irq,
		(interrupt_handler)dw_i2c_interrupt_handler, bus, 0);
	if (status != B_OK) {
		ERROR("install interrupt handler failed\n");
		goto err;
	}

	mutex_init(&bus->lock, "dw_i2c");
	*bus_cookie = bus;
	
	bus_status = read32(bus->registers + DW_IC_STATUS);
	TRACE_ALWAYS("status %x\n",bus_status);
	
	return status;

err:
	if (bus->registersArea >= 0)
		delete_area(bus->registersArea);
	return status;
}


static void
uninit_bus(void* bus_cookie)
{
	dw_i2c_sim_info* bus = (dw_i2c_sim_info*)bus_cookie;

	mutex_destroy(&bus->lock);
	remove_io_interrupt_handler(bus->irq,
		(interrupt_handler)dw_i2c_interrupt_handler, bus);
	if (bus->registersArea >= 0)
		delete_area(bus->registersArea);

}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info**)&gACPI },
	{ I2C_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gI2c },
	{}
};


static i2c_sim_interface sDwI2cDeviceModule = {
	{
		{
			DW_I2C_SIM_MODULE_NAME,
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
	acquire_bus,
	release_bus,
};


module_info* modules[] = {
	(module_info* )&gDwI2cAcpiDevice,
	(module_info* )&sDwI2cDeviceModule,
	NULL
};
