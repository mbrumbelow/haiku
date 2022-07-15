/*
 * Copyright 2007-2010, Axel Dörfler. All rights reserved.
 * Copyright 2009-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device.h"
#include "sysinit.h"

#include <stdlib.h>
#include <sys/sockio.h>

#include <Drivers.h>
#include <ether_driver.h>

#include <compat/sys/haiku-module.h>

#include <compat/sys/bus.h>
#include <compat/sys/mbuf.h>
#include <compat/net/ethernet.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usb_device.h>


//#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


static struct {
	driver_t* driver;
	int bus;
	struct pci_info pci_info;
	struct freebsd_usb_device usb_dev;
	struct usb_attach_arg uaa;
} sProbedDevices[MAX_DEVICES];

const char* gDeviceNameList[MAX_DEVICES + 1];
struct ifnet* gDevices[MAX_DEVICES];
int32 gDeviceCount;


static status_t
init_root_device(device_t *_root, int bus_type)
{
	static driver_t sRootDriverPCI = {
		"pci",
		NULL,
		sizeof(struct root_device_softc),
	};
	static driver_t sRootDriverUSB = {
		"uhub",
		NULL,
		sizeof(struct root_device_softc),
	};

	device_t root = device_add_child(NULL, NULL, 0);
	if (root == NULL)
		return B_NO_MEMORY;

	root->softc = malloc(sizeof(struct root_device_softc));
	if (root->softc == NULL) {
		device_delete_child(NULL, root);
		return B_NO_MEMORY;
	}

	bzero(root->softc, sizeof(struct root_device_softc));

	if (bus_type == BUS_pci)
		root->driver = &sRootDriverPCI;
	else if (bus_type == BUS_uhub)
		root->driver = &sRootDriverUSB;
	else
		panic("unknown bus type");
	((struct root_device_softc*)root->softc)->bus = bus_type;

	root->root = root;

	if (_root != NULL)
		*_root = root;

	return B_OK;
}


static status_t
add_child_device(driver_t* driver, device_t root, device_t* _child)
{
	device_t child = device_add_child_driver(root, driver->name, driver, 0);
	if (child == NULL)
		return B_ERROR;

	if (_child != NULL)
		*_child = child;

	return B_OK;
}


static void
uninit_probed_devices()
{
	for (int p = 0; sProbedDevices[p].bus != BUS_INVALID; p++) {
		if (sProbedDevices[p].bus == BUS_pci) {
			gPci->unreserve_device(sProbedDevices[p].pci_info.bus,
				sProbedDevices[p].pci_info.device, sProbedDevices[p].pci_info.function,
				gDriverName, NULL);
		} else if (sProbedDevices->bus == BUS_uhub) {
			usb_cleanup_device(&sProbedDevices[p].usb_dev);
		}
	}
}


//	#pragma mark - Haiku Driver API


static status_t
init_hardware_pci(driver_t* drivers[])
{
	status_t status;
	int i = 0;
	pci_info* info;
	device_t root;

	int p = 0;
	while (sProbedDevices[p].bus != BUS_INVALID)
		p++;

	status = init_pci();
	if (status != B_OK)
		return status;

	status = init_root_device(&root, BUS_pci);
	if (status != B_OK)
		return status;

	bool found = false;
	for (info = get_device_pci_info(root); gPci->get_nth_pci_info(i, info) == B_OK; i++) {
		int best = 0;
		driver_t* driver = NULL;

		for (int index = 0; drivers[index] != NULL; index++) {
			int result;
			device_t device = NULL;
			status = add_child_device(drivers[index], root, &device);
			if (status < B_OK)
				break;

			result = device->methods.probe(device);
			if (result >= 0 && (driver == NULL || result > best)) {
				TRACE(("%s, found %s at %d (%d)\n", gDriverName,
					device_get_desc(device), i, result));
				driver = drivers[index];
				best = result;
			}
			device_delete_child(root, device);
		}

		if (driver == NULL)
			continue;

		// We've found a driver; now try to reserve the device and store it
		if (gPci->reserve_device(info->bus, info->device, info->function,
				gDriverName, NULL) != B_OK) {
			dprintf("%s: Failed to reserve PCI:%d:%d:%d\n",
				gDriverName, info->bus, info->device, info->function);
			continue;
		}
		sProbedDevices[p].bus = BUS_pci;
		sProbedDevices[p].driver = driver;
		sProbedDevices[p].pci_info = *info;
		found = true;
		p++;
	}
	sProbedDevices[p].bus = BUS_INVALID;

	device_delete_child(NULL, root);

	if (found)
		return B_OK;

	uninit_pci();
	return B_NOT_SUPPORTED;
}


static status_t
init_hardware_uhub(driver_t* drivers[])
{
	status_t status;
	device_t root;

	int p = 0;
	while (sProbedDevices[p].bus != BUS_INVALID)
		p++;

	status = init_usb();
	if (status != B_OK)
		return status;

	status = init_root_device(&root, BUS_uhub);
	if (status != B_OK)
		return status;

	bool found = false;
	uint32 cookie = 0;
	struct freebsd_usb_device udev = {};
	while ((status = get_next_usb_device(&cookie, &udev)) == B_OK) {
		int best = 0;
		driver_t* driver = NULL;

		struct usb_attach_arg uaa;
		status = get_usb_device_attach_arg(&udev, &uaa);
		if (status != B_OK)
			continue;

		for (int index = 0; drivers[index] != NULL; index++) {
			int result;
			device_t device = NULL;
			status = add_child_device(drivers[index], root, &device);
			if (status < B_OK)
				break;

			device_set_ivars(device, &uaa);

			result = device->methods.probe(device);
			if (result >= 0 && (driver == NULL || result > best)) {
				TRACE(("%s, found %s at %d (%d)\n", gDriverName,
					device_get_desc(device), i, result));
				driver = drivers[index];
				best = result;
			}
			device_delete_child(root, device);
		}

		if (driver == NULL)
			continue;

		sProbedDevices[p].bus = BUS_uhub;
		sProbedDevices[p].driver = driver;
		sProbedDevices[p].usb_dev = udev;
		sProbedDevices[p].uaa = uaa;
		sProbedDevices[p].uaa.device = &sProbedDevices[p].usb_dev;

		// We just "transferred ownership" of usb_dev to sProbedDevices.
		memset(&udev, 0, sizeof(udev));

		found = true;
		p++;
	}
	sProbedDevices[p].bus = BUS_INVALID;

	device_delete_child(NULL, root);
	usb_cleanup_device(&udev);

	if (found)
		return B_OK;

	uninit_usb();
	return B_NOT_SUPPORTED;
}


status_t
_fbsd_init_hardware(driver_t* pci_drivers[], driver_t* uhub_drivers[])
{
	sProbedDevices[0].bus = BUS_INVALID;

	if (pci_drivers != NULL)
		init_hardware_pci(pci_drivers);

	if (uhub_drivers != NULL)
		init_hardware_uhub(uhub_drivers);

	return (sProbedDevices[0].bus != BUS_INVALID) ? B_OK : B_NOT_SUPPORTED;
}


status_t
_fbsd_init_drivers()
{
	status_t status = init_mutexes();
	if (status < B_OK)
		goto err2;

	status = init_mbufs();
	if (status < B_OK)
		goto err3;

	status = init_callout();
	if (status < B_OK)
		goto err4;

	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES)) {
		status = init_taskqueues();
		if (status < B_OK)
			goto err5;
	}

	init_sysinit();

	status = init_wlan_stack();
	if (status < B_OK)
		goto err6;

	// Always hold the giant lock during attach.
	mtx_lock(&Giant);

	for (int p = 0; sProbedDevices[p].bus != BUS_INVALID; p++) {
		device_t root, device = NULL;
		status = init_root_device(&root, sProbedDevices[p].bus);
		if (status != B_OK)
			break;

		if (sProbedDevices[p].bus == BUS_pci) {
			pci_info* info = get_device_pci_info(root);
			*info = sProbedDevices[p].pci_info;
		} else if (sProbedDevices[p].bus == BUS_uhub) {
			struct root_device_softc* root_softc = (struct root_device_softc*)root->softc;
			root_softc->usb_dev = &sProbedDevices[p].usb_dev;
		}

		status = add_child_device(sProbedDevices[p].driver, root, &device);
		if (status != B_OK)
			break;

		if (sProbedDevices[p].bus == BUS_uhub)
			device_set_ivars(device, &sProbedDevices[p].uaa);

		// some drivers expect probe() to be called before attach()
		// (i.e. they set driver softc in probe(), etc.)
		if (device->methods.probe(device) >= 0
				&& device_attach(device) == 0) {
			dprintf("%s: init_driver(%p)\n", gDriverName,
				sProbedDevices[p].driver);
		} else
			device_delete_child(NULL, root);
	}

	mtx_unlock(&Giant);

	if (gDeviceCount > 0)
		return B_OK;

	if (status == B_OK)
		status = B_ERROR;

err7:
	uninit_wlan_stack();
err6:
	uninit_sysinit();
	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES))
		uninit_taskqueues();
err5:
	uninit_callout();
err4:
	uninit_mbufs();
err3:
	uninit_mutexes();
err2:
	uninit_probed_devices();

	uninit_usb();
	uninit_pci();

	return status;
}


status_t
_fbsd_uninit_drivers()
{
	for (int i = 0; i < gDeviceCount; i++)
		device_delete_child(NULL, gDevices[i]->root_device);

	uninit_wlan_stack();
	uninit_sysinit();
	if (HAIKU_DRIVER_REQUIRES(FBSD_TASKQUEUES))
		uninit_taskqueues();
	uninit_callout();
	uninit_mbufs();
	uninit_mutexes();

	uninit_probed_devices();

	uninit_usb();
	uninit_pci();

	return B_OK;
}
