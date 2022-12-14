/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/msk/if_mskreg.h>


HAIKU_FBSD_DRIVER_GLUE(marvell_yukon, mskc, pci)
HAIKU_DRIVER_REQUIREMENTS(0);

extern driver_t *DRIVER_MODULE_NAME(e1000phy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(e1000phy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct msk_softc *sc = device_get_softc(dev);
	uint32_t status;

	/* Reading B0_Y2_SP_ISRC2 masks further interrupts. */
	status = CSR_READ_4(sc, B0_Y2_SP_ISRC2);
	if (status == 0 || status == 0xffffffff ||
	    (sc->msk_pflags & MSK_FLAG_SUSPEND) != 0 ||
	    (status & sc->msk_intrmask) == 0) {

		/* Clear possibly spurious TWSI IRQ, see FreeBSD r234666. */
		if ((status & Y2_IS_TWSI_RDY) != 0)
			CSR_WRITE_4(sc, B2_I2C_IRQ, 1);

		CSR_WRITE_4(sc, B0_Y2_SP_ICR, 2);
		return 0;
	}

	sc->haiku_interrupt_status = status;
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct msk_softc *sc = device_get_softc(dev);
	CSR_WRITE_4(sc, B0_Y2_SP_ICR, 2);
}
