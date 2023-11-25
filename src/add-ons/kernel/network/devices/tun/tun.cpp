/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Sean Brady, swangeon@gmail.com
 */

#include <new>
#include <string.h>

#include <fs/select_sync_pool.h>
#include <fs/devfs.h>
#include <util/AutoLock.h>

#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_tun.h>
#include <netinet/in.h>
#include <ethernet.h>


struct tun_device : net_device {
	net_fifo			send_queue, receive_queue;

	int32				open_count;

	mutex				select_lock;
	select_sync_pool*	select_pool;
};

#define TUN_QUEUES_MAX (ETHER_MAX_FRAME_SIZE * 32)


struct net_buffer_module_info* gBufferModule;
static net_stack_module_info* gStackModule;


//	#pragma mark - devices array


static tun_device* gDevices[10] = {};
static mutex gDevicesLock = MUTEX_INITIALIZER("TUN devices");


static tun_device*
find_tun_device(const char* name)
{
	ASSERT_LOCKED_MUTEX(&gDevicesLock);
	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] == NULL)
			continue;

		if (strcmp(gDevices[i]->name, name) == 0)
			return gDevices[i];
	}
	return NULL;
}


//	#pragma mark - devfs device


struct tun_cookie {
	tun_device*	device;
	uint32		flags;
};


status_t
tun_open(const char* name, uint32 flags, void** _cookie)
{
	MutexLocker devicesLocker(gDevicesLock);
	tun_device* device = find_tun_device(name);
	if (device == NULL)
		return ENODEV;
	if (atomic_or(&device->open_count, 1) != 0)
		return EBUSY;

	tun_cookie* cookie = new(std::nothrow) tun_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->device = device;
	cookie->flags = flags;

	*_cookie = cookie;
	return B_OK;
}


status_t
tun_close(void* _cookie)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	// Wake up the send queue, so that any threads waiting to read return at once.
	release_sem_etc(cookie->device->send_queue.notify, B_INTERRUPTED, B_RELEASE_ALL);

	return B_OK;
}


status_t
tun_free(void* _cookie)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;
	atomic_and(&cookie->device->open_count, 0);
	delete cookie;
	return B_OK;
}


status_t
tun_control(void* _cookie, uint32 op, void* data, size_t len)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	switch (op) {
		case B_SET_NONBLOCKING_IO:
			cookie->flags |= O_NONBLOCK;
			return B_OK;
		case B_SET_BLOCKING_IO:
			cookie->flags &= ~O_NONBLOCK;
			return B_OK;
	}

	return B_DEV_INVALID_IOCTL;
}


status_t
tun_read(void* _cookie, off_t position, void* data, size_t* _length)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	net_buffer* buffer = NULL;
	status_t status = gStackModule->fifo_dequeue_buffer(
		&cookie->device->send_queue, 0, B_INFINITE_TIMEOUT, &buffer);
	if (status != B_OK)
		return status;

	size_t offset = 0;
	if (true)
		offset = ETHER_HEADER_LENGTH;

	const size_t length = min_c(*_length, buffer->size - offset);
	status = gBufferModule->read(buffer, offset, data, length);
	if (status != B_OK)
		return status;
	*_length = length;

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
tun_write(void* _cookie, off_t position, const void* data, size_t* _length)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = gBufferModule->append(buffer, data, *_length);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	if (true) {
		uint8 version;
		status = gBufferModule->read(buffer, 0, &version, 1);
		if (status != B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		version &= 0xF;
		if (version != 4 && version != 6) {
			// Not any IP packet we recognize.
			gBufferModule->free(buffer);
			dprintf("TUN: invalid IP header version: %d\n", version);
			return B_BAD_DATA;
		}
		buffer->type = (version == 6) ? B_NET_FRAME_TYPE_IPV6
			: B_NET_FRAME_TYPE_IPV4;

		// Even loopback frames need an ethernet header.
		NetBufferPrepend<ether_header> bufferHeader(buffer);
		if (bufferHeader.Status() != B_OK)
			return bufferHeader.Status();

		ether_header &header = bufferHeader.Data();
		header.type = (version == 6) ? htons(ETHER_TYPE_IPV6)
			: htons(ETHER_TYPE_IP);

		memset(header.source, 0, ETHER_ADDRESS_LENGTH);
		memset(header.destination, 0, ETHER_ADDRESS_LENGTH);
		bufferHeader.Sync();
	}

	// We use a queue and the receive_data() hook instead of device_enqueue_buffer()
	// for two reasons: 1. listeners (e.g. packet capture) are only processed by the
	// reader thread that calls receive_data(), and 2. device_enqueue_buffer() has
	// to look up the device interface every time, which is inefficient.
	status = gStackModule->fifo_enqueue_buffer(&cookie->device->receive_queue, buffer);
	if (status != B_OK)
		gBufferModule->free(buffer);

	if (status == B_OK) {
		atomic_add((int32*)&cookie->device->stats.receive.packets, 1);
		atomic_add64((int64*)&cookie->device->stats.receive.bytes, buffer->size);
	} else {
		atomic_add((int32*)&cookie->device->stats.receive.errors, 1);
	}

	return status;
}


status_t
tun_select(void* _cookie, uint8 event, uint32 ref, selectsync* sync)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;

	MutexLocker selectLocker(cookie->device->select_lock);
	status_t status = add_select_sync_pool_entry(&cookie->device->select_pool, sync, event);
	if (status != B_OK)
		return B_BAD_VALUE;
	selectLocker.Unlock();

	MutexLocker fifoLocker(cookie->device->send_queue.lock);
	if (event == B_SELECT_READ && cookie->device->send_queue.current_bytes != 0)
		notify_select_event(sync, event);
	if (event == B_SELECT_WRITE)
		notify_select_event(sync, event);

	return B_OK;
}


status_t
tun_deselect(void* _cookie, uint8 event, selectsync* sync)
{
	tun_cookie* cookie = (tun_cookie*)_cookie;

	MutexLocker selectLocker(cookie->device->select_lock);
	if (event != B_SELECT_READ && event != B_SELECT_WRITE)
		return B_BAD_VALUE;
	return remove_select_sync_pool_entry(&cookie->device->select_pool, sync, event);
}


static device_hooks sDeviceHooks = {
	tun_open,
	tun_close,
	tun_free,
	tun_control,
	tun_read,
	tun_write,
	tun_select,
	tun_deselect,
};


//	#pragma mark - network stack device


status_t
tun_init(const char* name, net_device** _device)
{
	if (strncmp(name, "tun/", 4))
		return B_BAD_VALUE;
	if (strlen(name) >= sizeof(tun_device::name))
		return ENAMETOOLONG;

	// Make sure this device doesn't already exist.
	MutexLocker devicesLocker(gDevicesLock);
	if (find_tun_device(name) != NULL)
		return EEXIST;

	tun_device* device = new(std::nothrow) tun_device;
	if (device == NULL)
		return B_NO_MEMORY;

	ssize_t index = -1;
	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] != NULL)
			continue;

		gDevices[i] = device;
		index = i;
		break;
	}
	if (index < 0) {
		delete device;
		return ENOSPC;
	}
	devicesLocker.Unlock();

	memset(device, 0, sizeof(tun_device));
	strcpy(device->name, name);

	device->mtu = ETHER_MAX_FRAME_SIZE;
	device->media = IFM_ACTIVE;
	device->header_length = 0;

	device->flags = IFF_POINTOPOINT | IFF_LINK;
	device->type = IFT_TUN;

	status_t status = gStackModule->init_fifo(&device->send_queue,
		"TUN send queue", TUN_QUEUES_MAX);
	if (status != B_OK) {
		delete device;
		return status;
	}

	status = gStackModule->init_fifo(&device->receive_queue,
		"TUN receive queue", TUN_QUEUES_MAX);
	if (status != B_OK) {
		delete device;
		return status;
	}

	mutex_init(&device->select_lock, "TUN select lock");

	status = devfs_publish_device(name, &sDeviceHooks);
	if (status != B_OK) {
		delete device;
		return status;
	}

	*_device = device;
	return B_OK;
}


status_t
tun_uninit(net_device* _device)
{
	tun_device* device = (tun_device*)_device;

	MutexLocker devicesLocker(gDevicesLock);
	if (atomic_get(&device->open_count) != 0)
		return EBUSY;

	for (size_t i = 0; i < B_COUNT_OF(gDevices); i++) {
		if (gDevices[i] != device)
			continue;

		gDevices[i] = NULL;
		break;
	}
	status_t status = devfs_unpublish_device(device->name, false);
	if (status != B_OK)
		panic("devfs_unpublish_device failed: %" B_PRId32, status);

	gStackModule->uninit_fifo(&device->send_queue);
	gStackModule->uninit_fifo(&device->receive_queue);
	mutex_destroy(&device->select_lock);
	delete device;
	return B_OK;
}


status_t
tun_up(net_device* _device)
{
	return B_OK;
}


void
tun_down(net_device* _device)
{
	tun_device* device = (tun_device*)_device;

	// Wake up the receive queue, so that the reader thread returns at once.
	release_sem_etc(device->receive_queue.notify, B_INTERRUPTED, B_RELEASE_ALL);
}


status_t
tun_control(net_device* device, int32 op, void* argument, size_t length)
{
	return B_BAD_VALUE;
}


status_t
tun_send_data(net_device* _device, net_buffer* buffer)
{
	tun_device* device = (tun_device*)_device;
	status_t status = gStackModule->fifo_enqueue_buffer(
		&device->send_queue, buffer);
	if (status == B_OK) {
		atomic_add((int32*)&device->stats.send.packets, 1);
		atomic_add64((int64*)&device->stats.send.bytes, buffer->size);
	} else {
		atomic_add((int32*)&device->stats.send.errors, 1);
	}

	MutexLocker selectLocker(device->select_lock);
	notify_select_event_pool(device->select_pool, B_SELECT_READ);
	return status;
}


status_t
tun_receive_data(net_device* _device, net_buffer** _buffer)
{
	tun_device* device = (tun_device*)_device;
	return gStackModule->fifo_dequeue_buffer(&device->receive_queue,
		0, B_INFINITE_TIMEOUT, _buffer);
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


net_device_module_info sTunModule = {
	{
		"network/devices/tun/v1",
		0,
		NULL
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

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{}
};

module_info* modules[] = {
	(module_info*)&sTunModule,
	NULL
};
