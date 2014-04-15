#include <common.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <memory.h>
#include <elf.h>
#include "kexec.h"
#include "kexec-elf.h"

int build_elf_exec_info(const char *buf, off_t len, struct mem_ehdr *ehdr,
				uint32_t flags)
{
	struct mem_phdr *phdr, *end_phdr;
	int result;

	result = build_elf_info(buf, len, ehdr, flags);
	if (result < 0) {
		return result;
	}

	if (ehdr->e_type != ET_EXEC) {
		printf("Not ELF type ET_EXEC\n");
		return -1;
	}

	if (!ehdr->e_phdr) {
		printf("No ELF program header\n");
		return -1;
	}

	end_phdr = &ehdr->e_phdr[ehdr->e_phnum];
	for (phdr = ehdr->e_phdr; phdr != end_phdr; phdr++) {
		/* Kexec does not support loading interpreters.
		 * In addition this check keeps us from attempting
		 * to kexec ordinay executables.
		 */
		if (phdr->p_type == PT_INTERP) {
			printf("Requires an ELF interpreter\n");
			return -1;
		}
	}

	return 0;
}

int elf_exec_load(struct mem_ehdr *ehdr, struct kexec_info *info)
{
	int result;
	size_t i;

	if (!ehdr->e_phdr) {
		printf("No program header?\n");
		result = -1;
		goto out;
	}

	/* Read in the PT_LOAD segments */
	for (i = 0; i < ehdr->e_phnum; i++) {
		struct mem_phdr *phdr;
		size_t size;

		phdr = &ehdr->e_phdr[i];

		if (phdr->p_type != PT_LOAD) {
			continue;
		}

		size = phdr->p_filesz;

		if (size > phdr->p_memsz) {
			size = phdr->p_memsz;
		}

		add_segment(info,
			phdr->p_data, size,
			phdr->p_paddr, phdr->p_memsz);
	}

	result = 0;
 out:
	return result;
}
