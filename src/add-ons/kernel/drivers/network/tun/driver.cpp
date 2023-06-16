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

#include <fcntl.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>


#define TUN_DRIVER_NAME "misc/tun_driver"

#define NET_TUN_MODULE_NAME "network/devices/tun/v1"

const char* device_names[] = {TUN_DRIVER_NAME, "misc/tun_interface", NULL};

int32 api_version = B_CUR_DRIVER_API_VERSION;


static net_buffer* create_filled_buffer(uint8* data, size_t bytes);
static status_t get_packet_data(void* data, size_t* numbytes, net_buffer* buffer);
static status_t retreive_packet(BufferQueue* queueToUse, void* data, size_t* numbytes);

status_t tun_open(const char* name, uint32 flags, void** cookie);
status_t tun_close(void* cookie);
status_t tun_free(void* cookie);
status_t tun_ioctl(void* cookie, uint32 op, void* data, size_t len);
status_t tun_read(void* cookie, off_t position, void* data, size_t* numbytes);
status_t tun_write(void* cookie, off_t position, const void* data, size_t* numbytes);
status_t tun_readv(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes);
status_t tun_writev(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes);


device_hooks tun_hooks = {
        tun_open,
        tun_close,
        tun_free,
        tun_ioctl,
        tun_read,
        tun_write,
        NULL,
        NULL,
        tun_readv,
        tun_writev
};


typedef struct tun_struct {
	sem_id selfSendSem;
	sem_id selfRecvSem;
    sem_id* sendSem;
	sem_id* recvSem;
	BufferQueue* sendQueue;
    BufferQueue* recvQueue;
}tun_struct;


BufferQueue gAppQueue(30000);
BufferQueue gIntQueue(30000);
sem_id gIntReadSem;
sem_id gAppReadSem;
sem_id gIntWriteSem;
sem_id gAppWriteSem;
struct net_buffer_module_info* gBufferModule;


static net_buffer*
create_filled_buffer(uint8* data, size_t bytes)
{
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL) {
		return NULL;
	}

	status_t status = gBufferModule->append(buffer, data, bytes);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return NULL;
	}
	
	return buffer;
}


static status_t
get_packet_data(void* data, size_t* numbytes, net_buffer* buffer)
{
    status_t status;
	status = gBufferModule->read(buffer, 0, data, *numbytes);
	if (status != B_OK) {
		dprintf("Failed reading data\n");
        gBufferModule->free(buffer);
        return status;
    }
    return B_OK;
}


static status_t
retreive_packet(BufferQueue* queueToUse, void* data, size_t* numbytes)
{
    net_buffer* buffer;
    status_t status = B_OK;

    if (queueToUse->Used()) {
        status = queueToUse->Get(*numbytes, true, &buffer);
        if (status != B_OK) {
            return status;
        }
        *numbytes = buffer->size;
        status = get_packet_data(data, numbytes, buffer);
        if (status != B_OK) {
            dprintf("Couldn't get packet data\n");
            return B_ERROR;
        }
        status = B_OK;
    }
    return status;
}


status_t
init_hardware(void)
{
    /* No Hardware */
    return B_OK;
}


status_t
init_driver(void)
{
    /* Init driver */
    status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule);
    if (status != B_OK) {
        dprintf("Getting BufferModule failed\n");
        return status;
    }
    return B_OK;
}


void
uninit_driver(void)
{
    put_module(NET_BUFFER_MODULE_NAME);
    delete_sem(gIntReadSem);
    delete_sem(gAppReadSem);
    delete_sem(gIntWriteSem);
    delete_sem(gAppWriteSem);
    dprintf("tun:uninit_driver()\n");
}


status_t
tun_open(const char* name, uint32 flags, void** cookie)
{
    /* Setup driver for interface here */
    tun_struct* tun = new tun_struct();
    if (strncmp(name, "misc/tun_interface", 18) == 0) {
        gIntReadSem = create_sem(0, "tun_notify_read");
        if (gIntReadSem < B_NO_ERROR) {
            return B_ERROR;
        }
        gIntWriteSem = create_sem(1, "tun_notify_write");
        if (gIntWriteSem < B_NO_ERROR) {
            return B_ERROR;
        }
        tun->selfSendSem = gIntWriteSem;
        tun->selfRecvSem = gIntReadSem;
        tun->sendSem = &gAppWriteSem;
        tun->recvSem = &gAppReadSem;
        tun->sendQueue = &gAppQueue;
        tun->recvQueue = &gIntQueue;
        *cookie = static_cast<void*>(tun);
    } else {
        gAppReadSem = create_sem(0, "app_notify_read");
        if (gAppReadSem < B_NO_ERROR) {
            return B_ERROR;
        }
        gAppWriteSem = create_sem(1, "app_notify_write");
        if (gAppWriteSem < B_NO_ERROR) {
            return B_ERROR;
        }
        tun->selfSendSem = gAppWriteSem;
        tun->selfRecvSem = gAppReadSem;
        tun->sendSem = &gIntWriteSem;
        tun->recvSem = &gIntReadSem;
        tun->sendQueue = &gIntQueue;
        tun->recvQueue = &gAppQueue;
        *cookie = static_cast<void*>(tun);
    }

    return B_OK;
}


status_t
tun_close(void* cookie)
{
    /* Close interface here */
    dprintf("tun:close_driver()\n");
    cookie = NULL;
    return B_OK;
}


status_t
tun_free(void* cookie)
{
    return B_OK;
}


status_t
tun_ioctl(void* cookie, uint32 op, void* data, size_t len)
{
    return B_OK;
}


status_t
tun_read(void* cookie, off_t position, void* data, size_t* numbytes)
{
    tun_struct* tun = static_cast<tun_struct*>(cookie);
    status_t status;
    status = acquire_sem_etc(tun->selfRecvSem, 1, B_CAN_INTERRUPT, 0);
    if (status < B_OK) {
        dprintf("Could not acquire receiving sem\n");
        *numbytes = 0;
        return status;
    }
    status = retreive_packet(tun->recvQueue, data, numbytes);
    if (status != B_OK) {
        return status;
    }
    release_sem(*(tun->sendSem));
    return B_OK;
}


status_t
tun_write(void* cookie, off_t position, const void* data, size_t* numbytes)
{
    tun_struct* tun = static_cast<tun_struct*>(cookie);
    status_t status;
    status = acquire_sem_etc(tun->selfSendSem, 1, B_CAN_INTERRUPT, 0);
    if (status < B_OK) {
        dprintf("Could not acquire sending sem\n");
        *numbytes = 0;
        return status;
    }
    tun->sendQueue->Add(create_filled_buffer((uint8*)data, *numbytes));
    release_sem(*(tun->recvSem));
    return B_OK;
}


status_t
tun_readv(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes)
{
    return EOPNOTSUPP;
}


status_t
tun_writev(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes)
{
    return EOPNOTSUPP;
}


const char**
publish_devices()
{
    return device_names;
}


device_hooks*
find_device(const char* name)
{
    return &tun_hooks;

}

