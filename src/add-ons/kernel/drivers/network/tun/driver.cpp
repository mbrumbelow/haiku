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
// #include <fsproto.h>
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
static status_t retreive_packet(void* cookie, void* data, size_t* numbytes);

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

<<<<<<< Updated upstream
BufferQueue appQ(30000);
BufferQueue interfaceQ(30000);
sem_id intSem;
sem_id appSem;
=======
BufferQueue gAppQueue(30000);
BufferQueue gIntQueue(30000);
sem_id gIntSem;
sem_id gAppSem;
>>>>>>> Stashed changes
struct net_buffer_module_info* gBufferModule;


static net_buffer*
create_filled_buffer(uint8* data, size_t bytes)
{
	net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL) {
		// dprintf("creating a buffer failed!\n");
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
retreive_packet(void* cookie, void* data, size_t* numbytes)
{
    BufferQueue* queueToUse = nullptr;
    net_buffer* buffer;
    status_t status = B_OK;

    if ((strcmp((char*)cookie, "app") == 0)) {
        queueToUse = &gAppQueue;
    } else if ((strcmp((char*)cookie, "tun") == 0)) {
        queueToUse = &gIntQueue;
    } else {
        return B_ERROR;
    }

    if (queueToUse->Used()) {
        status = queueToUse->Get(*numbytes, true, &buffer);
        if (status != B_OK) {
            // dprintf("getting packet failed: %s\n", strerror(status));
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
    // dprintf("tun:init_driver() at /dev/misc\n");
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
    delete_sem(gIntSem);
    delete_sem(gAppSem);
    dprintf("tun:uninit_driver()\n");
}


status_t
tun_open(const char* name, uint32 flags, void** cookie)
{
    /* Make interface here */
    if (strncmp(name, "misc/tun_interface", 19) == 0) {
        *cookie = (void*)"tun";
<<<<<<< Updated upstream
        if ((intSem = create_sem(1, "tun_notify_int")) < B_NO_ERROR) {
=======
        if ((gIntSem = create_sem(1, "tun_notify_int")) < B_NO_ERROR) {
>>>>>>> Stashed changes
            return B_ERROR;
        }
    } else { 
        *cookie = (void*)"app";
<<<<<<< Updated upstream
        if ((appSem = create_sem(0, "tun_notify_app")) < B_NO_ERROR) {
=======
        if ((gAppSem = create_sem(0, "tun_notify_app")) < B_NO_ERROR) {
>>>>>>> Stashed changes
            return B_ERROR;
        }
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
    status_t status;
<<<<<<< Updated upstream
	status = gBufferModule->read(buffer, 0, data, *numbytes);
	if (status != B_OK) {
		dprintf("Failed reading data\n");
        gBufferModule->free(buffer);
        return status;
    }
    return B_OK;
}


status_t
retreive_packet(void *cookie, void *data, size_t *numbytes)
{
    BufferQueue* queueToUse = nullptr;
    net_buffer* buffer;
    status_t status = B_OK;

    dprintf("cookie: %s | appQ: %li | interfaceQ: %li | numbytes: %li\n", (char*)cookie, appQ.Used(), interfaceQ.Used(), *numbytes);

    if ((strcmp((char*)cookie, "app") == 0)) {
        queueToUse = &appQ;
    } else if ((strcmp((char*)cookie, "tun") == 0)) {
        queueToUse = &interfaceQ;
    } else {
        return B_ERROR;
    }

    if (queueToUse->Used()) {
        status = queueToUse->Get(*numbytes, true, &buffer);
        if (status != B_OK) {
            // dprintf("getting packet failed: %s\n", strerror(status));
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
tun_read(void *cookie, off_t position, void *data, size_t *numbytes)
{
    status_t status;
=======
>>>>>>> Stashed changes
    if (strncmp((char*)cookie, "tun", 3) == 0) {
        status = acquire_sem_etc(gIntSem, 1, B_CAN_INTERRUPT, 0);
        if (status < B_OK) {
            *numbytes = 0;
            return status;
        }
    } else {
        status = acquire_sem_etc(gAppSem, 1, B_CAN_INTERRUPT, 0);
        if (status < B_OK) {
            dprintf("Could not acquire sem\n");
            *numbytes = 0;
            return status;
        }
    }

    status = retreive_packet(cookie, data, numbytes);
    if (status != B_OK) {
        return status;
    }
    return B_OK;
}


status_t
tun_write(void* cookie, off_t position, const void* data, size_t* numbytes)
{
<<<<<<< Updated upstream
    if ((strcmp((char*)cookie, "app") == 0) && (interfaceQ.Used() < 30000)) {
        interfaceQ.Add(create_filled_buffer((uint8 *)data, *numbytes));
        dprintf("interfaceQ: %li\n", interfaceQ.Used());
        release_sem(intSem);
    } else if ((strcmp((char*)cookie, "tun") == 0) && (appQ.Used() < 30000)) {
        appQ.Add(create_filled_buffer((uint8 *)data, *numbytes));
        dprintf("appQ: %li\n", appQ.Used());
        release_sem(appSem);
=======
    if ((strcmp((char*)cookie, "app") == 0) && (gIntQueue.Used() < 30000)) {
        gIntQueue.Add(create_filled_buffer((uint8*)data, *numbytes));
        dprintf("gIntQueue: %li\n", gIntQueue.Used());
        release_sem(gIntSem);
    } else if ((strcmp((char*)cookie, "tun") == 0) && (gAppQueue.Used() < 30000)) {
        gAppQueue.Add(create_filled_buffer((uint8*)data, *numbytes));
        dprintf("gAppQueue: %li\n", gAppQueue.Used());
        release_sem(gAppSem);
>>>>>>> Stashed changes
    } else { 
        return B_ERROR;
    }
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

