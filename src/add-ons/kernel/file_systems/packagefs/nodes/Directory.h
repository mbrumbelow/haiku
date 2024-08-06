/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"

#include <lock.h>
#include <AutoLocker.h>


struct DirectoryIterator : DoublyLinkedListLinkImpl<DirectoryIterator> {
	Node*	node;

	DirectoryIterator()
		:
		node(NULL)
	{
	}
};

typedef DoublyLinkedList<DirectoryIterator> DirectoryIteratorList;


class Directory : public Node {
public:
								Directory(ino_t id);
	virtual						~Directory();

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

	virtual	status_t			Init(Directory* parent, const String& name);

	virtual	mode_t				Mode() const;
	virtual	off_t				FileSize() const;

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer, size_t* bufferSize);

			void				AddChild(Node* node);
			void				RemoveChild(Node* node);
			Node*				FindChild(const StringKey& name);

	inline	Node*				FirstChild() const;
	inline	Node*				NextChild(Node* node) const;

			void				AddDirectoryIterator(
									DirectoryIterator* iterator);
			void				RemoveDirectoryIterator(
									DirectoryIterator* iterator);

private:
			rw_lock				fLock;
			NodeNameHashTable	fChildTable;
			NodeList			fChildList;
			DirectoryIteratorList fIterators;
};


bool
Directory::ReadLock()
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Directory::ReadUnlock()
{
	rw_lock_read_unlock(&fLock);
}


bool
Directory::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
Directory::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


Node*
Directory::FirstChild() const
{
	return fChildList.First();
}


Node*
Directory::NextChild(Node* node) const
{
	return fChildList.GetNext(node);
}


struct NodeReadLocker : public AutoLocker<Directory, AutoLockerReadLocking<Directory> > {
	NodeReadLocker() : AutoLocker() {}
	NodeReadLocker(Directory* node) : AutoLocker(node) {}
	NodeReadLocker(Node* node) : AutoLocker(node->Parent()) {}
};
struct NodeWriteLocker : public AutoLocker<Directory, AutoLockerWriteLocking<Directory> > {
	NodeWriteLocker() : AutoLocker() {}
	NodeWriteLocker(Directory* node) : AutoLocker(node) {}
	NodeWriteLocker(Node* node) : AutoLocker(node->Parent()) {}
};


#endif	// DIRECTORY_H
