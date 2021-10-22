Porting Haiku to different architectures
########################################

This section contains notes on the various ports of Haiku, hardware specific
details, and informations on how to bring up Haiku for a new CPU architecture
or platform.

Architectures, platforms, and machines
======================================

In Haiku sources, "arch" is used to designate a CPU architecture. Platform
designates the firmware type (openfirmware, efi, ...). Some platforms (for
example bios or next_m68k) are specific to an architecture and hardware,
while others are compatible with multiple architectures (openfirmware for
PowerPC and Sparc, or EFI for x86_64, RISC-V and ARM). In the latter case, the
platform directory in the sources will have an arch subdirectory with the
platform specific things.

Some builds of Haiku involve multiple platforms with different boot methods.
For example, the x86_64 version can be booted from BIOS, UEFI, or PXE. So, 3 
different bootloaders are compiled during the build. However, the kernel is
generic, and the same kernel can be used with all 3 boot methods. In this case,
the bootloader performs all needed initializations so that the kernel starts
in a controlled environment, and does not need to interact with the firmware
for the most part.

Platform specific details
=========================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   /kernel/arch/long_double
   /kernel/arch/arm/overview
   /kernel/arch/m68k/overview
   /kernel/arch/ppc/overview
   /kernel/arch/sparc/overview
   /kernel/arch/sparc/mmu
