/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


.global gFixedFont
.type   gFixedFont, @object
.data
gFixedFont:
.incbin "/boot/data/packages/haiku/src/system/boot/platform/riscv/fixed_font.bin"
.size	gFixedFont, .-gFixedFont
