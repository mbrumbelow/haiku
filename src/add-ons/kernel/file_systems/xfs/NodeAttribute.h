/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_ATTRIBUTE_H
#define NODE_ATTRIBUTE_H


#include "LeafAttribute.h"
#include "Node.h"


class NodeAttribute {
public:
								NodeAttribute(Inode* inode);
								NodeAttribute(Inode* inode, const char* name);
								~NodeAttribute();

			xfs_fsblock_t		LogicalToFileSystemBlock(uint32 LogicalBlock);

			status_t			FillMapEntry(xfs_extnum_t num);

			status_t			FillBuffer(char* buffer, xfs_fsblock_t block);

			status_t			Init();

			status_t			Stat(struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

			status_t			GetNext(char* name, size_t* nameLength);

			status_t			Lookup(const char* name, size_t* nameLength);
private:
			Inode*				fInode;
			const char*			fName;
			ExtentMapEntry*		fMap;
			char*				fLeafBuffer;
            char*               fNodeBuffer;
			uint16				fLastEntryOffset;
			AttrLeafNameLocal*	fLocalEntry;
			AttrLeafNameRemote*	fRemoteEntry;
};

#endif