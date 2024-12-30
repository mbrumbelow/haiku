/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2024, Enrique Medina Gremaldos, quique@necos.es.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ACPI.h>
#include <ByteOrder.h>
#include <condition_variable.h>

#include "dw_i2c.h"

typedef struct {
	const char *hid;
	uint32 flags;
} dw_device_info;

dw_device_info device_list[] = {
	{"INT33C2", 0},
	{"INT33C3", 0},
	{"INT3432", 0},
	{"INT3433", 0},
	{"INT3442", 0},
	{"INT3443", 0},
	{"INT3444", 0},
	{"INT3445", 0},
	{"INT3446", 0},
	{"INT3447", 0},
	{"80860AAC", 0},
	{"80865AAC", 0},
	{"80860F41", 0},
	{"808622C1", 0},
	{"AMD0010", 0},
	{"AMDI0010", 0},
	{"AMDI0510", 0},
	{"APMC0D0F", 0},
	{0, 0}
};

typedef struct {
	dw_i2c_sim_info info;
	acpi_device_module_info* acpi;
	acpi_device device;

} dw_i2c_acpi_sim_info;


static status_t
dw_i2c_acpi_set_powerstate(dw_i2c_acpi_sim_info* info, uint8 power)
{
	status_t status = info->acpi->evaluate_method(info->device,
		power == 1 ? "_PS0" : "_PS3", NULL, NULL);
	return status;
}



static acpi_status
dw_i2c_scan_parse_callback(ACPI_RESOURCE *res, void *context)
{
	struct dw_i2c_crs* crs = (struct dw_i2c_crs*)context;

	if (res->Type == ACPI_RESOURCE_TYPE_IRQ) {
		crs->irq = res->Data.Irq.Interrupts[0];
		crs->irq_triggering = res->Data.Irq.Triggering;
		crs->irq_polarity = res->Data.Irq.Polarity;
		crs->irq_shareable = res->Data.Irq.Shareable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_EXTENDED_IRQ) {
		crs->irq = res->Data.ExtendedIrq.Interrupts[0];
		crs->irq_triggering = res->Data.ExtendedIrq.Triggering;
		crs->irq_polarity = res->Data.ExtendedIrq.Polarity;
		crs->irq_shareable = res->Data.ExtendedIrq.Shareable;
	} else if (res->Type == ACPI_RESOURCE_TYPE_FIXED_MEMORY32) {
		crs->addr_bas = res->Data.FixedMemory32.Address;
		crs->addr_len = res->Data.FixedMemory32.AddressLength;
	}

	return B_OK;
}


//	#pragma mark -


static status_t
acpi_scan_bus(i2c_bus_cookie cookie)
{
	CALLED();
	dw_i2c_acpi_sim_info* bus = (dw_i2c_acpi_sim_info*)cookie;

	bus->acpi->walk_namespace(bus->device, ACPI_TYPE_DEVICE, 1,
		dw_i2c_scan_bus_callback, NULL, bus, NULL);

	return B_OK;
}


static status_t
register_child_devices(void* cookie)
{
	CALLED();

	dw_i2c_acpi_sim_info* bus = (dw_i2c_acpi_sim_info*)cookie;
	device_node* node = bus->info.driver_node;

	char prettyName[32];
	sprintf(prettyName, "DesignWare I2C Controller");

	device_attr attrs[] = {
		// properties of this controller for i2c bus manager
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = prettyName }},
		{ B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{ .string = I2C_FOR_CONTROLLER_MODULE_NAME }},

		// private data to identify the device
		{ NULL }
	};

	return gDeviceManager->register_node(node, DW_I2C_SIM_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	status_t status = B_OK;

	dw_i2c_acpi_sim_info* bus = (dw_i2c_acpi_sim_info*)calloc(1,
		sizeof(dw_i2c_acpi_sim_info));
	if (bus == NULL)
		return B_NO_MEMORY;

	acpi_device_module_info* acpi;
	acpi_device device;
	{
		device_node* acpiParent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(acpiParent, (driver_module_info**)&acpi,
			(void**)&device);
		gDeviceManager->put_node(acpiParent);
	}

	bus->acpi = acpi;
	bus->device = device;
	bus->info.driver_node = node;
	bus->info.scan_bus = acpi_scan_bus;

	// Attach devices for I2C resources
	struct dw_i2c_crs crs;
	status = acpi->walk_resources(device, (ACPI_STRING)"_CRS",
		dw_i2c_scan_parse_callback, &crs);
	if (status != B_OK) {
		ERROR("Error while getting I2C devices\n");
		free(bus);
		return status;
	}
	if (crs.addr_bas == 0 || crs.addr_len == 0) {
		TRACE("skipping non configured I2C devices\n");
		free(bus);
		return B_BAD_VALUE;
	}

	bus->info.base_addr = crs.addr_bas;
	bus->info.map_size = crs.addr_len;
	bus->info.irq = crs.irq;

	dw_i2c_acpi_set_powerstate(bus, 1);

	*device_cookie = bus;
	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	dw_i2c_acpi_sim_info* bus = (dw_i2c_acpi_sim_info*)device_cookie;
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "DesignWare I2C ACPI"}},
		{}
	};

	return gDeviceManager->register_node(parent,
		DW_I2C_ACPI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	CALLED();
	const char* bus;

	// make sure parent is a DesignWare I2C ACPI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK) {
		return -1;
	}

	if (strcmp(bus, "acpi") != 0)
		return 0.0f;

	TRACE("found an acpi node\n");

	// check whether it's really a device
	uint32 device_type;
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}
	TRACE("found an acpi device\n");

	// check whether it's a DesignWare I2C device
	const char *name;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &name,
		false) != B_OK) {
		return 0.0;
	}
	TRACE("found an acpi device hid %s\n", name);

	dw_device_info *device_info = device_list;

	while (device_info && device_info->hid) {
		if (strcmp(name, device_info->hid) == 0) {
			TRACE("DesignWare I2C device found! name %s\n", name);
			return 0.6f;
		}

		device_info++;
	}

	return 0.0f;
}


//	#pragma mark -


driver_module_info gDwI2cAcpiDevice = {
	{
		DW_I2C_ACPI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	uninit_device,
	register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};


