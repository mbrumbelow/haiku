#include <OS.h>
#include "syscalls.h"

void __arch_clear_cache(void *address, size_t length, uint32 flags)
{
    _kern_clear_caches(address, length, flags);
}