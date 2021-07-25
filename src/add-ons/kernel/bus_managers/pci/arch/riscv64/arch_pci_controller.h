/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCI_CONTROLLER_H_
#define _PCI_CONTROLLER_H_


#include "pci_controller.h"
#include "pci.h"

#include <AutoDeleterOS.h>
#include <AutoDeleterDrivers.h>

#include <drivers/bus/FDT.h>


enum {
    kRegIo,
    kRegMmio32,
    kRegMmio64,
};

struct RegisterRange {
    phys_addr_t parentBase;
    phys_addr_t childBase;
    size_t size;
    phys_addr_t free;
};


struct InterruptMapMask {
    uint32_t childAdr;
    uint32_t childIrq;
};

struct InterruptMap {
    uint32_t childAdr;
    uint32_t childIrq;
    uint32_t parentIrqCtrl;
    uint32_t parentIrq;
};



class ArchPCIController {
public:
								ArchPCIController();
								~ArchPCIController();

// Implementation Specific
virtual	status_t				Init(fdt_device_module_info* parentNode);
virtual	void					InitDeviceMSI(uint8 bus, uint8 device, uint8 function);
virtual	addr_t					ConfigAddress(uint8 bus, uint8 device, uint8 function,
									uint16 offset);
virtual bool          AllocateBar();

// Generic Helpers
		void					DumpAtu();

		RegisterRange*				GetRegisterRange(int range);
		void					SetRegisterRange(int kind, phys_addr_t parentBase,
									phys_addr_t childBase, size_t size);

		phys_addr_t				AllocRegister(int kind, size_t size);
		void					AllocRegsForDevice(uint8 bus, uint8 device, uint8 function);

		InterruptMap*			LookupInterruptMap(uint32_t childAdr, uint32_t childIrq);
		void					AllocRegs();

		status_t				ReadConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
									uint16 offset, uint8 size, uint32 *value);
		status_t				WriteConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
									uint16 offset, uint8 size, uint32 value);

private:

			RegisterRange			fRegisterRanges[3];
			InterruptMapMask		fInterruptMapMask;

			uint32				fHostCtrlType;

			AreaDeleter			fConfigArea;
			addr_t				fConfigPhysBase;
			addr_t				fConfigBase;
			size_t				fConfigSize;

			AreaDeleter			fDbiArea;
			addr_t				fDbiPhysBase;
			addr_t				fDbiBase;
			size_t				fDbiSize;

			AreaDeleter			fIoArea;
			addr_t				fIoBase;

			ArrayDeleter<InterruptMap> fInterruptMap;
			uint32_t				fInterruptMapLen;
};


#endif /* _PCI_CONTROLLER_H_ */
