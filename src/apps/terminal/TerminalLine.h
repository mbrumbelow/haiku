/*
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMINAL_LINE_H
#define TERMINAL_LINE_H

#include <GraphicsDefs.h>
#include <SupportDefs.h>

#include "TermConst.h"

#include "UTF8Char.h"


struct Attribute {
	uint32 state;
	uint32 foreground;
	uint32 background;

	Attribute() : state(0), foreground(0), background(0) {}
	
	inline void Reset() { state = 0; foreground = 0; background = 0; }

	inline bool IsWidth() const { return (state & A_WIDTH) == A_WIDTH; }
	inline bool IsBold() const { return (state & BOLD) == BOLD; }
	inline bool IsUnder() const { return (state & UNDERLINE) == UNDERLINE; }
	inline bool IsInverse() const { return (state & INVERSE) == INVERSE; }
	inline bool IsMouse() const { return (state & MOUSE) == MOUSE; }
	inline bool IsForeSet() const { return (state & FORESET) == FORESET; }
	inline bool IsBackSet() const { return (state & BACKSET) == BACKSET; }
	inline bool IsFont() const { return (state & FONT) == FONT; }
	inline bool IsCR() const { return (state & DUMPCR) == DUMPCR; }
	inline int IndexedForeground() const { return (state & FORECOLOR) >> 16; }
	inline int IndexedBackground() const { return (state & BACKCOLOR) >> 24; }

	inline Attribute&
	operator&=(uint32 value) { state &= value; return *this; }

	inline Attribute&
	operator|=(uint32 value) { state |= value; return *this; }

	inline uint32
	operator|(uint32 value) { return state | value; }

	inline uint32
	operator&(uint32 value) { return state & value; }

	void SetForeground(uint8 red, uint8 green, uint8 blue)
	{
		foreground = 0x80000000 | (red << 16) | (green << 8) | blue;
	}

	void SetBackground(uint8 red, uint8 green, uint8 blue)
	{
		background = 0x80000000 | (red << 16) | (green << 8) | blue;
	}

	bool HasForeground() const { return (foreground & 0x80000000) != 0; }
	bool HasBackground() const { return (background & 0x80000000) != 0; }

	inline rgb_color
	Foreground() const
	{
		rgb_color color = make_color((foreground >> 16) & 0xFF,
			(foreground >> 8) & 0xFF,
			foreground & 0xFF);

		return color;
	}

	inline rgb_color
	Background() const
	{
		rgb_color color = make_color((background >> 16) & 0xFF,
			(background >> 8) & 0xFF,
			background & 0xFF);
		
		return color;
	}

	inline bool
	operator==(const Attribute& other) const
	{
		return state == other.state
			&& foreground == other.foreground
			&& background == other.background;
	}

	inline bool
	operator!=(const Attribute& other) const
	{
		return state != other.state
			|| foreground != other.foreground
			|| background != other.background;
	}
};


struct TerminalCell {
	UTF8Char			character;
	Attribute			attributes;
};


struct TerminalLine {
	uint16			length;
	bool			softBreak;	// soft line break
	Attribute		attributes;
	TerminalCell	cells[1];

	inline void Clear()
	{
		Clear(Attribute());
	}

	inline void Clear(size_t count)
	{
		Clear(Attribute(), count);
	}

	inline void Clear(Attribute attr, size_t count = 0)
	{
		length = 0;
		attributes = attr;
		softBreak = false;
		for (size_t i = 0; i < count; i++)
			cells[i].attributes = attr;
	}
};


struct AttributesRun {
	Attribute	attributes;
	uint16	offset;			// character offset
	uint16	length;			// length of the run in characters
};


struct HistoryLine {
	AttributesRun*	attributesRuns;
	uint16			attributesRunCount;	// number of attribute runs
	uint16			byteLength : 15;	// number of bytes in the line
	bool			softBreak : 1;		// soft line break;
	Attribute		attributes;

	AttributesRun* AttributesRuns() const
	{
		return attributesRuns;
	}

	char* Chars() const
	{
		return (char*)(attributesRuns + attributesRunCount);
	}

	int32 BufferSize() const
	{
		return attributesRunCount * sizeof(AttributesRun) + byteLength;
	}
};


#endif	// TERMINAL_LINE_H
