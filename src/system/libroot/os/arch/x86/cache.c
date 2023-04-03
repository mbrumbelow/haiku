#include <OS.h>
#include "syscalls.h"
#include <libroot_private.h>
#include <image.h>

void __arch_clear_caches(void *address, size_t length, uint32 flags)
{
    // _kern_clear_caches(address, length, flags);

    // in x86 for flushing we have clwb instruction and for flushing and invalidating we
    // have the clflush and clfushopt instruction
    // the clwb instruction is not present on all machines and has to be checked explicitly
    // with the cpuid instruction
    //
    // also we cant really differentiate between the instruction and the data cache as there are no
    // instructions that differentiate them
    int flush = 0, invalidate = 0;
    if (flags & B_FLUSH_DCACHE || flags & B_FLUSH_ICACHE)
    {
        flush = 1;
    }
    if (flags & B_INVALIDATE_ICACHE || flags & B_INVALIDATE_DCACHE)
    {
        invalidate = 1;
    }
    // if invalidate is set, then use the clflush instruction
    // else if flush is set, then use the clwb instruction, if exists
    // if does not exist, then return as we cant invalidate those addresses

    if (invalidate)
    {
        int num_lines = (length / 64) + 1;
        char *addr = (char *)address;
        for (int i = 0; i < num_lines; i++)
        {
            asm volatile("clflush (%0)" ::"r"(addr + i * 64)
                         : "memory");
        }
    }
    else if (flush)
    {
        // check if clwb exists or not
        int eax, ebx, ecx, edx;
        eax = 0x07;
        ecx = 0;

        asm volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(eax), "c"(ecx));
        if ((ebx & (1 << 24)) == 0)
        {
            return;
        }
        int num_lines = (length / 64) + 1;
        char *addr = (char *)address;
        for (int i = 0; i < num_lines; i++)
        {
            asm volatile("clwb (%0)" ::"r"(addr + i * 64)
                         : "memory");
        }
    }
    else
    {
        return;
    }
}