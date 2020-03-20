/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Volume.h"

//#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


status_t
Volume::Identify(int fd, ufs2_super_block *superBlock)
{
    if (read_pos(fd, UFS2_SUPER_BLOCK_OFFSET, superBlock,
	    sizeof(ufs2_super_block)) != sizeof(ufs2_super_block))
        return B_IO_ERROR;


    return B_OK;
}
