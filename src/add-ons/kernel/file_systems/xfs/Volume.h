/*
 * Copyright 2020 Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef __VOLUME_H__
#define __VOLUME_H__

#include "xfs_sb.h"

extern fs_volume_ops gxfsVolumeOps;
enum volume_flags
{
	VOLUME_READ_ONLY = 0x0001
};


class Volume
{
public:
	Volume(fs_volume *volume);
	~Volume();

	status_t Mount(const char *device, uint32 flags);
	status_t Unmount();
	status_t Initialize(int fd, const char *label,
						uint32 blockSize, uint32 sectorSize);

	bool IsValidSuperBlock();
	bool IsReadOnly() const
	{
		return (fFlags & VOLUME_READ_ONLY) != 0;
	}

	dev_t ID() const { return fFSVolume ? fFSVolume->id : -1; }
	fs_volume *FSVolume() const { return fFSVolume; }
	char *Name() { return fSuperBlock.Name(); }

	xfs_sb &SuperBlock() { return fSuperBlock; }
	int Device() const { return fDevice; }
	uint32 BlockSize() const { return fBlockSize; }
	static status_t Identify(int fd, xfs_sb *superBlock);

#if 0
	off_t NumBlocks() const
	{
		return fSuperBlock.NumBlocks();
	}

	off_t Root() const { return fSuperBlock.rootino; }

	static status_t Identify(int fd, xfs_sb *superBlock);
#endif

protected:
	fs_volume *fFSVolume;
	int fDevice;
	xfs_sb fSuperBlock;
	char fName[32]; /* filesystem name */
	int fbsize;		/* fs logical block size */

	uint32 fDeviceBlockSize;
	uint32 fBlockSize;
	mutex fLock;

	uint32 fFlags;
};

#endif