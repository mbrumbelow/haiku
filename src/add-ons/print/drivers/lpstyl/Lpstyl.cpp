/*
* Copyright 2017-2019, Haiku, Inc. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#include "Lpstyl.h"

#include <algorithm>

#include "BeDC.h"
#include "Halftone.h"


LpstylDriver::LpstylDriver(BMessage* message, PrinterData* printerData,
	const PrinterCap* printerCap)
	: GraphicsDriver(message, printerData, printerCap),
	fHalftone(NULL)
{
	fDebugConsole = new BeDC("Stylewriter");
}


bool
LpstylDriver::StartDocument()
{
	fDebugConsole->SendMessage("-- Start of document --", DC_CLEAR);

	_EjectAndReset();
	_IdentifyPrinter();
	fCanPrintColors = _ColorCartridge();
	_PrepareForPrinting();
	StartPage(); // Not called automatically for the first page
	fHalftone = new Halftone(GetJobData()->GetSurfaceType(),
		GetJobData()->GetGamma(), GetJobData()->GetInkDensity(),
		GetJobData()->GetDitherType());
	// TODO probably need to wait for the printer to be ready here, especially
	// when it is cold booting
	return true;
}


bool
LpstylDriver::StartPage()
{
	fDebugConsole->SendMessage("New page", DC_SEPARATOR);
	if (fPrinterType < kStyleWriter2400)
		_Write("nuA", 3);
	else
		_Write('L');
	for (int i = 0; i < 10; i++) {
		sleep(1);
		_GetStatus('1');
		_GetStatus('2');
	}
	return true;
}


bool
LpstylDriver::NextBand(BBitmap* bitmap, BPoint* offset)
{
	fDebugConsole->SendFormat("Next band at %f %f", offset->x, offset->y);
	BRect bounds = bitmap->Bounds();
	fDebugConsole->SendRect(bounds);

	int page_height = GetPageHeight();

	// The printer expects buffers in compressed 1-bit CMYK
	// (or just K, if printing greyscale)

	// TODO do a proper conversion of the input bitmap to 1bit CMYK.
	// We can use the Halftone class from libprint to achieve that, and we need
	// to adjust our "Caps" to advertise that (so it can be configured in print
	// settings)

	const uint8_t* source = (const uint8_t*)bitmap->Bits();
	BMallocIO output;

	uint16_t left = (uint16_t)offset->x;
	uint16_t top = (uint16_t)offset->y;
	uint16_t right = left + (uint16_t)bounds.Width();
	uint8_t buffer[1024];
	size_t bufferWidth = (size_t)(bounds.Width() + 7) / 8;
	uint8_t halftoneBuffer[bufferWidth];

	for (int y = 0; y < bounds.Height(); y++) {
		// Encode each scanline.
		// First of all, convert the input bitmap to Halftone pattern
		fHalftone->Dither(halftoneBuffer, source, offset->x, offset->y + y,
			bounds.Width());
		// TODO for all lines but the first, we should _Compress the XOR 
		// with the previous line. This allows for better compression.
		// TODO pass info to _Compress indicating wether the input is color or
		// greyscale.
		int length = _Compress(halftoneBuffer, buffer, bufferWidth);
		if (output.Position() + length > 0x4000 - 12) {
			// The new run may not fit (depending on the number of blocks)
			// So send what we have until now to the printer and start a new
			// rectangle
			
			uint16_t bottom = (uint16_t)offset->y + y - 1;
				// - 1 because there is one pending line we just compressed but
				// can't fit in the buffer
			// TODO "true" when printing in colors
			_WriteRectangleHeader(false, BRect(left, top, right, bottom));
			_WriteRectangleData(output.Position(), (const char*)output.Buffer());
			
			// Reset the output buffer for the next line
			output.Seek(0, SEEK_SET);
			top = bottom;
		}

		output.Write(buffer, length);

		// Get ready for the next line
		source += bitmap->BytesPerRow();
	}

	// Advance the cursor
	offset->y += bitmap->Bounds().Height();

	// Have we reached the end of the page yet?
	if (offset->y >= page_height)
	{
		offset->y = -1;
		offset->x = -1;
	}

	return true;
}


bool
LpstylDriver::EndPage(int page)
{
	fDebugConsole->SendFormat("end of page %d", page);
	_Write('\x0C');
	return true;
}


bool
LpstylDriver::EndDocument(bool success)
{
	fDebugConsole->SendMessage("End of document", success ? DC_SUCCESS : DC_ERROR);
	delete fHalftone;
	fHalftone = NULL;
	return true;
}


/** Eject the current page (if any) and reset the printer.
 */
void
LpstylDriver::_EjectAndReset(void)
{
	fDebugConsole->SendMessage("Eject and Reset");
	for (;;) {
		_WriteFFFx('I');
		sleep(2);

		int s1 = _GetStatus('1');

		if (s1 == 0x01) {
			// Check for stylewriter1, where status 1 doesn't go to 0 on init.
			if (_GetStatus('2') == 0 && _GetStatus('B') == 0xa0)
				break;
		} else if(s1 == 0x11) {
			break;
		}
	}
}


void
LpstylDriver::_WaitForReady()
{
	int s1;
	for(;;) {
		s1 = _GetStatus('1');
		if (s1 == 0x11 || s1 == 0x10)
			break;
		sleep(1);
	}
}


void
LpstylDriver::_IdentifyPrinter(void)
{
	fDebugConsole->SendMessage("Identify printer");
	_Write('?');

	char smallBuf[32];
	int i = 0;
	for (i = 0; i < 31; i++) {
		smallBuf[i] = _Read();
		if (smallBuf[i] == 0x0D)
			break;
	}
	smallBuf[i + 1] = 0;

	if (strcmp(smallBuf, "IJ10\x0D") == 0)
		fPrinterType = kStyleWriter;
	else if (strcmp(smallBuf, "SW\x0D") == 0)
		fPrinterType = kStyleWriter2;
	else if (strcmp(smallBuf, "SW3\x0D") == 0)
		fPrinterType = kStyleWriter3;
	else if (strcmp(smallBuf, "CS\x0D") == 0) {
		switch (_GetStatus('p'))
		{
			default:
			case 1:
				fPrinterType = kStyleWriter2400;
				break;
			case 2:
				fPrinterType = kStyleWriter2200;
				break;
			case 4:
				fPrinterType = kStyleWriter1500;
				break;
			case 5:
				fPrinterType = kStyleWriter2500;
				break;
		}
	} else {
		fDebugConsole->SendFormatT("Unknown printer type %s", DC_ERROR, smallBuf);
		return;
	}

	fDebugConsole->SendFormatT("Printer is identified", DC_SUCCESS, smallBuf);
}


bool
LpstylDriver::_ColorCartridge()
{
	fDebugConsole->SendMessage("Detect color cartridge");
	_Write('D');
	sleep(1);
	unsigned char i = _GetStatus('H');
	if (i & 0x80) {
		fDebugConsole->SendMessage("Color cartridge found", DC_SUCCESS);
	} else {
		fDebugConsole->SendMessage("Black and white or no cartridge found", DC_SUCCESS);
	}
	return i & 0x80;
}


void
LpstylDriver::_PrepareForPrinting(void)
{
	_Write("m0sAB", 5); // High quality printing
	sleep(1);
	_GetStatus('R'); // High quality printing
	_Write('N'); // Not documented
}


int
LpstylDriver::_Compress(const uint8_t* in, uint8_t* out, size_t inLength)
{
	// The encoding scheme is as follows:
	// 0x80 - Fully white scanline (or "end of scanline" ?)
	// FIXME this is not used yet, it can save a lot of space on text pages
	// 0x80 | len - Fully white run of len bytes (max 0x3E bytes)
	// 0xC0 | len - Fully black run of len bytes
	// 0x00 | len - Literal sequence (also max 0x3E bytes)

	size_t pos = 0;
	int outPos = 0;
	while(pos < inLength) {
		uint8_t count = 0;
		if (in[pos] == 0xFF) {
			// Encode a black run
			while(in[pos + count] == 0xFF && count < 0x3E) {
				count++;
			}

			out[outPos++] = 0xC0 | count;
		} else if (in[pos] == 0x00) {
			// Encode a white run
			while(in[pos + count] == 0x00 && count < 0x3E) {
				count++;
			}

			out[outPos++] = 0x80 | count;
		} else {
			// Encode a litteral run
			while(in[pos + count] != 0xFF && in[pos + count] != 0x00 && count < 0x3E) {
				count++;
			}

			out[outPos++] = count;
			memcpy(out + outPos, in + pos, count);
			outPos += count;
		}

		pos += count;
	}

#if 0
	BString str1;
	for (size_t i = 0; i < inLength; i++) {
		BString str2;
		str2.SetToFormat(" %02x", in[i]);
		str1 << str2;
	}
	fDebugConsole->SendMessage(str1);

	str1 = "";
	for (int i = 0; i < outPos; i++) {
		BString str2;
		str2.SetToFormat(" %02x", out[i]);
		str1 << str2;
	}
	fDebugConsole->SendMessage(str1, DC_SUCCESS);
#endif
	return outPos;
}



void
LpstylDriver::_WriteRectangleHeader(bool color, BRect area)
{
	_WaitForReady();

	fDebugConsole->SendRect(area);
	// Send the header telling what to print next. Should be followed by a
	// call to _WriteBitmapData
	_Write(color ? 'c' : 'R');
	_Write((uint16_t)area.left);
	_Write((uint16_t)area.top);
	_Write((uint16_t)(area.right - 1));
	_Write((uint16_t)(area.bottom - 1));
}


void
LpstylDriver::_WriteRectangleData(ssize_t size, const char* data)
{
	fDebugConsole->SendFormat("%d bytes in encoded rectangle", size);
	if (size > 65535) {
		debugger("Tried to write a too large block");
		return;
	}
	_Write('G');
	_Write((uint16_t)size);
	while (size > 0) {
		// Wait for space in the printer buffer
		ssize_t freeBlocks = (_GetStatus('B') & 0xFF);
		if (freeBlocks == 0) {
			sleep(2);
		} else {
			ssize_t writeSize = std::min(size, (ssize_t)256);
				// TODO use freeBlocks to compute how much we can actually write
			fDebugConsole->SendFormat("Sending %ld bytes to printer", writeSize);
			_DebugHex(data, writeSize);
			_Write(data, writeSize);
			size -= writeSize;
			data += writeSize;

			usleep(250);
			uint8_t sta2 = _GetStatus('2');
			if (sta2 & 0x04) {
				// Printer is out of paper
				fDebugConsole->SendMessage("Out of paper!", DC_ERROR);
				// TODO ask user to fix the problem, then retry (we probably
				// need to retry the whole page)
			}
		}
	}
	// Terminate the block
	_Write('\0');

	fDebugConsole->SendMessage("Rectangle data is sent.", DC_SUCCESS);
	_GetStatus('2');
	sleep(10);
	_GetStatus('2');
}


void
LpstylDriver::_Write(char data)
{
	WriteSpoolChar(data);
}


void
LpstylDriver::_Write(uint16_t data)
{
	// 16 bit values are written LSB first
	WriteSpoolChar(data & 0xFF);
	WriteSpoolChar(data >> 8);

	fDebugConsole->SendFormat("W16 %x %x", data & 0xFF, data >> 8);
}


void
LpstylDriver::_Write(const char* data, size_t length)
{
	WriteSpoolData(data, length);
}


char
LpstylDriver::_Read()
{
	char read = ReadSpoolChar();

	return read;
}


/** Send a 4-byte command to the printer.
 *
 * These commands can be sent at any time, because their prefix is FFFFFF, a
 * sequence which can't be generated by the data compression algorithm
 */
void
LpstylDriver::_WriteFFFx(char x)
{
	char str[4];
	str[0] = str[1] = str[2] = 0xFF;
	str[3] = x;
	_Write(str, 4);
}


/** Get one of the printer status bytes.
 *
 * There are 3 status registers, 1, 2, and B. Each returns some different
 * information about the printer state.
 */
int
LpstylDriver::_GetStatus(char reg)
{
	fDebugConsole->SendFormat("Read status register %c", reg);
	_WriteFFFx(reg);
	int result = _Read() & 0xFF;
	fDebugConsole->SendFormat("	%c =  %02x", reg, result);
	return result;
}


void
LpstylDriver::_DebugHex(const char* data, size_t size)
{
	BString str1;
	for (size_t i = 0; i < size; i++) {
		BString str2;
		str2.SetToFormat(" %02x", data[i] & 0xFF);
		str1 << str2;
	}
	fDebugConsole->SendMessage(str1);
}
