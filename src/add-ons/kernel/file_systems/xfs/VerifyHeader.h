/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef _XFS_VHEADER_H
#define _XFS_VHEADER_H


#include "BPlusTree.h"
#include "Checksum.h"
#include "Extent.h"
#include "Inode.h"
#include "LeafDirectory.h"
#include "Node.h"
#include "xfs_types.h"
#include "system_dependencies.h"


/*
	Template function to verify common data header
	checks like blockno, owner, uuid.
*/
template<class U>
bool VerifyHeader2(U* header, Inode* inode, int howManyBlocksFurther, ExtentMapEntry* map)
{
	uint64 actualBlockToRead = inode->FileSystemBlockToAddr(map->br_startblock
		+ howManyBlocksFurther) / XFS_MIN_BLOCKSIZE;

	if (actualBlockToRead != header->Blockno()) {
		ERROR("Wrong Block number");
		return false;
	}

	if (!inode->GetVolume()->UuidEquals(header->Uuid())) {
		ERROR("UUID is incorrect");
		return false;
	}

	if (inode->ID() != header->Owner()) {
		ERROR("Wrong data owner");
		return false;
	}

	return true;
}


/*
	template function to verify all types of data headers, we use magic number found
	in header to see which type of header we will check for crc calculation.
	If header magic number doesn't match with any in given function it is an
	invalid header.
*/
template<class T>
bool VerifyHeader1(T* header, char* buffer, Inode* inode,
	int howManyBlocksFurther, ExtentMapEntry* map)
{
	if (header->Magic() == DIR2_BLOCK_HEADER_MAGIC
		|| header->Magic() == DIR3_BLOCK_HEADER_MAGIC) {

		TRACE("Extent directory : Data header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_EXTENT_CRC_OFF - XFS_EXTENT_V5_VPTR_OFF)) {
			ERROR("Data block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}

	if (header->Magic() == V4_DATA_HEADER_MAGIC
		|| header->Magic() == V5_DATA_HEADER_MAGIC) {

		TRACE("Leaf/Node/B+Tree directory : Data header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_EXTENT_CRC_OFF - XFS_EXTENT_V5_VPTR_OFF)) {
			ERROR("Data block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}

	if (header->Magic() == V4_LEAF_HEADER_MAGIC
		|| header->Magic() == V5_LEAF_HEADER_MAGIC) {

		TRACE("Leaf directory : Leaf header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_LEAF_CRC_OFF - XFS_LEAF_V5_VPTR_OFF)) {
			ERROR("Leaf block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}

	if (header->Magic() == XFS_DIR2_LEAFN_MAGIC
		|| header->Magic() == XFS_DIR3_LEAFN_MAGIC) {

		TRACE("Node/B+Tree directory : Leaf header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_LEAF_CRC_OFF - XFS_LEAF_V5_VPTR_OFF)) {
			ERROR("Leaf block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}

	if (header->Magic() == XFS_DA_NODE_MAGIC
		|| header->Magic() == XFS_DA3_NODE_MAGIC) {

		TRACE("Node/B+Tree directory : Node header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_NODE_CRC_OFF - XFS_NODE_V5_VPTR_OFF)) {
			ERROR("Node block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}

	if (header->Magic() == XFS_BMAP_MAGIC
		|| header->Magic() == XFS_BMAP_CRC_MAGIC) {

		TRACE("B+Tree directory : Block header verify");

		if (inode->Version() == 1 || inode->Version() == 2)
			return true;

		if (!xfs_verify_cksum(buffer, inode->DirBlockSize(),
				XFS_LBLOCK_CRC_OFF)) {
			ERROR("Block is corrupted");
			return false;
		}

		if (!VerifyHeader2<T>(header, inode, howManyBlocksFurther, map))
			return false;

		return true;
	}
	// TODO : See if it can be done Block header as well

	ERROR("Wrong magic number");
	return false;
}

#endif