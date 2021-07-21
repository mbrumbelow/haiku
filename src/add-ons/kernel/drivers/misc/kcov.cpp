/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <atomic>

#include <Drivers.h>
#include <KernelExport.h>

#include <kcov.h>
#include <thread.h>
#include <util/AutoLock.h>


static status_t kcov_open(const char*, uint32, void**);
static status_t kcov_close(void*);
static status_t kcov_free(void*);
static status_t kcov_control(void*, uint32, void*, size_t);
static status_t kcov_read(void*, off_t, void*, size_t*);
static status_t kcov_write(void*, off_t, const void*, size_t*);


static const char* kcov_name[] = {
    "misc/kcov",
    NULL
};


device_hooks kcov_hooks = {
	kcov_open,
	kcov_close,
	kcov_free,
	kcov_control,
	kcov_read,
	kcov_write,
};

int32 api_version = B_CUR_DRIVER_API_VERSION;

static mutex gKcovLock;


status_t
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	mutex_init(&gKcovLock, " kcov lock");
	return B_OK;
}


void
uninit_driver(void)
{
}


const char**
publish_devices(void)
{
	return kcov_name;
}


device_hooks*
find_device(const char* name)
{
	return &kcov_hooks;
}


//	#pragma mark -


status_t
kcov_open(const char* name, uint32 flags, void** cookie)
{
	if (getuid() != 0 && geteuid() != 0)
		return EPERM;

	struct thread_kcov_info* info = (struct thread_kcov_info*)
		malloc(sizeof(struct thread_kcov_info));
	if (info == NULL)
		return B_NO_MEMORY;

	*cookie = info;
	info->buffer = NULL;
	info->count = 0;
	info->area = B_ERROR;
	info->mode = -1;
	info->state = KCOV_STATE_OPEN;

	return B_OK;
}


status_t
kcov_close(void* cookie)
{
	struct thread_kcov_info* info = (struct thread_kcov_info*)cookie;
	if (info->state == KCOV_STATE_RUNNING)
		return B_BUSY;

	info->state = KCOV_STATE_CLOSED;
	return B_OK;
}


status_t
kcov_free(void* cookie)
{
	struct thread_kcov_info* info = (struct thread_kcov_info*)cookie;
	if (info->area >= B_OK)
		delete_area(info->area);
	free(info);
	return B_OK;
}


status_t
kcov_control(void* cookie, uint32 op, void* arg, size_t length)
{
	struct thread_kcov_info* info = (struct thread_kcov_info*)cookie;
	Thread *thread = thread_get_current_thread();
	if (thread == NULL)
		return B_BAD_VALUE;

	MutexLocker locker(gKcovLock);
	switch (op) {
		case KIOENABLE:
		{
			if (info->state != KCOV_STATE_READY)
				return B_BUSY;
			if (thread->kcov_info != NULL)
				return B_BAD_VALUE;

			int mode;
			if (user_memcpy(&mode, arg, sizeof(mode)) != B_OK)
				return B_BAD_ADDRESS;
			if (mode != KCOV_MODE_TRACE_PC && mode != KCOV_MODE_TRACE_CMP)
				return B_BAD_VALUE;
			info->mode = mode;
			info->thread = thread;
			info->state = KCOV_STATE_RUNNING;
			thread->kcov_info = info;
			std::atomic_signal_fence(std::memory_order_acq_rel);

			return B_OK;
		}

		case KIODISABLE:
		{
			if (info->state != KCOV_STATE_RUNNING || info != thread->kcov_info)
				return B_BAD_VALUE;
			info->state = KCOV_STATE_READY;
			std::atomic_signal_fence(std::memory_order_acq_rel);
			thread->kcov_info = NULL;
			info->mode = -1;
			info->thread = NULL;
			return B_OK;
		}
		case KIOSETBUFSIZE:
		{
			if (info->state != KCOV_STATE_OPEN)
				return B_BUSY;
			int count;
			if (user_memcpy(&count, arg, sizeof(count)) != B_OK)
				return B_BAD_ADDRESS;

			if (count < 2 || count > KCOV_MAXENTRIES)
				return B_BAD_VALUE;
			size_t size = ROUNDUP(count * KCOV_ENTRY_SIZE, B_PAGE_SIZE);
			info->area = create_area("kcov buffer", (void**)&info->buffer,
				B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
				B_CLONEABLE_AREA);
			((uint64*)info->buffer)[0] = 0;
			if (info->area < B_OK)
				return info->area;
			info->count = count;
			info->state = KCOV_STATE_READY;
			return B_OK;
		}
		case KIOGETAREA:
		{
			if (info->state != KCOV_STATE_READY)
				return B_BAD_VALUE;
			if (user_memcpy(arg, &info->area, sizeof(info->area)) != B_OK)
				return B_BAD_ADDRESS;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


status_t
kcov_read(void* cookie, off_t position, void* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_NOT_ALLOWED;
}


status_t
kcov_write(void* cookie, off_t position, const void* buffer, size_t* numBytes)
{
	*numBytes = 0;
	return B_NOT_ALLOWED;
}
