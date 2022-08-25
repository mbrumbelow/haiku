/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H


#include "Inode.h"
#include "ShortAttribute.h"


// This class will act as an interface between all types of attributes for xfs
class Attribute {
public:
                                Attribute(Inode* inode);
								Attribute(Inode* inode, attr_cookie* cookie);
								~Attribute();

            status_t            Init();

            status_t			CheckAccess(int openMode);

			status_t			Open(const char* name, int openMode,
									attr_cookie** _cookie);

			status_t			Stat(struct stat& stat);

			status_t			Read(attr_cookie* cookie, off_t pos,
									uint8* buffer, size_t* length);

            status_t			GetNext(char* name, size_t* nameLength);

            status_t            Lookup(const char* name, size_t* nameLength);
private:
            Inode*				fInode;
            ShortAttribute*		fShortAttr;
            const char*		    fName;
};

#endif