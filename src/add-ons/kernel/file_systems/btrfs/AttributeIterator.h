/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef ATTRIBUTEITERATOR_H
#define ATTRIBUTEITERATOR_H


#include "BTree.h"
#include "Inode.h"

//! Used to traverse through attributes of a given inode
class AttributeIterator {
public:
			//! Constructs an AttributeIterator object for Inode object *inode
								AttributeIterator(Inode* inode);
								~AttributeIterator();
			/*! Check if fIterator pointer is valid and returns B_OK if it is, 
				B_NO_MEMORY otherwise
			 */
			status_t			InitCheck();

			//! Copy details of next Attribute into *name and *_nameLength
			status_t			GetNext(char* name, size_t* _nameLength);
			//! Reset fIterator and fOffset
			status_t			Rewind();
private:
			//! Value of current offset from beginning of Attribute list
			uint64				fOffset;
			//! Pointer to Inode object corresponding to AttributeIterator
			Inode* 				fInode;
			//! Pointer to beginning of current Attribute
			TreeIterator*		fIterator;
};


#endif	// ATTRIBUTEITERATOR_H
