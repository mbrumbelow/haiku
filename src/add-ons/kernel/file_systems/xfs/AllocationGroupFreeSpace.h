//
// Created by Priyanshu Gupta on 14/2/24.
//

#ifndef XFS_ALLOCATIONGROUPFREESPACE_H
#define XFS_ALLOCATIONGROUPFREESPACE_H


#include "system_dependencies.h"
#include "xfs_types.h"
#include "xfs.h"


#define XFS_AGF_MAGIC 0x58414746
#define XFS_AGF_VERSION 1
#define XFS_BTNUM_AGF 3

//b+ tree related macros
#define XFS_AGF_MAGIC_ABTB  0x41425442
#define XFS_AGF_MAGIC_ABTC  0x41425443
#define XFS_AGF_MAGIC_AB3B  0x41423342
#define XFS_AGF_MAGIC_AB3C  0x41423343


class AgfFreeList{
public:
            bool                IsVersion5()const;
            uint32              MagicNum()const;
            uint32              Version()const;
            bool                IsValid()const;
            uint32              Seq_no()const;
            uint32              Size()const;
            const uint32*       Root()const;
            const uint32*        Level()const;
            uint32              StartFreelistBlock()const;
             uint32              LastFreelistBlock()const;
            uint32              FreeBlockCount()const;
            uint32              LongestFreeBlock()const;
            uint32              BtreeBlocks()const;
            bool				UuidEquals(XfsSuperBlock SB)const;
            uint32              RmapBlocks()const;
            uint32              Ref_countBlocks()const;
            uint32              Ref_countLevel()const;
            xfs_lsn_t           Lsn();
            uint32              Crc();
            void				SwapEndian();

private:
    uint32              agf_magicnum;
    uint32              agf_versionnum;
    xfs_agnumber_t      agf_seqno;
    uint32              agf_length;
    uint32              agf_roots[XFS_BTNUM_AGF];
    uint32              agf_levels[XFS_BTNUM_AGF];
    uint32              agf_flfirst;
    uint32              agf_fllast;
    uint32              agf_flcount;
    uint32              agf_freeblks;
    uint32              agf_longest;
    uint32              agf_btreeblks;
    /* version 5 filesystem fields start here */
    uuid_t              agf_uuid;
    uint32              agf_rmap_blocks;
    uint32              agf_refcount_blocks;
    uint32              agf_refcount_root;
    uint32              agf_refcount_level;
    uint64              agf_spare64[14];
    /* unlogged fields, written during buffer writeback. */
    xfs_lsn_t          agf_lsn;
    uint32              agf_crc;
    uint32              agf_spare2;


};

#endif  //XFS_ALLOCATIONGROUPFREESPACE_H
