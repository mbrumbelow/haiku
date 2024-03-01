/*
 * Copyright 2024, Priyanshu Gupta, guptapiyush.0707@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AllocationGroupFreeSpace.h"

bool
AgfFreeList::IsVersion5() const
{
    if ((Version() & XFS_SB_VERSION_NUMBITS) == 5)
        return true;

    return false;
}


uint32
AgfFreeList::MagicNum() const
{
    return agf_magicnum;
}


uint32 AgfFreeList::Version() const
{
    return agf_versionnum;
}


bool
AgfFreeList::IsValid() const
{
    return Version() == XFS_AGF_VERSION && MagicNum()== XFS_AGF_MAGIC;
}


uint32
AgfFreeList::Seq_no() const
{
    return agf_seqno;
}


uint32
AgfFreeList::Size() const
{
    return agf_length;
}


const uint32*
AgfFreeList::Root() const
{
    return agf_roots;
}


const uint32*
AgfFreeList::Level() const
{
    return agf_levels;
}


uint32
AgfFreeList::StartFreelistBlock() const
{
    return agf_flfirst;
}


uint32
AgfFreeList::LastFreelistBlock() const
{
    return agf_fllast;
}


uint32
AgfFreeList::FreeBlockCount() const
{
    return agf_flcount;
}


uint32
AgfFreeList::LongestFreeBlock() const
{
    return agf_longest;
}


uint32
AgfFreeList::BtreeBlocks() const
{
    return agf_btreeblks;
}


bool
AgfFreeList::UuidEquals(XfsSuperBlock SB) const {
    return SB.UuidEquals(agf_uuid);
}


uint32
AgfFreeList::RmapBlocks() const
{
    return agf_rmap_blocks;
}


uint32
AgfFreeList::Ref_countBlocks() const
{
    return agf_refcount_blocks;
}


uint32
AgfFreeList::Ref_countLevel() const
{
    return agf_refcount_level;
}


xfs_lsn_t
AgfFreeList::Lsn()
{
    if (IsVersion5())
        return agf_lsn;

    return B_BAD_VALUE;
}


uint32
AgfFreeList::Crc()
{
    if (IsVersion5())
        return agf_crc;

    return B_BAD_VALUE;
}


void
AgfFreeList::SwapEndian()
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
//    agf_uuid			    =	B_BENDIAN_TO_HOST_INT64(agf_uuid);
    agf_rmap_blocks			=	B_BENDIAN_TO_HOST_INT32(agf_rmap_blocks);
    agf_refcount_blocks		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_blocks);
    agf_refcount_root		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_root);
    agf_refcount_level		=	B_BENDIAN_TO_HOST_INT32(agf_refcount_level);
    agf_lsn			        =	B_BENDIAN_TO_HOST_INT64(agf_lsn);
    agf_crc			        =	B_BENDIAN_TO_HOST_INT32(agf_crc);
    agf_spare2				=	B_BENDIAN_TO_HOST_INT32(agf_spare2);
}
