/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <fs_interface.h>

#include <AutoLocker.h>

#include <lock.h>
#include <util/AVLTree.h>
#include <util/OpenHashTable.h>

#include "InlineReferenceable.h"
#include "String.h"
#include "StringKey.h"


class AttributeCookie;
class AttributeDirectoryCookie;
class AttributeIndexer;
class Directory;
class PackageNode;


// internal node flags
enum {
	NODE_FLAG_KNOWN_TO_VFS		= 0x01,
	NODE_FLAG_VFS_INIT_ERROR	= 0x02,
		// used by subclasses
};


class Node : public AVLTreeNode {
public:
								Node(ino_t id);
	virtual						~Node();

			void				AcquireReference();
			void				ReleaseReference();

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

			ino_t				ID() const		{ return fID; }
			Directory*			Parent() const	{ return fParent; }
			const String&		Name() const	{ return fName; }

			Node*&				IDHashTableNext()
									{ return fIDHashTableNext; }

	virtual	status_t			Init(Directory* parent, const String& name);

	virtual	status_t			VFSInit(dev_t deviceID);
									// base class version must be called on
									// success
	virtual	void				VFSUninit();
									// base class version must be called
	inline	bool				IsKnownToVFS() const;
	inline	bool				HasVFSInitError() const;

			void				SetID(ino_t id);
			void				SetParent(Directory* parent);

	virtual	mode_t				Mode() const = 0;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const = 0;
	virtual	off_t				FileSize() const = 0;

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize) = 0;
	virtual	status_t			Read(io_request* request) = 0;

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize) = 0;

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const StringKey& name,
									int openMode, AttributeCookie*& _cookie);

	virtual	status_t			IndexAttribute(AttributeIndexer* indexer);
	virtual	void*				IndexCookieForAttribute(const StringKey& name)
									const;

protected:
			rw_lock				fLock;
			ino_t				fID;
			Directory*			fParent;
			String				fName;
			Node*				fIDHashTableNext;
			uint32				fFlags;
			InlineReferenceable	fReferenceable;
};


bool
Node::ReadLock()
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Node::ReadUnlock()
{
	rw_lock_read_unlock(&fLock);
}


bool
Node::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
Node::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


bool
Node::IsKnownToVFS() const
{
	return (fFlags & NODE_FLAG_KNOWN_TO_VFS) != 0;
}


bool
Node::HasVFSInitError() const
{
	return (fFlags & NODE_FLAG_VFS_INIT_ERROR) != 0;
}


// #pragma mark -


struct NodeNameTreeDefinition {
	typedef StringKey	Key;
	typedef	Node		Value;

	AVLTreeNode* GetAVLTreeNode(Value* value) const
	{
		return value;
	}

	Value* GetValue(AVLTreeNode* node) const
	{
		return static_cast<Value*>(node);
	}

	int Compare(const Key& a, const Value* _b) const
	{
		return a.Compare(_b->Name());
	}

	int Compare(const Value* a, const Value* b) const
	{
		return Compare(a->Name(), b);
	}
};


struct NodeIDHashDefinition {
	typedef ino_t	KeyType;
	typedef	Node	ValueType;

	size_t HashKey(ino_t key) const
	{
		return (uint64)(key >> 32) ^ (uint32)key;
	}

	size_t Hash(const Node* value) const
	{
		return HashKey(value->ID());
	}

	bool Compare(ino_t key, const Node* value) const
	{
		return value->ID() == key;
	}

	Node*& GetLink(Node* value) const
	{
		return value->IDHashTableNext();
	}
};

typedef AVLTree<NodeNameTreeDefinition> NodeNameAVLTree;
typedef BOpenHashTable<NodeIDHashDefinition> NodeIDHashTable;

typedef AutoLocker<Node, AutoLockerReadLocking<Node> > NodeReadLocker;
typedef AutoLocker<Node, AutoLockerWriteLocking<Node> > NodeWriteLocker;


#endif	// NODE_H
