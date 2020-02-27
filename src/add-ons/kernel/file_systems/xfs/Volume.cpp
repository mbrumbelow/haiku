
#include "Volume.h"

#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#define TRACE(x...) dprintf("\n\33[34mxfs:\33[0m " x)
#else
#define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\n\33[34mxfs:\33[0m " x)

class DeviceOpener
{
public:
    DeviceOpener(int fd, int mode);
    DeviceOpener(const char *device, int mode);
    ~DeviceOpener();

    int Open(const char *device, int mode);
    int Open(int fd, int mode);
    void *InitCache(off_t numBlocks, uint32 blockSize);
    void RemoveCache(bool allowWrites);

    void Keep();

    int Device() const { return fDevice; }
    int Mode() const { return fMode; }
    bool IsReadOnly() const { return _IsReadOnly(fMode); }

    status_t GetSize(off_t *_size, uint32 *_blockSize = NULL);

private:
    static bool _IsReadOnly(int mode)
    {
        return (mode & O_RWMASK) == O_RDONLY;
    }
    static bool _IsReadWrite(int mode)
    {
        return (mode & O_RWMASK) == O_RDWR;
    }

    int fDevice;
    int fMode;
    void *fBlockCache;
};

DeviceOpener::DeviceOpener(const char *device, int mode)
    : fBlockCache(NULL)
{
    Open(device, mode);
}

DeviceOpener::DeviceOpener(int fd, int mode)
    : fBlockCache(NULL)
{
    Open(fd, mode);
}

DeviceOpener::~DeviceOpener()
{
    if (fDevice >= 0)
    {
        RemoveCache(false);
        close(fDevice);
    }
}

int DeviceOpener::Open(const char *device, int mode)
{
    fDevice = open(device, mode | O_NOCACHE);
    if (fDevice < 0)
        fDevice = errno;

    if (fDevice < 0 && _IsReadWrite(mode))
    {
        // try again to open read-only (don't rely on a specific error code)
        return Open(device, O_RDONLY | O_NOCACHE);
    }

    if (fDevice >= 0)
    {
        // opening succeeded
        fMode = mode;
        if (_IsReadWrite(mode))
        {
            // check out if the device really allows for read/write access
            device_geometry geometry;
            if (!ioctl(fDevice, B_GET_GEOMETRY, &geometry))
            {
                if (geometry.read_only)
                {
                    // reopen device read-only
                    close(fDevice);
                    return Open(device, O_RDONLY | O_NOCACHE);
                }
            }
        }
    }

    return fDevice;
}

int DeviceOpener::Open(int fd, int mode)
{
    fDevice = dup(fd);
    if (fDevice < 0)
        return errno;

    fMode = mode;

    return fDevice;
}

void *
DeviceOpener::InitCache(off_t numBlocks, uint32 blockSize)
{
    return fBlockCache = block_cache_create(fDevice, numBlocks, blockSize,
                                            IsReadOnly());
}

void DeviceOpener::RemoveCache(bool allowWrites)
{
    if (fBlockCache == NULL)
        return;

    block_cache_delete(fBlockCache, allowWrites);
    fBlockCache = NULL;
}

void DeviceOpener::Keep()
{
    fDevice = -1;
}

/*!	Returns the size of the device in bytes. It uses B_GET_GEOMETRY
	to compute the size, or fstat() if that failed.
*/
status_t
DeviceOpener::GetSize(off_t *_size, uint32 *_blockSize)
{
    device_geometry geometry;
    if (ioctl(fDevice, B_GET_GEOMETRY, &geometry) < 0)
    {
        // maybe it's just a file
        struct stat stat;
        if (fstat(fDevice, &stat) < 0)
            return B_ERROR;

        if (_size)
            *_size = stat.st_size;
        if (_blockSize) // that shouldn't cause us any problems
            *_blockSize = 512;

        return B_OK;
    }

    if (_size)
    {
        *_size = 1LL * geometry.head_count * geometry.cylinder_count * geometry.sectors_per_track * geometry.bytes_per_sector;
    }
    if (_blockSize)
        *_blockSize = geometry.bytes_per_sector;

    return B_OK;
}

Volume::Volume(fs_volume *volume)
    : fFSVolume(volume)
{
    mutex_init(&fLock, "xfs volume");
    TRACE("Volume::Volume() : Initialising volume");
}

Volume::~Volume()
{
    mutex_destroy(&fLock);
    TRACE("Volume::Destructor : Removing Volume");
}

bool xfs_sb::IsValid() const
{
    /* Work here */
    return false;
}

bool Volume::IsValidSuperBlock()
{
    return fSuperBlock.isValid();
}

status_t
Volume::Identify(int fd, xfs_sb *superBlock)
{ /* Work here */

    TRACE("Volume::Identify() : Identifying Volume in progress");

    uint32 size = superblock.size();
    char *buffer[size];

    if (read_pos(fd, 0, buffer, size) != size)
        return B_IO_ERROR;

    superBlock = &xfs_sb(buffer);

    if (!superBlock->IsValid())
    {
        ERROR("Invalid Superblock!\n");
        return B_BAD_VALUE;
    }
    return B_OK;
}

status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
    /* Work here */

    TRACE("Volume::Mount() : Mounting in progress");

    flags |= B_MOUNT_READ_ONLY;

    if ((flags & B_MOUNT_READ_ONLY) != 0)
    {
        TRACE("Volume::Mount(): Read only\n");
    }
    else
    {
        TRACE("Volume::Mount(): Read write\n");
    }

    DeviceOpener opener(deviceName, (flags & B_MOUNT_READ_ONLY) != 0
                                        ? O_RDONLY
                                        : O_RDWR);
    fDevice = opener.Device();
    if (fDevice < B_OK)
    {
        ERROR("Volume::Mount(): couldn't open device\n");
        return fDevice;
    }

    if (opener.IsReadOnly())
        fFlags |= VOLUME_READ_ONLY;

    // read the superblock
    status_t status = Identify(fDevice, &fSuperBlock);
    if (status != B_OK)
    {
        ERROR("Invalid super block!\n");
        return B_BAD_VALUE;
    }

    // initialize short hands to the superblock (to save byte swapping)
    fBlockSize = fSuperBlock.BlockSize();

    // check if the device size is large enough to hold the file system
    off_t diskSize;
    if (opener.GetSize(&diskSize) != B_OK)
    {
        ERROR("Volume:Mount() Unable to get diskSize");
        return B_ERROR;
    }

    opener.Keep();
    return B_OK;
}

status_t
Volume::Unmount()
{
    /* Work here */
    TRACE("Unmounting");
    close(fDevice);

    return B_OK;
}
