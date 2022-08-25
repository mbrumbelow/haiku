/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef SHORT_ATTRIBUTE_H
#define SHORT_ATTRIBUTE_H


#include "Inode.h"


struct attr_cookie {
	char	name[B_ATTR_NAME_LENGTH];
	uint32	type;
	int		open_mode;
	bool	create;
};


// xfs_attr_sf_hdr
struct AShortFormHeader {
            uint16              totsize;
            uint8               count;
            uint8               padding;
};


// xfs_attr_sf_entry
struct AShortFormEntry {
            uint8               namelen;
            uint8               valuelen;
            uint8               flags;
            uint8               nameval[];
};


class ShortAttribute {
public:
                                ShortAttribute(Inode* inode);
                                ShortAttribute(Inode* inode, const char* name);
								~ShortAttribute();

            uint32              DataLength(AShortFormEntry* entry);

            AShortFormEntry*    FirstEntry();

            status_t			Stat(struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

            status_t			GetNext(char* name, size_t* nameLength);

            status_t            Lookup(const char* name, size_t* nameLength);
private:
            Inode*				fInode;
            const char*		    fName;
            AShortFormHeader*   fHeader;
            AShortFormEntry*    fEntry;
            uint32              fLastEntryOffset;
};

#endif