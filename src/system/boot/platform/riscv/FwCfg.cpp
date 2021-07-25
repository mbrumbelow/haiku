/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FwCfg.h"

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <Htif.h>
#include <KernelExport.h>

#include "graphics.h"


FwCfgRegs *volatile gFwCfgRegs = NULL;


#define _B(n)	((unsigned long long)((uint8_t *)&x)[n])
static inline uint16_t SwapEndian16(uint16_t x)
{
	return (_B(0) << 8) | _B(1);
}

static inline uint32_t SwapEndian32(uint32_t x)
{
	return (_B(0) << 24) | (_B(1) << 16) | (_B(2) << 8) | _B(3);
}

static inline uint64_t SwapEndian64(uint64_t x)
{
	return (_B(0) << 56) | (_B(1) << 48) | (_B(2) << 40) | (_B(3) << 32)
		| (_B(4) << 24) | (_B(5) << 16) | (_B(6) << 8) | _B(7);
}
#undef _B


namespace FwCfg {

void Select(uint16_t selector)
{
	// GCC, why are you so crazy?
	// gFwCfgRegs->selector = SwapEndian16(selector);
	*(uint16*)0x10100008 = SwapEndian16(selector);
}

void DmaOp(uint8_t *bytes, size_t count, uint32_t op)
{
	__attribute__ ((aligned (8))) FwCfgDmaAccess volatile dma;
	dma.control = SwapEndian32(1 << op);
	dma.length = SwapEndian32(count);
	dma.address = SwapEndian64((addr_t)bytes);
	// gFwCfgRegs->dmaAdr = SwapEndian64((addr_t)&dma);
	*(uint64*)0x10100010 = SwapEndian64((addr_t)&dma);
	while (uint32_t control = SwapEndian32(dma.control) != 0) {
		if (((1 << fwCfgDmaFlagsError) & control) != 0)
			abort();
	}
}

void ReadBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsRead);
}

void WriteBytes(uint8_t *bytes, size_t count)
{
	DmaOp(bytes, count, fwCfgDmaFlagsWrite);
}

uint8_t  Read8 () {uint8_t  val; ReadBytes(          &val, sizeof(val)); return val;}
uint16_t Read16() {uint16_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint32_t Read32() {uint32_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}
uint64_t Read64() {uint64_t val; ReadBytes((uint8_t*)&val, sizeof(val)); return val;}


void ListDir()
{
	uint32_t count = SwapEndian32(Read32());
	dprintf("count: %" B_PRIu32 "\n", count);
	for (uint32_t i = 0; i < count; i++) {
		FwCfgFile file;
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = SwapEndian32(file.size);
		file.select = SwapEndian16(file.select);
		file.reserved = SwapEndian16(file.reserved);
		dprintf("\n");
		dprintf("size: %" B_PRIu32 "\n", file.size);
		dprintf("select: %" B_PRIu32 "\n", file.select);
		dprintf("reserved: %" B_PRIu32 "\n", file.reserved);
		dprintf("name: %s\n", file.name);
	}
}

bool ThisFile(FwCfgFile& file, uint16_t dir, const char *name)
{
	Select(dir);
	uint32_t count = SwapEndian32(Read32());
	for (uint32_t i = 0; i < count; i++) {
		ReadBytes((uint8_t*)&file, sizeof(file));
		file.size = SwapEndian32(file.size);
		file.select = SwapEndian16(file.select);
		file.reserved = SwapEndian16(file.reserved);
		if (strcmp(file.name, name) == 0)
			return true;
	}
	return false;
}

void InitFramebuffer()
{
	FwCfgFile file;
	if (!ThisFile(file, fwCfgSelectFileDir, "etc/ramfb")) {
		dprintf("[!] ramfb not found\n");
		return;
	}
	dprintf("file.select: %" B_PRIu16 "\n", file.select);

	RamFbCfg cfg;
	uint32_t width = 1024, height = 768;

	gFramebuf.colors = (uint32_t*)malloc(4*width*height);
	gFramebuf.stride = width;
	gFramebuf.width = width;
	gFramebuf.height = height;

	cfg.addr = SwapEndian64((size_t)gFramebuf.colors);
	cfg.fourcc = SwapEndian32(ramFbFormatXrgb8888);
	cfg.flags = SwapEndian32(0);
	cfg.width = SwapEndian32(width);
	cfg.height = SwapEndian32(height);
	cfg.stride = SwapEndian32(4*width);
	Select(file.select);
	WriteBytes((uint8_t*)&cfg, sizeof(cfg));
}

void Init()
{
	dprintf("gFwCfgRegs: 0x%08" B_PRIx64 "\n", (addr_t)gFwCfgRegs);
	if (gFwCfgRegs == NULL)
		return;
	Select(fwCfgSelectSignature);
	dprintf("fwCfgSelectSignature: 0x%08" B_PRIx32 "\n", Read32());
	Select(fwCfgSelectId);
	dprintf("fwCfgSelectId: : 0x%08" B_PRIx32 "\n", Read32());
	Select(fwCfgSelectFileDir);
	ListDir();
	InitFramebuffer();
}

};
