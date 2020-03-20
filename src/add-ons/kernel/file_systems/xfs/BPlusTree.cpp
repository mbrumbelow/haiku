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


uint32 BPlusTree::BlockSize(){
	return fVolume.SuperBlock().BlockSize();
}


int BPlusTree::RecordSize(){
	if(rec_type == ALLOC_FLAG) return XFS_ALLOC_REC_SIZE;
}


int BPlusTree::MaxRecords(bool leaf){
	int block_len = BlockSize();
	if(ptr_type == SHORT_BLOCK_FLAG) block_len - XFS_BTREE_SBLOCK_SIZE;
	if(leaf){
		if(rec_type == ALLOC_FLAG) return block_len/sizeof(xfs_alloc_rec_t);
	}
	else{
		if(key_type) == ALLOC_FLAG){
			return block_len/(sizeof(xfs_alloc_key_t)
							+ sizeof(xfs_alloc_ptr_t));
		}
	}
}


int BPlusTree::KeyLen(){
	if(key_type == ALLOC_FLAG) return XFS_ALLOC_REC_SIZE;
}


int BPlusTree::BlockLen(){
	if(ptr_type == LONG_BLOCK_FLAG) return XFS_BTREE_LBLOCK_SIZE;
	else return XFS_BTREE_SBLOCK_SIZE;
}


int BPlusTree::PtrLen(){
	if(ptr_type == LONG_BLOCK_FLAG) return sizeof(uint64);
	else return sizeof(uint32);
}


int BPlusTree::RecordOffset(int pos){
	return block_len() + (pos-1)*rec_size();
}


int BPlusTree::KeyOffset(int pos){
	return block_len() + (pos-1)*key_len();
}


int BPlusTree::PtrOffset(int pos, int level){
	return block_len() + MaxRecords(level>0)*KeyLen() + (pos-1)*ptr_len();
}