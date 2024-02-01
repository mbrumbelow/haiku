/*
 * TigerLakePLL.cpp
 * Copyright (C) 2024 pulkomandy <pulkomandy@kitt>
 *
 * Distributed under terms of the MIT license.
 */

#include "TigerLakePLL.h"


/**
 * Compute the best PLL parameters for a given symbol clock frequency for a DVI or HDMI port.
 *
 * This is the algorithm documented in Intel Documentation: IHD-OS-TGL-Vol 12-12.21, page 182
 *
 * The clock generation on Tiger Lake is in two steps: first, a DCO generates a fractional
 * multiplication of the reference clock (in the GHz range). Then, 3 dividers bring this back into
 * the symbol clock frequency range.
 *
 * Reference clock (24 or 19.2MHz, as defined in DSSM Reference Frequency register)
 *             ||
 *             vv
 * DCO (multiply by non-integer value defined in DPLL_CFGCR0 register)
 *             ||
 *             vv
 * "DCO frequency" in the range 7998 - 10000 MHz
 *             ||
 *             vv
 * Divide by P, Q, and K
 *             ||
 *             vv
 * AFE clock (PLL output)
 *             ||
 *             vv
 * Divide by 5 (fixed)
 *             ||
 *             vv
 * Symbol clock (same as Pixel clock for 24-bit RGB)
 *
 * The algorithm to configure this is:
 * - Iterate over all allowed values for the divider obtained by P, Q and K
 * - Determine the one that results in the DCO frequency being as close as possible to 8999MHz
 * - Compute the corresponding values for P, Q and K and the DCO multiplier
 *
 * Since the DCO is a fractional multiplier (it can multiply by non-integer values), it will always
 * be possible to set the DCO to a "close enough" value in its available range. The only constraint
 * is getting it as close as possible to the midpoint (8999MHz), and at least have it in the
 * allowed range (7998 to 10000MHz). If this is not possible (too low or too high pixel clock), a
 * different video mode or setup will be needed (for example, enable dual link DVI to divide the
 * clock by two).
 *
 * This also means that this algorithm is independant of the initial reference frequency: there
 * will always be a way to setup the DCO so that it outputs the frequency computed here, no matter
 * what the input clock is.
 *
 * Unlinke in previous hardware generations, there is no need to satisfy multiple constraints at
 * the same time because of several stages of dividers and multipliers each with their own
 * frequency range.
 *
 * DCO multiplier = DCO integer + DCO fraction / 2^15
 * Symbol clock frequency = DCO multiplier * RefFreq in MHz / (5 * Pdiv * Qdiv * Kdiv)
 *
 * The symbol clock is the clock of the DVI/HDMI port. It defines how much time is needed to send
 * one "symbol", which corresponds to 8 bits of useful data for each channel (Red, Green and Blue).
 *
 * In our case (8 bit RGB videomode), the symbol clock is equal to the pixel rate. It would need
 * to be adjusted for 10 and 12-bit modes (more bits per pixel) as well as for YUV420 modes (the U
 * and V parts are sent only for some pixels, reducing the total bandwidth).
 *
 * @param[in] freq Desired symbol clock frequency in kHz
 * @param[out] Pdiv, Qdiv, Kdiv: dividers for the PLL
 * @param[out] bestdco Required DCO frequency, in the range 7998 to 10000, in MHz
 *
 * @note for DisplayPort, the logic is different: the symbol rate in DisplayPort does not depend
 * on the pixel clock, because the data is packetized. This allows to use spread spectrum on the
 * clock, allowing higher clock rates without degrading image quality.
 *
 * @todo write a separate function for DisplayPort clock setup (much simpler, there is a table of
 * allowed values in Intel specifications)
 *
 */
bool ComputeHdmiDpll(int freq, int* Pdiv, int* Qdiv, int* Kdiv, int* bestdco)
{
	int div = 0; bestdiv = 0; dco = 0; dcocentrality = 0;
	int bestdcocentrality = 999999;

	// The allowed values for the divider depending on the allowed values for P, Q, and K:
	// - P can be 2, 3, 5 or 7
	// - K can be 1, 2, or 3
	// - Q can be 1 to 255 if K = 2. Otherwise, Q must be 1.
	const int dividerlist[] = { 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 30, 32, 36,
		40, 42, 44, 48, 50, 52, 54, 56, 60, 64, 66, 68, 70, 72, 76, 78, 80, 84, 88, 90, 92,
		96, 98, 100, 102, 3, 5, 7, 9, 15, 21 };
	const int dcomin = 7998;
	const int dcomax = 10000;
	const int dcomid = (dcomin + dcomax) / 2;

	int afeclk = (5 * freq) / 1000;

	for (auto div: dividerlist) {
		dco = afeclk * div;
		if (dco <= dcomax && dco >= dcomin)
		{
			dcocentrality = abs(dco - dcomid);
			if (dcocentraility < bestdcocentrality)
			{
				bestdcocentrality = dcocentrality;
				bestdiv = div;
				*bestdco = dco;
			}
		}
	}

	if (bestdiv != 0) {
		// Good divider found
		if (bestdiv % 2 == 0) {
			// Divider is even
			if (bestdiv == 2) {
				*Pdiv = 2;
				*Qdiv = 1;
				*Kdiv = 1;
			} else if (bestdiv % 4 == 0) {
				*Pdiv = 2;
				*Qdiv = bestdiv / 4;
				*Kdiv = 2;
			} else if (bestdiv % 6 == 0) {
				*Pdiv = 3;
				*Qdiv = bestdiv / 6;
				*Kdiv = 2;
			} else if (bestdiv % 5 == 0) {
				*Pdiv = 5;
				*Qdiv = bestdiv / 10;
				*Kdiv = 2;
			} else if (bestdiv % 14 == 0) {
				*Pdiv = 7;
				*Qdiv = bestdiv / 14;
				*Kdiv = 2;
			}
		} else {
			// Divider is odd
			if (bestdiv == 3 || bestdiv == 5 || bestdiv == 7)
			{
				*Pdiv = bestdiv;
				*Qdiv = 1;
				*Kdiv = 1;
			} else {
				// Divider is 9, 15, or 21
				*Pdiv = bestdiv / 3;
				*Qdiv = 1;
				*Kdiv = 3;
			}
		}

		// SUCCESS
		return true;
		// Program DPLL_CFGCR0 with Best DCO / Reference frequency
		// Program DPLL_CFGCR1 with Pdiv, Qdiv and Kdiv
	} else {
		// No good divider found
		// FAIL, try a different frequency (different video mode)
		return false;
	}
}
