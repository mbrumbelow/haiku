/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "ufs2.h"


class Inode;

class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();
			status_t			Lookup(const char* name, size_t length, ino_t* id);
			status_t			GetNext(char* name, size_t* _nameLength, ino_t* _id);

private:

			int64				fOffset;
			cluster_t			fCluster;
			Inode* 				fInode;

};


#endif	// DIRECTORYITERATOR_H
