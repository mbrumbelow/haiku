#include "BPlusTree.h"

void bplustree_short_block::swap_endian()
{
	bb_magic = B_BENDIAN_TO_HOST_INT32(bb_magic);
	bb_level = B_BENDIAN_TO_HOST_INT16(bb_level);
	bb_numrecs = B_BENDIAN_TO_HOST_INT16(bb_numrecs);
	bb_leftsib = B_BENDIAN_TO_HOST_INT32(bb_leftsib);
	bb_rightsib = B_BENDIAN_TO_HOST_INT32(bb_rightsib);
}

void bplustree_long_block::swap_endian()
{
	bb_magic = B_BENDIAN_TO_HOST_INT32(bb_magic);
	bb_level = B_BENDIAN_TO_HOST_INT16(bb_level);
	bb_numrecs = B_BENDIAN_TO_HOST_INT16(bb_numrecs);
	bb_leftsib = B_BENDIAN_TO_HOST_INT64(bb_leftsib);
	bb_rightsib = B_BENDIAN_TO_HOST_INT64(bb_rightsib);
}

void xfs_alloc_rec::swap_endian()
{
	ar_startblock = B_BENDIAN_TO_HOST_INT32(ar_startblock);
	ar_blockcount =  B_BENDIAN_TO_HOST_INT32(ar_blockcount);
}

int BPlusTree::rec_size(){
	if(rec_type == ALLOC_FLAG) return XFS_ALLOC_REC_SIZE;
}

int BPlusTree::key_len(){
	if(key_type == ALLOC_FLAG) return XFS_ALLOC_REC_SIZE;
}

int BPlusTree::block_len(){
	if(ptr_type == LONG_BLOCK_FLAG) return XFS_BTREE_LBLOCK_SIZE;
	else return XFS_BTREE_SBLOCK_SIZE;
}

int BPlusTree::ptr_len(){
	if(ptr_type == LONG_BLOCK_FLAG) return sizeof(uint64);
	else return sizeof(uint32);
}

int BPlusTree::record_offset(int pos){
	return block_len() + (pos-1)*rec_size();
}

int BPlusTree::key_offset(int pos){
	return block_len() + (pos-1)*key_len();
}

int BPlusTree::ptr_offset(int pos){
	return block_len() + /* number of keys + */ (pos-1)*ptr_len();
}