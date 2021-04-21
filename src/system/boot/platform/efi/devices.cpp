/*
 * Copyright 2016-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/partitions.h>
#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"
#include <efi/protocol/block-io.h>
#include <efi/protocol/device-path.h>
#include <efi/protocol/loaded-image.h>
#include <efi/protocol/simple-file-system.h>


//#define TRACE_DEVICES
#ifdef TRACE_DEVICES
#   define TRACE(x...) dprintf("efi/devices: " x)
#else
#   define TRACE(x...) ;
#endif

static efi_guid BlockIoGUID = EFI_BLOCK_IO_PROTOCOL_GUID;

static efi_guid LoadedImageGUID = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static efi_guid SimpleFsGUID = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

class EfiDevice : public Node
{
	public:
		EfiDevice(efi_block_io_protocol *blockIo);
		virtual ~EfiDevice();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer,
			size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer,
			size_t bufferSize) { return B_UNSUPPORTED; }
		virtual off_t Size() const {
			return (fBlockIo->Media->LastBlock + 1) * BlockSize(); }

		uint32 BlockSize() const { return fBlockIo->Media->BlockSize; }
	private:
		efi_block_io_protocol*		fBlockIo;
};


EfiDevice::EfiDevice(efi_block_io_protocol *blockIo)
	:
	fBlockIo(blockIo)
{
}


EfiDevice::~EfiDevice()
{
}


ssize_t
EfiDevice::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	TRACE("%s called. pos: %" B_PRIdOFF ", %p, %" B_PRIuSIZE "\n", __func__,
		pos, buffer, bufferSize);

	off_t offset = pos % BlockSize();
	pos /= BlockSize();

	uint32 numBlocks = (offset + bufferSize + BlockSize() - 1) / BlockSize();

	// TODO: We really should implement memalign and align all requests to
	// fBlockIo->Media->IoAlign. This static alignment is large enough though
	// to catch most required alignments.
	char readBuffer[numBlocks * BlockSize()]
		__attribute__((aligned(2048)));

	if (fBlockIo->ReadBlocks(fBlockIo, fBlockIo->Media->MediaId,
		pos, sizeof(readBuffer), readBuffer) != EFI_SUCCESS) {
		dprintf("%s: blockIo error reading from device!\n", __func__);
		return B_ERROR;
	}

	memcpy(buffer, readBuffer + offset, bufferSize);

	return bufferSize;
}


static off_t
get_next_check_sum_offset(int32 index, off_t maxSize)
{
	TRACE("%s: called\n", __func__);

	if (index < 2)
		return index * 512;

	if (index < 4)
		return (maxSize >> 10) + index * 2048;

	return ((system_time() + index) % (maxSize >> 9)) * 512;
}


static uint32
compute_check_sum(Node *device, off_t offset)
{
	TRACE("%s: called\n", __func__);

	char buffer[512];
	ssize_t bytesRead = device->ReadAt(NULL, offset, buffer, sizeof(buffer));
	if (bytesRead < B_OK)
		return 0;

	if (bytesRead < (ssize_t)sizeof(buffer))
		memset(buffer + bytesRead, 0, sizeof(buffer) - bytesRead);

	uint32 *array = (uint32*)buffer;
	uint32 sum = 0;

	for (uint32 i = 0; i < (bytesRead + sizeof(uint32) - 1) / sizeof(uint32); i++)
		sum += array[i];

	return sum;
}

int
c16_strlen(char16_t *in)
{
	char16_t *cptr = in;
	int cnt = 0;

	while(*cptr++ != 0) {
		cnt++;
	}
	return cnt;
}


char*
c16str_to_cstr(char* dest, char16_t *src)
{
	char *cptr = dest;
	if(dest && src) {
		while((*cptr++  = (char)*src++) != u'\0') {
			;
		}
	}
	return dest;
}


char16_t*
get_boot_partition_filename(char16_t *imagepath, char16_t *filename)
{
	TRACE("%s: called\n", __func__);
	char16_t *_temp = imagepath;
	int index = c16_strlen(imagepath);
	while(index > 0) {
		if (_temp[index--] == u'\\') {
			// Undo the decrement
			index++;

			// step over the trailing slash,
			index++;
			break;
		}
	}
	// now append the filename
	while(*filename != u'\0') {
			_temp[index++] = *filename++;
	}
	_temp[index++] =  u'\0';

	return imagepath;
}


char16_t*
parse_image_path_from_device_path(efi_device_path_protocol *handle, char16_t *imagepath)
{
	TRACE("%s: called\n", __func__);

	efi_device_path_protocol *dpHandle = handle;
	char16_t *image = imagepath;

	unsigned char *byteptr = (unsigned char*)&image[1];
	// Note: intentional use of subscript [1] above

	size_t numBytes = 0;
	unsigned char *ptr = NULL;
	unsigned char *buf = (unsigned char*)dpHandle;
	do {
		dpHandle = (efi_device_path_protocol *)buf;
		if (dpHandle->Type == DEVICE_PATH_MEDIA
			&& dpHandle->SubType == MEDIA_FILEPATH_DP) {
				// The dpHandle->Length field contains the length of
				// the data AND the size of the fixed portion of the
				// structure (which is 4 bytes)
				ptr = (buf + 4);
				numBytes = dpHandle->Length[0] - 4;

				if(numBytes > 0 ) {
					byteptr -= sizeof(image[0]);
					// back up one to over write the previous NULL

					memcpy(byteptr, ptr, numBytes);
				}
		}
		buf += dpHandle->Length[0];

	} while (dpHandle->Type != DEVICE_PATH_END);

	return imagepath;
}


char16_t*
get_image_path(char16_t *imagepath)
{
	TRACE("%s: called\n", __func__);

	efi_status status;
	efi_loaded_image_protocol *liHandle;

	status = kBootServices->HandleProtocol(kImage, &LoadedImageGUID, (void**)&liHandle);

	if (status != EFI_SUCCESS) {
		TRACE("\t LoadedImageGUID failed: status = %lX \n", status);
		return NULL;
	}
	// This efi_device_path_protocol handle gets the filename
	if(liHandle->FilePath == NULL) {
		return NULL;
	}
	imagepath = parse_image_path_from_device_path(liHandle->FilePath, imagepath);

	return imagepath;
}


status_t
read_boot_partition_file(char16_t *imagepath)
{
	TRACE("%s: called\n", __func__);

	char raw[1024];
	memset(raw, 0 , sizeof(raw));

	char16_t c16_filename[] = { u"partition.txt"};

	get_boot_partition_filename(imagepath, c16_filename);

	efi_loaded_image_protocol *liHandle = NULL;
	efi_status status = kBootServices->HandleProtocol(kImage, &LoadedImageGUID, (void**)&liHandle);
	if (status != EFI_SUCCESS) {
		TRACE("\t LoadedImageGUID failed: status = %lX \n", status);
		return B_ERROR;
	}

	efi_simple_file_system_protocol *fsfHandle = NULL;
	status = kBootServices->HandleProtocol(liHandle->DeviceHandle, &SimpleFsGUID, (void**)&fsfHandle);
	if (status != EFI_SUCCESS) {
		TRACE("\t SimpleFsGUID failed: status = %lX \n", status);
		return B_ERROR;
	}

	efi_file_protocol *fsRoot = NULL;
	status = fsfHandle->OpenVolume(fsfHandle, &fsRoot);
	if (status != EFI_SUCCESS) {
		TRACE("\t OpenVolume failed: status = %lX \n", status);
		return B_ERROR;
	}

	efi_file_protocol *fsFile = NULL;
	status = fsRoot->Open(fsRoot, &fsFile, imagepath, EFI_FILE_MODE_READ, 0);
	if (status != EFI_SUCCESS) {
		TRACE("\t fsRoot->Open failed: status = %lX \n", status);
		return B_ERROR;
	}

	size_t bytesRead = sizeof(raw);
	status = fsFile->Read(fsFile, &bytesRead, (void*)raw);
	if (status != EFI_SUCCESS) {
		TRACE("\t fsFile->Read failed: status = %lX \n", status);
		fsFile->Close(fsFile);
		return B_ERROR;
	} else {
		if(bytesRead < sizeof(gPartitionToBoot)) {
			// NOTE: File content is NOT char16_t
			memset(gPartitionToBoot, 0 , sizeof(gPartitionToBoot));
			memcpy(gPartitionToBoot, raw , bytesRead);
			TRACE("\t gPartitionToBoot = %s \n", gPartitionToBoot);
		}
		fsFile->Close(fsFile);
	}

	return B_OK;
}


status_t
get_boot_partition_label()
{
	TRACE("%s: called\n", __func__);

	char16_t imageBuffer[1024] = u"\0";
	memset(imageBuffer, 0 , sizeof(imageBuffer));

	char16_t* imageName = imageBuffer;

	imageName = get_image_path(imageBuffer);
	if (imageName == NULL)
		return B_ERROR;

	status_t error = read_boot_partition_file(imageName);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
platform_add_boot_device(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("%s: called\n", __func__);

	efi_block_io_protocol *blockIo;
	size_t memSize = 0;

	status_t error = get_boot_partition_label();
	if (error != B_OK)
		return error;

	// Read to zero sized buffer to get memory needed for handles
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize, 0)
			!= EFI_BUFFER_TOO_SMALL)
		panic("Cannot read size of block device handles!");

	uint32 noOfHandles = memSize / sizeof(efi_handle);

	efi_handle handles[noOfHandles];
	if (kBootServices->LocateHandle(ByProtocol, &BlockIoGUID, 0, &memSize,
			handles) != EFI_SUCCESS)
		panic("Failed to locate block devices!");

	// All block devices has one for the disk and one per partition
	// There is a special case for a device with one fixed partition
	// But we probably do not care about booting on that kind of device
	// So find all disk block devices and let Haiku do partition scan
	for (uint32 n = 0; n < noOfHandles; n++) {
		if (kBootServices->HandleProtocol(handles[n], &BlockIoGUID,
				(void**)&blockIo) != EFI_SUCCESS)
			panic("Cannot get block device handle!");

		TRACE("%s: %p: present: %s, logical: %s, removeable: %s, "
			"blocksize: %" B_PRIu32 ", lastblock: %" B_PRIu64 "\n",
			__func__, blockIo,
			blockIo->Media->MediaPresent ? "true" : "false",
			blockIo->Media->LogicalPartition ? "true" : "false",
			blockIo->Media->RemovableMedia ? "true" : "false",
			blockIo->Media->BlockSize, blockIo->Media->LastBlock);

		if (!blockIo->Media->MediaPresent || blockIo->Media->LogicalPartition)
			continue;

		// The qemu flash device with a 256K block sizes sometime show up
		// in edk2. If flash is unconfigured, bad things happen on arm.
		// edk2 bug: https://bugzilla.tianocore.org/show_bug.cgi?id=2856
		// We're not ready for flash devices in efi, so skip anything odd.
		if (blockIo->Media->BlockSize > 8192)
			continue;

		EfiDevice *device = new(std::nothrow)EfiDevice(blockIo);
		if (device == NULL)
			panic("Can't allocate memory for block devices!");
		devicesList->Insert(device);
	}
	return devicesList->Count() > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}

status_t
platform_add_block_devices(struct stage2_args *args, NodeList *devicesList)
{
	TRACE("%s: called\n", __func__);

	//TODO: Currently we add all in platform_add_boot_device
	return B_ENTRY_NOT_FOUND;
}

status_t
platform_get_boot_partition(struct stage2_args *args, Node *bootDevice,
		NodeList *partitions, boot::Partition **_partition)
{
	TRACE("%s: called\n", __func__);
	*_partition = (boot::Partition*)partitions->GetIterator().Next();
	return *_partition != NULL ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
platform_register_boot_device(Node *device)
{
	TRACE("%s: called\n", __func__);

	EfiDevice *efiDevice = (EfiDevice *)device;
	disk_identifier identifier;

	identifier.bus_type = UNKNOWN_BUS;
	identifier.device_type = UNKNOWN_DEVICE;
	identifier.device.unknown.size = device->Size();

	for (uint32 i = 0; i < NUM_DISK_CHECK_SUMS; ++i) {
		off_t offset = get_next_check_sum_offset(i, device->Size());
		identifier.device.unknown.check_sums[i].offset = offset;
		identifier.device.unknown.check_sums[i].sum = compute_check_sum(device,
			offset);
	}

	// ...HARD_DISK, as we pick partition and have checksum (no need to use _CD)
	gBootVolume.SetInt32(BOOT_METHOD, BOOT_METHOD_HARD_DISK);
	gBootVolume.SetData(BOOT_VOLUME_DISK_IDENTIFIER, B_RAW_TYPE,
		&identifier, sizeof(disk_identifier));

	return B_OK;
}


void
platform_cleanup_devices()
{
}
