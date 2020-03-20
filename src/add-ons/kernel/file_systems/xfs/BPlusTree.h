#ifndef _BPLUS_TREE_H_
#define _BPLUS_TREE_H_

#include "system_dependencies.h"

/* Allocation B+ Tree Format */
#define XFS_ABTB_MAGICNUM 0x41425442	// For block offset B+Tree
#define XFS_ABTC_MAGICNUM 0x41425443	// For block count B+ Tree


/* Header for Short Format btree */
#define XFS_BTREE_SBLOCK_SIZE	18

/*
* Headers are the "nodes" really and are called "blocks". The records, keys and 
* pts are calculated using helpers
*/

struct bplustree_short_block {
			uint32				bb_magic;
			uint16				bb_level;
			uint16				bb_numrecs;
			uint32				bb_leftsib;
			uint32				bb_rightsib;
			
			void				swap_endian();
			
			uint32				magic() 
								{ return bb_magic; }
			
			uint16				level() 
								{ return bb_level; }
			
			uint16				numrecs()
								{ return bb_numrecs; }

			xfs_alloc_ptr_t		left()
								{ return bb_leftsib; }

			xfs_alloc_ptr_t		right()
								{ return bb_rightsib;}
}


/* Header for Long Format btree */
#define XFS_BTREE_LBLOCK_SIZE	24
struct bplustree_long_block {
			uint32				bb_magic;
			uint16				bb_level;
			uint16				bb_numrecs;
			uint64				bb_leftsib;
			uint64				bb_rightsib;

			void				swap_endian();

			uint32				magic() 
								{ return bb_magic; }
			
			uint16				level() 
								{ return bb_level; }
			
			uint16				numrecs()
								{ return bb_numrecs; }

			xfs_alloc_ptr_t		left()
								{ return bb_leftsib; }

			xfs_alloc_ptr_t		right()
								{ return bb_rightsib;}
}


/* Array of these records in the leaf node along with above headers */
#define XFS_ALLOC_REC_SIZE	8
typedef struct xfs_alloc_rec {
			uint32				ar_startblock;
			uint32				ar_blockcount;
			
			void				swap_endian();
			
			uint32				startblock(){
								return ar_startblock;
								}
			uint32				blockcount(){
								return ar_blockcount;
								}
} xfs_alloc_rec_t, xfs_alloc_key_t;

// Swap Endians while returning itself
typedef uint32 xfs_alloc_ptr_t;	//  Node pointers, AG relative block pointer

#define ALLOC_FLAG 0x1

#define LONG_BLOCK_FLAG 0x1
#define SHORT_BLOCK_FLAG 0x2

union btree_ptr{
	bplustree_long_block long_block;
	bplustree_short_block short_block;
}

union btree_key{
	xfs_alloc_key_t alloc;
}

union btree_rec{
	xfs_alloc_rec_t alloc;
}

class BPlusTree{
			Volume*				fVolume;
			xfs_agnumber_t		agnumber;
			btree_ptr*			root;
			int					rec_type;
			int					key_type;
			int					ptr_type;

	public:
			uint32				BlockSize();
			int					RecordSize();
			int					MaxRecords(bool leaf);
			int					KeyLen();
			int					BlockLen();
			int					PtrLen();
			int					RecordOffset(int pos); // get the pos'th record
			int					KeyOffset(int pos); // get the pos'th key
			int					PtrOffset(int pos); // get the pos'th ptr
}

#endif