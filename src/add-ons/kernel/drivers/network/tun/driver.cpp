/*
 * /dev/config/tun network tunnel driver for BeOS
 * (c) 2003, mmu_man, revol@free.fr
 * licenced under MIT licence.
 */
#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>

#include "BufferQueue.h"
#include <net_buffer.h>

#include <condition_variable.h>
#include <net/if_tun.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>


// #define TUN_DRIVER_NAME "tun/0"
#define TAP_DRIVER_NAME "tap/0"
#define NET_TUN_MODULE_NAME "network/devices/tun/v1"
#define BUFFER_QUEUE_MAX 30000

const char *device_names[] = {TAP_DRIVER_NAME, NULL};

typedef struct tun_struct {
	uint32_t                    name[3];
	unsigned long               flags;
	BufferQueue*                sendQueue;
	BufferQueue*                recvQueue;
	ConditionVariable*          selfReadWait;
	ConditionVariable*          peerReadWait;
	mutex                       readLock;
} tun_struct;

int32 api_version = B_CUR_DRIVER_API_VERSION;

static net_buffer *create_filled_buffer(uint8* data, size_t bytes);
static status_t get_packet_data(void* data, size_t* numbytes, net_buffer* buffer);
static status_t retrieve_packet(BufferQueue* queueToUse, void* data, size_t* numbytes);
static status_t app_read_data(tun_struct* tun);
static status_t iface_read_data(tun_struct* tun);

status_t tun_open(const char* name, uint32 flags, void** cookie);
status_t tun_close(void* cookie);
status_t tun_free(void* cookie);
status_t tun_ioctl(void* cookie, uint32 op, void* data, size_t len);
status_t tun_read(void* cookie, off_t position, void* data, size_t* numbytes);
status_t tun_write(void* cookie, off_t position, const void* data, size_t* numbytes);

device_hooks tun_hooks = {
	tun_open,
	tun_close,
	tun_free,
	tun_ioctl,
	tun_read,
	tun_write,
	NULL,
	NULL,
	NULL,
	NULL};

static BufferQueue gAppQueue(BUFFER_QUEUE_MAX);
static BufferQueue gIntQueue(BUFFER_QUEUE_MAX);
static ConditionVariable gIntWait;
static ConditionVariable gAppWait;
struct net_buffer_module_info* gBufferModule;


/**
 * @brief Creates a filled net_buffer based on the given data and size.
 *
 * @param data the pointer to the data that will be copied to the net_buffer
 * @param bytes the size of the data in bytes
 *
 * @return a net_buffer pointer containing the copied data, or NULL if creation fails
 */
static net_buffer*
create_filled_buffer(uint8* data, size_t bytes)
{
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return NULL;

	status_t status = gBufferModule->append(buffer, data, bytes);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return NULL;
	}

	return buffer;
}


/**
 * @brief Retrieves packet data from the packet queue.
 *
 * @param data a pointer to the buffer where the packet data will be stored
 * @param numbytes a pointer to the variable that will hold the number of bytes read
 * @param buffer the packet to read from
 *
 * @return the status of the operation, indicating success or failure
 */
static status_t
get_packet_data(void* data, size_t* numbytes, net_buffer* buffer)
{
	status_t status;
	status = gBufferModule->read(buffer, 0, data, *numbytes);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return status;
	}
	return B_OK;
}


/**
 * @brief Retrieves a packet from the given buffer queue.
 *
 * @param queueToUse The buffer queue to retrieve the packet from.
 * @param data A pointer to the memory location where the packet data will be copied.
 * @param numbytes A pointer to the variable that will store the size of the retrieved packet.
 *
 * @return The status of the retrieval operation or B_OK if successful.
 */
static status_t
retrieve_packet(BufferQueue* queueToUse, void* data, size_t* numbytes)
{
	net_buffer* buffer;
	status_t status = B_OK;

	if (queueToUse->Used() > 0) {
		status = queueToUse->Get(*numbytes, true, &buffer);
		if (status != B_OK)
			return status;
		*numbytes = buffer->size;
		status = get_packet_data(data, numbytes, buffer);
		if (status != B_OK)
			return status;
	} else
		*numbytes = 0;
	return status;
}


/**
 * @brief Waits for data from the application.
 *
 * @param tun pointer to the tun_struct object
 *
 * @return the status of the wait operation
 */
static status_t app_read_data(tun_struct* tun)
{
	return tun->selfReadWait->Wait(&tun->readLock,
					B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 10000);
}


/**
 * @brief Waits for data from the interface.
 *
 * @param tun pointer to the tun_struct object
 *
 * @return the status of the wait operation
 */
static status_t iface_read_data(tun_struct* tun)
{
	return tun->selfReadWait->Wait(&tun->readLock, B_CAN_INTERRUPT);
}


/**
 * @brief First initialization step for the driver.
 *
 * @return the status of the initialization process.
 */
status_t
init_hardware(void)
{
	/* No Hardware */
	dprintf("tun:init_hardware()\n");
	return B_NO_ERROR;
}


/**
 * @brief Initializes the driver and the net buffer module for packet operations.
 *
 * @return the status of the initialization.
 */
status_t
init_driver(void)
{
	/* Init driver */
	dprintf("tun:init_driver()\n");
	status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
	if (status != B_OK)
		return status;
	return B_OK;
}


/**
 * @brief Uninitializes the driver.
 *
 * @return void
 */
void uninit_driver(void)
{
	dprintf("tun:uninit_driver()\n");
	put_module(NET_BUFFER_MODULE_NAME);
}


/**
 * @brief Sets the tun_struct for this session of open.
 *
 * @param name the name of the interface
 * @param flags the flags for the interface
 * @param cookie a pointer to store the cookie
 *
 * @return the status of the function
 */
status_t
tun_open(const char* name, uint32 flags, void** cookie)
{
	/* Setup driver for interface here */
	tun_struct* tun = new tun_struct();
	memcpy(tun->name, "app", sizeof(tun->name));
	tun->sendQueue = &gIntQueue;
	tun->recvQueue = &gAppQueue;
	tun->selfReadWait = &gAppWait;
	tun->peerReadWait = &gIntWait;
	tun->flags = 0;
	mutex_init(&tun->readLock, "read_avail");
	*cookie = static_cast<void*>(tun);
	return B_OK;
}


/**
 * @brief Close the interface.
 *
 * @param cookie a pointer to the tun_struct object to be used
 *
 * @return The status of the function
 */
status_t
tun_close(void* cookie)
{
	/* Close interface here */
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	tun->selfReadWait->NotifyAll(B_ERROR);
	snooze(10); // Due to a lock timing issue, we sleep for 10ms
	return B_OK;
}


/**
 * @brief Frees the memory allocated for a tun_struct object.
 *
 * @param cookie a pointer to the tun_struct object to be freed
 *
 * @return a status_t indicating the result of the operation
 */
status_t
tun_free(void* cookie)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	mutex_destroy(&tun->readLock);
	delete tun;
	return B_OK;
}


/**
 * @brief Updates the tun interface based on the given IOCTL operation and data.
 *
 * @param cookie a pointer to the tun_struct object
 * @param op the IOCTL operation to perform
 * @param data a pointer to the data to be used in the IOCTL operation
 * @param len the size of the data in bytes
 *
 * @return the status of the IOCTL operation
 */
status_t
tun_ioctl(void* cookie, uint32 op, void* data, size_t len)
{
	/* IOCTL for driver */
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	switch (op) {
		case TUNSETIFF: // Reconfigures tun_struct to interface settings
			memcpy(tun->name, "int", sizeof(tun->name));
			tun->sendQueue = &gAppQueue;
			tun->recvQueue = &gIntQueue;
			tun->selfReadWait = &gIntWait;
			tun->peerReadWait = &gAppWait;
			return B_OK;
		case B_SET_NONBLOCKING_IO:
			tun->flags |= B_SET_NONBLOCKING_IO;
			return B_OK;
		default:
			return B_DEV_INVALID_IOCTL;
	};

	return B_OK;
}


/**
 * @brief Reads data from the TUN/TAP device.
 *
 * @param cookie a pointer to the tun_struct
 * @param position the position in the data stream to read from
 * @param data a pointer to the buffer where the read data will be copied
 * @param numbytes a pointer to the size to read, updated with the number of bytes read
 *
 * @return the status of the read operation
 */
status_t
tun_read(void* cookie, off_t position, void* data, size_t* numbytes)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	status_t status;
	mutex_lock(&tun->readLock);
	while (tun->recvQueue->Used() == 0) {
		/* Since app side will always want non-blocking I/O */
		if (tun->flags & B_SET_NONBLOCKING_IO)
			status = app_read_data(tun);
		else
			status = iface_read_data(tun);

		if (status != B_OK) {
			mutex_unlock(&tun->readLock);
			return status;
		}
	}
	status = retrieve_packet(tun->recvQueue, data, numbytes);
	if (status != B_OK)
		return status;
	mutex_unlock(&tun->readLock);
	return B_OK;
}


/**
 * @brief Writes data to the specified sending queue.
 *
 * @param cookie a pointer to the tun_struct
 * @param position the position in the file to write to (NOT USED)
 * @param data a pointer to the data to write
 * @param numbytes a pointer to the number of bytes to write
 *
 * @return a status code indicating the result of the write operation
 */
status_t
tun_write(void* cookie, off_t position, const void* data, size_t* numbytes)
{
	tun_struct* tun = static_cast<tun_struct*>(cookie);
	size_t used = tun->sendQueue->Used();
	net_buffer* packet = NULL;
	// Buffer is full or will be so we have to drop the packet
	if ((used + *numbytes) >= BUFFER_QUEUE_MAX)
		return B_WOULD_BLOCK;
	packet = create_filled_buffer((uint8*)data, *numbytes);
	if (packet == NULL)
		return B_ERROR;
	tun->sendQueue->Add(packet);
	tun->peerReadWait->NotifyOne();
	return B_OK;
}


/**
 * @brief Publishes the driver names to devfs.
 *
 * @return A pointer to a constant character pointer representing the device names.
 */
const char**
publish_devices()
{
	/* Publish driver names to devfs */
	return device_names;
}


/**
 * @brief Finds hooks for driver functions.
 *
 * @param name the name of the device.
 *
 * @return a pointer to the device hooks (all devices return the same hooks)
 */
device_hooks*
find_device(const char* name)
{
	/* Find hooks for driver functions */
	return &tun_hooks;
}
