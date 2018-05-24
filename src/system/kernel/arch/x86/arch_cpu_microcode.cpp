/*
 * Copyright 2018, Nikolas Zimmermann, zimmermann@physik.rwth-aachen.de
 * Distributed under the terms of the MIT License.
 *
 * Copyright (C) 2008-2013 Advanced Micro Devices Inc.
 *               2013-2016 Borislav Petkov <bp@alien8.de>
 *
 * Distributed under the terms of the GNU General Public
 * License version 2.
 */


#include <cpu.h>

#include <stdio.h>

#include <debug.h>
#include <microcode_amd.h>

#include <arch_system_info.h>
#include <boot/kernel_args.h>

#define MSR_AMD64_PATCH_LEVEL  0x0000008b
#define MSR_AMD64_PATCH_LOADER 0xc0010020

/*
 * This points to the current valid container of microcode patches which we will
 * save from the initrd/builtin before jettisoning its contents. @mc is the
 * microcode patch we found to match.
 */
struct cont_desc {
        struct microcode_amd* mc;
        uint32 cpuid_1_eax;
        uint32 psize;
        uint8* data;
        size_t size;
};

static uint16
find_equiv_id(struct equiv_cpu_entry* equiv_table, uint32 sig)
{
        for (; equiv_table && equiv_table->installed_cpu; equiv_table++) {
                if (sig == equiv_table->installed_cpu)
                        return equiv_table->equiv_cpu;
        }

        return 0;
}

/*
 * This scans the ucode blob for the proper container as we can have multiple
 * containers glued together. Returns the equivalence ID from the equivalence
 * table or 0 if none found.
 * Returns the amount of bytes consumed while scanning. @desc contains all the
 * data we're going to use in later stages of the application.
 */
static ssize_t
parse_container(uint8* ucode, ssize_t size, struct cont_desc* desc)
{
        struct equiv_cpu_entry* eq;
        ssize_t orig_size = size;
        uint32* hdr = (uint32*) ucode;
        uint16 eq_id;
        uint8* buf;

        /* Am I looking at an equivalence table header? */
        if (hdr[0] != UCODE_MAGIC ||
            hdr[1] != UCODE_EQUIV_CPU_TABLE_TYPE ||
            hdr[2] == 0)
                return CONTAINER_HDR_SZ;

        buf = ucode;

        eq = (struct equiv_cpu_entry*)(buf + CONTAINER_HDR_SZ);

        /* Find the equivalence ID of our CPU in this table: */
        eq_id = find_equiv_id(eq, desc->cpuid_1_eax);

        buf  += hdr[2] + CONTAINER_HDR_SZ;
        size -= hdr[2] + CONTAINER_HDR_SZ;

        /*
         * Scan through the rest of the container to find where it ends. We do
          some basic sanity-checking too.
         */
        while (size > 0) {
                struct microcode_amd *mc;
                uint32 patch_size;

                hdr = (uint32 *)buf;

                if (hdr[0] != UCODE_UCODE_TYPE)
                        break;

                /* Sanity-check patch size. */
                patch_size = hdr[1];
                if (patch_size > PATCH_MAX_SIZE)
                        break;

                /* Skip patch section header: */
                buf  += SECTION_HDR_SIZE;
                size -= SECTION_HDR_SIZE;

                mc = (struct microcode_amd *)buf;
                if (eq_id == mc->hdr.processor_rev_id) {
                        desc->psize = patch_size;
                        desc->mc = mc;
                }

                buf  += patch_size;
                size -= patch_size;
        }

        /*
         * If we have found a patch (desc->mc), it means we're looking at the
         * container which has a patch for this CPU so return 0 to mean, @ucode
         * already points to the proper container. Otherwise, we return the size
         * we scanned so that we can advance to the next container in the
         * buffer.
         */
        if (desc->mc) {
                desc->data = ucode;
                desc->size = orig_size - size;

                return 0;
        }

        return orig_size - size;
}

/*
 * Scan the ucode blob for the proper container as we can have multiple
 * containers glued together.
 */
static void
scan_containers(uint8* ucode, size_t size, struct cont_desc* desc)
{
        ssize_t rem = size;

        while (rem >= 0) {
                ssize_t s = parse_container(ucode, rem, desc);
                if (!s)
                        return;

                ucode += s;
                rem   -= s;
        }
}

status_t
arch_cpu_load_microcode()
{
        cpu_ent* cpu = get_cpu_struct();

        // TODO: Extend to Intel as well, and all AMD variants.
        if (cpu->arch.vendor != VENDOR_AMD)
                return B_OK;

        // Currently only AMD 15h family is supported.
        if (cpu->arch.family == 0xf && cpu->arch.extended_family == 6) {
                dprintf("microcode: Obtaining AMD microcode update\n");

                cpuid_info cpuid;
                get_current_cpuid(&cpuid, 1, 0);

                struct cont_desc desc = { 0 };
                desc.cpuid_1_eax = cpuid.regs.eax;
                scan_containers((uint8*) gMicrocode_amd_15h, gMicrocodeSize_amd_15h, &desc);
                struct microcode_amd *mc = desc.mc;
                if (!mc) {
                        dprintf("microcode: Failed to extract AMD microcode update. This should not happen!\n");
                        return B_ERROR;
                }

                dprintf("microcode: Successfully extracted AMD microcode firmware: patch_id: %d, processor_rev_id %d\n", mc->hdr.patch_id, mc->hdr.processor_rev_id);

                uint32 rev = (uint32) x86_read_msr(MSR_AMD64_PATCH_LEVEL);
                dprintf("microcode: Currently loaded revision %d\n", rev);

                if (rev >= mc->hdr.patch_id) {
                        dprintf("microcode: No need to patch, microcode is using the newest available revision\n");
                        return B_OK;
                }

                x86_write_msr(MSR_AMD64_PATCH_LOADER, (uint64)(long)&mc->hdr.data_code);

                uint32 old_rev = rev;
                rev = (uint32) x86_read_msr(MSR_AMD64_PATCH_LEVEL);
                if (rev != mc->hdr.patch_id) {
                        dprintf("microcode: Patching microcode failed, expecting revision %d, but got %d\n", mc->hdr.patch_id, rev);
                        return B_OK;
                }

                dprintf("microcode: Successfully patched microcode to revision %d, from previous %d\n", mc->hdr.patch_id, old_rev);
        }

        return B_OK;
}
