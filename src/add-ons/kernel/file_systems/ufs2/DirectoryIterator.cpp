#include "DirectoryIterator.h"

#include <stdlib.h>

#include "Inode.h"

#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("\33[34mufs2:\33[0m " x)

DirectoryIterator::DirectoryIterator(Inode* inode)
	:
	fOffset(-2),
	fInode(inode)
{
	TRACE("DirectoryIterator::DirectoryIterator() \n");
}


DirectoryIterator::~DirectoryIterator()
{
}


status_t
DirectoryIterator::InitCheck()
{
	return B_OK;
}

