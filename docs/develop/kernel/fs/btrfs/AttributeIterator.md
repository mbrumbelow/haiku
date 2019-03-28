# AttributeIterator
Class used to iterate through the attributes of a given inode.
# Public members
## AttributeIterator(Inode\* inode)
Constructs an AttributeIterator object for the Inode object pointed by inode.
## ~AttributeIterator()
## status_t	InitCheck()
Check if the fIterator pointer is valid and return B_OK if it is, B_NO_MEMORY
otherwise.
## status_t GetNext(char\* name, size_t\* \_nameLength)
Copies the details of the next Attribute into \*name and \_nameLength.
## status_t	Rewind()
Resets fIterator and fOffset.
# Private members
## uint64	fOffset;
Value of current offset from beginning of Attribute list.
## Inode\*	fInode;
Pointer to Inode object corresponding to the AttributeIterator object.
## TreeIterator\*	fIterator;
Pointer to the beginning of the current Attribute.
