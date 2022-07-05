/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _LEAFDIRECTORY_H_
#define _LEAFDIRECTORY_H_


#include "Extent.h"
#include "Inode.h"
#include "system_dependencies.h"


#define V4_DATA_HEADER_MAGIC 0x58443244
#define V5_DATA_HEADER_MAGIC 0x58444433

#define V4_LEAF_HEADER_MAGIC 0xd2f1
#define V5_LEAF_HEADER_MAGIC 0x3df1


enum ContentType { DATA, LEAF };


//xfs_dir2_leaf_hdr_t
struct ExtentLeafHeaderV4 {
			uint16				Magic()
								{ return B_BENDIAN_TO_HOST_INT16(info.magic); }

			uint16				Count()
								{ return B_BENDIAN_TO_HOST_INT16(count); }

			BlockInfo			info;
			uint16				count;
			uint16				stale;
};


//xfs_dir3_leaf_hdr_t
struct ExtentLeafHeader {

			uint16				Magic()
								{ return B_BENDIAN_TO_HOST_INT16(info.magic); }

			uint64				Blockno()
								{ return B_BENDIAN_TO_HOST_INT64(info.blkno); }

			uint64				Lsn()
								{ return B_BENDIAN_TO_HOST_INT64(info.lsn); }

			uint64				Owner()
								{ return B_BENDIAN_TO_HOST_INT64(info.owner); }

			uint16				Count()
								{ return B_BENDIAN_TO_HOST_INT16(count); }

			BlockInfoV5			info;
			uint16				count;
			uint16				stale;
			uint32				pad;

};

#define XFS_LEAF_CRC_OFF offsetof(struct ExtentLeafHeader, info.crc)


// xfs_dir2_leaf_tail_t
struct ExtentLeafTail {
			uint32				bestcount;
				// # of best free entries
};


class LeafDirectory {
public:
								LeafDirectory(Inode* inode);
								~LeafDirectory();
			status_t			Init();
			bool				IsLeafType();
			bool				VerifyDataHeader();
			bool				VerifyLeafHeader();
			void				FillMapEntry(int num, ExtentMapEntry* map);
			status_t			FillBuffer(int type, char* buffer,
									int howManyBlocksFurthur);
			void				SearchAndFillDataMap(uint64 blockNo);
			ExtentLeafEntry*	FirstLeaf();
			xfs_ino_t			GetIno();
			uint32				GetOffsetFromAddress(uint32 address);
			int					EntrySize(int len) const;
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);
private:
			Inode*				fInode;
			ExtentMapEntry*		fDataMap;
			ExtentMapEntry*		fLeafMap;
			uint32				fOffset;
			char*				fDataBuffer;
				// This isn't inode data. It holds the directory block.
			char*				fLeafBuffer;
			uint32				fCurBlockNumber;
};


#endif
