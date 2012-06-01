/*
 * kexec-elf-mips.c - kexec Elf loader for mips
 * Copyright (C) 2007 Francesco Chiechi, Alessandro Rubini
 * Copyright (C) 2007 Tvblob s.r.l.
 *
 * derived from ../ppc/kexec-elf-ppc.c
 * Copyright (C) 2004 Albert Herranz
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/types.h>
#include <fcntl.h>
#include <elf.h>
#include "../../../lib/kexec/kexec.h"
#include "../../../lib/kexec/kexec-elf.h"
#include "kexec-mips.h"

int elf_mips_probe(const char *buf, off_t len)
{
	struct mem_ehdr ehdr;
	int result;

	result = build_elf_exec_info(buf, len, &ehdr, 0);
	if (result < 0) {
		goto out;
	}

	/* Verify the architecuture specific bits */
	if (ehdr.e_machine != EM_MIPS) {
		/* for a different architecture */
		printf("Not for this architecture.\n");
		result = -1;
		goto out;
	}
	result = 0;

 out:
	free_elf_info(&ehdr);

	return result;
}

int elf_mips_load(const char *buf, off_t len, struct kexec_info *info)
{
	struct mem_ehdr ehdr;
	int result;
	size_t i;

	result = build_elf_exec_info(buf, len, &ehdr, 0);
	if (result < 0) {
		printf("ELF exec parse failed\n");
		goto out;
	}

	/* Read in the PT_LOAD segments and remove CKSEG0 mask from address */
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct mem_phdr *phdr;
		phdr = &ehdr.e_phdr[i];
		if (phdr->p_type == PT_LOAD) {
			phdr->p_paddr = virt_to_phys(phdr->p_paddr);
		}
	}

	/* Load the ELF data */
	result = elf_exec_load(&ehdr, info);
	if (result < 0) {
		printf("ELF exec load failed\n");
		goto out;
	}

	info->entry = (void *)virt_to_phys(ehdr.e_entry);

out:
	return result;
}
