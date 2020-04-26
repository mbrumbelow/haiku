/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef INTEL_FDI_H
#define INTEL_FDI_H


#include "intel_extreme.h"


struct fdi_pll {
	uint32_t data_m;
	uint32_t data_n;
	uint32_t link_m;
	uint32_t link_n;
};


class FDITransmitter {
public:
									FDITransmitter(pipe_index pipeIndex);
									~FDITransmitter();

		void						Enable();
		void						Disable();

		void						ConfigureClocks(fdi_pll);
		bool						IsPLLEnabled();
		void						EnablePLL(uint32 lanes);
		void						DisablePLL();

		uint32						Base()
										 { return fRegisterBase; };

private:
		uint32						fRegisterBase;
};


class FDIReceiver {
public:
									FDIReceiver(pipe_index pipeIndex);
									~FDIReceiver();

		void						Enable();
		void						Disable();

		void						ConfigureClocks(fdi_pll);
		bool						IsPLLEnabled();
		void						EnablePLL(uint32 lanes);
		void						DisablePLL();

		void						SwitchClock(bool toPCDClock);

		uint32						Base()
										 { return fRegisterBase; };

protected:
		uint32						fRegisterBase;
};


class FDILink {
public:
									FDILink(pipe_index pipeIndex);
									~FDILink();

		FDITransmitter&				Transmitter()
										{ return fTransmitter; };
		FDIReceiver&				Receiver()
										{ return fReceiver; };

		status_t					ConfigureTx(display_mode* target);
		status_t					Train(display_mode* target);

private:
		status_t					_NormalTrain(uint32 lanes);
		status_t					_IlkTrain(uint32 lanes);
		status_t					_SnbTrain(uint32 lanes);
		status_t					_ManualTrain(uint32 lanes);
		status_t					_AutoTrain(uint32 lanes);

		FDITransmitter				fTransmitter;
		FDIReceiver					fReceiver;

		pipe_index					fPipeIndex;

		uint32						fLanes;
		struct fdi_pll				fPllConfig;
};


#endif // INTEL_FDI_H
