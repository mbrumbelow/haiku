/*
 * /dev/config/tun network tunnel driver for BeOS
 * (c) 2003, mmu_man, revol@free.fr
 * licenced under MIT licence.
 */


#include <net_tun.h>

#include <net_buffer.h>
#include <net_device.h>
#include <net_stack.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>

#include <fcntl.h>
#include <fsproto.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#define TUN_MODULE_NAME "net/tun"

#define TUN_DEVICE_MODULE_NAME "drivers/network/drivertun_v1"

struct net_buffer_module_info* gBufferModule;
static struct net_stack_module_info* sStackModule;

const char * device_names[] = {TUN_MODULE_NAME, NULL};

device_hooks tun_hooks = {
	(device_open_hook)tun_open,
	(device_close_hook) tun_close,
	(device_free_hook)tun_free,
	(device_control_hook)tun_ioctl,
	(device_read_hook)tun_read,
	(device_write_hook)tun_write,
	(device_readv_hook)tun_readv,
	(device_writev_hook)tun_writev
};

int32 api_version = B_CUR_DRIVER_API_VERSION;

tun_struct* gUtil = NULL;


status_t
init_hardware(void)
{
	/* No Hardware */
	return B_OK;
}


status_t
init_driver(void)
{
	dprintf("tun:init_driver()\n");
	return B_OK;
}


void
uninit_driver(void)
{
}


const char**
publish_devices()
{
	return device_names;
}


device_hooks*
find_device(const char *name)
{
	(void)name;
	return &tun_hooks;
}


status_t
tun_open(const char *name, uint32 flags, void* cookie)
{
	// status_t err = B_OK;

	// dprintf("tun:open(%s, 0x%08lx,)\n", name, flags);

	// err = get_module(TUN_DEVICE_MODULE_NAME, (struct module_info **)&gUtil);
	// if (err < B_OK)
	// 	return err;

	// cookie = (void*)malloc(sizeof(cookie_t));
	// if (*cookie == NULL) {
	// 	dprintf("tun_open : error allocating cookie\n");
	// 	goto err0;
	// }
	// memset(*cookie, 0, sizeof(cookie_t));
	// (*cookie)->blocking_io = true;
	return B_OK;

// err1:
// 	dprintf("tun_open : cleanup : will free cookie\n");
// 	free(cookie);
// 	*cookie = NULL;
// 	put_module(TUN_DEVICE_MODULE_NAME);
// err0:
// 	return B_ERROR;
}


status_t
tun_close(void *cookie)
{
	// (void)cookie;
	// return B_OK;
}


status_t
tun_free(cookie_t *cookie)
{
	return B_OK
}


status_t
tun_ioctl(cookie_t *cookie, uint32 op, void *data, size_t len)
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
}


status_t
tun_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes)
{
}


status_t
tun_write(cookie_t *cookie, off_t position, const void *data, size_t *numbytes)
{
}


status_t
tun_readv(cookie_t *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
	return EOPNOTSUPP;
}


status_t
tun_writev(cookie_t *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes)
{
	return EOPNOTSUPP;
}
