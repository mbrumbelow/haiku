/* Same as elf_x86_64_fbsd_efi.lds, except for OUTPUT_FORMAT below & ctor support - KEEP IN SYNC */
OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;
  .hash : { *(.hash) }	/* this MUST come first! */
  . = ALIGN(4096);
  .eh_frame :
  {
    *(.eh_frame)
  }
  . = ALIGN(4096);
  .text :
  {
   _text = .;
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   . = ALIGN(16);
  }
  _etext = .;
  _text_size = . - _text;
  . = ALIGN(4096);
  .reloc :
  {
   *(.reloc)
  }
  . = ALIGN(4096);
  .data :
  {
   . = ALIGN(0x4);
   __ctor_list = .;
   *(.ctors)
   __ctor_end = .;
   _data = .;
   *(.rodata*)
   *(.got.plt)
   *(.got)
   *(.data*)
   *(.sdata)
  }
  .note.gnu.build-id : { *(.note.gnu.build-id) }

  _edata = .;
  _data_size = . - _etext;

  .bss:
  {
   __bss_start = . ;
   *(.sbss)
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
   *(.rel.local)

   __bss_end = . ;
  }

  . = ALIGN(4096);
  .dynamic  : { *(.dynamic) }
  . = ALIGN(4096);
  .rela :
  {
    *(.rela.data*)
    *(.rela.ctors)
    *(.rela.got)
    *(.rela.stab)
  }
  . = ALIGN(4096);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(4096);
  .ignored.reloc :
  {
    *(.rela.reloc)
    *(.eh_frame)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
}
