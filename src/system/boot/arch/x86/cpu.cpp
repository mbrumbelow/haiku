#include "cpu.h"

#include "efi_platform.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arch_system_info.h>

#include <string.h>

static void
calculate_cpu_conversion_factor(uint8 channel)
{
	// When using channel 2, enable the input and disable the speaker.
	if (channel == 2) {
		uint8 control = in8(PIT_CHANNEL_2_CONTROL);
		control &= PIT_CHANNEL_2_SPEAKER_OFF_MASK;
		control |= PIT_CHANNEL_2_GATE_HIGH;
		out8(control, PIT_CHANNEL_2_CONTROL);
	}
		
	uint64 tscDeltaQuick, tscDeltaSlower, tscDeltaSlow;
	double conversionFactorQuick, conversionFactorSlower, conversionFactorSlow;
	uint16 expired;

	uint32 quickSampleCount = 1;
	uint32 slowSampleCount = 1;

quick_sample:
	calibration_loop(224, channel, tscDeltaQuick, conversionFactorQuick,
		expired);

slower_sample:
	calibration_loop(192, channel, tscDeltaSlower, conversionFactorSlower,
		expired);

	double deviation = conversionFactorQuick / conversionFactorSlower;
	if (deviation < 0.99 || deviation > 1.01) {
		// We might have been hit by a SMI or were otherwise stalled
		if (quickSampleCount++ < MAX_QUICK_SAMPLES)
			goto quick_sample;
	}

	// Slow sample
	calibration_loop(128, channel, tscDeltaSlow, conversionFactorSlow,
		expired);

	deviation = conversionFactorSlower / conversionFactorSlow;
	if (deviation < 0.99 || deviation > 1.01) {
		// We might have been hit by a SMI or were otherwise stalled
		if (slowSampleCount++ < MAX_SLOW_SAMPLES)
			goto slower_sample;
	}

	// Scale the TSC delta to timer units
	tscDeltaSlow *= TIMER_CLKNUM_HZ;

	uint64 clockSpeed = tscDeltaSlow / expired;
	gTimeConversionFactor = ((uint128(expired) * uint32(1000000)) << 32)
		/ uint128(tscDeltaSlow);

#ifdef TRACE_CPU
	if (clockSpeed > 1000000000LL) {
		dprintf("CPU at %Ld.%03Ld GHz\n", clockSpeed / 1000000000LL,
			(clockSpeed % 1000000000LL) / 1000000LL);
	} else {
		dprintf("CPU at %Ld.%03Ld MHz\n", clockSpeed / 1000000LL,
			(clockSpeed % 1000000LL) / 1000LL);
	}
#endif

	gKernelArgs.arch_args.system_time_cv_factor = gTimeConversionFactor;
	gKernelArgs.arch_args.cpu_clock_speed = clockSpeed;
	//dprintf("factors: %lu %llu\n", gTimeConversionFactor, clockSpeed);

	if (quickSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " quick samples for TSC calibration\n",
			quickSampleCount);
	}

	if (slowSampleCount > 1) {
		dprintf("needed %" B_PRIu32 " slow samples for TSC calibration\n",
			slowSampleCount);
	}

	if (channel == 2) {
		// Set the gate low again
		out8(in8(PIT_CHANNEL_2_CONTROL) & ~PIT_CHANNEL_2_GATE_HIGH,
			PIT_CHANNEL_2_CONTROL);
	}
}