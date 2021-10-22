The UltraSPARC MMUs
###################

While the SPARC CPU architecture is standardized by the SPARC foundation, the
MMU was originally not. As a result, there are several designs in different
generations of SPARC hardware. Later on, Sun/Oracle and Fujitsu published
common specifications for their MMU implementation. For the Haiku port, we will
ignore any of the early SPARC v8 (32 bit) work, and focus on SPARC v9
implementations.

One particularity of the SPARC architecture is that the smallest MMU page size
is 8K, where other architectures Haiku is ported to use 4K pages.

UltraSparc and UltraSparc II
============================

In this generation, the MMU is rather minimal. In particular, there is no
support for hardware page table walk. The MMU has a cache (TLB, translation
lookaside buffer), which automatically stores recently accessed entries but
can also have some entries "locked in".

When the MMU cache misses, it triggers a MMU fault, and it is up to the
software to catch this, fill the MMU cache with the needed data, and resume
execution. Of course, for this to work, the code handling this must itself
never trigger any MMU cache miss, which is where the locking of things in the
MMU cache becomes useful.

Information about this MMU implementation is found in sections 4 and 6 of the
`UltraSparc user guide <https://www.oracle.com/technetwork/server-storage/sun-sparc-enterprise/documentation/sparc-usersmanual-2516676.pdf>`_

The virtual addresses use 48 bits (sign extended to 64) and the physical
addresses use 41 bits. So virtual addresses between 800 0000 0000 and FFFF F7FF FFFF FFFF
are invalid and cannot be used.

The software implementation can still get some help from the MMU, which
pre computes offsets into a table stored in RAM called the TSB (translation
storage buffer). This acts as a level 2 cache and will probably contain entries
for the currently running process. It is not a particularly efficient data
structure, and the complete mapping must be stored elsewhere by the OS.
