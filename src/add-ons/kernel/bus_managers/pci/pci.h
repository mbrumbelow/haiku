/*
 * Copyright 2003-2008, Marcus Overhagen. All rights reserved.
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_H__
#define __PCI_H__

#include <PCI.h>

#include <VectorMap.h>

#include "pci_controller.h"
#include "pci_msi.h"


#define TRACE_PCI
#ifndef TRACE_PCI
#	define TRACE(x)
#else
#	define TRACE(x) dprintf x
#endif

struct PCIDev;

struct PCIBus {
	PCIBus *			next;
	PCIDev *			parent;
	PCIDev *			child;
	uint8				domain;
	uint8				bus;
};

struct PCIDev {
	PCIDev *			next;
	PCIBus *			parent;
	PCIBus *			child;
	uint8				domain;
	uint8				bus;
	uint8				device;
	uint8				function;
	pci_info			info;

	msi_info			msi;
	msix_info			msix;
	ht_mapping_info		ht_mapping;
};


struct domain_data {
	// These two are set in PCI::AddController:
	pci_controller_module_info *controller;
	void *				controller_cookie;
	device_node *		root_node;

	// All the rest is set in PCI::InitDomainData
	int					max_bus_devices;
	pci_resource_range 	ranges[kPciRangeEnd];

#if !(defined(__i386__) || defined(__x86_64__))
	area_id				io_port_area;
	uint8 *				io_port_adr;
#endif
};


class PCI {
public:
							PCI();
							~PCI();

			void			InitDomainData();
			void			InitBus();
			status_t	Finalize();

			status_t		AddController(pci_controller_module_info *controller,
								void *controller_cookie, device_node *root_node);

			status_t		LookupRange(uint32 type, phys_addr_t pciAddr,
								uint8 &domain, pci_resource_range &range, uint8 **mappedAdr = NULL);

			status_t		GetNthInfo(long index, pci_info *outInfo);

			status_t		ReadConfig(uint8 domain, uint8 bus, uint8 device,
								uint8 function, uint16 offset, uint8 size,
								uint32 *value);
			uint32			ReadConfig(uint8 domain, uint8 bus, uint8 device,
								uint8 function, uint16 offset, uint8 size);
			uint32			ReadConfig(PCIDev *device, uint16 offset,
								uint8 size);

			status_t		WriteConfig(uint8 domain, uint8 bus, uint8 device,
								uint8 function, uint16 offset, uint8 size,
								uint32 value);
			status_t		WriteConfig(PCIDev *device, uint16 offset,
								uint8 size, uint32 value);

			status_t		FindCapability(uint8 domain, uint8 bus,
								uint8 device, uint8 function, uint8 capID,
								uint8 *offset = NULL);
			status_t		FindCapability(PCIDev *device, uint8 capID,
								uint8 *offset = NULL);
			status_t		FindExtendedCapability(uint8 domain, uint8 bus,
								uint8 device, uint8 function, uint16 capID,
								uint16 *offset = NULL);
			status_t		FindExtendedCapability(PCIDev *device,
								uint16 capID, uint16 *offset = NULL);
			status_t		FindHTCapability(uint8 domain, uint8 bus,
								uint8 device, uint8 function, uint16 capID,
								uint8 *offset);
			status_t		FindHTCapability(PCIDev *device,
								uint16 capID, uint8 *offset = NULL);

			status_t		ResolveVirtualBus(uint8 virtualBus, uint8 *domain,
								uint8 *bus);

			PCIDev *		FindDevice(uint8 domain, uint8 bus, uint8 device,
								uint8 function);

			void			ClearDeviceStatus(PCIBus *bus, bool dumpStatus);

			uint8			GetPowerstate(PCIDev *device);
			status_t		GetPowerstate(uint8 domain, uint8 bus, uint8 device,
								uint8 function, uint8* state);
			void			SetPowerstate(PCIDev *device, uint8 state);
			status_t		SetPowerstate(uint8 domain, uint8 bus, uint8 device,
								uint8 function, uint8 newState);

			void			RefreshDeviceInfo();

			status_t		UpdateInterruptLine(uint8 domain, uint8 bus,
								uint8 device, uint8 function,
								uint8 newInterruptLineValue);

			uint8			GetMSICount(PCIDev *device);
			status_t		ConfigureMSI(PCIDev *device, uint8 count, uint8 *startVector);
			status_t		UnconfigureMSI(PCIDev *device);
			status_t		EnableMSI(PCIDev *device);
			status_t		DisableMSI(PCIDev *device);
			uint8			GetMSIXCount(PCIDev *device);
			status_t		ConfigureMSIX(PCIDev *device, uint8 count, uint8 *startVector);
			status_t		EnableMSIX(PCIDev *device);

private:
			void			_EnumerateBus(uint8 domain, uint8 bus,
								uint8 *subordinateBus = NULL);

			void			_FixupDevices(uint8 domain, uint8 bus);

			void			_DiscoverBus(PCIBus *bus);
			void			_DiscoverDevice(PCIBus *bus, uint8 dev,
								uint8 function);

			PCIDev *		_CreateDevice(PCIBus *parent, uint8 dev,
								uint8 function);
			PCIBus *		_CreateBus(PCIDev *parent, uint8 domain,
								uint8 bus);

			status_t		_GetNthInfo(PCIBus *bus, long *currentIndex,
								long wantIndex, pci_info *outInfo);
			void			_ReadBasicInfo(PCIDev *dev);
			void			_ReadHeaderInfo(PCIDev *dev);

			void			_ConfigureBridges(PCIBus *bus);
			void			_RefreshDeviceInfo(PCIBus *bus);

			uint64			_BarSize(uint64 bits);
			size_t			_GetBarInfo(PCIDev *dev, uint8 offset,
								uint32 &ramAddress, uint32 &pciAddress,
								uint32 &size, uint8 &flags,
								uint32 *highRAMAddress = NULL,
								uint32 *highPCIAddress = NULL,
								uint32 *highSize = NULL);
			void			_GetRomBarInfo(PCIDev *dev, uint8 offset,
								uint32 &address, uint32 *size = NULL,
								uint8 *flags = NULL);

public:
			domain_data *	_GetDomainData(uint8 domain);

private:
			status_t		_CreateVirtualBus(uint8 domain, uint8 bus,
								uint8 *virtualBus);

			int				_NumFunctions(uint8 domain, uint8 bus,
								uint8 device);
			PCIDev *		_FindDevice(PCIBus *current, uint8 domain,
								uint8 bus, uint8 device, uint8 function);

			void			_HtMSIMap(PCIDev *device, uint64 address);
			void			_ReadMSIInfo(PCIDev *device);
			void			_ReadMSIXInfo(PCIDev *device);
			void			_ReadHtMappingInfo(PCIDev *device);
			status_t		_UnconfigureMSIX(PCIDev *device);
			status_t		_DisableMSIX(PCIDev *device);

private:
	PCIBus *				fRootBus;

	enum { MAX_PCI_DOMAINS = 8 };

	domain_data				fDomainData[MAX_PCI_DOMAINS];
	uint8					fDomainCount;
	bool					fBusEnumeration;

	typedef VectorMap<uint8, uint16> VirtualBusMap;

	VirtualBusMap			fVirtualBusMap;
	int						fNextVirtualBus;
};

extern PCI *gPCI;


extern "C" {

status_t	pci_init(void);
status_t	pci_init_deferred(void);
void		pci_uninit(void);

long		pci_get_nth_pci_info(long index, pci_info *outInfo);

uint32		pci_read_config(uint8 virtualBus, uint8 device, uint8 function,
				uint16 offset, uint8 size);
void		pci_write_config(uint8 virtualBus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 value);

void		__pci_resolve_virtual_bus(uint8 virtualBus, uint8 *domain, uint8 *bus);

}

#endif	/* __PCI_H__ */
