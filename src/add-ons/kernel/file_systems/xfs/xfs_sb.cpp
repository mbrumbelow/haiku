#include "xfs_sb.h"

xfs_sb::xfs_sb()
{
    TRACE("Initializing the superblock");
}

void xfs_sb::FillSuperBlock(char *data)
{
    memcpy(&xfs_sb_t, data, sizeof(xfs_sb_t));
    SwapEndian();
}

bool xfs_sb::IsValid()
{
    if (xfs_sb_t.sb_magicnum != XFS_SB_MAGIC)
    {
        return false;
    }
    return true;
}

uint32 xfs_sb::BlockSize()
{
    return xfs_sb_t.sb_blocksize;
}

uint32 xfs_sb::Size()
{
    return sizeof(xfs_sb_t);
}

char *xfs_sb::Name()
{
    return xfs_sb_t.sb_fname;
}
void xfs_sb::SwapEndian()
{
    xfs_sb_t.sb_magicnum = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_magicnum);
    xfs_sb_t.sb_blocksize = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_blocksize);
    xfs_sb_t.sb_dblocks = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_dblocks);
    xfs_sb_t.sb_rblocks = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_rblocks);
    xfs_sb_t.sb_rextents = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_rextents);
    xfs_sb_t.sb_logstart = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_logstart);
    xfs_sb_t.sb_rootino = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_rootino);
    xfs_sb_t.sb_rbmino = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_rbmino);
    xfs_sb_t.sb_rsumino = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_rsumino);
    xfs_sb_t.sb_rextsize = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_rextsize);
    xfs_sb_t.sb_agblocks = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_agblocks);
    xfs_sb_t.sb_agcount = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_agcount);
    xfs_sb_t.sb_rbmblocks = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_rbmblocks);
    xfs_sb_t.sb_logblocks = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_logblocks);
    xfs_sb_t.sb_versionnum = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_versionnum);
    xfs_sb_t.sb_sectsize = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_sectsize);
    xfs_sb_t.sb_inodesize = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_inodesize);
    xfs_sb_t.sb_inopblock = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_inopblock);
    xfs_sb_t.sb_icount = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_icount);
    xfs_sb_t.sb_ifree = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_ifree);
    xfs_sb_t.sb_fdblocks = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_fdblocks);
    xfs_sb_t.sb_frextents = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_frextents);
    xfs_sb_t.sb_uquotino = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_uquotino);
    xfs_sb_t.sb_gquotino = B_BENDIAN_TO_HOST_INT64(xfs_sb_t.sb_gquotino);
    xfs_sb_t.sb_qflags = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_qflags);
    xfs_sb_t.sb_inoalignmt = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_inoalignmt);
    xfs_sb_t.sb_unit = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_unit);
    xfs_sb_t.sb_width = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_width);
    xfs_sb_t.sb_logsectsize = B_BENDIAN_TO_HOST_INT16(xfs_sb_t.sb_logsectsize);
    xfs_sb_t.sb_logsunit = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_logsunit);
    xfs_sb_t.sb_features2 = B_BENDIAN_TO_HOST_INT32(xfs_sb_t.sb_features2);
}
