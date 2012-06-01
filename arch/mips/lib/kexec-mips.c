/*
 * kexec-mips.c - kexec for mips
 * Copyright (C) 2007 Francesco Chiechi, Alessandro Rubini
 * Copyright (C) 2007 Tvblob s.r.l.
 *
 * derived from ../ppc/kexec-mips.c
 * Copyright (C) 2004, 2005 Albert Herranz
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <memory.h>
#include "../../../lib/kexec/kexec.h"
#include "kexec-mips.h"

struct kexec_file_type kexec_file_type[] = {
	{"elf-mips", elf_mips_probe, elf_mips_load },
};
int kexec_file_types = sizeof(kexec_file_type) / sizeof(kexec_file_type[0]);

#ifdef __mips64
struct arch_options_t arch_options = {
	.core_header_type = CORE_TYPE_ELF64
};
#endif

const struct arch_map_entry arches[] = {
	/* For compatibility with older patches
	 * use KEXEC_ARCH_DEFAULT instead of KEXEC_ARCH_MIPS here.
	 */
	{ "mips", KEXEC_ARCH_MIPS },
	{ "mips64", KEXEC_ARCH_MIPS },
	{ NULL, 0 },
};

/*
 * add_segment() should convert base to a physical address on mips,
 * while the default is just to work with base as is */
void add_segment(struct kexec_info *info, const void *buf, size_t bufsz,
		 unsigned long base, size_t memsz)
{
	add_segment_phys_virt(info, buf, bufsz, virt_to_phys(base), memsz, 1);
}

typedef void (*noretfun_t)(long, long, long, long) __attribute__((noreturn));

/* relocator parameters */
extern unsigned long kexec_start_address;
extern unsigned long kexec_segments;
extern unsigned long kexec_nr_segments;

long kexec_load(void *entry, unsigned long nr_segments,
		struct kexec_segment *segments, unsigned long flags)
{
	int i;
	struct resource *elf;
	resource_size_t start;
	unsigned long reboot_code_buffer;
	LIST_HEAD(elf_segments);

	for (i = 0; i < nr_segments; i++) {
		elf = create_resource("elf segment",
			segments[i].mem,
			segments[i].mem + segments[i].memsz - 1);

		list_add_used_region(&elf->sibling, &elf_segments);
	}

	if (check_room_for_elf(&elf_segments)) {
		printf("ELF can't be loaded!\n");
		return 0;
	}

	/* FIXME: common code? */
	start = dcheck_res(&elf_segments);

	/* FIXME: we don't support cache just now su use KSEG1 */
	start = CKSEG1ADDR(start);

	/* relocate_new_kernel() copy by register (4 or 8 bytes)
	   so start address must be aligned to 4/8 */
	start = (start + 15) & 0xfffffff0;

	for (i = 0; i < nr_segments; i++) {
		segments[i].buf = (void *)(CKSEG1ADDR(segments[i].buf));
		segments[i].mem = (void *)(CKSEG1ADDR(segments[i].mem));
		memcpy(start, segments[i].buf, segments[i].bufsz);
		request_sdram_region("kexec relocatable segments",
			(unsigned long)start,
			(unsigned long)segments[i].bufsz);

		/* FIXME */
		/* relocate_new_kernel() copy by register (4 or 8 bytes)
		   so bufsz must be aligned to 4/8 */
		segments[i].bufsz = (segments[i].bufsz + 15) & 0xfffffff0;
		start = start + segments[i].bufsz;
	}

	start = (start + 15) & 0xfffffff0;

	extern u32 relocate_new_kernel;
	extern u32 relocate_new_kernel_size;
	reboot_code_buffer = start;

	memcpy(reboot_code_buffer, &relocate_new_kernel, relocate_new_kernel_size);
	request_sdram_region("kexec relocator",
		(unsigned long)reboot_code_buffer,
		(unsigned long)relocate_new_kernel_size);

	start = reboot_code_buffer + relocate_new_kernel_size;
	start = (start + 15) & 0xfffffff0;

	kexec_start_address = CKSEG1ADDR(entry);
	kexec_segments = start;
	kexec_nr_segments = nr_segments;

	memcpy(start, segments, nr_segments * sizeof(*segments));
	request_sdram_region("kexec control segments",
		(unsigned long)start,
		(unsigned long)nr_segments * sizeof(*segments));

	return 0;
}
