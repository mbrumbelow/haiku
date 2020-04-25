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

#include <AutoDeleter.h>
#include <kernel.h>


struct videooutput_driver_cookie {
	device_node*				node;
	acpi_device_module_info*	acpi;
	acpi_device					acpi_cookie;

	uint32*						bcl;
	size_t						bcl_length;
};


struct videooutput_device_cookie {
	videooutput_driver_cookie*	driver_cookie;
};


// Table B-8 Notification Values for Output Devices, ACPI 6.3 Page 1130
#define ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_CYCLE		0x85
#define ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_UP			0x86
#define ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_DOWN			0x87
#define ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_ZERO			0x88
#define ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_OFF			0x89


#define ACPI_VIDEOOUTPUT_DRIVER_NAME "drivers/display/acpi_video/output/" \
	"driver_v1"
#define ACPI_VIDEOOUTPUT_DEVICE_NAME "drivers/display/acpi_video/output/" \
	"device_v1"

/* Base Namespace devices are published to */
#define ACPI_VIDEOOUTPUT_BASENAME "display/acpi_video/output/%d"

// name of pnp generator of path ids
#define ACPI_VIDEOOUTPUT_PATHID_GENERATOR "acpi_video/output/path_id"

#define TRACE_VIDEOOUTPUT
#ifdef TRACE_VIDEOOUTPUT
#	define TRACE(x...) dprintf("acpi_videooutput: " x)
#else
#	define TRACE(x...)
#endif
#define ERROR(x...) dprintf("acpi_videooutput: " x)


extern device_manager_info *gDeviceManager;
extern acpi_module_info *gAcpi;


static int
callerCompareInteger(const void* _a, const void* _b)
{
	const uint32* a = (const uint32*)_a;
	const uint32* b = (const uint32*)_b;
	return (int)*a - (int)*b;
}


static status_t
videooutput_get_brightness(videooutput_driver_cookie* device, uint32 *_level)
{
	status_t err;
	acpi_data response = { ACPI_ALLOCATE_BUFFER, NULL };
	err = device->acpi->evaluate_method(device->acpi_cookie, "_BQC", NULL,
		&response);
	if (err	!= B_OK) {
		ERROR("can't get brightness\n");
		return err;
	}
	acpi_object_type* object = (acpi_object_type*)response.pointer;
	if (object == NULL)
		return B_BAD_VALUE;
	uint64 level = 0;
	if (object->object_type == ACPI_TYPE_INTEGER)
		level = object->integer.integer;
	free(object);
	if (level > 100)
		return B_BAD_VALUE;
	*_level = level;
	TRACE("videooutput_get_brightness %" B_PRIu32 "\n", *_level);
	return B_OK;
}


static status_t
videooutput_set_brightness(videooutput_driver_cookie* device, uint32 _level)
{
	TRACE("videooutput_set_brightness %" B_PRIu32 "\n", _level);
	status_t err;
	acpi_object_type object;
	object.object_type = ACPI_TYPE_INTEGER;
	object.integer.integer = _level;
	acpi_objects objects = { 1, &object};
	err = device->acpi->evaluate_method(device->acpi_cookie, "_BCM", &objects,
		NULL);
	if (err	!= B_OK) {
		ERROR("can't set brightness\n");
	}
	return err;
}



void
videooutput_notify_handler(acpi_handle _device, uint32 value, void *context)
{
	TRACE("videooutput_notify_handler event 0x%" B_PRIx32 "\n", value);
	videooutput_driver_cookie* device = (videooutput_driver_cookie*)context;

	switch (value) {
		case ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_UP:
		case ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_DOWN:
			uint32 level;
			size_t step;
			if (videooutput_get_brightness(device, &level) != B_OK) {
				ERROR("videooutput_get_brightness failed\n");
				break;
			}
			step = 1;
			if (device->bcl_length > 20)
				step = device->bcl_length / 20;
			for (size_t i = 0; i < device->bcl_length; i++) {
				if (device->bcl[i] == level) {
					uint32 newLevel = level;
					if (value == ACPI_VIDEOOUTPUT_NOTIFY_BRIGHTNESS_DOWN) {
						if (i > step - 1)
							newLevel = device->bcl[i - step];
					} else if (i < device->bcl_length - step)
						newLevel = device->bcl[i + step];
					if (level != newLevel)
						videooutput_set_brightness(device, newLevel);
				}
			}
			break;
		default:
			ERROR("videooutput_notify_handler unknown event 0x%" B_PRIx32 "\n",
				value);
	}
}





//	#pragma mark - device module API


static status_t
acpi_videooutput_init_device(device_node *node, void **cookie)
{
	*cookie = node;
	return B_OK;
}


static void
acpi_videooutput_uninit_device(void *_cookie)
{

}


//	#pragma mark - driver module API


static float
acpi_videooutput_support(device_node *parent)
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

	// check _BCL
	acpi_handle handle;
	if (gAcpi->get_handle(NULL, path, &handle) != B_OK)
		return 0.0;

	acpi_handle method;
	if (gAcpi->get_handle(handle, "_BCL", &method) != B_OK)
		return 0.0;

	TRACE("found _BCL method! at %p\n", parent);

	// check parent attached acpi_video
	device_node *gfxParent = gDeviceManager->get_parent_node(parent);
	device_attr acpiAttrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Video" }},
		{ NULL }
	};
	device_node *gfxDriver = NULL;
	if (gfxParent == NULL
		|| gDeviceManager->find_child_node(gfxParent, acpiAttrs, &gfxDriver)
			!= B_OK) {
		TRACE("couldn't find gfxDriver at %p\n", gfxParent);
		kernel_debugger("couldn't find gfxDriver\n");
		return 0.0;
	}

	TRACE("ACPI Video Output device found!\n");

	return 0.6;
}


static status_t
acpi_videooutput_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Video Output" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, ACPI_VIDEOOUTPUT_DRIVER_NAME,
		attrs, NULL, NULL);
}


static status_t
acpi_videooutput_init_driver(device_node *node, void **driverCookie)
{
	status_t status;
	videooutput_driver_cookie *device;
	device = (videooutput_driver_cookie *)calloc(1,
		sizeof(videooutput_driver_cookie));
	if (device == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deviceDeleter(device);
	*driverCookie = device;

	device->node = node;

	device_node *parent;
	parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);

#ifdef TRACE_VIDEOOUTPUT
	const char* device_path;
	if (gDeviceManager->get_attr_string(parent, ACPI_DEVICE_PATH_ITEM,
		&device_path, false) == B_OK) {
		TRACE("acpi_videooutput_init_driver %s\n", device_path);
	}
#endif

	gDeviceManager->put_node(parent);

	acpi_data buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	status = device->acpi->evaluate_method(device->acpi_cookie, "_BCL", NULL,
		&buffer);
	if (status != B_OK) {
		ERROR("Method call _BCL failed\n");
		return B_ERROR;
	}

	acpi_object_type* object = (acpi_object_type*)buffer.pointer;
	MemoryDeleter _(object);
	if (object->object_type != ACPI_TYPE_PACKAGE
		|| object->package.count < 2) {
		ERROR("Method call _BCL failed\n");
		return B_ERROR;
	}
	device->bcl_length = object->package.count - 2;
	device->bcl = (uint32*)calloc(device->bcl_length, sizeof(uint32));
	if (device->bcl == NULL) {
		device->bcl_length = 0;
		return B_NO_MEMORY;
	}
	TRACE("acpi_videooutput_init_driver bcl_length %" B_PRIuSIZE " \n",
		device->bcl_length);

	for (size_t i = 0; i < device->bcl_length; i++) {
		acpi_object_type* pointer = &object->package.objects[i + 2];
		uint32 value = 0;
		if (pointer->object_type == ACPI_TYPE_INTEGER) {
			value = pointer->integer.integer;
		}
		device->bcl[i] = value;
	}
	qsort(device->bcl, device->bcl_length, sizeof(device->bcl[0]),
		callerCompareInteger);

	// install notify handler
	device->acpi->install_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, videooutput_notify_handler, device);

	deviceDeleter.Detach();
	return B_OK;
}


static void
acpi_videooutput_uninit_driver(void *driverCookie)
{
	TRACE("acpi_videooutput_uninit_driver\n");
	videooutput_driver_cookie *device =
		(videooutput_driver_cookie*)driverCookie;

	device->acpi->remove_notify_handler(device->acpi_cookie,
		ACPI_DEVICE_NOTIFY, videooutput_notify_handler);

	free(device);
}


static status_t
acpi_videooutput_register_child_devices(void *cookie)
{
	videooutput_driver_cookie *device = (videooutput_driver_cookie*)cookie;

	int pathID = gDeviceManager->create_id(ACPI_VIDEOOUTPUT_PATHID_GENERATOR);
	if (pathID < 0) {
		ERROR("register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_VIDEOOUTPUT_BASENAME, pathID);

	return gDeviceManager->publish_device(device->node, name,
		ACPI_VIDEOOUTPUT_DEVICE_NAME);
}


driver_module_info acpi_videooutput_driver_module = {
	{
		ACPI_VIDEOOUTPUT_DRIVER_NAME,
		0,
		NULL
	},

	acpi_videooutput_support,
	acpi_videooutput_register_device,
	acpi_videooutput_init_driver,
	acpi_videooutput_uninit_driver,
	acpi_videooutput_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


driver_module_info acpi_videooutput_device_module = {
	{
		ACPI_VIDEOOUTPUT_DEVICE_NAME,
		0,
		NULL
	},

	NULL,
	NULL,
	acpi_videooutput_init_device,
	acpi_videooutput_uninit_device,
	NULL,
	NULL,
	NULL
};

