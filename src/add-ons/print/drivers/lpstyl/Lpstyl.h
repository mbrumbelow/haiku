/*
* Copyright 2017-2019, Haiku, Inc. All rights reserved.
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


class BeDC;
class Halftone;


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
		void			_WaitForReady();
		void			_IdentifyPrinter(void);
		bool			_ColorCartridge(void);
		void			_PrepareForPrinting(void);
		int				_Compress(const uint8_t* in, uint8_t* out,
							size_t inLength);
		void			_WriteRectangleHeader(bool color, BRect area);
		void			_WriteRectangleData(ssize_t size, const char* data);

		void			_Write(const char* data, size_t length);
		void			_Write(char data);
		void			_Write(uint16_t data);
		char			_Read();

		void			_WriteFFFx(char x);
		int				_GetStatus(char reg);

		void			_DebugHex(const char* data, size_t size);

	private:
		PrinterType		fPrinterType;
		bool			fCanPrintColors;

		BeDC*			fDebugConsole;
		Halftone*		fHalftone;
};


#endif
