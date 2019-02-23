/*
 * Copyright 2018, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BYTE_POINTER_H
#define _BYTE_POINTER_H


// Behaves like a char* pointer, but -> return the right pointed type.
// Assumes the offsets passed to + and += maintain the alignment for the type
template<class T> union BytePointer {
	T* data;
	char* address;

	BytePointer(void* base) { address = (char*)base; }

	T* operator&() { return data; }
	T* operator->() { return data; }
	void operator+=(size_t offset) { address += offset; }
	char* operator+(size_t offset) const { return address + offset; }
};


#endif
