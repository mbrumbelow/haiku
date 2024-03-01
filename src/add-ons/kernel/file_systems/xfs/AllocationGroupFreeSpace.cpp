/*
 * Copyright 2024, Priyanshu Gupta, guptapiyush.0707@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AllocationGroupFreeSpace.h"


uint32
AgfFreeList::Version() const
{
	return freelist->agf_versionnum;
}


bool
AgfFreeList::Valid_crc()
{
	uint_32 len=sizeof(agflist);
	fbuffer = new(std::nothrow) char[len];
	if (fBlockBuffer == NULL)
		return B_NO_MEMORY;

	fbuffer=(char*)freelist;
	if(!xfs_verify_cksum(fBlockBuffer,len,Offset_crc()))
		return false;

	return true;
}


bool
AgfFreeList::IsValid() const
{
    return freelist->Version() == XFS_AGF_VERSION && Valid_crc();
}


uint32
AgfFreeList::Seq_no() const
{
    return freelist->agf_seqno;
}


uint32
AgfFreeList::Size() const
{
    return freelist->agf_length;
}


const uint32*
AgfFreeList::Root() const
{
    return freelist->agf_roots;
}


const uint32*
AgfFreeList::Level() const
{
    return freelist->agf_levels;
}


uint32
AgfFreeList::StartFreelistBlock() const
{
    return freelist->agf_flfirst;
}


uint32
AgfFreeList::LastFreelistBlock() const
{
    return freelist->agf_fllast;
}


uint32
AgfFreeList::FreeBlockCount() const
{
    return freelist->agf_flcount;
}


uint32
AgfFreeList::LongestFreeBlock() const
{
    return freelist->agf_longest;
}


uint32
AgfFreeList::BtreeBlocks() const
{
    return freelist->agf_btreeblks;
}


bool
AgfFreeList::UuidEquals(XfsSuperBlock SB) const {
    return SB.UuidEquals(freelist->agf_uuid);
}


uint32
AgfFreeList::RmapBlocks() const
{
    return freelist->agf_rmap_blocks;
}


uint32
AgfFreeList::Ref_countBlocks() const
{
    return freelist->agf_refcount_blocks;
}


uint32
AgfFreeList::Ref_countLevel() const
{
    return freelist->agf_refcount_level;
}


xfs_lsn_t
AgfFreeList::Lsn()
{
	return freelist->agf_lsn;
}


void
agflist::SwapEndian()
{
    agf_magicnum			=   B_BENDIAN_TO_HOST_INT32(agf_magicnum);
    agf_versionnum			=	B_BENDIAN_TO_HOST_INT32(agf_versionnum);
    agf_seqno			    =	B_BENDIAN_TO_HOST_INT32(agf_seqno);
    agf_length				=	B_BENDIAN_TO_HOST_INT32(agf_length);
    agf_flfirst				=	B_BENDIAN_TO_HOST_INT32(agf_flfirst);
    agf_fllast			    =	B_BENDIAN_TO_HOST_INT32(agf_fllast);
    agf_flcount			    =	B_BENDIAN_TO_HOST_INT32(agf_flcount);
    agf_freeblks		    =	B_BENDIAN_TO_HOST_INT32(agf_freeblks);
    agf_longest		        =	B_BENDIAN_TO_HOST_INT32(agf_longest);
    agf_btreeblks			=	B_BENDIAN_TO_HOST_INT32(agf_btreeblks);
    agf_rmap_blocks			=	B_BENDIAN_TO_HOST_INT32(agf_rmap_blocks);
    agf_refcount_blocks		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_blocks);
    agf_refcount_root		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_root);
    agf_refcount_level		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_level);
    agf_lsn			        =	B_BENDIAN_TO_HOST_INT64(agf_lsn);
    agf_crc			        =	B_BENDIAN_TO_HOST_INT32(agf_crc);
    agf_spare2				=	B_BENDIAN_TO_HOST_INT32(agf_spare2);
}
