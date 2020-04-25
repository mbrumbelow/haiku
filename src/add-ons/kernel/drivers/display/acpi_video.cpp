/*
 * Copyright 2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval <jerome.duval@gmail.com>
 */


#include <ACPI.h>
#include <Drivers.h>
#include <Errors.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel.h>


struct video_driver_cookie {
	device_node*				node;
	acpi_device_module_info*	acpi;
	acpi_device					acpi_cookie;
};


struct video_device_cookie {
	video_driver_cookie*		driver_cookie;
	int32						stop_watching;
};


// Section A.3.1 _DOS (Enable/Disable Output Switching), ACPI 6.3 Page 1118
#define ACPI_VIDEO_DOS_SWITCH_BY_OSPM		0
#define ACPI_VIDEO_DOS_SWITCH_BY_BIOS		1
#define ACPI_VIDEO_DOS_SWITCH_LOCKED		2
#define ACPI_VIDEO_DOS_SWITCH_BY_OSPM_EXT	3
#define ACPI_VIDEO_DOS_BRIGHTNESS_BY_OSPM	4


#define ACPI_VIDEO_DRIVER_NAME "drivers/display/acpi_video/driver_v1"
#define ACPI_VIDEO_DEVICE_NAME "drivers/display/acpi_video/device_v1"

/* Base Namespace devices are published to */
#define ACPI_VIDEO_BASENAME "display/acpi_video/%d"

// name of pnp generator of path ids
#define ACPI_VIDEO_PATHID_GENERATOR "acpi_video/path_id"

#define TRACE_VIDEO
#ifdef TRACE_VIDEO
#	define TRACE(x...) dprintf("acpi_video: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...) dprintf("acpi_video: " x)


device_manager_info *gDeviceManager;
acpi_module_info *gAcpi;
extern driver_module_info acpi_videooutput_device_module;
extern driver_module_info acpi_videooutput_driver_module;


void
video_notify_handler(acpi_handle device, uint32 value, void *context)
{
	TRACE("video_notify_handler event 0x%" B_PRIx32 "\n", value);
}


status_t
video_set_policy(video_driver_cookie *device, uint32 policy)
{
	acpi_object_type object;
	object.object_type = ACPI_TYPE_INTEGER;
	object.integer.integer = policy;
	acpi_objects objects = { 1, &object};
	status_t err = device->acpi->evaluate_method(device->acpi_cookie, "_DOS",
		&objects, NULL);
	if (err	!= B_OK)
		ERROR("can't set _DOS\n");
	return err;

}


//	#pragma mark - device module API


static status_t
acpi_video_init_device(device_node *node, void **cookie)
{
	*cookie = node;
	return B_OK;
}


static void
acpi_video_uninit_device(void *_cookie)
{

}


//	#pragma mark - driver module API


static float
acpi_video_support(device_node *parent)
{
	// make sure parent is really the ACPI bus manager
	const char *bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a device
	uint32 device_type;
	if (gDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM,
			&device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	const char *path;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM, &path,
			false) != B_OK)
		return 0.0;

	// check _DOS
	acpi_handle handle;
	if (gAcpi->get_handle(NULL, path, &handle) != B_OK)
		return 0.0;

	acpi_handle method;
	if (gAcpi->get_handle(handle, "_DOS", &method) != B_OK)
		return 0.0;

	TRACE("ACPI Video device found! at %p\n", parent);

	return 0.6;
}


static status_t
acpi_video_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Video" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, ACPI_VIDEO_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
acpi_video_init_driver(device_node *node, void **driverCookie)
{
	video_driver_cookie *device;
	device = (video_driver_cookie *)calloc(1, sizeof(video_driver_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	*driverCookie = device;

	device->node = node;

	device_node *parent;
	parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);

#ifdef TRACE_VIDEO
	const char* device_path;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM,
		&device_path, false) == B_OK) {
		TRACE("acpi_video_init_driver %s\n", device_path);
	}
#endif

	gDeviceManager->put_node(parent);

	// call _DOS
	video_set_policy(device, ACPI_VIDEO_DOS_SWITCH_BY_OSPM
		| ACPI_VIDEO_DOS_BRIGHTNESS_BY_OSPM);

	// install notify handler
	device->acpi->install_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, video_notify_handler, device);

	return B_OK;
}


static void
acpi_video_uninit_driver(void *driverCookie)
{
	TRACE("acpi_video_uninit_driver\n");
	video_driver_cookie *device = (video_driver_cookie*)driverCookie;

	device->acpi->remove_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, video_notify_handler);

	// call _DOS
	video_set_policy(device, ACPI_VIDEO_DOS_SWITCH_BY_BIOS);

	free(device);
}


static status_t
acpi_video_register_child_devices(void *cookie)
{
	video_driver_cookie *device = (video_driver_cookie*)cookie;

	int pathID = gDeviceManager->create_id(ACPI_VIDEO_PATHID_GENERATOR);
	if (pathID < 0) {
		ERROR("register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_VIDEO_BASENAME, pathID);

	return gDeviceManager->publish_device(device->node, name,
		ACPI_VIDEO_DEVICE_NAME);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info **)&gAcpi},
	{}
};


driver_module_info acpi_video_driver_module = {
	{
		ACPI_VIDEO_DRIVER_NAME,
		0,
		NULL
	},

	acpi_video_support,
	acpi_video_register_device,
	acpi_video_init_driver,
	acpi_video_uninit_driver,
	acpi_video_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


driver_module_info acpi_video_device_module = {
	{
		ACPI_VIDEO_DEVICE_NAME,
		0,
		NULL
	},

	NULL,
	NULL,
	acpi_video_init_device,
	acpi_video_uninit_device,
	NULL,
	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_video_driver_module,
	(module_info *)&acpi_video_device_module,
	(module_info *)&acpi_videooutput_driver_module,
	(module_info *)&acpi_videooutput_device_module,
	NULL
};
