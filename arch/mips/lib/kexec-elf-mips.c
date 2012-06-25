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
#include <linux/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h>
//#include <getopt.h>
#include <elf.h>
//#include <boot/elf_boot.h>
//#include <ip_checksum.h>
#include "../../../lib/kexec/kexec.h"
#include "../../../lib/kexec/kexec-elf.h"
//#include "../../kexec-syscall.h"
#include "kexec-mips.h"
//#include "crashdump-mips.h"
//#include <arch/options.h>

static const int probe_debug = 0;

#define BOOTLOADER         "kexec"
#define MAX_COMMAND_LINE   256
#define UPSZ(X) ((sizeof(X) + 3) & ~3)
static char cmdline_buf[256] = "kexec ";

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
		if (probe_debug) {
			fprintf(stderr, "Not for this architecture.\n");
		}
		result = -1;
		goto out;
	}
	result = 0;
 out:
	free_elf_info(&ehdr);
	return result;
}


int elf_mips_load(/*int argc, char **argv, */const char *buf, off_t len,
	struct kexec_info *info)
{
	struct mem_ehdr ehdr;
	const char *command_line;
	int command_line_len;
	int opt;
	int result;
	unsigned long cmdline_addr;
	size_t i;

	command_line = 0;
	command_line_len = 0;

//printf("%s:%d\n", __func__, __LINE__);
	result = build_elf_exec_info(buf, len, &ehdr, 0);
	if (result < 0)
		die("ELF exec parse failed\n");

//printf("%s:%d\n", __func__, __LINE__);
#if 1
	/* Read in the PT_LOAD segments and remove CKSEG0 mask from address*/
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct mem_phdr *phdr;
		phdr = &ehdr.e_phdr[i];
		if (phdr->p_type == PT_LOAD) {
			/* FIXME: skipped */
//			phdr->p_paddr = virt_to_phys(phdr->p_paddr);
		}
	}
#endif

	/* Load the Elf data */
	result = elf_exec_load(&ehdr, info);
	if (result < 0)
		die("ELF exec load failed\n");

//printf("%s:%d\n", __func__, __LINE__);
	/* FIXME */
	info->entry = ehdr.e_entry;
#if 0
	info->entry = (void *)virt_to_phys(ehdr.e_entry);

	if (command_line) {
		command_line_len = strlen(command_line) + 1;
		strncat(cmdline_buf, command_line, command_line_len);
	}

	cmdline_addr = 0;

	add_buffer(info, cmdline_buf, sizeof(cmdline_buf),
			sizeof(cmdline_buf), sizeof(void *),
			cmdline_addr, 0x0fffffff, 1);

#endif
	return 0;
}

