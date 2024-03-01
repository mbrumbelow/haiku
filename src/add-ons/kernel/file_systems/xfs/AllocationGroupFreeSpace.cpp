/*
 * Copyright 2024, Priyanshu Gupta, guptapiyush.0707@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AllocationGroupFreeSpace.h"


uint32
AgfFreeList::Version() const
{
	return AgData.agf_versionnum;
}


bool
AgfFreeList::Valid_crc() const
{
	uint32 len=sizeof(AgData);
	char* fbuffer = new(std::nothrow) char[len];
	if (fbuffer == NULL)
		return false;

	memcpy(fbuffer, &AgData, len);
	if(!xfs_verify_cksum(fbuffer,len,Offset_crc()))
		return false;

	return true;
}


bool
AgfFreeList::IsValid() const
{
	return Version() == XFS_AGF_VERSION && Valid_crc();
}


uint32
AgfFreeList::Seq_no() const
{
	return AgData.agf_seqno;
}


uint32
AgfFreeList::Size() const
{
	return AgData.agf_length;
}


const uint32*
AgfFreeList::Root() const
{
	return AgData.agf_roots;
}


const uint32*
AgfFreeList::Level() const
{
	return AgData.agf_levels;
}


uint32
AgfFreeList::StartFreelistBlock() const
{
	return AgData.agf_flfirst;
}


uint32
AgfFreeList::LastFreelistBlock() const
{
	return AgData.agf_fllast;
}


uint32
AgfFreeList::FreeBlockCount() const
{
	return AgData.agf_flcount;
}


uint32
AgfFreeList::LongestFreeBlock() const
{
	return AgData.agf_longest;
}


uint32
AgfFreeList::BtreeBlocks() const
{
	return AgData.agf_btreeblks;
}


bool
AgfFreeList::UuidEquals(XfsSuperBlock SB) const {
	return SB.UuidEquals(AgData.agf_uuid);
}


uint32
AgfFreeList::RmapBlocks() const
{
	return AgData.agf_rmap_blocks;
}


uint32
AgfFreeList::Ref_countBlocks() const
{
	return AgData.agf_refcount_blocks;
}


uint32
AgfFreeList::Ref_countLevel() const
{
	return AgData.agf_refcount_level;
}


xfs_lsn_t
AgfFreeList::Lsn()
{
	return AgData.agf_lsn;
}


void
AgfFreeList::SwapEndian()
{
	AgData.agf_magicnum			    =   B_BENDIAN_TO_HOST_INT32(AgData.agf_magicnum);
	AgData.agf_versionnum			=	B_BENDIAN_TO_HOST_INT32(AgData.agf_versionnum);
	AgData.agf_seqno			    =	B_BENDIAN_TO_HOST_INT32(AgData.agf_seqno);
	AgData.agf_length				=	B_BENDIAN_TO_HOST_INT32(AgData.agf_length);
	AgData.agf_flfirst				=	B_BENDIAN_TO_HOST_INT32(AgData.agf_flfirst);
	AgData.agf_fllast			    =	B_BENDIAN_TO_HOST_INT32(AgData.agf_fllast);
	AgData.agf_flcount			    =	B_BENDIAN_TO_HOST_INT32(AgData.agf_flcount);
	AgData.agf_freeblks		        =	B_BENDIAN_TO_HOST_INT32(AgData.agf_freeblks);
	AgData.agf_longest		        =	B_BENDIAN_TO_HOST_INT32(AgData.agf_longest);
	AgData.agf_btreeblks			=	B_BENDIAN_TO_HOST_INT32(AgData.agf_btreeblks);
	AgData.agf_rmap_blocks			=	B_BENDIAN_TO_HOST_INT32(AgData.agf_rmap_blocks);
	AgData.agf_refcount_blocks		=	B_BENDIAN_TO_HOST_INT32(AgData.agf_refcount_blocks);
	AgData.agf_refcount_root		=	B_BENDIAN_TO_HOST_INT32(AgData.agf_refcount_root);
	AgData.agf_refcount_level		=	B_BENDIAN_TO_HOST_INT32(AgData.agf_refcount_level);
	AgData.agf_lsn			        =	B_BENDIAN_TO_HOST_INT64(AgData.agf_lsn);
	AgData.agf_crc			        =	B_BENDIAN_TO_HOST_INT32(AgData.agf_crc);
	AgData.agf_spare2				=	B_BENDIAN_TO_HOST_INT32(AgData.agf_spare2);
}
