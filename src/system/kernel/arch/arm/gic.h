#ifndef ARCH_ARM_GIC_H
#define ARCH_ARM_GIC_H

#include "soc.h"

class GICv2InterruptController : public InterruptController {
public:
	GICv2InterruptController();
	void EnableInterrupt(int irq);
	void DisableInterrupt(int irq);
	void HandleInterrupt();
};

#endif
