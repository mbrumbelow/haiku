/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef XFS_SYMLINK_H
#define XFS_SYMLINK_H


#include "Inode.h"


#define SYMLINK_MAGIC 0x58534c4d


// Used only on Version 5
struct SymlinkHeader {
public:

			uint32				Magic() const;
			uint64				Blockno() const;
			uuid_t*				Uuid();
			uint64				Owner() const;
	static	uint32				ExpectedMagic(int8 WhichDirectory,
										Inode* inode);
	static	uint32				CRCOffset();

private:
			uint32				sl_magic;
			uint32				sl_offset;
			uint32				sl_bytes;
public:
			uint32				sl_crc;
private:
			uuid_t				sl_uuid;
			uint64				sl_owner;
			uint64				sl_blkno;
			uint64				sl_lsn;
};


// This class will handle all formats of Symlinks in xfs
class Symlink {
public:
								Symlink(Inode* inode);
								~Symlink();
			status_t			FillMapEntry();
			status_t			FillBuffer();
			status_t			ReadLink(off_t pos, char* buffer, size_t* _length);
			status_t			ReadLocalLink(off_t pos, char* buffer, size_t* _length);
			status_t			ReadExtentLink(off_t pos, char* buffer, size_t* _length);
private:
			Inode*				fInode;
			ExtentMapEntry*		fMap;
			char*				fSymlinkBuffer;
};

#endif