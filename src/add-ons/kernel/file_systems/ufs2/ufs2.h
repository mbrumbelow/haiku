/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef UFS2_H
#define UFS2_H

#include "system_dependencies.h"

#define UFS2_SUPER_BLOCK_MAGIC		0x19540119
#define UFS2_SUPER_BLOCK_OFFSET		65536
#define MAXMNTLEN					512
#define	MAXCSBUFS					32
#define UFS2_MAXMNTLEN				468
#define UFS2_MAXCSBUFS				31
#define UFS2_MAXVOLLEN				32
#define	UFS2_NOCSPTRS				28

struct ufs2_csum {
	uint32	cs_ndir;	/* number of directories */
	uint32	cs_nbfree;	/* number of free blocks */
	uint32	cs_nifree;	/* number of free inodes */
	uint32	cs_nffree;	/* number of free frags */
};


struct ufs2_csum_total {
	uint64	cs_ndir;	/* number of directories */
	uint64	cs_nbfree;	/* number of free blocks */
	uint64	cs_nifree;	/* number of free inodes */
	uint64	cs_nffree;	/* number of free frags */
	uint64   cs_numclusters;	/* number of free clusters */
	uint64   cs_spare[3];	/* future expansion */
};

struct ufs2_timeval {
	uint32	tv_sec;
	uint32	tv_usec;
};

struct ufs2_super_block {
	union{
		struct{
			uint32	ufs2_link;
		}ufs_42;
		struct{
			uint32	ufs2_state;
		}ufs2_sun;
		//ufs2_link
	}ufs2_uo;		/* linked list of file systems */
	uint32	ufs2_rolled;		/* logging only: fs fully rolled */
	uint32	ufs2_sblkno;		/* addr of super-block in filesys */
	uint32	ufs2_cblkno;		/* offset of cyl-block in filesys */
	uint32	ufs2_iblkno;		/* offset of inode-blocks in filesys */
	uint32	ufs2_dblkno;		/* offset of first data after cg */
	uint32	ufs2_cgoffset;		/* cylinder group offset in cylinder */
	uint32	ufs2_cgmask;		/* used to calc mod ufs2_ntrak */
	uint32	ufs2_time;		/* last time written */
	uint32	ufs2_size;		/* number of blocks in fs */
	uint32	ufs2_dsize;		/* number of data blocks in fs */
	uint32	ufs2_ncg;			/* number of cylinder groups */
	uint32	ufs2_bsize;		/* size of basic blocks in fs */
	uint32	ufs2_fsize;		/* size of fragments blocks in fs */
	uint32	ufs2_frag;		/* number of fragments in a block in fs */
	uint32	ufs2_minfree;		/* minimum percentage of free blocks */
	uint32	ufs2_rotdelay;		/* num of ms for optimal next block */
	uint32	ufs2_rps;			/* disk revolutions per second */
	/* these fields can be computed from the others */
	uint32	ufs2_bmask;		//'blkoff' calc of blk offsets or blk address mask
	uint32	ufs2_fmask;		/* ``fragoff'' calc of frag offsets */
	uint32	ufs2_bshift;		/* ``lblkno'' calc of logical blkno */
	uint32	ufs2_fshift;		/* ``numfrags'' calc number of frags */
	/* these are configuration parameters */
	uint32	ufs2_maxcontig;		/* max number of contiguous blks */
	uint32	ufs2_maxbpg;		/* max number of blks per cyl group */
	/* these fields can be computed from the others */
	uint32	ufs2_fragshift;		/* block to frag shift */
	uint32	ufs2_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	uint32	ufs2_sbsize;		/* actual size of super block */
	uint32	ufs2_csmask;		/* csum block offset */
	uint32	ufs2_csshift;		/* csum block number */
	uint32	ufs2_nindir;		/* value of NINDIR */
	uint32	ufs2_inopb;		/* value of INOPB */
	uint32	ufs2_nspf;		/* value of NSPF */
	/* yet another configuration parameter */
	uint32	ufs2_optim;		/* optimization preference, see below */
	union{
		struct{
			uint32	ufs2_npsect;		/* # sectors/track including spares */
		}ufs2_sun;
		struct{
			uint32	ufs2_state;
		}ufs2_sunx86;
	}ufs2_u1;
	uint32 ufs2_interleave;		/* hardware sector interleave */
	uint32	ufs2_trackskew;		/* sector 0 skew, per track */
	uint32	ufs2_id[2];		/* file system id */
	/* sizes determined by number of cylinder groups and their sizes */
	uint32	ufs2_csaddr;		/* blk addr of cyl grp summary area */
	uint32	ufs2_cssize;		/* size of cyl grp summary area */
	uint32	ufs2_cgsize;		/* cylinder group size */
	/* these fields are derived from the hardware */
	uint32	ufs2_ntrak;		/* tracks per cylinder */
	uint32	ufs2_nsect;		/* sectors per track */
	uint32	ufs2_spc;			/* sectors per cylinder */
	/* this comes from the disk driver partitioning */
	uint32	ufs2_ncyl;		/* cylinders in file system */
	/* these fields can be computed from the others */
	uint32	ufs2_cpg;			/* cylinders per group */
	uint32	ufs2_ipg;			/* inodes per group */
	uint32	ufs2_fpg;			/* blocks per group * ufs2_frag */
	struct	ufs2_csum	ufs2_cstotal;
	/* these fields are cleared at mount time */
	char	ufs2_fmod;		/* super block modified flag */
	char	ufs2_clean;		/* file system state flag */
	char	ufs2_ronly;		/* mounted read-only flag */
	char	ufs2_flags;		/* largefiles flag, etc. */
	union{
		struct {
			char	ufs2_fsmnt[UFS2_MAXMNTLEN];/* name mounted on */
			uint32	ufs2_cgrotor;	/* last cg searched */
			uint32	ufs2_csp[UFS2_MAXCSBUFS];/*list of ufs2_cs info buffers */
			uint32	ufs2_maxcluster;
			uint32	ufs2_cpc;		/* cyl per cycle in postbl */
			uint16	ufs2_opostbl[16][8]; /* old rotation block list head */
		} ufs2_u1;
		struct {
			char	ufs2_fsmnt[UFS2_MAXMNTLEN];	/* name mounted on */
			unsigned char	ufs2_volname[UFS2_MAXVOLLEN]; /* volume name */
			uint64	ufs2_swuid;		/* system-wide uid   712*/
			uint32	ufs2_pad;	/* due to alignment of fs_swuid */
			uint32	ufs2_cgrotor;     /* last cg searched */
			uint32	ufs2_ocsp[UFS2_NOCSPTRS]; /*list of fs_cs info buffers */
			uint32	ufs2_contigdirs;/*# of contiguously allocated dirs */
			uint32	ufs2_csp;	/* cg summary info buffer for fs_cs */
			uint32	ufs2_maxcluster;
			uint32	ufs2_active;/* used by snapshots to track fs */
			uint32	ufs2_old_cpc;	/* cyl per cycle in postbl */
			uint32	ufs2_maxbsize;/*maximum blocking factor permitted */
			uint32	ufs2_sparecon64[31];/*old rotation block list head */
			uint64	ufs2_sblockloc; /* byte offset of standard superblock */
			struct	ufs2_csum_total	fs_cstotal;/*cylinder summary information*/
			struct	ufs2_timeval	fs_time;		/* last time written */
			uint64	ufs2_size;		/* number of fragments in fs */
			uint64	ufs2_dsize;	/*fragements that can be stored in file data */
			uint64	ufs2_csaddr;	/* blk addr of cyl grp summary area */
			uint64	ufs2_pendingblocks;/* blocks in process of being freed */
			uint32	ufs2_pendinginodes;/*inodes in process of being freed */
		} ufs2_u2;
	}ufs2_u11;
	union{
		struct {
			uint32	fs_sparecon[53];/* reserved for future constants */
			uint32	fs_reclaim;
			uint32	fs_sparecon2[1];
			uint32	fs_state;	/* file system state time stamp */
			uint32	fs_qbmask[2];	/* ~usb_bmask */
			uint32	fs_qfmask[2];	/* ~usb_fmask */
		} ufs2_sun;
		struct {
			uint32	fs_sparecon[53];/* reserved for future constants */
			uint32	fs_reclaim;
			uint32	fs_sparecon2[1];
			uint32	fs_npsect;	/* # sectors/track including spares */
			uint32	fs_qbmask[2];	/* ~usb_bmask */
			uint32	fs_qfmask[2];	/* ~usb_fmask */
		} ufs2_sunx86;
		struct {
			uint32	fs_sparecon[49];/* reserved for future constants */
			uint32	ufs2_contigsumsize;/* size of cluster summary array */
			uint32	fs_maxsymlinklen;/* max length of an internal symlink */
			uint32	fs_inodefmt;	/* format of on-disk inodes */
			uint32	fs_maxfilesize[2];	/* max representable file size */
			uint32	fs_qbmask[2];	/* ~usb_bmask */
			uint32	fs_qfmask[2];	/* ~usb_fmask */
			uint32	fs_state;	/* file system state time stamp */
		} ufs2_44;
	}ufs2_u22;
	/* these fields retain the current block allocation info */
	uint32	ufs2_nrpos;		/* number of rotaional positions */
	uint32	ufs2_postbloff;		/* (short) rotation block list head */
	uint32	ufs2_rotbloff;		/* (uchar_t) blocks for each rotation */
	uint32	ufs2_magic;		/* magic number */
	char	ufs2_space[1];		/* list of blocks for each rotation */
	bool	IsValid();
};

#endif
