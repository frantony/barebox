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
#include "../../../lib/kexec/kexec.h"
#include "kexec-mips.h"

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

int arch_compat_trampoline(struct kexec_info *UNUSED(info))
{

	return 0;
}

void arch_update_purgatory(struct kexec_info *UNUSED(info))
{
}
