/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <fs_interface.h>

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>
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


class Node : public DoublyLinkedListLinkImpl<Node> {
public:
								Node(ino_t id);
	virtual						~Node();

			void				AcquireReference();
			void				ReleaseReference();
			int32				CountReferences();

			BReference<Directory> GetParent() const;
			Directory*			GetParentUnchecked() const	{ return fParent; }

			ino_t				ID() const		{ return fID; }
			const String&		Name() const	{ return fName; }

			Node*&				NameHashTableNext()
									{ return fNameHashTableNext; }
			Node*&				IDHashTableNext()
									{ return fIDHashTableNext; }

	virtual	status_t			Init(const String& name);
			void				SetID(ino_t id);

	virtual	status_t			VFSInit(dev_t deviceID);
									// base class version must be called on
									// success
	virtual	void				VFSUninit();
									// base class version must be called
	inline	bool				IsKnownToVFS() const;
	inline	bool				HasVFSInitError() const;

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

private:
			friend class Directory;

			void				_SetParent(Directory* parent);

protected:
			ino_t				fID;
			Directory*			fParent;
			String				fName;
			Node*				fNameHashTableNext;
			Node*				fIDHashTableNext;
			uint32				fFlags;
			InlineReferenceable	fReferenceable;
};


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


struct NodeNameHashDefinition {
	typedef StringKey	KeyType;
	typedef	Node		ValueType;

	size_t HashKey(const StringKey& key) const
	{
		return key.Hash();
	}

	size_t Hash(const Node* value) const
	{
		return value->Name().Hash();
	}

	bool Compare(const StringKey& key, const Node* value) const
	{
		return key == value->Name();
	}

	Node*& GetLink(Node* value) const
	{
		return value->NameHashTableNext();
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

typedef DoublyLinkedList<Node> NodeList;

typedef BOpenHashTable<NodeNameHashDefinition> NodeNameHashTable;
typedef BOpenHashTable<NodeIDHashDefinition> NodeIDHashTable;


#endif	// NODE_H
