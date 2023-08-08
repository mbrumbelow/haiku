 /* Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include <ethernet.h>
#include <net_buffer.h>
#include <net_datalink.h>
#include <net_device.h>
#include <net_stack.h>
#include <net_tun.h>

#include <ByteOrder.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <KernelExport.h>
#include <NetBufferUtilities.h>

#include <debug.h>
#include <errno.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <new>
#include <stdlib.h>
#include <string.h>


struct tun_device : net_device, DoublyLinkedListLinkImpl<tun_device> {
	~tun_device()
	{
		free(read_buffer);
		free(write_buffer);
	}

	int		fd;
	uint32	frame_size;

	void* read_buffer, *write_buffer;
	mutex read_buffer_lock, write_buffer_lock;
};

struct net_buffer_module_info* gBufferModule;
static struct net_stack_module_info* sStackModule;


//	#pragma mark -


static status_t
prepend_ethernet_frame(net_buffer* buffer)
{
	NetBufferPrepend<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK) {
		dprintf("Failed Making ETH Header\n");
		return bufferHeader.Status();
	}
	ether_header& header = bufferHeader.Data();

	header.type = B_HOST_TO_BENDIAN_INT16(ETHER_TYPE_IP);

	memset(header.source, 0, ETHER_ADDRESS_LENGTH);
	memset(header.destination, 0, ETHER_ADDRESS_LENGTH);
	bufferHeader.Sync();
	return B_OK;
}


static status_t
ethernet_header_deframe(net_buffer* buffer)
{
	// there is not that much to do...
	NetBufferHeaderRemover<ether_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	return B_OK;
}


status_t
tun_init(const char* name, net_device** _device)
{
	if (strncmp(name, "tun", 3)
		&& strncmp(name, "tap", 3)
		&& strncmp(name, "dns", 3))	/* iodine uses that */
		return B_BAD_VALUE;

	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK) {
		dprintf("Get Mod Failed\n");
		return status;
	}

	tun_device* device = new (std::nothrow) tun_device;
	if (device == NULL) {
		put_module(NET_BUFFER_MODULE_NAME);
		return B_NO_MEMORY;
	}

	// struct net_hardware_address* mac_addr = new (std::nothrow) ;
	memset(device, 0, sizeof(tun_device));
	strcpy(device->name, name);

	if (strncmp(name, "tun", 3) == 0) {
		device->flags = IFF_POINTOPOINT | IFF_LINK;
		device->type = IFT_ETHER;
	} else if (strncmp(name, "tap", 3) == 0) {
		device->flags = IFF_BROADCAST | IFF_ALLMULTI | IFF_LINK;
		device->type = IFT_ETHER;
		for (int i {0}; i < 6; i++) {
			device->address.data[i] = i * 0x11;
		}
		device->address.length = 6;
	} else {
		return B_BAD_VALUE;
	}

	device->mtu = 1500;
	device->media = IFM_ACTIVE | IFM_ETHER;
	device->header_length = ETHER_HEADER_LENGTH;
	device->fd = -1;
	device->read_buffer_lock = MUTEX_INITIALIZER("tun read_buffer"),
	device->write_buffer_lock = MUTEX_INITIALIZER("tun write_buffer");

	*_device = device;
	return B_OK;
}


status_t
tun_uninit(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	put_module(NET_STACK_MODULE_NAME);
	put_module(NET_BUFFER_MODULE_NAME);
	close(device->fd);
	device->fd = -1;
	delete device;
	return B_OK;
}


status_t
tun_up(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	device->fd = open("/dev/misc/tun_interface", O_RDWR);
	if (device->fd < 0) {
		dprintf("Module Name %s failed in opening driver\n", device->name);
		return errno;
	}
	device->frame_size = ETHER_MAX_FRAME_SIZE;
	return B_OK;
}


void
tun_down(net_device* _device)
{
	tun_device* device = (tun_device*)_device;
	delete device;
	/* TODO: Get driver to be closed and opened */
	// close(device->fd);
	// device->fd = -1;
}


status_t
tun_control(net_device* device, int32 op, void* argument,
	size_t length)
{
	return B_BAD_VALUE;
}


status_t
tun_send_data(net_device* _device, net_buffer* buffer)
{
	tun_device* device = (tun_device*)_device;
	status_t status;

	if (strncmp(device->name, "tun", 3) == 0) {
		status = ethernet_header_deframe(buffer);
		if (status != B_OK) {
			return B_ERROR;
		}
	}

	if (buffer->size > device->mtu)
		return B_BAD_VALUE;
	
	net_buffer* allocated = NULL;
	net_buffer* original = buffer;

	MutexLocker bufferLocker;
	struct iovec iovec;
	if (gBufferModule->count_iovecs(buffer) > 1) {
		if (device->write_buffer != NULL) {
			bufferLocker.SetTo(device->write_buffer_lock, false);
			status_t status = gBufferModule->read(buffer, 0,
				device->write_buffer, buffer->size);
			if (status != B_OK)
				return status;
			iovec.iov_base = device->write_buffer;
			iovec.iov_len = buffer->size;
		} else {
			// Fall back to creating a new buffer.
			allocated = gBufferModule->duplicate(original);
			if (allocated == NULL)
				return ENOBUFS;

			buffer = allocated;

			if (gBufferModule->count_iovecs(allocated) > 1) {
				dprintf("tun_send_data: no write buffer, cannot perform scatter I/O\n");
				gBufferModule->free(allocated);
				device->stats.send.errors++;
				return B_NOT_SUPPORTED;
			}

			gBufferModule->get_iovecs(buffer, &iovec, 1);
		}
	} else {
		gBufferModule->get_iovecs(buffer, &iovec, 1);
	}

	ssize_t bytesWritten = write(device->fd, iovec.iov_base, iovec.iov_len);
	if (bytesWritten < 0) {
		device->stats.send.errors++;
		if (allocated)
			gBufferModule->free(allocated);
		return errno;
	}

	device->stats.send.packets++;
	device->stats.send.bytes += bytesWritten;

	gBufferModule->free(original);
	if (allocated)
		gBufferModule->free(allocated);

	return B_OK;
}


status_t
tun_receive_data(net_device* _device, net_buffer** _buffer)
{
	/* Recieve Data */
	tun_device* device = (tun_device*)_device;
	if (device->fd == -1)
		return B_FILE_ERROR;

	// TODO: better header space
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return ENOBUFS;

	MutexLocker bufferLocker;
	struct iovec iovec;
	size_t bytesRead;
	status_t status;
	if (device->read_buffer != NULL) {
		bufferLocker.SetTo(device->read_buffer_lock, false);

		iovec.iov_base = device->read_buffer;
		iovec.iov_len = device->frame_size;
	} else {
		void* data;
		status = gBufferModule->append_size(buffer, device->mtu, &data);
		if (status == B_OK && data == NULL) {
			dprintf("tun_receive_data: no read buffer, cannot perform scattered I/O!\n");
			status = B_NOT_SUPPORTED;
		}
		if (status < B_OK) {
			dprintf("Error in making net_buffer\n");
			gBufferModule->free(buffer);
			return status;
		}

		iovec.iov_base = data;
		iovec.iov_len = device->frame_size;
	}

	dprintf("Reading Bytes\n");
	bytesRead = read(device->fd, iovec.iov_base, iovec.iov_len);
	if (bytesRead < 0 || iovec.iov_base == NULL) {
		device->stats.receive.errors++;
		status = errno;
		dprintf("Error in Read\n");
		gBufferModule->free(buffer);
		return status;
	}
	dprintf("Got Bytes\n");

	if (strncmp(device->name, "tun", 3) == 0) {
		status = prepend_ethernet_frame(buffer);
		if (status != B_OK) {
			return status;
		}
	}

	if (iovec.iov_base == device->read_buffer)
		status = gBufferModule->append(buffer, iovec.iov_base, buffer->size);
	else
		status = gBufferModule->trim(buffer, buffer->size);
	if (status < B_OK) {
		device->stats.receive.dropped++;
		gBufferModule->free(buffer);
		return status;
	}

	device->stats.receive.bytes += buffer->size;
	device->stats.receive.packets++;

	*_buffer = buffer;
	return B_OK;
}


status_t
tun_set_mtu(net_device* device, size_t mtu)
{
	if (mtu > 65536 || mtu < 16)
		return B_BAD_VALUE;
	device->mtu = mtu;
	return B_OK;
}


status_t
tun_set_promiscuous(net_device* device, bool promiscuous)
{
	return EOPNOTSUPP;
}


status_t
tun_set_media(net_device* device, uint32 media)
{
	return EOPNOTSUPP;
}


status_t
tun_add_multicast(net_device* device, const sockaddr* address)
{
	return B_OK;
}


status_t
tun_remove_multicast(net_device* device, const sockaddr* address)
{
	return B_OK;
}


static status_t
tun_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status = get_module(NET_STACK_MODULE_NAME,
				(module_info**)&sStackModule);
			if (status < B_OK)
				return status;
			status = get_module(NET_BUFFER_MODULE_NAME,
				(module_info**)&gBufferModule);
			if (status < B_OK) {
				put_module(NET_STACK_MODULE_NAME);
				return status;
			}
			return B_OK;
		}
		case B_MODULE_UNINIT:
			put_module(NET_BUFFER_MODULE_NAME);
			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		default:
			return B_ERROR;
	}
}


net_device_module_info sTunModule = {
	{
		"network/devices/tun/v1",
		0,
		tun_std_ops
	},
	tun_init,
	tun_uninit,
	tun_up,
	tun_down,
	tun_control,
	tun_send_data,
	tun_receive_data,
	tun_set_mtu,
	tun_set_promiscuous,
	tun_set_media,
	tun_add_multicast,
	tun_remove_multicast,
};


module_info* modules[] = {
	(module_info*)&sTunModule,
	NULL
};
