/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef UFS2_H
#define UFS2_H

#include "system_dependencies.h"

#define UFS2_MAGIC                  0x19540119
#define UFS2_SUPER_BLOCK_OFFSET     8192

struct ufs2_super_block {
	uint32	ufs2_sblkno;
	uint32	ufs2_cblkno;
	uint32	ufs2_iblkno;
	uint32	ufs2_dblkno;
	uint32	ufs2_cgoffset;
	uint32	ufs2_cgmask;
	uint32	ufs2_time;
	uint32	ufs2_size;
	uint32	ufs2_dsize;
	uint32	ufs2_ncg;
	uint32	ufs2_bsize;
	uint32	ufs2_fsize;
	uint32	ufs2_frag;
	uint32	ufs2_magic;
	uint32	ufs2_minfree;
	uint32	ufs2_rotdelay;
	uint32	ufs2_rps;
	uint32	ufs2_maxcontig;
	uint32	ufs2_rolled;
	uint32	ufs2_si;
	uint32	ufs2_maxbpg;
	uint32	ufs2_sbsize;
	uint32	ufs2_optim;
	uint32	ufs2_cpg;
	uint32	ufs2_ipg;
	uint32	ufs2_fpg;
	uint32  ufs2_clean;
    uint32  ufs2_logbno;
    uint32  ufs2_reclaim;
};

#endif
