/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _NODE_H_
#define _NODE_H_


#include "Extent.h"
#include "LeafDirectory.h"


#define XFS_DIR2_LEAFN_MAGIC (0xd2ff)
#define XFS_DIR3_LEAFN_MAGIC (0x3dff)
#define XFS_DA_NODE_MAGIC (0xfebe)
#define XFS_DA3_NODE_MAGIC (0x3ebe)


class NodeHeader {
public:

			virtual						~NodeHeader()			=	0;
			virtual	uint16				Magic()					=	0;
			virtual	uint64				Blockno()				=	0;
			virtual	uint64				Lsn()					=	0;
			virtual	uint64				Owner()					=	0;
			virtual	uuid_t*				Uuid()					=	0;
			virtual	uint16				Count()					=	0;
			static	uint32				ExpectedMagic(int8 WhichDirectory,
										Inode* inode);
			static	uint32				CRCOffset();
};


//xfs_da_node_hdr
class NodeHeaderV4 : public NodeHeader {
public:

								NodeHeaderV4(const char* buffer);
								~NodeHeaderV4();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Lsn();
			uint64				Owner();
			uuid_t*				Uuid();
			uint16				Count();

			BlockInfo			info;
private:
			uint16				count;
			uint16				level;
};


class NodeHeaderV5 : public NodeHeader {
public:

								NodeHeaderV5(const char* buffer);
								~NodeHeaderV5();
			void				SwapEndian();
			uint16				Magic();
			uint64				Blockno();
			uint64				Lsn();
			uint64				Owner();
			uuid_t*				Uuid();
			uint16				Count();

			BlockInfoV5			info;
private:
			uint16				count;
			uint16				level;
			uint32				pad32;
};

#define XFS_NODE_CRC_OFF offsetof(NodeHeaderV5, info.crc)
#define XFS_NODE_V5_VPTR_OFF offsetof(NodeHeaderV5, info.forw)
#define XFS_NODE_V4_VPTR_OFF offsetof(NodeHeaderV4, info.forw)


//xfs_da_node_entry
struct NodeEntry {
			uint32				hashval;
			uint32				before;
};


class NodeDirectory {
public:
								NodeDirectory(Inode* inode);
								~NodeDirectory();
			status_t			Init();
			bool				IsNodeType();
			void				FillMapEntry(int num, ExtentMapEntry* map);
			status_t			FillBuffer(int type, char* buffer,
									int howManyBlocksFurther);
			void				SearchAndFillDataMap(uint64 blockNo);
			status_t			FindHashInNode(uint32 hashVal, uint32* rightMapOffset);
			uint32				GetOffsetFromAddress(uint32 address);
			xfs_extnum_t		FirstLeafMapIndex();
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
			uint8				fCurLeafMapNumber;
			uint8				fCurLeafBufferNumber;
			xfs_extnum_t		fFirstLeafMapIndex;
};


NodeHeader*
CreateNodeHeader(Inode* inode, const char* buffer);


uint32
SizeOfNodeHeader(Inode* inode);

#endif