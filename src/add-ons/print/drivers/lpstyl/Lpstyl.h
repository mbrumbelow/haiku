/*
* Copyright 2017-2018, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#ifndef LPSTYL_H
#define LPSTYL_H


#include "GraphicsDriver.h"


enum PrinterType {
	kStyleWriter,
	kStyleWriter2,
	kStyleWriter3,
	kStyleWriter2400,
	kStyleWriter2200,
	kStyleWriter1500,
	kStyleWriter2500
};


class DebugWindow;


class LpstylDriver: public GraphicsDriver {
	public:
						LpstylDriver(BMessage* message, PrinterData* printerData,
							const PrinterCap* printerCap);

	protected:
		bool			StartDocument();
		bool			StartPage();
		bool			NextBand(BBitmap* bitmap, BPoint* offset);
		bool			EndPage(int page);
		bool			EndDocument(bool success);

	private:
		void			_EjectAndReset(void);
		void			_IdentifyPrinter(void);
		bool			_ColorCartridge(void);

		void			_Write(const char* data, size_t length);
		void			_Write(char data);
		char			_Read();

		void			_WriteFFFx(char x);
		int				_GetStatus(char reg);

	private:
		PrinterType		fPrinterType;
		bool			fCanPrintColors;

		DebugWindow*	fDebugWindow;
};


#endif
