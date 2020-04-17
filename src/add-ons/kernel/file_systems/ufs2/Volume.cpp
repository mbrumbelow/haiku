/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Volume.h"
#include "../common/DeviceOpener.h"

//#define TRACE_UFS2
#ifdef TRACE_UFS2
#	define TRACE(x...) dprintf("\33[34mufs2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#   define ERROR(x...) dprintf("\33[34mexfat:\33[0m " x)

DeviceOpener::DeviceOpener(const char* device, int mode)
	:
	fBlockCache(NULL)
{
	Open(device, mode);
}

DeviceOpener::DeviceOpener(int fd, int mode)
	:
	fBlockCache(NULL)
{
	Open(fd, mode);
}

DeviceOpener::~DeviceOpener()
{
	if (fDevice >= 0){
		RemoveCache(false);
		close(fDevice);
	}
}

int
DeviceOpener::Open(const char* device, int mode)
{
	fDevice = open(device, mode | O_NOCACHE);
	if (fDevice < 0)
		fDevice = errno;

	if (fDevice < 0 && _IsReadWrite(mode)) {
		return Open(device, O_RDONLY | O_NOCACHE);
	}

	if (fDevice >= 0) {
		fMode = mode;
		if (_IsReadWrite(mode)) {
			device_geometry geometry;
			if (!ioctl(fDevice, B_GET_GEOMETRY, &geometry, sizeof(device_geometry)))
			{
				if (geometry.read_only) {
					close(fDevice);
					return Open(device, O_RDONLY | O_NOCACHE);
				}
			}
		}
	}
	return fDevice;
}

int
DeviceOpener::Open(int fd, int mode)
{
	fDevice = dup(fd);
	if (fDevice < 0)
		return errno;

	fMode = mode;

	return fDevice;
}

void*
DeviceOpener::InitCache(off_t numBlocks, uint32 blockSize)
{
	return fBlockCache = block_cache_create(fDevice, numBlocks, blockSize,
		IsReadOnly());
}

void
DeviceOpener::RemoveCache(bool allowWrites)
{
	if (fBlockCache == NULL)
		return;

	block_cache_delete(fBlockCache, allowWrites);
	fBlockCache = NULL;
}


void
DeviceOpener::Keep()
{
	fDevice = -1;
}

status_t
DeviceOpener::GetSize(off_t* _size, uint32* _blockSize)
{
	device_geometry geometry;
	if (ioctl(fDevice, B_GET_GEOMETRY, &geometry, sizeof(device_geometry)) < 0) {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fDevice, &stat) < 0)
			return B_ERROR;

		if (_size)
			*_size = stat.st_size;
		if (_blockSize)	// that shouldn't cause us any problems
			*_blockSize = 512;

		return B_OK;
	}

	if (_size) {
		*_size = 1ULL * geometry.head_count * geometry.cylinder_count
			* geometry.sectors_per_track * geometry.bytes_per_sector;
	}
	if (_blockSize)
		*_blockSize = geometry.bytes_per_sector;

	return B_OK;
}

bool
ufs2_super_block::IsValid()
{
	if (ufs2_magic != UFS2_SUPER_BLOCK_MAGIC)
		return false;

	return true;
}

bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}

Volume::Volume(fs_volume *volume)
    : fFSVolume(volume)
{
	fFlags = 0;
	mutex_init(&fLock, "ufs2 volume");
	TRACE("Volume::Volume() : Initialising volume");
}


Volume::~Volume()
{
	mutex_destroy(&fLock);
	TRACE("Volume::Destructor : Removing Volume");
}

status_t
Volume::Identify(int fd, ufs2_super_block *superBlock)
{
	if (read_pos(fd, UFS2_SUPER_BLOCK_OFFSET, superBlock,
		sizeof(ufs2_super_block)) != sizeof(ufs2_super_block))
		return B_IO_ERROR;

	if (!superBlock->IsValid()) {
		ERROR("invalid superblock! Identify failed!!\n");
		return B_BAD_VALUE;
	}

	return B_OK;
}

status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
	TRACE("Mounting volume... Please wait.\n");
	flags |= B_MOUNT_READ_ONLY;
	if ((flags & B_MOUNT_READ_ONLY) != 0)
	{
		TRACE("Volume is read only\n");
	}
	else
	{
		TRACE("Volume is read write\n");
	}

	DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0 
									? O_RDONLY:O_RDWR);
	fDevice = opener.Device();
	if (fDevice < B_OK) {
		ERROR("Could not open device\n");
		return fDevice;
	}

	if (opener.IsReadOnly())
		fFlags |= VOLUME_READ_ONLY;

	status_t status = Identify(fDevice, &fSuperBlock);
	if (status != B_OK) {
		ERROR("Invalid super block\n");
		return status;
	}

	TRACE("Valid super block\n");

	opener.Keep();
	return B_OK;

}

status_t
Volume::Unmount()
{
	TRACE("Unmounting the volume");

	TRACE("Closing device");
	close(fDevice);

	return B_OK;
}
