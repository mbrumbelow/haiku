#ifndef DIRECTORYITERATOR_H
#define DIRECTORYITERATOR_H


#include "ufs2.h"

class Inode;

class DirectoryIterator {
public:
								DirectoryIterator(Inode* inode);
								~DirectoryIterator();

			status_t			InitCheck();

private:

			int64				fOffset;
			cluster_t			fCluster;
			Inode* 				fInode;

};


#endif	// DIRECTORYITERATOR_H
