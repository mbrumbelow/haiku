/*
 * Copyright 2009, Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2009, Clemens Zeidler, haiku@clemens-zeidler.de
 * Copyright 2008-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Bryan Varner. All rights reserved.
 * Copyright 2005, Nathan Whitehorn. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ACPI.h>
#include <dpc.h>
#include <KernelExport.h>
#include <PCI.h>

#include <safemode.h>

extern "C" {
#include "uacpi/acpi.h"
#include "uacpi/event.h"
#include "uacpi/namespace.h"
#include "uacpi/notify.h"
#include "uacpi/opregion.h"
#include "uacpi/resources.h"
#include "uacpi/sleep.h"
#include "uacpi/status.h"
#include "uacpi/tables.h"
#include "uacpi/types.h"
#include "uacpi/uacpi.h"
#include "uacpi/utilities.h"

#include "uacpi/internal/registers.h"
}
#include "ACPIPrivate.h"

#include "arch_init.h"


//#define TRACE_ACPI_BUS
#ifdef TRACE_ACPI_BUS
#define TRACE(x...) dprintf("acpi: " x)
#else
#define TRACE(x...)
#endif

#define ERROR(x...) dprintf("acpi: " x)

#define ACPI_DEVICE_ID_LENGTH	0x08

extern dpc_module_info* gDPC;
void* gDPCHandle = NULL;


static bool
checkAndLogFailure(const uacpi_status status, const char* msg)
{
	bool failure = uacpi_unlikely_error(status);
	if (failure)
		dprintf("acpi: %s %s\n", msg, uacpi_status_to_string(status));

	return failure;
}


struct get_device_by_hid_context {
	uint32 search_index;
	uint32 current_index;
	const char* return_value;
};

static uacpi_iteration_decision
get_device_by_hid_callback(void* context, uacpi_namespace_node* object, uint32_t depth)
{
	auto counter = (get_device_by_hid_context*)context;

	TRACE("get_device_by_hid_callback %p, %d, %p\n", object, depth, context);

	counter->return_value = NULL;

	if (counter->search_index == counter->current_index) {
		counter->return_value = uacpi_namespace_node_generate_absolute_path(object);
		return UACPI_ITERATION_DECISION_BREAK;
	}

	counter->current_index++;
	return UACPI_ITERATION_DECISION_CONTINUE;
}


#ifdef ACPI_DEBUG_OUTPUT


static void
globalGPEHandler(UINT32 eventType, ACPI_HANDLE device, UINT32 eventNumber,
	void* context)
{
	ACPI_BUFFER path;
	char deviceName[256];
	path.Length = sizeof(deviceName);
	path.Pointer = deviceName;

	uacpi_status status = AcpiNsHandleToPathname(device, &path);
	if (ACPI_FAILURE(status))
		strcpy(deviceName, "(missing)");

	switch (eventType) {
		case ACPI_EVENT_TYPE_GPE:
			dprintf("acpi: GPE Event %d for %s\n", eventNumber, deviceName);
			break;

		case ACPI_EVENT_TYPE_FIXED:
		{
			switch (eventNumber) {
				case ACPI_EVENT_PMTIMER:
					dprintf("acpi: PMTIMER(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_GLOBAL:
					dprintf("acpi: Global(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_POWER_BUTTON:
					dprintf("acpi: Powerbutton(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_SLEEP_BUTTON:
					dprintf("acpi: sleepbutton(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				case ACPI_EVENT_RTC:
					dprintf("acpi: RTC(%d) event for %s\n", eventNumber,
						deviceName);
					break;

				default:
					dprintf("acpi: unknown fixed(%d) event for %s\n",
						eventNumber, deviceName);
			}
			break;
		}

		default:
			dprintf("acpi: unknown event type (%d:%d)  event for %s\n",
				eventType, eventNumber, deviceName);
	}
}


static void globalNotifyHandler(ACPI_HANDLE device, UINT32 value, void* context)
{
	ACPI_BUFFER path;
	char deviceName[256];
	path.Length = sizeof(deviceName);
	path.Pointer = deviceName;

	uacpi_status status = AcpiNsHandleToPathname(device, &path);
	if (ACPI_FAILURE(status))
		strcpy(deviceName, "(missing)");

	dprintf("acpi: Notify event %d for %s\n", value, deviceName);
}


#endif


//	#pragma mark - ACPI bus manager API


static status_t
acpi_std_ops(int32 op,...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			void *settings;
			bool acpiDisabled = false;

			settings = load_driver_settings("kernel");
			if (settings != NULL) {
				acpiDisabled = !get_driver_boolean_parameter(settings, "acpi",
					true, true);
				unload_driver_settings(settings);
			}

			if (!acpiDisabled) {
				// check if safemode settings disable ACPI
				settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
				if (settings != NULL) {
					acpiDisabled = get_driver_boolean_parameter(settings,
						B_SAFEMODE_DISABLE_ACPI, false, false);
					unload_driver_settings(settings);
				}
			}

			if (acpiDisabled) {
				ERROR("ACPI disabled\n");
				return ENOSYS;
			}

			if (gDPC->new_dpc_queue(&gDPCHandle, "acpi_task",
					B_URGENT_DISPLAY_PRIORITY + 1) != B_OK) {
				ERROR("failed to create os execution queue\n");
				return B_ERROR;
			}

#ifdef ACPI_DEBUG_OUTPUT
			AcpiDbgLevel = ACPI_DEBUG_ALL | ACPI_LV_VERBOSE;
			AcpiDbgLayer = ACPI_ALL_COMPONENTS;
#endif

			if (checkAndLogFailure(uacpi_initialize(0),
					"uacpi_initialize failed"))
				goto err_dpc;

			/* Install the default address space handlers. */

			arch_init_interrupt_controller();

			if (checkAndLogFailure(uacpi_namespace_load(),
					"uacpi_namespace_load failed"))
				goto err_acpi;

			if (checkAndLogFailure(uacpi_namespace_initialize(),
					"uacpi_namespace_iniialize failed"))
				goto err_acpi;

			//TODO: Walk namespace init ALL _PRW's

#ifdef ACPI_DEBUG_OUTPUT
			checkAndLogFailure(
				AcpiInstallGlobalEventHandler(globalGPEHandler, NULL),
				"Failed to install global GPE-handler.");

			checkAndLogFailure(AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT,
					ACPI_ALL_NOTIFY, globalNotifyHandler, NULL),
				"Failed to install global Notify-handler.");
#endif
			checkAndLogFailure(uacpi_finalize_gpe_initialization(),
				"Failed to enable all runtime Gpes");

			TRACE("ACPI initialized\n");
			return B_OK;

		err_acpi:
			uacpi_state_reset();

		err_dpc:
			gDPC->delete_dpc_queue(gDPCHandle);
			gDPCHandle = NULL;

			return B_ERROR;
		}

		case B_MODULE_UNINIT:
		{
			uacpi_state_reset();

			gDPC->delete_dpc_queue(gDPCHandle);
			gDPCHandle = NULL;
			break;
		}

		default:
			return B_ERROR;
	}
	return B_OK;
}


status_t
get_handle(uacpi_namespace_node* parent, const char *pathname, uacpi_namespace_node *retHandle)
{
	return uacpi_namespace_node_find(parent, pathname, &retHandle) == UACPI_STATUS_OK
		? B_OK : B_ERROR;
}


status_t
get_name(uacpi_namespace_node* handle, uint32 nameType, char* returnedName,
	size_t bufferLength)
{
	if (nameType == 1) {
		// This was ACPI_SINGLE_NAME
		uacpi_object_name name = uacpi_namespace_node_name(handle);
		strlcpy(returnedName, name.text, bufferLength);
	} else {
		// ACPI_FULL_PATHNAME or ACPI_FULL_PATHNAME_NO_TRAILING
		// TODO unused in Haiku as far as I can see, maybe remove nameType from the parameters?
		const char* buffer = uacpi_namespace_node_generate_absolute_path(handle);
		if (buffer == NULL)
			return B_ERROR;
		strlcpy(returnedName, buffer, bufferLength);
		uacpi_free_absolute_path(buffer);
	}
	return B_OK;
}


status_t
acquire_global_lock(uint16 timeout, uint32 *handle)
{
	return uacpi_acquire_global_lock(timeout, handle) == UACPI_STATUS_OK
		? B_OK : B_ERROR;
}


status_t
release_global_lock(uint32 handle)
{
	return uacpi_release_global_lock(handle) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
install_notify_handler(uacpi_namespace_node* device, uint32 handlerType,
	uacpi_notify_handler handler, void *context)
{
	// TODO uacpi API has no "handlerType". All handlers are of type ACPI_ALL_NOTIFY, meaning
	// they will get called for all events (no matter if it's < 0x80 or >= 0x80).
	// ACPICA allows separate "system" and "device" event handlers, but some buggy firmwares will
	// trigger the wrong event type for some things anyway, and so all drivers end up subsribing to
	// ACPI_ALL_NOTIFY "just in case".
	// Either document the handlerType as unused, or remove it completely.
	return uacpi_install_notify_handler(device, handler, context)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
remove_notify_handler(uacpi_namespace_node* device, uint32 handlerType,
	uacpi_notify_handler handler)
{
	// TODO uacpi API has no "handlerType". Handlers are always for all events.
	// Remove the handlerType parameter or document it as unused.
	return uacpi_uninstall_notify_handler(device, handler)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
update_all_gpes()
{
	return uacpi_finalize_gpe_initialization() == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
enable_gpe(uacpi_namespace_node* handle, uint32 gpeNumber)
{
	return uacpi_enable_gpe(handle, gpeNumber) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
disable_gpe(uacpi_namespace_node* handle, uint32 gpeNumber)
{
	return uacpi_disable_gpe(handle, gpeNumber) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
clear_gpe(uacpi_namespace_node* handle, uint32 gpeNumber)
{
	return uacpi_clear_gpe(handle, gpeNumber) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
set_gpe(uacpi_namespace_node* handle, uint32 gpeNumber, uint8 action)
{
	uacpi_status status;
	if (action == 2 /* formerly GPE_STATE_DISABLED */)
		status = uacpi_suspend_gpe(handle, gpeNumber);
	else
		status = uacpi_resume_gpe(handle, gpeNumber);
	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
finish_gpe(uacpi_namespace_node* handle, uint32 gpeNumber)
{
	return uacpi_finish_handling_gpe(handle, gpeNumber) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
install_gpe_handler(uacpi_namespace_node* handle, uint32 gpeNumber, uacpi_gpe_triggering type,
	uacpi_gpe_handler handler, void *data)
{
	return uacpi_install_gpe_handler(handle, gpeNumber, type,
		handler, data) == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
remove_gpe_handler(uacpi_namespace_node* handle, uint32 gpeNumber,
	uacpi_gpe_handler address)
{
	return uacpi_uninstall_gpe_handler(handle, gpeNumber, address)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
install_address_space_handler(uacpi_namespace_node* handle, uacpi_address_space spaceId,
	uacpi_region_handler handler, void *data)
{
	return uacpi_install_address_space_handler(handle, spaceId, handler, data)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
remove_address_space_handler(uacpi_namespace_node* handle, uacpi_address_space spaceId,
	uacpi_region_handler handler)
{
	// TODO passing the handler back is not needed (ACPICA used it to sanity check that the handler
	// was indeed the installed one). Remove it from the parameters or mark it as undocumented.
	return uacpi_uninstall_address_space_handler(handle, spaceId)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


void
enable_fixed_event(uacpi_fixed_event event)
{
	uacpi_enable_fixed_event(event);
}


void
disable_fixed_event(uacpi_fixed_event event)
{
	uacpi_disable_fixed_event(event);
}


uint32
fixed_event_status(uacpi_fixed_event event)
{
	uacpi_event_info status;
	if (uacpi_fixed_event_info(event, &status) != UACPI_STATUS_OK)
		return (uacpi_event_info)0;
	return status/* & ACPI_EVENT_FLAG_SET*/;
}


void
reset_fixed_event(uacpi_fixed_event event)
{
	uacpi_clear_fixed_event(event);
}


status_t
install_fixed_event_handler(uacpi_fixed_event event, acpi_event_handler handler, void *data)
{
	return uacpi_install_fixed_event_handler(event, handler, data) == UACPI_STATUS_OK
		? B_OK : B_ERROR;
}


status_t
remove_fixed_event_handler(uacpi_fixed_event event, acpi_event_handler handler)
{
	// TODO handler is not neded (single handler per event)
	return uacpi_uninstall_fixed_event_handler(event) == UACPI_STATUS_OK
		? B_OK : B_ERROR;
}


#if 0
status_t
get_next_entry(uint32 objectType, const char *base, char *result,
	size_t length, void **counter)
{
	uacpi_namespace_node* parent, *child, *newChild;
	uacpi_status status;

	TRACE("get_next_entry %ld, %s\n", objectType, base);

	if (base == NULL || !strcmp(base, "\\")) {
		parent = uacpi_namespace_root();
	} else {
		status = uacpi_namespace_node_find(NULL, base, &parent);
		if (status != UACPI_STATUS_OK)
			return B_ENTRY_NOT_FOUND;
	}

	child = (uacpi_namespace_node*)*counter;

	status = AcpiGetNextObject(objectType, parent, child, &newChild);
	if (status != UACPI_STATUS_OK)
		return B_ENTRY_NOT_FOUND;

	*counter = newChild;

	const char* path = uacpi_namespace_node_generate_absolute_path(newChild);
	if (status != UACPI_STATUS_OK)
		return B_NO_MEMORY; /* Corresponds to AE_BUFFER_OVERFLOW */
	strlcpy(result, path, length);
	uacpi_free_absolute_path(path);

	return B_OK;
}


status_t
get_next_object(uint32 objectType, acpi_handle parent,
	acpi_handle* currentChild)
{
	acpi_handle child = *currentChild;
	return AcpiGetNextObject(objectType, parent, child, currentChild) == UACPI_STATUS_OK
		? B_OK : B_ERROR;
}
#endif


status_t
get_device(const char* hid, uint32 index, char* result, size_t resultLength)
{
	uacpi_status status;
	const uacpi_char* hids[] = { hid, nullptr };
	get_device_by_hid_context counter = {index, 0};

	TRACE("get_device %s, index %ld\n", hid, index);
	status = uacpi_find_devices_at(NULL, hids, get_device_by_hid_callback, &counter);
	if (status != UACPI_STATUS_OK || counter.return_value == NULL)
		return B_ENTRY_NOT_FOUND;

	strlcpy(result, counter.return_value, resultLength);
	uacpi_free_absolute_path(counter.return_value);
	return B_OK;
}


status_t
get_device_info(const char *path, char** hid, char** cidList,
	size_t cidListCount, char** uid, char** cls)
{
	uacpi_namespace_node* handle;
	uacpi_namespace_node_info *info;

	TRACE("get_device_info: path %s\n", path);
	if (uacpi_namespace_node_find(NULL, path, &handle) != UACPI_STATUS_OK)
		return B_ENTRY_NOT_FOUND;

	if (uacpi_get_namespace_node_info(handle, &info) != UACPI_STATUS_OK)
		return B_BAD_TYPE;

	if ((info->flags & UACPI_NS_NODE_INFO_HAS_HID) != 0 && hid != NULL)
		*hid = strndup(info->hid.value, info->hid.size);

	if ((info->flags & UACPI_NS_NODE_INFO_HAS_CID) != 0 && cidList != NULL) {
		if (cidListCount > info->cid.num_ids)
			cidListCount = info->cid.num_ids;
		for (size_t i = 0; i < cidListCount; i++) {
			cidList[i] = strndup(info->cid.ids[i].value, info->cid.ids[i].size);
		}
	}

	if ((info->flags & UACPI_NS_NODE_INFO_HAS_UID) != 0 && uid != NULL)
		*uid = strndup(info->uid.value, info->uid.size);

	if ((info->flags & UACPI_NS_NODE_INFO_HAS_CLS) != 0 && cls != NULL
		&& info->cls.size >= 0) {
		*cls = strndup(info->cls.value, info->cls.size);
	}

	uacpi_free_namespace_node_info(info);
	return B_OK;
}


status_t
get_device_addr(const char *path, uint32 *addr)
{
	uacpi_namespace_node* handle;

	TRACE("get_device_adr: path %s, hid %s\n", path, hid);
	if (uacpi_namespace_node_find(NULL, path, &handle) != UACPI_STATUS_OK)
		return B_ENTRY_NOT_FOUND;

	status_t status = B_BAD_VALUE;
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);
	if (addr != NULL
		&& evaluate_method(handle, "_ADR", NULL, &buf) == B_OK
		&& object.object_type == ACPI_TYPE_INTEGER) {
		status = B_OK;
		*addr = object.integer.integer;
	}
	return status;
}


uint32
get_object_type(const char* path)
{
	uacpi_namespace_node* handle;
	uacpi_object_type type;

	if (uacpi_namespace_node_find(NULL, path, &handle) != UACPI_STATUS_OK)
		return B_ENTRY_NOT_FOUND;

	uacpi_namespace_node_type(handle, &type);
	return type;
}


status_t
get_object(const char* path, uacpi_object** _returnValue)
{
	uacpi_status status;

	status = uacpi_eval_simple(NULL, path, _returnValue);

	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
get_object_typed(const char* path, uacpi_object** _returnValue,
	uacpi_object_type_bits objectType)
{
	uacpi_status status;

	status = uacpi_eval_simple_typed(NULL, path, objectType, _returnValue);

	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
ns_handle_to_pathname(uacpi_namespace_node* targetHandle, const uacpi_char** buffer)
{
	*buffer = uacpi_namespace_node_generate_absolute_path(targetHandle);
	return (*buffer != NULL) ? B_OK : B_ERROR;
}


status_t
evaluate_object(uacpi_namespace_node* handle, const char* object, uacpi_object_array *args,
	uacpi_object* returnValue, size_t bufferLength)
{
	uacpi_status status;

	status = uacpi_eval(handle, object, args, &returnValue);
	if (uacpi_unlikely_error(status))
		dprintf("evaluate_object: %s\n", uacpi_status_to_string(status));

	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
evaluate_method(uacpi_namespace_node* handle, const char* method,
	uacpi_object_array *args, uacpi_object *returnValue)
{
	uacpi_status status;

	status = uacpi_eval(handle, method, args, &returnValue);
	if (uacpi_unlikely_error(status))
		dprintf("evaluate_method: %s\n", uacpi_status_to_string(status));

	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
get_irq_routing_table(uacpi_namespace_node* busDeviceHandle, uacpi_pci_routing_table **retBuffer)
{
	uacpi_status status;

	status = uacpi_get_pci_routing_table(busDeviceHandle, retBuffer);
	if (uacpi_unlikely_error(status))
		dprintf("get_irq_routing_table: %s\n", uacpi_status_to_string(status));

	return status == UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
get_current_resources(uacpi_namespace_node* busDeviceHandle, uacpi_resources *retBuffer)
{
	return uacpi_get_current_resources(busDeviceHandle, &retBuffer)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
get_possible_resources(uacpi_namespace_node* busDeviceHandle, uacpi_resources *retBuffer)
{
	return uacpi_get_possible_resources(busDeviceHandle, &retBuffer)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
set_current_resources(uacpi_namespace_node* busDeviceHandle, uacpi_resources *buffer)
{
	return uacpi_set_resources(busDeviceHandle, buffer)
		== UACPI_STATUS_OK ? B_OK : B_ERROR;
}


status_t
walk_resources(uacpi_namespace_node* busDeviceHandle, const char* method,
	uacpi_resource_iteration_callback callback, void* context)
{
	return uacpi_for_each_device_resource(busDeviceHandle, method,
		callback, context);
}


status_t
walk_namespace(uacpi_namespace_node* busDeviceHandle, uacpi_object_type_bits objectType,
	uint32 maxDepth, uacpi_iteration_callback descendingCallback,
	uacpi_iteration_callback ascendingCallback, void* context)
{
	return uacpi_namespace_for_each_child(busDeviceHandle,
		descendingCallback,
		ascendingCallback, objectType, maxDepth, context);
}


status_t
prepare_sleep_state(uacpi_sleep_state state, void (*wakeFunc)(void), size_t size)
{
	uacpi_status acpiStatus;

	TRACE("prepare_sleep_state %d, %p, %ld\n", state, wakeFunc, size);

	if (state != UACPI_SLEEP_STATE_S5) {
		physical_entry wakeVector;
		status_t status;

		// Note: The supplied code must already be locked into memory.
		status = get_memory_map((const void*)wakeFunc, size, &wakeVector, 1);
		if (status != B_OK)
			return status;

#	if B_HAIKU_PHYSICAL_BITS > 32
		if (wakeVector.address >= 0x100000000LL) {
			ERROR("prepare_sleep_state(): ACPI 2.0c says use 32 bit "
				"vector, but we have a physical address >= 4 GB\n");
		}
#	endif
		acpiStatus = uacpi_set_waking_vector(wakeVector.address,
			wakeVector.address);
		if (acpiStatus != UACPI_STATUS_OK)
			return B_ERROR;
	}

	acpiStatus = uacpi_prepare_for_sleep_state(state);
	if (acpiStatus != UACPI_STATUS_OK)
		return B_ERROR;

	return B_OK;
}


status_t
enter_sleep_state(uacpi_sleep_state state)
{
	uacpi_status status;

	TRACE("enter_sleep_state %d\n", state);

	cpu_status cpu = disable_interrupts();
	status = uacpi_enter_sleep_state(state);
	restore_interrupts(cpu);
	panic("AcpiEnterSleepState should not return.");
	if (status != UACPI_STATUS_OK)
		return B_ERROR;

	/*status = AcpiLeaveSleepState(state);
	if (status != UACPI_STATUS_OK)
		return B_ERROR;*/

	return B_OK;
}


status_t
reboot(void)
{
	uacpi_status status;

	TRACE("reboot\n");

	status = uacpi_reboot();
	if (status == UACPI_STATUS_NOT_FOUND)
		return B_UNSUPPORTED;

	if (status != UACPI_STATUS_OK) {
		ERROR("Reset failed, status = %d\n", status);
		return B_ERROR;
	}

	snooze(1000000);
	ERROR("Reset failed, timeout\n");
	return B_ERROR;
}


status_t
get_table(const char* signature, uint32 instance, uacpi_table* tableHeader)
{
	uacpi_status status = uacpi_table_find_by_signature((char*)signature, tableHeader);
	if (uacpi_unlikely_error(status))
		return B_ERROR;

	for (uint32 i = 1; i < instance; i++) {
		status = uacpi_table_find_next_with_same_signature(tableHeader);
		if (uacpi_unlikely_error(status))
			return B_ERROR;
	}

	return B_OK;
}


status_t
read_bit_register(int regid, uint64 *val)
{
	return uacpi_read_register((uacpi_register)regid, val);
}


status_t
write_bit_register(int regid, uint64 val)
{
	return uacpi_write_register((uacpi_register)regid, val);
}


struct acpi_module_info gACPIModule = {
	{
		B_ACPI_MODULE_NAME,
		B_KEEP_LOADED,
		acpi_std_ops
	},

	get_handle,
	get_name,
	acquire_global_lock,
	release_global_lock,
	install_notify_handler,
	remove_notify_handler,
	update_all_gpes,
	enable_gpe,
	disable_gpe,
	clear_gpe,
	set_gpe,
	finish_gpe,
	install_gpe_handler,
	remove_gpe_handler,
	install_address_space_handler,
	remove_address_space_handler,
	enable_fixed_event,
	disable_fixed_event,
	fixed_event_status,
	reset_fixed_event,
	install_fixed_event_handler,
	remove_fixed_event_handler,
#if 0
	get_next_entry,
	get_next_object,
#endif
	walk_namespace,
	get_device,
	get_device_info,
	get_object_type,
	get_object,
	get_object_typed,
	ns_handle_to_pathname,
	evaluate_object,
	evaluate_method,
	get_irq_routing_table,
	get_current_resources,
	get_possible_resources,
	set_current_resources,
	walk_resources,
	prepare_sleep_state,
	enter_sleep_state,
	reboot,
	get_table,
	read_bit_register,
	write_bit_register
};
