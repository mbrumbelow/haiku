#ifndef MALLOC_GLUE_H
#define MALLOC_GLUE_H

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <shared/locks.h>

#include "atomic.h"
#include "libc.h"

// use macros to appropriately namespace these.
#define size_classes __malloc_size_classes
#define ctx __malloc_context
#define alloc_meta __malloc_alloc_meta
#define is_allzero __malloc_allzerop
#define dump_heap __dump_heap

#define mmap __malloc_mmap
#define mremap(p,o,n,f) MAP_FAILED
#define brk(p) ((p)-1)

void* __malloc_mmap(void* address, size_t length, int protection, int flags,
	int _fd, off_t _offset);

static inline uint64_t get_random_secret()
{
	// TODO: Improve this.
	uint64_t secret = (uintptr_t)&secret * 1103515245;
	return secret;
}

#define MT 1
#define DISABLE_ALIGNED_ALLOC 0
#define RDLOCK_IS_EXCLUSIVE 0

#define LOCK_OBJ_DEF \
rw_lock __malloc_lock = RW_LOCK_INITIALIZER("malloc");

__attribute__((__visibility__("hidden")))
extern rw_lock __malloc_lock;

static inline void rdlock()
{
	__malloc_lock.lock.flags |= MUTEX_FLAG_ADAPTIVE;
	rw_lock_read_lock(&__malloc_lock);
}
static inline void wrlock()
{
	rw_lock_write_lock(&__malloc_lock);
}
static inline void unlock()
{
	rw_lock_read_unlock(&__malloc_lock);
}
static inline void upgradelock()
{
	// This is technically wrong, but the consumers of this function tolerate it.
	rw_lock_read_unlock(&__malloc_lock);
	rw_lock_write_lock(&__malloc_lock);
}

#endif
