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
//#include <stdint.h>
#include <string.h>
#include "../../../lib/kexec/kexec.h"
//#include "../../../lib/kexec/kexec-syscall.h"
#include "kexec-mips.h"
//#include <arch/options.h>

static struct memory_range memory_range[MAX_MEMORY_RANGES];

/* Return a sorted list of memory ranges. */
int get_memory_ranges(struct memory_range **range, int *ranges,
		      unsigned long UNUSED(kexec_flags))
{
	int memory_ranges = 0;

	memory_range[memory_ranges].start = 0;
	memory_range[memory_ranges].end = 256 * 1024 * 1024;
	memory_range[memory_ranges].type = RANGE_RAM;
	memory_ranges++;

	*range = memory_range;
	*ranges = memory_ranges;
	return 0;
}

struct kexec_file_type kexec_file_type[] = {
	{"elf-mips", elf_mips_probe, elf_mips_load, /*elf_mips_usage*/ NULL},
};
int kexec_file_types = sizeof(kexec_file_type) / sizeof(kexec_file_type[0]);

void arch_usage(void)
{
#ifdef __mips64
	fprintf(stderr, "     --elf32-core-headers Prepare core headers in "
			"ELF32 format\n");
#endif
}

#ifdef __mips64
struct arch_options_t arch_options = {
	.core_header_type = CORE_TYPE_ELF64
};
#endif

int arch_process_options(int argc, char **argv)
{
	return 0;
}

const struct arch_map_entry arches[] = {
	/* For compatibility with older patches
	 * use KEXEC_ARCH_DEFAULT instead of KEXEC_ARCH_MIPS here.
	 */
	{ "mips", KEXEC_ARCH_MIPS },
	{ "mips64", KEXEC_ARCH_MIPS },
	{ NULL, 0 },
};

int arch_compat_trampoline(struct kexec_info *UNUSED(info))
{

	return 0;
}

void arch_update_purgatory(struct kexec_info *UNUSED(info))
{
}

#if 0
unsigned long virt_to_phys(unsigned long addr)
{
	return addr & 0x7fffffff;
}

/*
 * add_segment() should convert base to a physical address on mips,
 * while the default is just to work with base as is */
void add_segment(struct kexec_info *info, const void *buf, size_t bufsz,
		 unsigned long base, size_t memsz)
{
	add_segment_phys_virt(info, buf, bufsz, virt_to_phys(base), memsz, 1);
}

/*
 * add_buffer() should convert base to a physical address on mips,
 * while the default is just to work with base as is */
unsigned long add_buffer(struct kexec_info *info, const void *buf,
			 unsigned long bufsz, unsigned long memsz,
			 unsigned long buf_align, unsigned long buf_min,
			 unsigned long buf_max, int buf_end)
{
	return add_buffer_phys_virt(info, buf, bufsz, memsz, buf_align,
				    buf_min, buf_max, buf_end, 1);
}
#endif
