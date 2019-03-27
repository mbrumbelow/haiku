# BTRFS overview
This file contains a short summary of each file under the
src/add-ons/kernel/file_systems/btrfs directory. See individual documents for
specific information.
## Attribute.h
Contains declaration for Attribute, a class used to represent file attributes.
Also contains the declaration for attr_cookie, an in-memory representation of an
Attribute object.
## Attribute.cpp
Contains the implementations for the class Attribute.
## AttributeIterator.h
Contains the declartion for the class AttributeIterator, a class used to 
iterate through the attributes of a file.
## AttributeIterator.cpp
Contains the implementation for the class AttributeIterator.
## BTree.h
Contains declartions for the classes BTree, Path, Node and TreeIterator which
are used to represent a b-tree, a path along a b-tree, a node in a b-tree and a
class used to iterate through a b-tree respectively. Also contains the
declaration for btree_traversing and node_and_key, an enumaration used to 
represent the direction in which iteration should occur and a structure used for
searching.
## BTree.cpp
Contains the implementation for Node, Path, BTree and TreeIterator classes
## btrfs.h
Contains declarations for on-disk data structures. Specifically, it contains the
declarations for the following structures
* btrfs_backup_roots - contains super_roots
* btrfs_key - used to locate items in a tree
* btrfs_timespec - used to represent access, change and modification times
* btrfs_header - used to identify the tree to which a block (node or leaf) 
  belongs. This is always the first section in a node.
* btrfs_index - in internal nodes, the node header is followed by a number of 
  key pointers used for tree traversing.
* btrfs_entry - in leaf nodes, the node header is followed by a number of items.
  The items' data is stored at the end of the node, and the contents of the 
  item data depends on the item type stored in the key. 
* btrfs_stream - it represents a tree node, which can be either an internal node
  (btrfs_index) or a leaf node (btrfs_entry) depending on the item's type.
* btrfs_stripe - used to define the backing device storage that compose a btrfs 
  chunk.
* btrfs_chunk - contains the mapping from a virtualized usable byte range 
  within the backing storage to a set of one or more stripes on individual 
  backing devices
* btrfs_device - represents a complete block device
* btrfs_super_block - contains the metadata describing the filesystem
* btrfs_inode -  contains information associated with a inode
* btrfs_inode_ref - contains the mapping for a inode to a name in a directory
* btrfs_root - represents to root of a b-tree
* btrfs_dir_entry - represents the header for a directory entry item
* btrfs_extent_data - represents a chunk of data on disk belonging to a file
* btrfs_block_group - used to define location, properties, and usage of a block group
* btrfs_extent - used as the header for extent record items
* btrfs_extent_inline_ref - used as as the header for several types of inline 
  extent back references
* btrfs_extent_data_ref - contains an indirect back reference for a file data 
  extent

For more information on these structures see 
[Data Structures](https://btrfs.wiki.kernel.org/index.php/Data_Structures) and 
[On-disk Format](https://btrfs.wiki.kernel.org/index.php/On-disk_Format) on the
[btrfs wiki](https://btrfs.wiki.kernel.org/index.php).

It also contains declarations for various internal symbolic constants.
## CachedBlock.h
Contains the declaration and implementation of the class CachedBlock, which is
used to deal with in-memory blocks.
## Chunk.h
Contains the declaration for the class Chunk, which is used to deal with
in-memory chunks.
## Chunk.cpp
Contains the implementation for the class Chunk.
## crc_table.cpp
Contains code to generate CRC 03667067501 table which is used for checksums.
## CRCTable.h
Contains the declaration for calculate_crc, a method use to calculate checksum.
## CRCTable.cpp
Contains the implementation for calculate_crc and CRC 03667067501 table.
## DirectoryIterator.h
Contains the declaration for the class DirectoryIterator, a class used to
iterate through a directory.
## DirectoryIterator.cpp
Contains the implementation for the class DirectoryIterator.
## ExtentAllocator.h
Contains the declarations for the structures CachedExtent, TreeDefinition,
CachedExtentTree and for the classes BlockGroup and ExtentAllocator.
## ExtentAllocator.cpp
Contains the implementation for CachedExtent, CachedExtentTree, BlockGroup and 
ExtentAllocator.
## Inode.h
Contains the declaration for the class Inode and the declaration and
implementation for the class Vnode.
## Inode.cpp
Contains the implementation for the class Inode.
## Journal.h
Contains the declarations for the classes Journal and Transaction.
## Journal.cpp
Contains the implementation for the classes Journal and Transaction.
## kernel_interface.cpp
Contains the implementation for various functions declared in fs_interface.h
## system_dependencies.h
Contains a list of #includes required by various files.
## Utility.h
Contains the helper functions get_filetype and open_mode_to_access.
## Volume.h
Contains the declaration for the class Volume.
## Volume.cpp
Contains the implementation for the class Volume and the declaration and
implementation for the class DeviceOpener.
