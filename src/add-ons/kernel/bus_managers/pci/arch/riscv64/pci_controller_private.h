/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCI_CONTROLLER_PRIVATE_H_
#define _PCI_CONTROLLER_PRIVATE_H_


#include <SupportDefs.h>


struct PciDbiRegs {
	uint8 unknown0[0x700];

	uint32 unknown1[3];
	uint32 portAfr;
	uint32 linkControl;
	uint32 unknown2[5];
	uint32 portDebug0;
	uint32 portDebug1;
	uint32 unknown3[55];
	uint32 linkWidthSpeedControl;
	uint32 unknown4[4];
	uint32 msiAddrLo;
	uint32 msiAddrHi;
	struct {
		uint32 enable;
		uint32 mask;
		uint32 status;
	} msiIntr[8];
	uint32 unknown5[13];
	uint32 miscControl1Off;
	uint32 miscPortMultiLaneCtrl;
	uint32 unknown6[15];

	uint32 atuViewport;
	uint32 atuCr1;
	uint32 atuCr2;
	uint32 atuBaseLo;
	uint32 atuBaseHi;
	uint32 atuLimit;
	uint32 atuTargetLo;
	uint32 atuTargetHi;
	uint32 unknown7;
	uint32 atuLimitHi;
	uint32 unknown8[8];

	uint32 msixDoorbell;
	uint32 unknown9[117];

	uint32 plChkRegControlStatus;
	uint32 unknown10;
	uint32 plChkRegErrAddr;
	uint32 unknown11[309];
};


enum {
	kPciAtuOffset = 0x300000,
};

enum {
	kPciAtuOutbound  = 0,
	kPciAtuInbound   = 1,
};

enum {
	// ctrl1
	kPciAtuTypeMem  = 0,
	kPciAtuTypeIo   = 2,
	kPciAtuTypeCfg0 = 4,
	kPciAtuTypeCfg1 = 5,
	// ctrl2
	kPciAtuBarModeEnable = 1 << 30,
	kPciAtuEnable        = 1 << 31,
};

struct PciAtuRegs {
	uint32 ctrl1;
	uint32 ctrl2;
	uint32 baseLo;
	uint32 baseHi;
	uint32 limit;
	uint32 targetLo;
	uint32 targetHi;
	uint32 unused[57];
};


extern addr_t gPCIeConfigPhysBase;
extern addr_t gPCIeConfigBase;
extern size_t sPCIeConfigSize;

extern addr_t gPCIeDbiPhysBase;
extern addr_t gPCIeDbiBase;
extern size_t gPCIeDbiSize;


#endif	// _PCI_CONTROLLER_PRIVATE_H_
