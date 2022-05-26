#include <kernel/image.h>

#define _DEFAULT_SOURCE
#include <link.h>
#include <stddef.h>

#if B_HAIKU_32_BIT
	#define Elf_Ehdr Elf32_Ehdr
#elif B_HAIKU_64_BIT
	#define Elf_Ehdr Elf64_Ehdr
#endif

int dl_iterate_phdr(
	int (*callback)(struct dl_phdr_info* info, size_t size, void* data),
	void* data)
{
	image_info info;
	int32 cookie = 0;
	
	struct dl_phdr_info phdr_info;
	
	int returnValue = 0;
	
	while ((get_next_image_info(0, &cookie, &info) == B_OK) && (returnValue == 0))
	{
		const Elf_Ehdr* header = (const Elf_Ehdr*)info.text;

		if (!IS_ELF(*header))
		{
			continue;
		}
		
		phdr_info.dlpi_addr = (Elf_Addr)info.text;
		phdr_info.dlpi_name = info.name;
		phdr_info.dlpi_phnum = header->e_phnum;
		phdr_info.dlpi_phdr = (const Elf_Phdr*)((const char*)info.text + header->e_phoff);
		
		returnValue = callback(&phdr_info, sizeof(phdr_info), data);
	}
	
	return returnValue;
}