/*
 * Copyright 2011-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include "FlexibleDisplayInterface.h"

#include <stdlib.h>
#include <string.h>
#include <Debug.h>
#include <KernelExport.h>

#include "accelerant.h"
#include "intel_extreme.h"


#undef TRACE
#define TRACE_FDI
#ifdef TRACE_FDI
#   define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#   define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED() TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static const int gSnbBFDITrainParam[] = {
	FDI_LINK_TRAIN_400MV_0DB_SNB_B,
	FDI_LINK_TRAIN_400MV_6DB_SNB_B,
	FDI_LINK_TRAIN_600MV_3_5DB_SNB_B,
	FDI_LINK_TRAIN_800MV_0DB_SNB_B,
};


// #pragma mark - FDITransmitter


FDITransmitter::FDITransmitter(pipe_index pipeIndex)
	:
	fRegisterBase(PCH_FDI_TX_BASE_REGISTER)
{
	if (pipeIndex == INTEL_PIPE_B)
		fRegisterBase += PCH_FDI_TX_PIPE_OFFSET * 1;
}


FDITransmitter::~FDITransmitter()
{
}


void
FDITransmitter::Enable()
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	uint32 value = read32(targetRegister);

	write32(targetRegister, value | FDI_TX_ENABLE);
	read32(targetRegister);
	spin(150);
}


void
FDITransmitter::Disable()
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	uint32 value = read32(targetRegister);

	write32(targetRegister, value & ~FDI_TX_ENABLE);
	read32(targetRegister);
	spin(150);
}


void
FDITransmitter::ConfigureClocks(fdi_pll config)
{
	CALLED();
	uint32 targetRegister = fRegisterBase + FDI_DATA_M_OFFSET;
	write32(targetRegister, config.data_m | FDI_RX_TRANS_UNIT_SIZE(64));

	targetRegister = fRegisterBase + FDI_DATA_N_OFFSET;
	write32(targetRegister, config.data_n);

	targetRegister = fRegisterBase + FDI_LINK_M_OFFSET;
	write32(targetRegister, config.link_m);

	targetRegister = fRegisterBase + FDI_LINK_N_OFFSET;
	write32(targetRegister, config.link_n);
}


bool
FDITransmitter::IsPLLEnabled()
{
	CALLED();
	return (read32(fRegisterBase + PCH_FDI_TX_CONTROL) & FDI_TX_PLL_ENABLED)
		!= 0;
}


void
FDITransmitter::EnablePLL(uint32 lanes)
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & FDI_TX_PLL_ENABLED) != 0) {
		// already enabled, possibly IronLake where it always is
		TRACE("%s: Already enabled.\n", __func__);
		return;
	}

	write32(targetRegister, value | FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100); // warmup 10us + dmi delay 20us, be generous
}


void
FDITransmitter::DisablePLL()
{
	CALLED();
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_ILK)) {
		// on IronLake the FDI PLL is alaways enabled, so no point in trying...
		return;
	}

	uint32 targetRegister = fRegisterBase + PCH_FDI_TX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~FDI_TX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


// #pragma mark - FDIReceiver


FDIReceiver::FDIReceiver(pipe_index pipeIndex)
	:
	fRegisterBase(PCH_FDI_RX_BASE_REGISTER)
{
	if (pipeIndex == INTEL_PIPE_B)
		fRegisterBase += PCH_FDI_RX_PIPE_OFFSET * 1;
}


FDIReceiver::~FDIReceiver()
{
}


void
FDIReceiver::Enable()
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	uint32 value = read32(targetRegister);

	write32(targetRegister, value | FDI_RX_ENABLE);
	read32(targetRegister);
	spin(150);
}


void
FDIReceiver::Disable()
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	uint32 value = read32(targetRegister);

	write32(targetRegister, value & ~FDI_RX_ENABLE);
	read32(targetRegister);
	spin(150);
}


void
FDIReceiver::ConfigureClocks(fdi_pll config)
{
	CALLED();
	uint32 targetRegister = fRegisterBase + FDI_DATA_M_OFFSET;
	write32(targetRegister, config.data_m | FDI_RX_TRANS_UNIT_SIZE(64));

	targetRegister = fRegisterBase + FDI_DATA_N_OFFSET;
	write32(targetRegister, config.data_n);

	targetRegister = fRegisterBase + FDI_LINK_M_OFFSET;
	write32(targetRegister, config.link_m);

	targetRegister = fRegisterBase + FDI_LINK_N_OFFSET;
	write32(targetRegister, config.link_n);
}


bool
FDIReceiver::IsPLLEnabled()
{
	CALLED();
	return (read32(fRegisterBase + PCH_FDI_RX_CONTROL) & FDI_RX_PLL_ENABLED)
		!= 0;
}


void
FDIReceiver::EnablePLL(uint32 lanes)
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	uint32 value = read32(targetRegister);
	if ((value & FDI_RX_PLL_ENABLED) != 0) {
		// already enabled, possibly IronLake where it always is
		TRACE("%s: Already enabled.\n", __func__);
		return;
	}

	value &= ~(FDI_DP_PORT_WIDTH_MASK | (0x7 << 16));
	value |= FDI_DP_PORT_WIDTH(lanes);
	//value |= (read32(PIPECONF(pipe)) & PIPECONF_BPC_MASK) << 11;

	write32(targetRegister, value | FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(200); // warmup 10us + dmi delay 20us, be generous
}


void
FDIReceiver::DisablePLL()
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	write32(targetRegister, read32(targetRegister) & ~FDI_RX_PLL_ENABLED);
	read32(targetRegister);
	spin(100);
}


void
FDIReceiver::SwitchClock(bool toPCDClock)
{
	CALLED();
	uint32 targetRegister = fRegisterBase + PCH_FDI_RX_CONTROL;
	write32(targetRegister, (read32(targetRegister) & ~FDI_RX_CLOCK_MASK)
		| (toPCDClock ? FDI_RX_CLOCK_PCD : FDI_RX_CLOCK_RAW));
	read32(targetRegister);
	spin(200);
}


// #pragma mark - FDILink


FDILink::FDILink(pipe_index pipeIndex)
	:
	fTransmitter(pipeIndex),
	fReceiver(pipeIndex),
	fPipeIndex(pipeIndex)
{
	// FIXME the indexing by a single pipe index is incorrect. The transcoders
	// (on the receiver side) can be mapped to any pipe (on the transmitter
	// side). But that will do for now as we don't attempt such fancy things
	// in the driver yet.
}


#if __GNUC__ <= 2
static inline uint32_t __builtin_clzl(uint64_t value)
{
	asm ("bsrl %0, %0" : "=r" (value) : "0" (value));
	return 63 - (uint32_t)value;
}
#endif


static uint64_t round_up_pow2(uint64_t value)
{
	int magnitude = (sizeof(value) * CHAR_BIT) - 1 - __builtin_clzl(value - 1);
	return 1UL << magnitude;
}


static fdi_pll compute_clocks(uint64_t pixelClock, uint32_t bpp,
	uint64_t baseClock, uint32_t lanes)
{
	// Compute M and N so that they fit on 24 bits while minimizing the
	// rounding error
	uint64 data_n = round_up_pow2(baseClock * lanes);
	uint64 data_m = pixelClock * bpp * data_n / (baseClock * lanes);

	while ((data_n | data_m) >= (1 << 24)) {
		data_m >>= 1;
		data_n >>= 1;
	}

	uint64_t link_n = round_up_pow2(baseClock);
	uint64_t link_m = pixelClock * link_n / baseClock;

	while ((link_n | link_m) >= (1 << 24)) {
		link_m >>= 1;
		link_n >>= 1;
	}

	return (fdi_pll){data_m, data_n, link_m, link_n};
}


status_t
FDILink::ConfigureTx(display_mode* target)
{
	CALLED();

	uint32 bitsPerPixel;
	switch (target->space) {
		case B_RGB32_LITTLE:
			bitsPerPixel = 32;
			break;
		case B_RGB16_LITTLE:
			bitsPerPixel = 16;
			break;
		case B_RGB15_LITTLE:
			bitsPerPixel = 15;
			break;
		case B_CMAP8:
		default:
			bitsPerPixel = 8;
			break;
	}

	// The fdi_link_frequency given by the driver is the bit clock, given
	// in MHz. Each symbol is made of 10 bits (8 data bytes plus two bytes
	// for balancing and clock recovery), and we need kHz precision for our
	// math here.
	uint32 symbolClock = gInfo->shared_info->fdi_link_frequency * 1000 / 10;

	// Compute number of lanes needed
	uint32 bps = target->timing.pixel_clock * bitsPerPixel * 21 / 20;
	fLanes = bps / (symbolClock * 8);
	TRACE("%s: FDI Link Lanes: %" B_PRIu32 "\n", __func__, fLanes);

	// Set the Data M/N and Link M/N before enabling the transmitter PLL.
	// We can also set the transfer unit size here since it is in the same
	// register. Its value is fixed (63), but is not the default value in the
	// register at reset.
	fPllConfig = compute_clocks(target->timing.pixel_clock,
		bitsPerPixel / 8, symbolClock, fLanes);

	Transmitter().ConfigureClocks(fPllConfig);

	return B_OK;
}


status_t
FDILink::Train(display_mode* target)
{
	CALLED();

	// XXX 9a. set TU size for Rx here before training (we use the default value anyway)
	status_t result = B_ERROR;

	// TODO: Only _AutoTrain on IVYB Stepping B or later
	// otherwise, _ManualTrain
	if (gInfo->shared_info->device_type.Generation() >= 7)
		result = _AutoTrain(fLanes);
	else if (gInfo->shared_info->device_type.Generation() == 6)
		result = _SnbTrain(fLanes);
	else if (gInfo->shared_info->device_type.Generation() == 5)
		result = _IlkTrain(fLanes);
	else
		result = _NormalTrain(fLanes);

	if (result != B_OK) {
		ERROR("%s: FDI training fault.\n", __func__);
	}

#if 0
    c.   Configure and enable PCH DPLL, wait for PCH DPLL warmup (Can be done anytime before enabling
        PCH transcoder)
    d.   [DevCPT] Configure DPLL SEL to set the DPLL to transcoder mapping and enable DPLL to the
        transcoder.
    e.   [DevCPT] Configure DPLL_CTL DPLL_HDMI_multipler.
#endif

    // 9f.   Configure PCH transcoder timings, M/N/TU, and other transcoder
	// settings (should match CPU settings).
	Receiver().ConfigureClocks(fPllConfig);

#if 0
    g.   [DevCPT] Configure and enable Transcoder DisplayPort Control if DisplayPort will be used
    h.   Enable PCH transcoder
#endif
	return result;
}


status_t
FDILink::_NormalTrain(uint32 lanes)
{
	CALLED();
	uint32 txControl = Transmitter().Base() + PCH_FDI_TX_CONTROL;
	uint32 rxControl = Receiver().Base() + PCH_FDI_RX_CONTROL;

	// Enable normal link training
	uint32 tmp = read32(txControl);
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IVB)) {
		tmp &= ~FDI_LINK_TRAIN_NONE_IVB;
		tmp |= FDI_LINK_TRAIN_NONE_IVB | FDI_TX_ENHANCE_FRAME_ENABLE;
	} else {
		tmp &= ~FDI_LINK_TRAIN_NONE;
		tmp |= FDI_LINK_TRAIN_NONE | FDI_TX_ENHANCE_FRAME_ENABLE;
	}
	write32(txControl, tmp);

	tmp = read32(rxControl);
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		tmp &= ~FDI_LINK_TRAIN_PATTERN_MASK_CPT;
		tmp |= FDI_LINK_TRAIN_NORMAL_CPT;
	} else {
		tmp &= ~FDI_LINK_TRAIN_NONE;
		tmp |= FDI_LINK_TRAIN_NONE;
	}
	write32(rxControl, tmp | FDI_RX_ENHANCE_FRAME_ENABLE);

	// Wait 1x idle pattern
	read32(rxControl);
	spin(1000);

	// Enable ecc on IVB
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IVB)) {
		write32(rxControl, read32(rxControl)
			| FDI_FS_ERRC_ENABLE | FDI_FE_ERRC_ENABLE);
		read32(rxControl);
	}

	return B_OK;
}


status_t
FDILink::_IlkTrain(uint32 lanes)
{
	CALLED();
	uint32 txControl = Transmitter().Base() + PCH_FDI_TX_CONTROL;
	uint32 rxControl = Receiver().Base() + PCH_FDI_RX_CONTROL;

	// Train 1: unmask FDI RX Interrupt symbol_lock and bit_lock
	uint32 tmp = read32(Receiver().Base() + PCH_FDI_RX_IMR);
	tmp &= ~FDI_RX_SYMBOL_LOCK;
	tmp &= ~FDI_RX_BIT_LOCK;
	write32(Receiver().Base() + PCH_FDI_RX_IMR, tmp);
	spin(150);

	// Enable CPU FDI TX and RX
	tmp = read32(txControl);
	tmp &= ~FDI_DP_PORT_WIDTH_MASK;
	tmp |= FDI_DP_PORT_WIDTH(lanes);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_1;
	write32(txControl, tmp);
	Transmitter().Enable();

	tmp = read32(rxControl);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_1;
	write32(rxControl, tmp);
	Receiver().Enable();

	// ILK Workaround, enable clk after FDI enable
	if (fPipeIndex == INTEL_PIPE_B) {
		write32(PCH_FDI_RXB_CHICKEN, FDI_RX_PHASE_SYNC_POINTER_OVR);
		write32(PCH_FDI_RXB_CHICKEN, FDI_RX_PHASE_SYNC_POINTER_OVR
			| FDI_RX_PHASE_SYNC_POINTER_EN);
	} else {
		write32(PCH_FDI_RXA_CHICKEN, FDI_RX_PHASE_SYNC_POINTER_OVR);
		write32(PCH_FDI_RXA_CHICKEN, FDI_RX_PHASE_SYNC_POINTER_OVR
			| FDI_RX_PHASE_SYNC_POINTER_EN);
	}

	uint32 iirControl = Receiver().Base() + PCH_FDI_RX_IIR;
	TRACE("%s: FDI RX IIR Control @ 0x%" B_PRIx32 "\n", __func__, iirControl);

	int tries = 0;
	for (tries = 0; tries < 5; tries++) {
		tmp = read32(iirControl);
		TRACE("%s: FDI RX IIR 0x%" B_PRIx32 "\n", __func__, tmp);

		if ((tmp & FDI_RX_BIT_LOCK)) {
			TRACE("%s: FDI train 1 done\n", __func__);
			write32(iirControl, tmp | FDI_RX_BIT_LOCK);
			break;
		}
	}

	if (tries == 5) {
		ERROR("%s: FDI train 1 failure!\n", __func__);
		return B_ERROR;
	}

	// Train 2
	tmp = read32(txControl);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_2;
	write32(txControl, tmp);

	tmp = read32(rxControl);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_2;
	write32(rxControl, tmp);

	read32(rxControl);
	spin(150);

	for (tries = 0; tries < 5; tries++) {
		tmp = read32(iirControl);
		TRACE("%s: FDI RX IIR 0x%" B_PRIx32 "\n", __func__, tmp);

		if (tmp & FDI_RX_SYMBOL_LOCK) {
			TRACE("%s: FDI train 2 done\n", __func__);
			write32(iirControl, tmp | FDI_RX_SYMBOL_LOCK);
			break;
		}
	}

	if (tries == 5) {
		ERROR("%s: FDI train 2 failure!\n", __func__);
		return B_ERROR;
	}

	return B_OK;
}


status_t
FDILink::_SnbTrain(uint32 lanes)
{
	// This implements step 9b of the modesetting sequence
	// (volume 3, part 2, Section 1.1.2)
	CALLED();
	uint32 txControl = Transmitter().Base() + PCH_FDI_TX_CONTROL;
	uint32 rxControl = Receiver().Base() + PCH_FDI_RX_CONTROL;

	// Train 1
	// Unmask the interrupt status bits we need to read
	uint32 imrControl = Receiver().Base() + PCH_FDI_RX_IMR;
	uint32 tmp = read32(imrControl);
	tmp &= ~FDI_RX_SYMBOL_LOCK;
	tmp &= ~FDI_RX_BIT_LOCK;
	write32(imrControl, tmp);
	read32(imrControl);
	spin(150);

	// i. Set pre-emphasis and voltage
	// ii. Set training pattern 1 and enable Rx and Tx
	tmp = read32(txControl);
	tmp &= ~FDI_DP_PORT_WIDTH_MASK;
	tmp |= FDI_DP_PORT_WIDTH(lanes);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_1;
	tmp &= ~FDI_LINK_TRAIN_VOL_EMP_MASK;

	tmp |= FDI_LINK_TRAIN_400MV_0DB_SNB_B;
	write32(txControl, tmp);
	Transmitter().Enable();

	write32(Receiver().Base() + PCH_FDI_RX_MISC,
		FDI_RX_TP1_TO_TP2_48 | FDI_RX_FDI_DELAY_90);

	tmp = read32(rxControl);
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		tmp &= ~FDI_LINK_TRAIN_PATTERN_MASK_CPT;
		tmp |= FDI_LINK_TRAIN_PATTERN_1_CPT;
	} else {
		tmp &= ~FDI_LINK_TRAIN_NONE;
		tmp |= FDI_LINK_TRAIN_PATTERN_1;
	}
	write32(rxControl, rxControl);
	Receiver().Enable();

	// Initial read to clear any pending interrupt
	uint32 iirControl = Receiver().Base() + PCH_FDI_RX_IIR;

	// i. Try again with different pre-emphasis and voltage if training fails
	int i = 0;
	for (i = 0; i < 4; i++) {
		tmp = read32(txControl);
		tmp &= ~FDI_LINK_TRAIN_VOL_EMP_MASK;
		tmp |= gSnbBFDITrainParam[i];
		write32(txControl, tmp);

		read32(txControl);

		// iii. Wait for training pattern 1 time
		spin(500);

		int retry = 0;
		for (retry = 0; retry < 5; retry++) {
			// iv. Check PCH ISR (IIR on IBX+) for bit lock and retry if it
			// fails
			tmp = read32(iirControl);
			if (tmp & FDI_RX_BIT_LOCK) {
				TRACE("%s: FDI train 1 done\n", __func__);
				write32(iirControl, tmp | FDI_RX_BIT_LOCK);
				goto train1_success;
			}
			spin(50);
		}

	}

	ERROR("%s: FDI train 1 failure!\n", __func__);
	return B_ERROR;

train1_success:
	// Train 2
	// v. Set train pattern 2 on CPU and PCH
	tmp = read32(txControl);
	tmp &= ~FDI_LINK_TRAIN_NONE;
	tmp |= FDI_LINK_TRAIN_PATTERN_2;

	// FIXME if we can detect problems with voltage and emphasis only in
	// training pattern 2, we should make sure we perform the whole training
	// with the same value. What's the point of doing training pattern 1 and
	// then changing all the settings?
	// In other words, the two for (i= 0 to 4) loops should be just one single
	// loop enclosing the two steps of the training.
	tmp &= ~FDI_LINK_TRAIN_VOL_EMP_MASK;
	tmp |= FDI_LINK_TRAIN_400MV_0DB_SNB_B;
	write32(txControl, tmp);

	tmp = read32(rxControl);
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		tmp &= ~FDI_LINK_TRAIN_PATTERN_MASK_CPT;
		tmp |= FDI_LINK_TRAIN_PATTERN_2_CPT;
	} else {
		tmp &= ~FDI_LINK_TRAIN_NONE;
		tmp |= FDI_LINK_TRAIN_PATTERN_2;
	}
	write32(rxControl, tmp);

	read32(rxControl);
	spin(150);

	for (i = 0; i < 4; i++) {
		tmp = read32(txControl);
		tmp &= ~FDI_LINK_TRAIN_VOL_EMP_MASK;
		tmp |= gSnbBFDITrainParam[i];
		write32(txControl, tmp);

		read32(txControl);

		// vi. Wait for train 2 pattern time
		spin(1500);

		int retry = 0;
		for (retry = 0; retry < 5; retry++) {
			// vii. Wait for symbol lock
			tmp = read32(iirControl);
			TRACE("%s: FDI RX IIR 0x%" B_PRIx32 "\n", __func__, tmp);

			if (tmp & FDI_RX_SYMBOL_LOCK) {
				TRACE("%s: FDI train 2 done\n", __func__);
				write32(iirControl, tmp | FDI_RX_SYMBOL_LOCK);
				break;
			}
			spin(50);
		}
		if (retry < 5)
			break;
	}

	if (i == 4) {
		ERROR("%s: FDI train 2 failure!\n", __func__);
		return B_ERROR;
	}

	// viii. Enable normal pixel output
	tmp = read32(txControl);
	tmp |= FDI_LINK_TRAIN_NONE;
	write32(txControl, tmp);

	tmp = read32(rxControl);
	if (gInfo->shared_info->pch_info == INTEL_PCH_CPT) {
		tmp |= FDI_LINK_TRAIN_NORMAL_CPT;
	} else {
		tmp |= FDI_LINK_TRAIN_NONE;
	}
	write32(rxControl, tmp);

	// ix. Wait for FDI idle pattern time
	spin(3100);

	TRACE("FDI training succesful!\n");
	return B_OK;
}


status_t
FDILink::_ManualTrain(uint32 lanes)
{
	CALLED();
	//uint32 txControl = Transmitter().Base() + PCH_FDI_TX_CONTROL;
	//uint32 rxControl = Receiver().Base() + PCH_FDI_RX_CONTROL;

	ERROR("%s: TODO\n", __func__);

	return B_ERROR;
}


status_t
FDILink::_AutoTrain(uint32 lanes)
{
	CALLED();
	uint32 txControl = Transmitter().Base() + PCH_FDI_TX_CONTROL;
	uint32 rxControl = Receiver().Base() + PCH_FDI_RX_CONTROL;

	uint32 buffer = read32(txControl);

	// Clear port width selection and set number of lanes
	buffer &= ~(7 << 19);
	buffer |= (lanes - 1) << 19;

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IVB))
		buffer &= ~FDI_LINK_TRAIN_NONE_IVB;
	else
		buffer &= ~FDI_LINK_TRAIN_NONE;
	write32(txControl, buffer);

	bool trained = false;

	for (uint32 i = 0; i < (sizeof(gSnbBFDITrainParam)
		/ sizeof(gSnbBFDITrainParam[0])); i++) {
		for (int j = 0; j < 2; j++) {
			buffer = read32(txControl);
			buffer |= FDI_AUTO_TRAINING;
			buffer &= ~FDI_LINK_TRAIN_VOL_EMP_MASK;
			buffer |= gSnbBFDITrainParam[i];
			write32(txControl, buffer | FDI_TX_ENABLE);

			write32(rxControl, read32(rxControl) | FDI_RX_ENABLE);

			spin(5);

			buffer = read32(txControl);
			if ((buffer & FDI_AUTO_TRAIN_DONE) != 0) {
				TRACE("%s: FDI auto train complete!\n", __func__);
				trained = true;
				break;
			}

			write32(txControl, read32(txControl) & ~FDI_TX_ENABLE);
			write32(rxControl, read32(rxControl) & ~FDI_RX_ENABLE);
			read32(rxControl);

			spin(31);
		}

		// If Trained, we fall out of autotraining
		if (trained)
			break;
	}

	if (!trained) {
		ERROR("%s: FDI auto train failed!\n", __func__);
		return B_ERROR;
	}

	// Enable ecc on IVB
	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_IVB)) {
		write32(rxControl, read32(rxControl)
			| FDI_FS_ERRC_ENABLE | FDI_FE_ERRC_ENABLE);
		read32(rxControl);
	}

	return B_OK;
}


FDILink::~FDILink()
{
}
