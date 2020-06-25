/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef __SHORT_DIR_H__
#define __SHORT_DIR_H__

#include "Inode.h"


struct xfs_dir2_sf_off_t{
			uint16				i;
				//offset into the literal area
} __attribute__((packed));

union xfs_dir2_inou_t{
			uint64				i8;
			uint32				i4;
} __attribute__((packed));


// Short form directory header
struct xfs_dir2_sf_hdr_t{
			uint8				count;
				// number of entries
			uint8				i8count;
				// # of 64bit inode entries
			xfs_dir2_inou_t		parent;
				// absolute inode # of parent
} __attribute__((packed));


struct xfs_dir2_sf_entry_t {
			uint8				namelen; // length of the name, in bytes
			xfs_dir2_sf_off_t	offset;	// offset tag, for directory iteration
			uint8				name[]; // name of directory entry
/*
 * Following will be a single byte file type variable
 * and inode number (64bits or 32 bits)
 */
} __attribute__((packed));


class ShortDirectory
{
public:
								ShortDirectory(Inode* inode);
								~ShortDirectory();
			status_t			GetNext(char* name, size_t* length,
									xfs_ino_t* ino);
			xfs_ino_t			GetParentIno();
			status_t			Lookup(const char* name, size_t length,
									xfs_ino_t* id);

private:
			Inode*				fInode;
			xfs_dir2_sf_hdr_t*	fHeader;
			uint16				fLastEntryOffset;
				// offset into the literal area
			uint8				fTrack;
				// Takes the values 0, 1 or 2 only

};


#endif