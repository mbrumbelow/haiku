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
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <util/queue.h>


#define TUN_MODULE_NAME "tun_driver"

#define NET_TUN_MODULE_NAME "network/devices/tun/v1"

const char * device_names[] = {"misc/" TUN_MODULE_NAME, NULL};

int32 api_version = B_CUR_DRIVER_API_VERSION;


status_t tun_open(const char *name, uint32 flags, void **cookie);
status_t tun_close(void *cookie);
status_t tun_free(void *cookie);
status_t tun_ioctl(void *cookie, uint32 op, void *data, size_t len);
status_t tun_read(void *cookie, off_t position, void *data, size_t *numbytes);
status_t tun_write(void *cookie, off_t position, const void *data, size_t *numbytes);
status_t tun_readv(void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes);
status_t tun_writev(void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes);


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

BufferQueue appQ(3000);
BufferQueue interfaceQ(3000);


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
    dprintf("tun:init_driver() at /dev/misc\n");
    size_t appAvail = appQ.Available();
    size_t interfaceAvail = interfaceQ.Available();
    dprintf("%li bytes available in appQ\n", appAvail);
    dprintf("%li bytes available in interfaceQ\n", interfaceAvail);
    return B_OK;
}


void
uninit_driver(void)
{
    dprintf("tun:uninit_driver()\n");
}


status_t
tun_open(const char *name, uint32 flags, void **cookie)
{
    /* Make interface here */
    dprintf("tun:open_driver with name %s and flags %u\n", name, flags);
    if (flags == 2) {
        *cookie = (void*)"tun";
        dprintf("Marked as interface for driver\n");
    } else { 
        *cookie = NULL;
        dprintf("Marked as application for driver\n");
    }
    return B_OK;
}


status_t
tun_close(void *cookie)
{
    /* Close interface here */
    dprintf("tun:close_driver()\n");
    // (void)cookie;
    return B_OK;
}


status_t
tun_free(void *cookie)
{
    return B_OK;
}


status_t
tun_ioctl(void *cookie, uint32 op, void *data, size_t len)
{
// 	switch (op) {
// 	case B_SET_NONBLOCKING_IO:
// 		cookie->blocking_io = false;
// 		return B_OK;
// 	case B_SET_BLOCKING_IO:
// 		cookie->blocking_io = true;
// 		return B_OK;
// 	case TUNSETNOCSUM:
// 		return B_OK;//EOPNOTSUPP;
// 	case TUNSETDEBUG:
// 		return B_OK;//EOPNOTSUPP;
// 	case TUNSETIFF:
// 		if (data == NULL)
// 			return EINVAL;
// 		ifr = (ifreq_t *)data;

// 		iface = gIfaceModule->tun_reuse_or_create(ifr, cookie);
// 		if (iface != NULL) {
// 			dprintf("tun: new tunnel created: %s, flags: 0x%08lx\n", ifr->ifr_name, iface->flags);
// 			return B_OK;
// 		} else
// 			dprintf("tun: can't allocate a new tunnel!\n");
// 		break;

// 	case SIOCGIFHWADDR:
// 		if (data == NULL)
// 			return EINVAL;
// 		ifr = (ifreq_t *)data;
// 		if (iface == NULL)
// 			return EINVAL;
// 		if (strncmp(ifr->ifr_name, iface->ifn->if_name, IFNAMSIZ) != 0)
// 			return EINVAL;
// 		memcpy(ifr->ifr_hwaddr, iface->fakemac.octet, 6);
// 		return B_OK;
// 	case SIOCSIFHWADDR:
// 		if (data == NULL)
// 			return EINVAL;
// 		ifr = (ifreq_t *)data;
// 		if (iface == NULL)
// 			return EINVAL;
// 		if (strncmp(ifr->ifr_name, iface->ifn->if_name, IFNAMSIZ) != 0)
// 			return EINVAL;
// 		memcpy(iface->fakemac.octet, ifr->ifr_hwaddr, 6);
// 		return B_OK;

// 	}
// 	return B_ERROR;
    return B_OK;
}


status_t
get_packet_data(void *data, size_t *numbytes, net_buffer* buffer)
{
    struct net_buffer_module_info* gBufferModule;
    status_t status;
    status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
    if (status != B_OK){
        dprintf("Getting BufferModule failed\n");
        return status;
    }
	ASSERT(buffer->size == *numbytes);
    // set data = to the uint8* byte stream?
	status = gBufferModule->read(buffer, 0, data, *numbytes);
	if (status != B_OK) {
		dprintf("Failed reading data\n");
        gBufferModule->free(buffer);
        put_module(NET_BUFFER_MODULE_NAME);
        return status;
    }
    put_module(NET_BUFFER_MODULE_NAME);
    return B_OK;
}


status_t
tun_read(void *cookie, off_t position, void *data, size_t *numbytes)
{
    /* Read data from driver 
    TODO:
        1. If cookie is null, read from appQ else read from interfaceQ
    */
    status_t status;
    net_buffer* buffer = NULL;
    dprintf("TUN: Reading %li bytes of data\n", *numbytes);
    if (strcmp((char*)cookie, "tun") == 0) {
        status = interfaceQ.Get(*numbytes, true, &buffer);
        if (status != B_OK) {
            dprintf("getting packet failed: %s\n", strerror(status));
            return status;
        }
        // Returns full net_buffer packet to interface instead of data itself
        data = buffer;
        return B_OK;
    } else {
        status = appQ.Get(*numbytes, true, &buffer);
        if (status != B_OK) {
            dprintf("getting packet failed: %s\n", strerror(status));
            return status;
        }
        status = get_packet_data(data, numbytes, buffer);
        if (status != B_OK) {
            dprintf("getting %lu bytes failed: %s\n", *numbytes, strerror(status));
            return status;
        }
    }
    return B_OK;
}


static net_buffer*
create_buffer(const void *data, size_t *numbytes)
{
    // This first part kernel panics the system I believe it is specifically gBufferModule->create
    struct net_buffer_module_info* gBufferModule;
    status_t status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK) {
		dprintf("Get Mod Failed: %s\n", strerror(status));
		return NULL;
	}
    net_buffer* buffer = gBufferModule->create(256);
	if (buffer == NULL) {
		dprintf("creating a buffer failed!\n");
		return NULL;
	}
    status = gBufferModule->append(buffer, data, *numbytes);
	if (status != B_OK) {
		dprintf("appending %lu bytes to buffer %p failed: %s\n", *numbytes, buffer,
			strerror(status));
		gBufferModule->free(buffer);
		return NULL;
	}
	put_module(NET_BUFFER_MODULE_NAME);
	return buffer;
}


status_t
tun_write(void *cookie, off_t position, const void *data, size_t *numbytes)
{
    /* Write data to driver 
    TODO:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
        1. When tun interface opens we need to do a write that will set the cookie for the interface
        2. Need to contact interface (tun interface) to grab its IP address for later comparisons
        3. Store IP address or tun interface name in cookie?
    */
    dprintf("tun:write_driver: writting %s with length of %li bytes\n", (char*)data, *numbytes);
    if (!cookie) {
        if (strcmp((char*)data, "tun") == 0) {
            cookie = (void*)"tun";
            dprintf("Cookie set to %s\n", (char*)cookie);
        } else {
            dprintf("Appending to interfaceQ\n");
            interfaceQ.Add(create_buffer(data, numbytes));
        }
    } else {
        dprintf("%s appending to appQ\n", (char*)cookie);
        appQ.Add(create_buffer(data, numbytes));
    }
    return B_OK;
}


status_t
tun_readv(void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
    return EOPNOTSUPP;
}


status_t
tun_writev(void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
    return EOPNOTSUPP;
}


const char**
publish_devices()
{
    return device_names;
}


device_hooks*
find_device(const char *name)
{
    return &tun_hooks;
}

