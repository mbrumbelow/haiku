#ifndef _PCI_MSI_H_
#define _PCI_MSI_H_


#include <SupportDefs.h>


extern long gStartMsiIrq;
extern phys_addr_t gMsiPhysAddr;


int32 AllocMsiInterrupt();
void FreeMsiInterrupt(int32 irq);

void InitPciMsi(int32 msiIrq);


#endif	// _PCI_MSI_H_
