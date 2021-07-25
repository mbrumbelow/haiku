/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"


class PCIFU740 : public ArchPCIController {

			status_t			Init(fdt_device_module_info* parentModule);
			addr_t				ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);
			void				InitDeviceMSI(uint8 bus, uint8 device, uint8 function);
			bool 				AllocateBar() { return false; }
};
