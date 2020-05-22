/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef INODE_H
#define INODE_H

#include "ufs2.h"
#include "Volume.h"

#define	UFS2_ROOT	((ino_t)2)


struct ufs2_inode {
	u_int16_t	fileMode;
	int16_t		linkCount;
	int32_t		userId;
	int32_t		groupId;
	int32_t		inodeBlockSize;
	int64_t		size;
	int64_t		byteHeld;
	int64_t		accessTime;
	int64_t		modifiedTime;
	int64_t		changeTime;
	int64_t		createTime;
	/* time in nano seconds */
	int32_t		nsModifiedTime;
	int32_t		nsAccessTime;
	int32_t		nsChangeTime;
	int32_t		nsCreateTime;
	int32_t		generationNumber;
	int32_t		kernelFlags;
	int32_t		statusFlags;
	int32_t		extendedAttSize; /* Extended attribute size */
	/* 2 Direct extended attribute block pointers */
	int64_t		extendedBklPtr1;
	int64_t		extendedBklPtr2;
	/* 12 direct block pointers */
	int64_t		directBlkPtr1;
	int64_t		directBlkPtr2;
	int64_t		directBlkPtr3;
	int64_t		directBlkPtr4;
	int64_t		directBlkPtr5;
	int64_t		directBlkPtr6;
	int64_t		directBlkPtr7;
	int64_t		directBlkPtr8;
	int64_t		directBlkPtr9;
	int64_t		directBlkPtr10;
	int64_t		directBlkPtr11;
	int64_t		directBlkPtr12;
	int64_t		indirectBlkPtr;  /* 1 Indirect block pointer */
	int64_t		doubleIndriectBlkPtr; // 1 Double Indirect block pointer
	int64_t		tripleIndriectBlkPtr; // 1 Triple Indirect block pointer
	/* Unused but need to figure out what are they */
	int64_t		unused1;
	int64_t		unused2;
	int64_t		unused3;


	static void _DecodeTime(struct timespec *timespec, uint64 time,
		uint32 time_extra, bool extra)
	{
		timespec->tv_sec = B_LENDIAN_TO_HOST_INT32(time);
		if (extra && sizeof(timespec->tv_sec) > 4)
			timespec->tv_sec |=
				(uint64)(B_LENDIAN_TO_HOST_INT32(time_extra) & 0x2) << 32;
		if (extra)
			timespec->tv_nsec = B_LENDIAN_TO_HOST_INT32(time_extra) >> 2;
		else
			timespec->tv_nsec = 0;
	}
	void GetAccessTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, accessTime, nsAccessTime, extra); }
	void GetChangeTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, changeTime, nsChangeTime, extra); }
	void GetModificationTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, modifiedTime, nsModifiedTime, extra); }
	void GetCreationTime(struct timespec *timespec, bool extra) const
		{ _DecodeTime(timespec, createTime, nsCreateTime, extra); }
};

class Inode {
	public:
						Inode(Volume* volume, ino_t id);
						Inode(Volume* volume, ino_t id,
							const ufs2_inode& item);
						~Inode();

			status_t	InitCheck();

			ino_t		ID() const { return fID; }
//
			rw_lock*	Lock() { return& fLock; }
//
//			status_t	UpdateNodeFromDisk();

			bool		IsDirectory() const
							{ return S_ISDIR(Mode()); }
			bool		IsFile() const
							{ return S_ISREG(Mode()); }
			bool		IsSymLink() const
							{ return S_ISLNK(Mode()); }
//			status_t	CheckPermissions(int accessMode) const;

			mode_t		Mode() const { return fNode.fileMode; }
			off_t		Size() const { return fNode.size; }
			uid_t		UserID() const { return fNode.userId; }
			gid_t		GroupID() const { return fNode.groupId; }
			void		GetChangeTime(struct timespec *timespec) const
							{ fNode.GetChangeTime(timespec, fHasNanoTime); }
			void		GetModificationTime(struct timespec *timespec) const
							{ fNode.GetModificationTime(timespec, fHasNanoTime); }
			void		GetCreationTime(struct timespec *timespec) const
							{ fNode.GetCreationTime(timespec, fHasNanoTime); }
			void		GetAccessTime(struct timespec *timespec) const
							{ fNode.GetAccessTime(timespec, fHasNanoTime); }

			Volume*		GetVolume() const { return fVolume; }

//			status_t	FindBlock(off_t logical, off_t& physical,
//							off_t* _length = NULL);
//			status_t	ReadAt(off_t pos, uint8* buffer, size_t* length);
//			status_t	FillGapWithZeros(off_t start, off_t end);

			void*		FileCache() const { return fCache; }
			void*		Map() const { return fMap; }

//			status_t	FindParent(ino_t* id);
	//static	Inode*		Create(Transaction& transaction, ino_t id, Inode* parent, int32 mode, uint64 size = 0, uint64 flags = 0);
private:
						Inode(Volume* volume);
						Inode(const Inode&);
						Inode& operator=(const Inode&);

//			uint64		_NumBlocks();
private:

			rw_lock		fLock;
			::Volume*	fVolume;
			ino_t		fID;
			void*		fCache;
			bool		fHasNanoTime;
			void*		fMap;
			status_t	fInitStatus;
			ufs2_inode	fNode;
};

#endif	// INODE_H
