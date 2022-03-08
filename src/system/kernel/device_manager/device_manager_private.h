/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2022, Jacob Secunda
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _DEVICE_MANAGER_PRIVATE_H
#define _DEVICE_MANAGER_PRIVATE_H


#include <util/DoublyLinkedList.h>
#include <util/Stack.h>

#include "io_resources.h"


namespace {


class Device : public AbstractModuleDevice, public DoublyLinkedListLinkImpl<Device> {
public:
							Device(device_node* node, const char* moduleName);
	virtual					~Device();

			status_t		InitCheck() const;

			const char*		ModuleName() const { return fModuleName; }

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

	virtual void			Removed();

			void			SetRemovedFromParent(bool removed) { fRemovedFromParent = removed; }

private:
	const char*				fModuleName;
	bool					fRemovedFromParent;
};


} // unnamed namespace

typedef DoublyLinkedList<Device> DeviceList;


struct device_attr_private : device_attr, DoublyLinkedListLinkImpl<device_attr_private> {
						device_attr_private();
						device_attr_private(const device_attr& attr);
						~device_attr_private();

			status_t	InitCheck();
			status_t	CopyFrom(const device_attr& attr);

	static	int			Compare(const device_attr* attrA, const device_attr* attrB);

private:
			void		_Unset();
};

typedef DoublyLinkedList<device_attr_private> AttributeList;


typedef DoublyLinkedList<device_node> NodeList;

struct device_node : DoublyLinkedListLinkImpl<device_node> {
									device_node(const char* moduleName, const device_attr* attrs);
									~device_node();

			status_t				InitCheck() const;

			status_t				AcquireResources(const io_resource* resources);

			const char*				ModuleName() const { return fModuleName; }
			device_node*			Parent() const { return fParent; }
			AttributeList&			Attributes() { return fAttributes; }
			const AttributeList&	Attributes() const { return fAttributes; }

			status_t				InitDriver();
			bool					UninitDriver();
			void					UninitUnusedDriver();

			// The following three are only valid if the node's driver is initialized
			driver_module_info*		DriverModule() const { return fDriver; }
			void*					DriverData() const { return fDriverData; }
			status_t				GetDriverPath(KPath* path) const;

			void					AddChild(device_node* node);
			void					RemoveChild(device_node* node);
			const NodeList&			Children() const { return fChildren; }
			void					DeviceRemoved();

			status_t				Register(device_node* parent);
			status_t				Probe(const char* devicePath, uint32 updateCycle);
			status_t				Reprobe();
			status_t				Rescan();

			bool					IsRegistered() const { return fRegistered; }
			bool					IsInitialized() const { return fInitialized > 0; }
			bool					IsProbed() const { return fLastUpdateCycle != 0; }
			uint32					Flags() const { return fFlags; }

			void					Acquire();
			bool					Release();

			const DeviceList&		Devices() const { return fDevices; }
			void					AddDevice(Device* device);
			void					RemoveDevice(Device* device);

			int						CompareTo(const device_attr* attributes) const;
			device_node*			FindChild(const device_attr* attributes) const;
			device_node*			FindChild(const char* moduleName) const;

			int32					Priority();

			void					Dump(int32 level = 0);

private:
			status_t				_RegisterFixed(uint32& registered);
			bool					_AlwaysRegisterDynamic();
			status_t				_AddPath(Stack<KPath*>& stack, const char* path,
									 	const char* subPath = NULL);
			status_t				_GetNextDriverPath(void*& cookie, KPath& _path);
			status_t				_GetNextDriver(void* list, driver_module_info*& driver);
			status_t				_FindBestDriver(const char* path,
										driver_module_info*& bestDriver, float& bestSupport,
										device_node* previous = NULL);
			status_t				_RegisterPath(const char* path);
			status_t				_RegisterDynamic(device_node* previous = NULL);
			status_t				_RemoveChildren();
			device_node*			_FindCurrentChild();
			status_t				_Probe();
			void					_ReleaseWaiting();

private:
			device_node*			fParent;
			NodeList				fChildren;
			int32					fRefCount;
			int32					fInitialized;
			bool					fRegistered;
			uint32					fFlags;
			float					fSupportsParent;
			uint32					fLastUpdateCycle;

			const char*				fModuleName;

			driver_module_info*		fDriver;
			void*					fDriverData;

			DeviceList				fDevices;
			AttributeList			fAttributes;
			ResourceList			fResources;
};


// flags in addition to those specified by B_DEVICE_FLAGS
enum node_flags {
	NODE_FLAG_REGISTER_INITIALIZED	= 0x00010000,
	NODE_FLAG_DEVICE_REMOVED		= 0x00020000,
	NODE_FLAG_OBSOLETE_DRIVER		= 0x00040000,
	NODE_FLAG_WAITING_FOR_DRIVER	= 0x00080000,

	NODE_FLAG_PUBLIC_MASK			= 0x0000ffff
};


#endif	// _DEVICE_MANAGER_PRIVATE_H
