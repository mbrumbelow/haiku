// kernel_interface.cpp
//
// Copyright (c) 2003-2008, Axel Dörfler (axeld@pinc-software.de)
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

#include <fs_index.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_query.h>
#include <fs_volume.h>
#include <fs_ops_support.h>
#include <vfs.h>
#include <KernelExport.h>
#include <NodeMonitor.h>
#include <TypeConstants.h>

#include <AutoDeleter.h>

#include "AllocationInfo.h"
#include "AttributeIndex.h"
#include "AttributeIterator.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "File.h"
#include "Index.h"
#include "IndexDirectory.h"
#include "Locking.h"
#include "Misc.h"
#include "Node.h"
#include "Query.h"
#include "ramfs_ioctl.h"
#include "SpecialNode.h"
#include "SymLink.h"
#include "Volume.h"


static const size_t kOptimalIOSize = 65536;
static const bigtime_t kNotificationInterval = 1000000LL;


// notify_if_stat_changed
void
notify_if_stat_changed(Volume *volume, Node *node)
{
	if (volume && node && node->IsModified()) {
		uint32 statFields = node->MarkUnmodified();
		notify_stat_changed(volume->GetID(), -1, node->GetID(), statFields);
	}
}


// #pragma mark - FS


static status_t
ramfs_mount(fs_volume* _volume, const char* /*device*/, uint32 flags,
	const char* /*args*/, ino_t* _rootID)
{
	FUNCTION_START();
	// parameters are ignored for now

	// fail, if read-only mounting is requested
	if (flags & B_MOUNT_READ_ONLY)
		return B_BAD_VALUE;

	// allocate and init the volume
	Volume *volume = new(std::nothrow) Volume(_volume);

	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status = volume->Mount(flags);

	if (status != B_OK) {
		delete volume;
		RETURN_ERROR(status);
	}

	*_rootID = volume->GetRootDirectory()->GetID();
	_volume->private_volume = volume;
	_volume->ops = &gRamFSVolumeOps;

	RETURN_ERROR(B_OK);
}


static status_t
ramfs_unmount(fs_volume* _volume)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	status_t error = B_OK;
	if (volume->WriteLock()) {
		error = volume->Unmount();
		if (error == B_OK)
			delete volume;
	} else
		SET_ERROR(error, B_ERROR);
	RETURN_ERROR(error);
}


static status_t
ramfs_read_fs_info(fs_volume* _volume, struct fs_info *info)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	info->flags =  B_FS_HAS_ATTR | B_FS_HAS_MIME | B_FS_HAS_QUERY
		| B_FS_IS_REMOVABLE;
	info->block_size = B_PAGE_SIZE;
	info->io_size = kOptimalIOSize;
	info->total_blocks = volume->CountBlocks();
	info->free_blocks = volume->CountFreeBlocks();
	info->device_name[0] = '\0';
	strlcpy(info->volume_name, volume->GetName(),
		sizeof(info->volume_name));
	strcpy(info->fsh_name, "ramfs");
	return B_OK;
}


static status_t
ramfs_write_fs_info(fs_volume* _volume, const struct fs_info *info, uint32 mask)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	if (mask & FS_WRITE_FSINFO_NAME)
		error = volume->SetName(info->volume_name);

	RETURN_ERROR(error);
}


// ramfs_sync
static status_t
ramfs_sync(fs_volume* /*fs*/)
{
	FUNCTION_START();
	return B_OK;
}


// #pragma mark - VNodes


static status_t
ramfs_lookup(fs_volume* _volume, fs_vnode* _dir, const char* entryName,
	ino_t* _vnodeID)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	FUNCTION(("dir: (%llu), entry: `%s'\n", (dir ? dir->GetID() : -1),
		entryName));

	// check for non-directories
	if (dir == NULL)
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	Node *node = NULL;

	// special entries: "." and ".."
	if (!strcmp(entryName, ".")) {
		*_vnodeID = dir->GetID();
		if (volume->GetVNode(*_vnodeID, &node) != B_OK)
			error = B_BAD_VALUE;
	} else if (!strcmp(entryName, "..")) {
		Directory *parent = dir->GetParent();
		if (parent && volume->GetVNode(parent->GetID(), &node) == B_OK)
			*_vnodeID = node->GetID();
		else
			error = B_BAD_VALUE;

	// ordinary entries
	} else {
		// find the entry
		error = dir->FindAndGetNode(entryName, &node);
		if (error == B_OK)
			*_vnodeID = node->GetID();
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_get_vnode(fs_volume* _volume, ino_t vnid, fs_vnode* node, int* _type,
	uint32* _flags, bool reenter)
{
	FUNCTION(("node: %lld\n", vnid));
	Volume* volume = (Volume*)_volume->private_volume;
	Node *foundNode = NULL;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = volume->FindNode(vnid, &foundNode);
	if (error == B_OK) {
		node->private_node = foundNode;
		node->ops = &gRamFSVnodeOps;
		*_type = foundNode->GetMode();
		*_flags = 0;
	}
	RETURN_ERROR(error);
}


static status_t
ramfs_write_vnode(fs_volume* /*fs*/, fs_vnode* DARG(_node), bool /*reenter*/)
{
// DANGER: If dbg_printf() is used, this thread will enter another FS and
// even perform a write operation. The is dangerous here, since this hook
// may be called out of the other FSs, since, for instance a put_vnode()
// called from another FS may cause the VFS layer to free vnodes and thus
// invoke this hook.
//	FUNCTION_START();
//FUNCTION(("node: %lld\n", ((Node*)_node)->GetID()));
	status_t error = B_OK;
	RETURN_ERROR(error);
}


static status_t
ramfs_remove_vnode(fs_volume* _volume, fs_vnode* _node, bool /*reenter*/)
{
	FUNCTION(("node: %lld\n", ((Node*)_node)->GetID()));
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	volume->NodeRemoved(node);
	delete node;
	RETURN_ERROR(error);
}


// #pragma mark - Nodes


static status_t
ramfs_ioctl(fs_volume* _volume, fs_vnode* /*node*/, void* /*cookie*/,
	uint32 cmd, void *buffer, size_t /*length*/)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;

	status_t error = B_OK;
	switch (cmd) {
		case RAMFS_IOCTL_GET_ALLOCATION_INFO:
		{
			if (buffer) {
				VolumeReadLocker locker(volume);
				if (!locker.IsLocked()) {
					AllocationInfo *info = (AllocationInfo*)buffer;
					volume->GetAllocationInfo(*info);
				} else
					SET_ERROR(error, B_ERROR);
			} else
				SET_ERROR(error, B_BAD_VALUE);
			break;
		}
		case RAMFS_IOCTL_DUMP_INDEX:
		{
			if (buffer) {
				VolumeReadLocker locker(volume);
				if (!locker.IsLocked()) {
					const char *name = (const char*)buffer;
PRINT("  RAMFS_IOCTL_DUMP_INDEX, `%s'\n", name);
					IndexDirectory *indexDir = volume->GetIndexDirectory();
					if (indexDir) {
						if (Index *index = indexDir->FindIndex(name))
							index->Dump();
						else
							SET_ERROR(error, B_ENTRY_NOT_FOUND);
					} else
						SET_ERROR(error, B_ENTRY_NOT_FOUND);
				} else
					SET_ERROR(error, B_ERROR);
			} else
				SET_ERROR(error, B_BAD_VALUE);
			break;
		}
		default:
			error = B_DEV_INVALID_IOCTL;
			break;
	}
	RETURN_ERROR(error);
}


static status_t
ramfs_set_flags(fs_volume* /*fs*/, fs_vnode* /*node*/, void* /*cookie*/,
	int /*flags*/)
{
	FUNCTION_START();
	// TODO : ramfs_set_flags
	return B_OK;
}


static status_t
ramfs_fsync(fs_volume* /*fs*/, fs_vnode* /*node*/, bool dataOnly)
{
	FUNCTION_START();
	return B_OK;
}


static status_t
ramfs_read_symlink(fs_volume* _volume, fs_vnode* _node, char *buffer,
	size_t *bufferSize)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// read symlinks only
	if (!node->IsSymLink())
		error = B_BAD_VALUE;
	if (error == B_OK) {
		if (SymLink *symLink = dynamic_cast<SymLink*>(node)) {
			// copy the link contents
			size_t toRead = min(*bufferSize,
								symLink->GetLinkedPathLength());
			if (toRead > 0)
				memcpy(buffer, symLink->GetLinkedPath(),
					toRead);

			*bufferSize = symLink->GetLinkedPathLength();
		} else {
			FATAL("Node %" B_PRIdINO " pretends to be a SymLink, but isn't!\n",
				node->GetID());
			error = B_BAD_VALUE;
		}
	}
	RETURN_ERROR(error);
}


static status_t
ramfs_create_symlink(fs_volume* _volume, fs_vnode* _dir, const char *name,
	const char *path, int mode)
{
	FUNCTION(("name: `%s', path: `%s'\n", name, path));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	if (name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);
	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	NodeMTimeUpdater mTimeUpdater(dir);
	// directory deleted?
	bool removed;
	if (get_vnode_removed(volume->FSVolume(), dir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}

	// check directory write permissions
	error = dir->CheckPermissions(W_OK);
	Node *node = NULL;
	if (error == B_OK) {
		// check if entry does already exist
		if (dir->FindNode(name, &node) == B_OK) {
			SET_ERROR(error, B_FILE_EXISTS);
		} else {
			// entry doesn't exist: create a symlink
			SymLink *symLink = NULL;
			error = dir->CreateSymLink(name, path, &symLink);
			if (error == B_OK) {
				node = symLink;
				// set permissions, owner and group
				node->SetMode(mode);
				node->SetUID(geteuid());
				node->SetGID(getegid());
				// put the node
				volume->PutVNode(node);
			}
		}
	}
	NodeMTimeUpdater mTimeUpdater2(node);
	// notify listeners
	if (error == B_OK) {
		notify_entry_created(volume->GetID(), dir->GetID(), name,
			node->GetID());
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_link(fs_volume* _volume, fs_vnode* _dir, const char *name,
	fs_vnode* _node)
{
	FUNCTION(("name: `%s'\n", name));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);
	Node* node = (Node*)_node->private_node;

	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	NodeMTimeUpdater mTimeUpdater(dir);
	// directory deleted?
	bool removed;
	if (get_vnode_removed(volume->FSVolume(), dir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}
	// check directory write permissions
	error = dir->CheckPermissions(W_OK);
	Entry *entry = NULL;
	if (error == B_OK) {
		// check if entry does already exist
		if (dir->FindEntry(name, &entry) == B_OK) {
			SET_ERROR(error, B_FILE_EXISTS);
		} else {
			// entry doesn't exist: create a link
			error = dir->CreateEntry(node, name);
		}
	}
	// notify listeners
	if (error == B_OK) {
		notify_entry_created(volume->GetID(), dir->GetID(), name,
			node->GetID());
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_unlink(fs_volume* _volume, fs_vnode* _dir, const char *name)
{
	FUNCTION(("name: `%s'\n", name));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);
	status_t error = B_OK;

	if (name == NULL || *name == '\0' || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		RETURN_ERROR(B_BAD_VALUE);
	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(dir);
	// check directory write permissions
	error = dir->CheckPermissions(W_OK);
	ino_t nodeID = -1;
	if (error == B_OK) {
		// check if entry exists
		Node *node = NULL;
		Entry *entry = NULL;
		if (dir->FindAndGetNode(name, &node, &entry) == B_OK) {
			nodeID = node->GetID();
			// unlink the entry, if it isn't a non-empty directory
			if (node->IsDirectory()
				&& !dynamic_cast<Directory*>(node)->IsEmpty()) {
				SET_ERROR(error, B_DIRECTORY_NOT_EMPTY);
			} else
				error = dir->DeleteEntry(entry);
			volume->PutVNode(node);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	}
	// notify listeners
	if (error == B_OK)
		notify_entry_removed(volume->GetID(), dir->GetID(), name, nodeID);

	RETURN_ERROR(error);
}


static status_t
ramfs_rename(fs_volume* _volume, fs_vnode* _oldDir, const char *oldName,
	fs_vnode* _newDir, const char *newName)
{
	Volume* volume = (Volume*)_volume->private_volume;

	Directory* oldDir = dynamic_cast<Directory*>((Node*)_oldDir->private_node);
	Directory* newDir = dynamic_cast<Directory*>((Node*)_newDir->private_node);

	FUNCTION(("old dir: %lld, old name: `%s', new dir: %lld, new name: `%s'\n",
		oldDir->GetID(), oldName, newDir->GetID(), newName));

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;

	NodeMTimeUpdater mTimeUpdater1(oldDir);
	NodeMTimeUpdater mTimeUpdater2(newDir);

	// target directory deleted?
	bool removed;
	if (get_vnode_removed(volume->FSVolume(), newDir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}

	// check directory write permissions
	if (error == B_OK)
		error = oldDir->CheckPermissions(W_OK);
	if (error == B_OK)
		error = newDir->CheckPermissions(W_OK);

	Node *node = NULL;
	Entry *entry = NULL;
	if (error == B_OK) {
		// check if entry exists
		if (oldDir->FindAndGetNode(oldName, &node, &entry) != B_OK) {
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
		} else {
			if (oldDir != newDir) {
				// check whether the entry is a descendent of the target
				// directory
				for (Directory *parent = newDir; parent != NULL;
						parent = parent->GetParent()) {
					if (parent == node) {
						error = B_BAD_VALUE;
						break;
					} else if (parent == oldDir)
						break;
				}
			}
		}

		// check the target directory situation
		Node *clobberNode = NULL;
		Entry *clobberEntry = NULL;
		if (error == B_OK) {
			if (newDir->FindAndGetNode(newName, &clobberNode,
					&clobberEntry) == B_OK) {
				if (clobberNode->IsDirectory()
						&& !dynamic_cast<Directory*>(clobberNode)->IsEmpty()) {
					SET_ERROR(error, B_NAME_IN_USE);
				}
			}
		}

		// do the job
		if (error == B_OK) {
			// temporarily acquire an additional reference to make
			// sure the node isn't deleted when we remove the entry
			error = node->AddReference();
			if (error == B_OK) {
				// delete the original entry
				error = oldDir->DeleteEntry(entry);
				if (error == B_OK) {
					// create the new one/relink the target entry
					if (clobberEntry != NULL) {
						clobberEntry->Unlink();
						error = clobberEntry->Link(node);
					} else
						error = newDir->CreateEntry(node, newName);

					if (error == B_OK) {
						// send a "removed" notification for the clobbered
						// entry
						if (clobberEntry) {
							notify_entry_removed(volume->GetID(),
								newDir->GetID(), newName,
								clobberNode->GetID());
						}
					} else {
						// try to recreate the original entry, in case of
						// failure
						newDir->CreateEntry(node, oldName);
					}
				}
				node->RemoveReference();
			}
		}

		// release the entries
		if (clobberEntry)
			volume->PutVNode(clobberNode);
		if (entry)
			volume->PutVNode(node);
	}

	// notify listeners
	if (error == B_OK) {
		notify_entry_moved(volume->GetID(), oldDir->GetID(), oldName,
			newDir->GetID(), newName, node->GetID());
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_access(fs_volume* _volume, fs_vnode* _node, int mode)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = node->CheckPermissions(mode);
	RETURN_ERROR(error);
}


static status_t
ramfs_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat *st)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	FUNCTION(("node: %lld\n", node->GetID()));

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	st->st_dev = volume->GetID();
	st->st_ino = node->GetID();
	st->st_mode = node->GetMode();
	st->st_nlink = node->GetRefCount();
	st->st_uid = node->GetUID();
	st->st_gid = node->GetGID();
	st->st_size = node->GetSize();
	st->st_blksize = kOptimalIOSize;
	st->st_blocks = (st->st_size + st->st_blksize - 1) / st->st_blksize;
	st->st_atime = node->GetATime();
	st->st_mtime = node->GetMTime();
	st->st_ctime = node->GetCTime();
	st->st_crtime = node->GetCrTime();

	RETURN_ERROR(B_OK);
}


static status_t
ramfs_write_stat(fs_volume* _volume, fs_vnode* _node, const struct stat *st,
	uint32 mask)
{
	FUNCTION(("mask: %lx\n", mask));

	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	if (check_write_stat_permissions(node->GetGID(), node->GetUID(), node->GetMode(),
			mask, st) != B_OK)
		RETURN_ERROR(B_NOT_ALLOWED);

	NodeMTimeUpdater mTimeUpdater(node);
	status_t error = B_OK;

	if ((mask & B_STAT_SIZE) != 0)
		error = node->SetSize(st->st_size);

	if (error == B_OK) {
		// permissions
		if (mask & B_STAT_MODE) {
			node->SetMode((node->GetMode() & ~S_IUMSK)
				| (st->st_mode & S_IUMSK));
		}
		// UID
		if (mask & B_STAT_UID)
			node->SetUID(st->st_uid);
		// GID
		if (mask & B_STAT_GID)
			node->SetGID(st->st_gid);
		// mtime
		if (mask & B_STAT_MODIFICATION_TIME)
			node->SetMTime(st->st_mtime);
		// crtime
		if (mask & B_STAT_CREATION_TIME)
			node->SetCrTime(st->st_crtime);
	}

	// notify listeners
	if (error == B_OK)
		notify_if_stat_changed(volume, node);

	RETURN_ERROR(error);
}


// #pragma mark - Files


class FileCookie {
public:
	FileCookie(int openMode) : fOpenMode(openMode), fLastNotificationTime(0) {}

	inline int GetOpenMode() { return fOpenMode; }
	inline bigtime_t GetLastNotificationTime() { return fLastNotificationTime; }

	inline bool	NotificationIntervalElapsed(bool set = false)
	{
		bigtime_t currentTime = system_time();
		bool result = (currentTime
			- fLastNotificationTime > kNotificationInterval);

		if (set && result)
			fLastNotificationTime = currentTime;

		return result;
	}

private:
	int fOpenMode;
	bigtime_t fLastNotificationTime;
};


static status_t
ramfs_create(fs_volume* _volume, fs_vnode* _dir, const char *name, int openMode,
	int mode, void** _cookie, ino_t *vnid)
{
	FUNCTION(("name: `%s', open mode: %x, mode: %x\n", name, openMode, mode));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	if (name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);
	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		RETURN_ERROR(B_FILE_EXISTS);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(dir);
	status_t error = B_OK;

	// directory deleted?
	bool removed;
	if (get_vnode_removed(volume->FSVolume(), dir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}
	// create the file cookie
	FileCookie *cookie = NULL;
	if (error == B_OK) {
		cookie = new(nothrow) FileCookie(openMode);
		if (!cookie)
			SET_ERROR(error, B_NO_MEMORY);
	}
	Node *node = NULL;
	if (error == B_OK) {
		// check if entry does already exist
		if (dir->FindNode(name, &node) == B_OK) {
			// entry does already exist
			// fail, if we shall fail, when the file exists
			if (openMode & O_EXCL) {
				SET_ERROR(error, B_FILE_EXISTS);
			// don't create a file over an existing directory or symlink
			} else if (!node->IsFile()) {
				SET_ERROR(error, B_NOT_ALLOWED);
			// the user must have write permission for an existing entry
			} else if ((error = node->CheckPermissions(W_OK)) == B_OK) {
				// truncate, if requested
				if (openMode & O_TRUNC)
					error = node->SetSize(0);
				// we ignore the supplied permissions in this case
				// get vnode
				if (error == B_OK) {
					*vnid = node->GetID();
					error = volume->GetVNode(node->GetID(), &node);
				}
			}
		// the user must have dir write permission to create a new entry
		} else if ((error = dir->CheckPermissions(W_OK)) == B_OK) {
			// entry doesn't exist: create a file
			File *file = NULL;
			error = dir->CreateFile(name, &file);
			if (error == B_OK) {
				node = file;
				*vnid = node->GetID();
				// set permissions, owner and group
				node->SetMode(mode);
				node->SetUID(geteuid());
				node->SetGID(getegid());

				// set cache in vnode
				struct vnode* vnode;
				if (vfs_lookup_vnode(_volume->id, node->GetID(), &vnode) == B_OK) {
					vfs_set_vnode_cache(vnode, file->GetCache(vnode));
				}
			}
		}
		// set result / cleanup on failure
		if (error == B_OK)
			*_cookie = cookie;
		else if (cookie)
			delete cookie;
	}
	NodeMTimeUpdater mTimeUpdater2(node);
	// notify listeners
	if (error == B_OK)
		notify_entry_created(volume->GetID(), dir->GetID(), name, *vnid);

	RETURN_ERROR(error);
}


static status_t
ramfs_create_special_node(fs_volume *_volume, fs_vnode *_dir, const char *name,
	fs_vnode *subVnode, mode_t mode, uint32 flags, fs_vnode *_superVnode,
	ino_t *vnid)
{
	FUNCTION(("name: `%s', mode: %x\n", name, mode));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	if (name == NULL || subVnode != NULL)
		RETURN_ERROR(B_UNSUPPORTED);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		RETURN_ERROR(B_FILE_EXISTS);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(dir);
	status_t error = B_OK;

	// directory deleted?
	bool removed;
	if (get_vnode_removed(volume->FSVolume(), dir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}

	Node *existingNode = NULL;
	if (dir->FindNode(name, &existingNode) == B_OK)
		RETURN_ERROR(B_FILE_EXISTS);

	error = dir->CheckPermissions(W_OK);
	if (error != B_OK)
		RETURN_ERROR(error);

	SpecialNode* node = new(std::nothrow) SpecialNode(volume, mode);
	if (node == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*vnid = node->GetID();

	node->SetUID(geteuid());
	node->SetGID(getegid());

	Entry* entry;
	error = node->InitCheck();
	if (error == B_OK) {
		// add node to directory
		error = dir->CreateEntry(node, name, &entry);
	}
	if (error != B_OK) {
		delete node;
		RETURN_ERROR(error);
	}

	NodeMTimeUpdater mTimeUpdater2(node);

	// notify listeners
	notify_entry_created(volume->GetID(), dir->GetID(), name, *vnid);

	return B_OK;
}


static status_t
ramfs_open(fs_volume* _volume, fs_vnode* _node, int openMode, void** _cookie)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	FUNCTION(("node: %lld\n", node->GetID()));

	VolumeReadLocker readLocker(volume);
	if (!readLocker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// directory can be opened read-only
	if (node->IsDirectory() && (openMode & O_RWMASK) != O_RDONLY)
		error = B_IS_A_DIRECTORY;
	if (error == B_OK && (openMode & O_DIRECTORY) != 0 && !node->IsDirectory())
		error = B_NOT_A_DIRECTORY;

	int accessMode = open_mode_to_access(openMode);
	// check open mode against permissions
	if (error == B_OK)
		error = node->CheckPermissions(accessMode);
	// create the cookie
	FileCookie *cookie = NULL;
	if (error == B_OK) {
		cookie = new(nothrow) FileCookie(openMode);
		if (!cookie)
			SET_ERROR(error, B_NO_MEMORY);
	}
	readLocker.Unlock();

	// truncate if requested
	if (error == B_OK && (openMode & O_TRUNC)) {
		VolumeWriteLocker writeLocker(volume);

		error = node->SetSize(0);
		NodeMTimeUpdater mTimeUpdater(node);
	}

	// set result / cleanup on failure
	if (error == B_OK)
		*_cookie = cookie;
	else if (cookie)
		delete cookie;

	RETURN_ERROR(error);
}


static status_t
ramfs_close(fs_volume* _volume, fs_vnode* _node, void* /*cookie*/)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	FUNCTION(("node: %lld\n", node->GetID()));

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// notify listeners
	notify_if_stat_changed(volume, node);

	return error;
}


static status_t
ramfs_free_cookie(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	FileCookie *cookie = (FileCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
ramfs_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void *buffer, size_t *bufferSize)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;
	FileCookie *cookie = (FileCookie*)_cookie;

//	FUNCTION(("((%lu, %lu), %lld, %p, %lu)\n", node->GetDirID(),
//			  node->GetObjectID(), pos, buffer, *bufferSize));

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// don't read anything but files
	if (!node->IsFile())
		SET_ERROR(error, B_BAD_VALUE);

	// check, if reading is allowed
	int rwMode = cookie->GetOpenMode() & O_RWMASK;
	if (error == B_OK && rwMode != O_RDONLY && rwMode != O_RDWR)
		SET_ERROR(error, B_FILE_ERROR);

	// read
	if (error == B_OK) {
		if (File *file = dynamic_cast<File*>(node)) {
			error = file->ReadAt(pos, buffer, *bufferSize, bufferSize);
		} else {
			FATAL("Node %" B_PRIdINO " pretends to be a File, but isn't!\n",
				node->GetID());
			error = B_BAD_VALUE;
		}
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_write(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	const void *buffer, size_t *bufferSize)
{
//	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	FileCookie *cookie = (FileCookie*)_cookie;
//	FUNCTION(("((%lu, %lu), %lld, %p, %lu)\n", node->GetDirID(),
//			  node->GetObjectID(), pos, buffer, *bufferSize));

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// don't write anything but files
	if (!node->IsFile())
		SET_ERROR(error, B_BAD_VALUE);
	if (error == B_OK) {
		// check, if reading is allowed
		int rwMode = cookie->GetOpenMode() & O_RWMASK;
		if (error == B_OK && rwMode != O_WRONLY && rwMode != O_RDWR)
			SET_ERROR(error, B_FILE_ERROR);
		if (error == B_OK) {
			// reset the position, if opened in append mode
			if (cookie->GetOpenMode() & O_APPEND)
				pos = node->GetSize();
			// write
			if (File *file = dynamic_cast<File*>(node)) {
				error = file->WriteAt(pos, buffer, *bufferSize,
					bufferSize);
			} else {
				FATAL("Node %" B_PRIdINO " pretends to be a File, but isn't!\n",
					node->GetID());
				error = B_BAD_VALUE;
			}
		}
	}
	// notify listeners
	if (error == B_OK && cookie->NotificationIntervalElapsed(true))
		notify_if_stat_changed(volume, node);
	NodeMTimeUpdater mTimeUpdater(node);
	RETURN_ERROR(error);
}


// #pragma mark - Directories


class DirectoryCookie {
public:
	DirectoryCookie(Directory *directory = NULL)
		:
		fIterator(directory),
		fDotIndex(DOT_INDEX)
	{
	}

	void Unset() { fIterator.Unset(); }

	status_t Next()
	{
		status_t error = B_OK;
		if (fDotIndex < ENTRY_INDEX)
			fDotIndex++;
		if (fDotIndex == ENTRY_INDEX) {
			Entry* entry = NULL;
			error = fIterator.GetNext(&entry);
		}
		return error;
	}

	status_t GetCurrent(ino_t *nodeID, const char **entryName)
	{
		status_t error = B_OK;
		if (fDotIndex == DOT_INDEX) {
			// "."
			Node *entry = fIterator.GetDirectory();
			*nodeID = entry->GetID();
			*entryName = ".";
		} else if (fDotIndex == DOT_DOT_INDEX) {
			// ".."
			Directory *dir = fIterator.GetDirectory();
			if (dir->GetParent())
				*nodeID = dir->GetParent()->GetID();
			else
				*nodeID = dir->GetID();
			*entryName = "..";
		} else {
			// ordinary entries
			Entry *entry = fIterator.GetCurrent();
			if (entry == NULL)
				error = B_ENTRY_NOT_FOUND;
			if (error == B_OK) {
				*nodeID = entry->GetNode()->GetID();
				*entryName = entry->GetName();
			}
		}

		PRINT("EntryIterator<%p>::GetNext(): entry: %p (%" B_PRIdINO ")\n",
			this, fIterator.GetCurrent(),
			(fIterator.GetCurrent() != NULL
				? fIterator.GetCurrent()->GetNode()->GetID() : -1));
		return error;
	}

	status_t Rewind()
	{
		fDotIndex = DOT_INDEX;
		return fIterator.Rewind();
	}

	status_t Suspend() { return fIterator.Suspend(); }
	status_t Resume() { return fIterator.Resume(); }

private:
	enum {
		DOT_INDEX		= 0,
		DOT_DOT_INDEX	= 1,
		ENTRY_INDEX		= 2,
	};

private:
	EntryIterator	fIterator;
	uint32			fDotIndex;
};


static status_t
ramfs_create_dir(fs_volume* _volume, fs_vnode* _dir, const char *name, int mode)
{
	FUNCTION(("name: `%s', mode: %x\n", name, mode));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	if (name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);
	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		RETURN_ERROR(B_FILE_EXISTS);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(dir);
	// directory deleted?
	bool removed;
	status_t error = B_OK;
	if (get_vnode_removed(volume->FSVolume(), dir->GetID(), &removed)
			!= B_OK || removed) {
		SET_ERROR(error, B_NOT_ALLOWED);
	}

	// check directory write permissions
	error = dir->CheckPermissions(W_OK);
	Node *node = NULL;
	if (error == B_OK) {
		// check if entry does already exist
		if (dir->FindNode(name, &node) == B_OK) {
			SET_ERROR(error, B_FILE_EXISTS);
		} else {
			// entry doesn't exist: create a directory
			Directory *newDir = NULL;
			error = dir->CreateDirectory(name, &newDir);
			if (error == B_OK) {
				node = newDir;
				// set permissions, owner and group
				node->SetMode(mode);
				node->SetUID(geteuid());
				node->SetGID(getegid());
				// put the node
				volume->PutVNode(node);
			}
		}
	}
	NodeMTimeUpdater mTimeUpdater2(node);
	// notify listeners
	if (error == B_OK) {
		notify_entry_created(volume->GetID(), dir->GetID(), name,
			node->GetID());
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_remove_dir(fs_volume* _volume, fs_vnode* _dir, const char *name)
{
	FUNCTION(("name: `%s'\n", name));
	Volume* volume = (Volume*)_volume->private_volume;
	Directory* dir = dynamic_cast<Directory*>((Node*)_dir->private_node);

	if (name == NULL || *name == '\0' || !strcmp(name, ".") || !strcmp(name, ".."))
		RETURN_ERROR(B_BAD_VALUE);
	if (dir == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(dir);
	// check directory write permissions
	status_t error = dir->CheckPermissions(W_OK);
	ino_t nodeID = -1;
	if (error == B_OK) {
		// check if entry exists
		Node *node = NULL;
		Entry *entry = NULL;
		if (dir->FindAndGetNode(name, &node, &entry) == B_OK) {
			nodeID = node->GetID();
			if (!node->IsDirectory()) {
				SET_ERROR(error, B_NOT_A_DIRECTORY);
			} else if (!dynamic_cast<Directory*>(node)->IsEmpty()) {
				SET_ERROR(error, B_DIRECTORY_NOT_EMPTY);
			} else
				error = dir->DeleteEntry(entry);
			volume->PutVNode(node);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	}
	// notify listeners
	if (error == B_OK)
		notify_entry_removed(volume->GetID(), dir->GetID(), name, nodeID);

	RETURN_ERROR(error);
}


static status_t
ramfs_open_dir(fs_volume* /*fs*/, fs_vnode* _node, void** _cookie)
{
//	FUNCTION_START();
//	Volume *volume = (Volume*)fs;
	Node* node = (Node*)_node->private_node;

	FUNCTION(("dir: (%Lu)\n", node->GetID()));

	if (!node->IsDirectory())
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	// get the Directory
	Directory *dir = dynamic_cast<Directory*>(node);
	status_t error = B_OK;
	if (dir == NULL) {
		FATAL("Node %" B_PRIdINO " pretends to be a Directory, but isn't!\n",
			node->GetID());
		error = B_NOT_A_DIRECTORY;
	}

	// create a DirectoryCookie
	if (error == B_OK) {
		DirectoryCookie *cookie = new(nothrow) DirectoryCookie(dir);
		if (cookie) {
			error = cookie->Suspend();
			if (error == B_OK)
				*_cookie = cookie;
			else
				delete cookie;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	FUNCTION_END();
	RETURN_ERROR(error);
}


static status_t
ramfs_close_dir(fs_volume* /*fs*/, fs_vnode* DARG(_node), void* _cookie)
{
	FUNCTION_START();
	FUNCTION(("dir: (%Lu)\n", ((Node*)_node)->GetID()));
	// No locking needed, since the Directory is guaranteed to live at this
	// time and for iterators there is a separate locking.
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	cookie->Unset();
	return B_OK;
}


static status_t
ramfs_free_dir_cookie(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
ramfs_read_dir(fs_volume* _volume, fs_vnode* DARG(_node), void* _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	DARG(Node *node = (Node*)_node; )

	FUNCTION(("dir: (%Lu)\n", node->GetID()));
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t status = cookie->Resume();
	if (status != B_OK)
		RETURN_ERROR(status);

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount && bufferSize > sizeof(struct dirent)) {
		size_t length = bufferSize - offsetof(struct dirent, d_name);

		ino_t nodeID = -1;
		const char *name = NULL;
		if (cookie->GetCurrent(&nodeID, &name) != B_OK)
			break;

		PRINT("  entry: `%s'\n", name);

		size_t nameLength = strlen(name);
		if (length < (nameLength + 1)) {
			// the remaining name buffer length is too small
			if (count == 0)
				RETURN_ERROR(B_BUFFER_OVERFLOW);
			break;
		}
		length = nameLength;

		dirent->d_dev = volume->GetID();
		dirent->d_ino = nodeID;
		memcpy(dirent->d_name, name, nameLength);
		dirent->d_name[nameLength] = '\0';

		dirent = next_dirent(dirent, length, bufferSize);
		count++;

		if (cookie->Next() != B_OK)
			break;
	}

	cookie->Suspend();

	*_num = count;
	return B_OK;
}


static status_t
ramfs_rewind_dir(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	// No locking needed, since the Directory is guaranteed to live at this
	// time and for iterators there is a separate locking.
	DirectoryCookie *cookie = (DirectoryCookie*)_cookie;
	// no need to Resume(), iterator remains suspended
	status_t error = cookie->Rewind();
	RETURN_ERROR(error);
}


// #pragma mark - Attribute Directories


static status_t
ramfs_open_attr_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// check permissions
	error = node->CheckPermissions(R_OK);
	// create iterator
	AttributeIterator *iterator = NULL;
	if (error == B_OK) {
		iterator = new(nothrow) AttributeIterator(node);
		if (iterator)
			error = iterator->Suspend();
		else
			SET_ERROR(error, B_NO_MEMORY);
	}
	// set result / cleanup on failure
	if (error == B_OK)
		*_cookie = iterator;
	else
		delete iterator;
	RETURN_ERROR(error);
}


static status_t
ramfs_close_attr_dir(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)_cookie;
	iterator->Unset();
	return B_OK;
}


static status_t
ramfs_free_attr_dir_cookie(fs_volume* /*fs*/, fs_vnode* /*_node*/,
	void* _cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)_cookie;
	delete iterator;
	return B_OK;
}


static status_t
ramfs_read_attr_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;

	AttributeIterator *iterator = (AttributeIterator*)_cookie;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = iterator->Resume();
	if (error == B_OK) {
		// get next attribute
		Attribute *attribute = NULL;
		if (iterator->GetNext(&attribute) == B_OK) {
			const char *name = attribute->GetName();
			size_t nameLen = strlen(name);
			// check, whether the entry fits into the buffer,
			// and fill it in
			size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
			if (length <= bufferSize) {
				buffer->d_dev = volume->GetID();
				buffer->d_ino = -1;	// attributes don't have a node ID
				memcpy(buffer->d_name, name, nameLen);
				buffer->d_name[nameLen] = '\0';
				buffer->d_reclen = length;
				*count = 1;
			} else {
				SET_ERROR(error, B_BUFFER_OVERFLOW);
			}
		} else
			*count = 0;

		iterator->Suspend();
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_rewind_attr_dir(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	// No locking needed, since the Node is guaranteed to live at this time
	// and for iterators there is a separate locking.
	AttributeIterator *iterator = (AttributeIterator*)_cookie;
	// no need to Resume(), iterator remains suspended
	status_t error = iterator->Rewind();
	RETURN_ERROR(error);
}


// #pragma mark - Attributes


class AttributeCookie {
public:
	AttributeCookie() : fOpenMode(0), fLastNotificationTime(0) {}

	status_t Init(const char* name, int openMode)
	{
		if (!fName.SetTo(name))
			return B_NO_MEMORY;
		fOpenMode = openMode;

		return B_OK;
	}

	inline const char* GetName() const		{ return fName.GetString(); }

	inline int GetOpenMode() const			{ return fOpenMode; }

	inline bigtime_t GetLastNotificationTime() const
		{ return fLastNotificationTime; }

	inline bool NotificationIntervalElapsed(bool set = false)
	{
		bigtime_t currentTime = system_time();
		bool result = (currentTime - fLastNotificationTime
			> kNotificationInterval);
		if (set && result)
			fLastNotificationTime = currentTime;
		return result;
	}

private:
	String		fName;
	int			fOpenMode;
	bigtime_t	fLastNotificationTime;
};


static status_t
ramfs_create_attr(fs_volume* _volume, fs_vnode* _node, const char *name,
	uint32 type, int openMode, void** _cookie)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// try to find the attribute
	Attribute *attribute = NULL;
	node->FindAttribute(name, &attribute);

	// in case the attribute exists we fail if required by the openMode
	if (attribute && (openMode & O_EXCL))
		RETURN_ERROR(B_FILE_EXISTS);

	// creating and truncating require write permission
	int accessMode = open_mode_to_access(openMode);

	// check required permissions against node permissions
	status_t error = node->CheckPermissions(accessMode);
	if (error != B_OK)
		RETURN_ERROR(error);

	// create the cookie
	AttributeCookie *cookie = new(nothrow) AttributeCookie();
	if (!cookie)
		return B_NO_MEMORY;
	ObjectDeleter<AttributeCookie> cookieDeleter(cookie);

	// init the cookie
	error = cookie->Init(name, openMode);
	if (error != B_OK)
		RETURN_ERROR(error);

	// if not existing yet, create the attribute and set its type
	if (!attribute) {
		error = node->CreateAttribute(name, &attribute);
		if (error != B_OK)
			RETURN_ERROR(error);

		attribute->SetType(type);

		notify_attribute_changed(volume->GetID(), -1, node->GetID(), name,
			B_ATTR_CREATED);

	// else truncate if requested
	} else if (openMode & O_TRUNC) {
		error = attribute->SetSize(0);
		if (error != B_OK)
			return error;

		notify_attribute_changed(volume->GetID(), -1, node->GetID(), name,
			B_ATTR_CHANGED);
	}
	NodeMTimeUpdater mTimeUpdater(node);

	// success
	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


static status_t
ramfs_open_attr(fs_volume* _volume, fs_vnode* _node, const char *name,
	int openMode, void** _cookie)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	FUNCTION(("node: %lld\n", node->GetID()));

	status_t error = B_OK;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// find the attribute
	Attribute *attribute = NULL;
	if (error == B_OK)
		error = node->FindAttribute(name, &attribute);

	// truncating requires write permission
	int accessMode = open_mode_to_access(openMode);

	// check open mode against permissions
	if (error == B_OK)
		error = node->CheckPermissions(accessMode);

	// create the cookie
	AttributeCookie *cookie = NULL;
	if (error == B_OK) {
		cookie = new(nothrow) AttributeCookie();
		if (cookie) {
			SET_ERROR(error, cookie->Init(name, openMode));
		} else {
			SET_ERROR(error, B_NO_MEMORY);
		}
	}

	// truncate if requested
	if (error == B_OK && (openMode & O_TRUNC)) {
		error = attribute->SetSize(0);

		if (error == B_OK) {
			notify_attribute_changed(volume->GetID(), -1, node->GetID(),
				name, B_ATTR_CHANGED);
		}
	}
	NodeMTimeUpdater mTimeUpdater(node);

	// set result / cleanup on failure
	if (error == B_OK)
		*_cookie = cookie;
	else if (cookie)
		delete cookie;

	RETURN_ERROR(error);
}


static status_t
ramfs_close_attr(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	FUNCTION(("node: %lld\n", node->GetID()));

	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// notify listeners
	notify_if_stat_changed(volume, node);
	return B_OK;
}


static status_t
ramfs_free_attr_cookie(fs_volume* /*fs*/, fs_vnode* /*_node*/, void* _cookie)
{
	FUNCTION_START();
	AttributeCookie *cookie = (AttributeCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
ramfs_read_attr(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void *buffer, size_t *bufferSize)
{
	FUNCTION_START();

	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;

	AttributeCookie *cookie = (AttributeCookie*)_cookie;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;

	// find the attribute
	Attribute *attribute = NULL;
	if (error == B_OK)
		error = node->FindAttribute(cookie->GetName(), &attribute);

	// check permissions
	int accessMode = open_mode_to_access(cookie->GetOpenMode());
	if (error == B_OK && !(accessMode & R_OK))
		SET_ERROR(error, B_NOT_ALLOWED);

	// read
	if (error == B_OK)
		error = attribute->ReadAt(pos, buffer, *bufferSize, bufferSize);

	RETURN_ERROR(error);
}


static status_t
ramfs_write_attr(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const void *buffer, size_t *bufferSize)
{
	FUNCTION_START();

	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;
	AttributeCookie *cookie = (AttributeCookie*)_cookie;

	status_t error = B_OK;
	// Don't allow writing the reserved attributes.
	const char *name = cookie->GetName();
	if (name[0] == '\0' || !strcmp(name, "name")
			|| !strcmp(name, "last_modified") || !strcmp(name, "size")) {
		// FUNCTION(("failed: node: %s, attribute: %s\n",
		//	node->GetName(), name));
		RETURN_ERROR(B_NOT_ALLOWED);
	}

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(node);

	// find the attribute
	Attribute *attribute = NULL;
	if (error == B_OK)
		error = node->FindAttribute(cookie->GetName(), &attribute);

	// check permissions
	int accessMode = open_mode_to_access(cookie->GetOpenMode());
	if (error == B_OK && !(accessMode & W_OK))
		SET_ERROR(error, B_NOT_ALLOWED);

	// write the data
	if (error == B_OK)
		error = attribute->WriteAt(pos, buffer, *bufferSize, bufferSize);

	// notify listeners
	if (error == B_OK) {
		notify_attribute_changed(volume->GetID(), -1, node->GetID(), name,
			B_ATTR_CHANGED);
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_read_attr_stat(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	struct stat *st)
{
//	FUNCTION_START();

	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;
	AttributeCookie *cookie = (AttributeCookie*)_cookie;
	status_t error = B_OK;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// find the attribute
	Attribute *attribute = NULL;
	if (error == B_OK)
		error = node->FindAttribute(cookie->GetName(), &attribute);

	// check permissions
	int accessMode = open_mode_to_access(cookie->GetOpenMode());
	if (error == B_OK && !(accessMode & R_OK))
		SET_ERROR(error, B_NOT_ALLOWED);

	// read
	if (error == B_OK) {
		st->st_type = attribute->GetType();
		st->st_size = attribute->GetSize();
	}

	RETURN_ERROR(error);
}


static status_t
ramfs_rename_attr(fs_volume* /*fs*/, fs_vnode* /*_fromNode*/,
	const char */*fromName*/, fs_vnode* /*_toNode*/, const char */*toName*/)
{
	// TODO : ramfs_rename_attr
	return B_BAD_VALUE;
}


static status_t
ramfs_remove_attr(fs_volume* _volume, fs_vnode* _node, const char *name)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	Node* node = (Node*)_node->private_node;
	status_t error = B_OK;

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	NodeMTimeUpdater mTimeUpdater(node);

	// check permissions
	error = node->CheckPermissions(W_OK);

	// find the attribute
	Attribute *attribute = NULL;
	if (error == B_OK)
		error = node->FindAttribute(name, &attribute);

	// delete it
	if (error == B_OK)
		error = node->DeleteAttribute(attribute);

	// notify listeners
	if (error == B_OK) {
		notify_attribute_changed(volume->GetID(), -1, node->GetID(), name,
			B_ATTR_REMOVED);
	}

	RETURN_ERROR(error);
}


// #pragma mark - Indices


// IndexDirCookie
class IndexDirCookie {
public:
	IndexDirCookie() : index_index(0) {}

	int32	index_index;
};


static status_t
ramfs_open_index_dir(fs_volume* _volume, void** _cookie)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t error = B_OK;
	// check whether an index directory exists
	if (volume->GetIndexDirectory()) {
		IndexDirCookie *cookie = new(nothrow) IndexDirCookie;
		if (cookie)
			*_cookie = cookie;
		else
			SET_ERROR(error, B_NO_MEMORY);
	} else
		SET_ERROR(error, B_ENTRY_NOT_FOUND);

	RETURN_ERROR(error);
}


static status_t
ramfs_close_index_dir(fs_volume* /*fs*/, void* /*_cookie*/)
{
	FUNCTION_START();
	return B_OK;
}


static status_t
ramfs_free_index_dir_cookie(fs_volume* /*fs*/, void* _cookie)
{
	FUNCTION_START();
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	delete cookie;
	return B_OK;
}


static status_t
ramfs_read_index_dir(fs_volume* _volume, void* _cookie,
	struct dirent *buffer, size_t bufferSize, uint32 *count)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	status_t error = B_OK;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// get the next index
	Index *index = volume->GetIndexDirectory()->IndexAt(
		cookie->index_index++);
	if (index) {
		const char *name = index->GetName();
		size_t nameLen = strlen(name);
		// check, whether the entry fits into the buffer,
		// and fill it in
		size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
		if (length <= bufferSize) {
			buffer->d_dev = volume->GetID();
			buffer->d_ino = -1;	// indices don't have a node ID
			memcpy(buffer->d_name, name, nameLen);
			buffer->d_name[nameLen] = '\0';
			buffer->d_reclen = length;
			*count = 1;
		} else {
			SET_ERROR(error, B_BUFFER_OVERFLOW);
		}
	} else
		*count = 0;

	RETURN_ERROR(error);
}


static status_t
ramfs_rewind_index_dir(fs_volume* /*fs*/, void* _cookie)
{
	FUNCTION_START();
	IndexDirCookie *cookie = (IndexDirCookie*)_cookie;
	cookie->index_index = 0;
	return B_OK;
}


static status_t
ramfs_create_index(fs_volume* _volume, const char *name, uint32 type,
	uint32 /*flags*/)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	status_t error = B_OK;

	// only root is allowed to manipulate the indices
	if (geteuid() != 0)
		RETURN_ERROR(B_NOT_ALLOWED);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// get the index directory
	if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
		// check whether an index with that name does already exist
		if (indexDir->FindIndex(name)) {
			SET_ERROR(error, B_FILE_EXISTS);
		} else {
			// create the index
			AttributeIndex *index;
			error = indexDir->CreateIndex(name, type, &index);
		}
	} else
		SET_ERROR(error, B_ENTRY_NOT_FOUND);

	RETURN_ERROR(error);
}


static status_t
ramfs_remove_index(fs_volume* _volume, const char *name)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	status_t error = B_OK;

	// only root is allowed to manipulate the indices
	if (geteuid() != 0)
		RETURN_ERROR(B_NOT_ALLOWED);

	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// get the index directory
	if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
		// check whether an index with that name does exist
		if (Index *index = indexDir->FindIndex(name)) {
			// don't delete a special index
			if (indexDir->IsSpecialIndex(index)) {
				SET_ERROR(error, B_BAD_VALUE);
			} else
				indexDir->DeleteIndex(index);
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ENTRY_NOT_FOUND);

	RETURN_ERROR(error);
}


static status_t
ramfs_read_index_stat(fs_volume* _volume, const char *name, struct stat *st)
{
	FUNCTION_START();
	Volume* volume = (Volume*)_volume->private_volume;
	status_t error = B_OK;

	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// get the index directory
	if (IndexDirectory *indexDir = volume->GetIndexDirectory()) {
		// find the index
		if (Index *index = indexDir->FindIndex(name)) {
			st->st_type = index->GetType();
			if (index->HasFixedKeyLength())
				st->st_size = index->GetKeyLength();
			else
				st->st_size = kMaxIndexKeyLength;
			st->st_atime = 0;	// TODO: index times
			st->st_mtime = 0;	// ...
			st->st_ctime = 0;	// ...
			st->st_crtime = 0;	// ...
			st->st_uid = 0;		// root owns the indices
			st->st_gid = 0;		//
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	} else
		SET_ERROR(error, B_ENTRY_NOT_FOUND);

	RETURN_ERROR(error);
}


// #pragma mark - Queries


static status_t
ramfs_open_query(fs_volume* _volume, const char *queryString, uint32 flags,
	port_id port, uint32 token, void** _cookie)
{
	FUNCTION_START();
	PRINT("query = \"%s\", flags = %lu, port_id = %" B_PRId32 ", token = %" B_PRId32 "\n",
		queryString, flags, port, token);

	Volume* volume = (Volume*)_volume->private_volume;

	// lock the volume
	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	Query* query;
	status_t error = Query::Create(volume, queryString, flags, port, token, query);
	if (error != B_OK)
		return error;
	// TODO: The Query references an Index, but nothing prevents the Index
	// from being deleted, while the Query is in existence.

	*_cookie = (void *)query;

	return B_OK;
}


static status_t
ramfs_close_query(fs_volume* /*fs*/, void* /*cookie*/)
{
	FUNCTION_START();
	return B_OK;
}


static status_t
ramfs_free_query_cookie(fs_volume* _volume, void* _cookie)
{
	FUNCTION_START();

	Volume* volume = (Volume*)_volume->private_volume;

	// lock the volume
	VolumeWriteLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	Query *query = (Query *)_cookie;
	delete query;

	return B_OK;
}


static status_t
ramfs_read_query(fs_volume* _volume, void* _cookie, struct dirent *buffer,
	size_t bufferSize, uint32 *count)
{
	FUNCTION_START();
	Query *query = (Query *)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	// lock the volume
	VolumeReadLocker locker(volume);
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	status_t status = query->GetNextEntry(buffer, bufferSize);
	if (status == B_OK)
		*count = 1;
	else if (status == B_ENTRY_NOT_FOUND)
		*count = 0;
	else
		return status;

	return B_OK;
}


// TODO: status_t (*rewind_query)(fs_volume fs, void** _cookie);


// #pragma mark - Module Interface


static status_t
ramfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			init_debugging();
			PRINT("ramfs_std_ops(): B_MODULE_INIT\n");
			return B_OK;
		}

		case B_MODULE_UNINIT:
			PRINT("ramfs_std_ops(): B_MODULE_UNINIT\n");
			exit_debugging();
			return B_OK;

		default:
			return B_ERROR;
	}
}


fs_volume_ops gRamFSVolumeOps = {
	&ramfs_unmount,
	&ramfs_read_fs_info,
	&ramfs_write_fs_info,
	&ramfs_sync,
	&ramfs_get_vnode,

	/* index directory & index operations */
	&ramfs_open_index_dir,
	&ramfs_close_index_dir,
	&ramfs_free_index_dir_cookie,
	&ramfs_read_index_dir,
	&ramfs_rewind_index_dir,

	&ramfs_create_index,
	&ramfs_remove_index,
	&ramfs_read_index_stat,

	/* query operations */
	&ramfs_open_query,
	&ramfs_close_query,
	&ramfs_free_query_cookie,
	&ramfs_read_query,
	NULL	// rewind_query
};


fs_vnode_ops gRamFSVnodeOps = {
	/* vnode operations */
	&ramfs_lookup,			// lookup
	NULL,					// get name
	&ramfs_write_vnode,		// write
	&ramfs_remove_vnode,	// remove

	/* VM file access */
	NULL,					// can_page
	NULL,					// read pages
	NULL,					// write pages

	NULL,					// io?
	NULL,					// cancel io

	NULL,					// get file map

	&ramfs_ioctl,
	&ramfs_set_flags,
	NULL,   // &ramfs_select,
	NULL,   // &ramfs_deselect,
	&ramfs_fsync,

	&ramfs_read_symlink,
	&ramfs_create_symlink,

	&ramfs_link,
	&ramfs_unlink,
	&ramfs_rename,

	&ramfs_access,
	&ramfs_read_stat,
	&ramfs_write_stat,
	NULL,   // &ramfs_preallocate,

	/* file operations */
	&ramfs_create,
	&ramfs_open,
	&ramfs_close,
	&ramfs_free_cookie,
	&ramfs_read,
	&ramfs_write,

	/* directory operations */
	&ramfs_create_dir,
	&ramfs_remove_dir,
	&ramfs_open_dir,
	&ramfs_close_dir,
	&ramfs_free_dir_cookie,
	&ramfs_read_dir,
	&ramfs_rewind_dir,

	/* attribute directory operations */
	&ramfs_open_attr_dir,
	&ramfs_close_attr_dir,
	&ramfs_free_attr_dir_cookie,
	&ramfs_read_attr_dir,
	&ramfs_rewind_attr_dir,

	/* attribute operations */
	&ramfs_create_attr,
	&ramfs_open_attr,
	&ramfs_close_attr,
	&ramfs_free_attr_cookie,
	&ramfs_read_attr,
	&ramfs_write_attr,

	&ramfs_read_attr_stat,
	NULL,   // &ramfs_write_attr_stat,
	&ramfs_rename_attr,
	&ramfs_remove_attr,

	/* special nodes */
	&ramfs_create_special_node,
};

static file_system_module_info sRamFSModuleInfo = {
	{
		"file_systems/ramfs" B_CURRENT_FS_API_VERSION,
		0,
		ramfs_std_ops,
	},

	"ramfs",				// short_name
	"RAM File System",		// pretty_name
	0						// DDM flags
	| B_DISK_SYSTEM_SUPPORTS_WRITING,

	// scanning
	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

	&ramfs_mount,

	NULL,	// TODO : &ramfs_get_supported_operations

	NULL,   // validate_resize
	NULL,   // validate_move
	NULL,   // validate_set_content_name
	NULL,   // validate_set_content_parameters
	NULL,   // validate_initialize,

	/* shadow partition modification */
	NULL,   // shadow_changed

	/* writing */
	NULL,   // defragment
	NULL,   // repair
	NULL,   // resize
	NULL,   // move
	NULL,   // set_content_name
	NULL,   // set_content_parameters
	NULL	// bfs_initialize
};

module_info *modules[] = {
	(module_info *)&sRamFSModuleInfo,
	NULL,
};
