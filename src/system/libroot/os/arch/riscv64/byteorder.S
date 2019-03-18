/*
 * Copyright 2009, Johannes Wischert. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <asm_defs.h>

.text

/* uint16 __swap_int16(uint16 value)
 */
FUNCTION(__swap_int16):
#warning IMPLEMENT_ME
		and r1,r0,#0x000000ff
		and r2,r0,#0x0000ff00
		mov r1,r1,LSL #8
		mov r2,r2,ROR #8
		orr r0,r1,r2
		mov pc,lr
FUNCTION_END(__swap_int16)

/* uint32 __swap_int32(uint32 value)
 */
FUNCTION(__swap_int32):
#warning IMPLEMENT_ME
		mov r3,#0xff000000
		orr r3,#0x0000ff00
		and r1,r0,r3
		mov r1,r1,ROR #24
		mov r3,r3,ROR #8
		and r2,r0,r3
		mov r2,r2,ROR #8
		orr r0,r1,r2
		mov pc,lr
FUNCTION_END(__swap_int32)

/* uint64 __swap_int64(uint64 value)
 */
FUNCTION(__swap_int64):
#warning IMPLEMENT_ME
		mov pc,lr
FUNCTION_END(__swap_int64)

/* TODO: The following functions can surely be optimized. A simple optimization
 * would be to define macros with the contents of the __swap_int{32,64}
 * functions and use those instead of calling the functions.
 */

/* float __swap_float(float value)
 */
FUNCTION(__swap_float):
		b		__swap_int32
		//rts
FUNCTION_END(__swap_float)


/* double __swap_double(double value)
 */
FUNCTION(__swap_double):
		b		__swap_int32
		//rts
#warning M68K: XXX:check sizeof(double)
FUNCTION_END(__swap_double)

