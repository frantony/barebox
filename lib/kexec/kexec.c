/*
 * kexec: Linux boots Linux
 *
 * Copyright (C) 2003-2005  Eric Biederman (ebiederm@xmission.com)
 *
 * Modified (2007-05-15) by Francesco Chiechi to rudely handle mips platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define _GNU_SOURCE
#include <linux/types.h>
#include <common.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
//#include <limits.h>
//#include <sys/types.h>
#include <linux/stat.h>
//#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
//#include <ctype.h>

#include "config.h"

#include "kexec.h"
//#include "kexec-syscall.h"
#include "kexec-elf.h"
#include <fs.h>

unsigned long long mem_min = 0;
unsigned long long mem_max = ULONG_MAX;
int kexec_debug = 0;

void die(char *fmt, ...)
{
}

int valid_memory_range(struct kexec_info *info,
		       unsigned long sstart, unsigned long send)
{
	int i;
	if (sstart > send) {
		return 0;
	}
	if ((send > mem_max) || (sstart < mem_min)) {
		return 0;
	}
	for (i = 0; i < info->memory_ranges; i++) {
		unsigned long mstart, mend;
		/* Only consider memory ranges */
		if (info->memory_range[i].type != RANGE_RAM)
			continue;
		mstart = info->memory_range[i].start;
		mend = info->memory_range[i].end;
		if (i < info->memory_ranges - 1
		    && mend == info->memory_range[i+1].start
		    && info->memory_range[i+1].type == RANGE_RAM)
			mend = info->memory_range[i+1].end;

		/* Check to see if we are fully contained */
		if ((mstart <= sstart) && (mend >= send)) {
			return 1;
		}
	}
	return 0;
}

static int valid_memory_segment(struct kexec_info *info,
				struct kexec_segment *segment)
{
	unsigned long sstart, send;
	sstart = (unsigned long)segment->mem;
	send   = sstart + segment->memsz - 1;

	return valid_memory_range(info, sstart, send);
}

static void print_segments(int f, struct kexec_info *info)
{
	int i;

	fprintf(f, "nr_segments = %d\n", info->nr_segments);
	for (i = 0; i < info->nr_segments; i++) {
		fprintf(f, "segment[%d].buf   = %p\n",	i,
			info->segment[i].buf);
		fprintf(f, "segment[%d].bufsz = %zx\n", i,
			info->segment[i].bufsz);
		fprintf(f, "segment[%d].mem   = %p\n",	i,
			info->segment[i].mem);
		fprintf(f, "segment[%d].memsz = %zx\n", i,
			info->segment[i].memsz);
	}
}

int sort_segments(struct kexec_info *info)
{
	int i, j;
	void *end;

	/* Do a stupid insertion sort... */
	for (i = 0; i < info->nr_segments; i++) {
		int tidx;
		struct kexec_segment temp;
		tidx = i;
		for (j = i +1; j < info->nr_segments; j++) {
			if (info->segment[j].mem < info->segment[tidx].mem) {
				tidx = j;
			}
		}
		if (tidx != i) {
			temp = info->segment[tidx];
			info->segment[tidx] = info->segment[i];
			info->segment[i] = temp;
		}
	}
	/* Now see if any of the segments overlap */
	end = 0;
	for (i = 0; i < info->nr_segments; i++) {
		if (end > info->segment[i].mem) {
			fprintf(stderr, "Overlapping memory segments at %p\n",
				end);
			return -1;
		}
		end = ((char *)info->segment[i].mem) + info->segment[i].memsz;
	}
	return 0;
}

/*
 *	Load the new kernel
 */
static int my_load(char *kernel, unsigned long kexec_flags)
{
	char *kernel_buf;
	off_t kernel_size;
	int i = 0;
	int result;
	struct kexec_info info;
	long native_arch;

//printf("%s:%d\n", __func__, __LINE__);
	memset(&info, 0, sizeof(info));
	info.segment = NULL;
	info.nr_segments = 0;
	info.entry = NULL;
	info.backup_start = 0;
	info.kexec_flags = kexec_flags;

	result = 0;
	/* slurp in the input kernel */
	/* FIXME: add a decompresion routines insted of read_file() */
	kernel_buf = read_file(kernel, &kernel_size);
	printf("kernel: %p kernel_size: %lx\n", kernel_buf, kernel_size);

//printf("%s:%d\n", __func__, __LINE__);
	/* FIXME: check memory banks */
	if (get_memory_ranges(&info.memory_range, &info.memory_ranges,
		info.kexec_flags) < 0) {
		fprintf(stderr, "Could not get memory layout\n");
		return -1;
	}

//printf("%s:%d\n", __func__, __LINE__);
	for (i = 0; i < kexec_file_types; i++) {
		if (kexec_file_type[i].probe(kernel_buf, kernel_size) >= 0)
			break;
	}
//printf("%s:%d\n", __func__, __LINE__);
	if (i == kexec_file_types) {
		fprintf(stderr, "Cannot determine the file type "
				"of %s\n", kernel);
		return -1;
	}
//printf("%s:%d\n", __func__, __LINE__);
	/* Figure out our native architecture before load */
#if 0
	native_arch = physical_arch();
	if (native_arch < 0) {
		return -1;
	}
#endif
	native_arch = 0;
	info.kexec_flags |= native_arch;

//printf("%s:%d\n", __func__, __LINE__);
	result = kexec_file_type[i].load(kernel_buf, kernel_size, &info);
//printf("%s:%d\n", __func__, __LINE__);
	if (result < 0) {
		switch (result) {
		case EFAILED:
		default:
			fprintf(stderr, "Cannot load %s\n", kernel);
			break;
		}
		return result;
	}
#if 0
	/* If we are not in native mode setup an appropriate trampoline */
	if (arch_compat_trampoline(&info) < 0) {
		return -1;
	}
#endif
//printf("%s:%d\n", __func__, __LINE__);
	/* Verify all of the segments load to a valid location in memory */
	for (i = 0; i < info.nr_segments; i++) {
		if (!valid_memory_segment(&info, info.segment +i)) {
			fprintf(stderr, "Invalid memory segment %p - %p\n",
				info.segment[i].mem,
				((char *)info.segment[i].mem) + 
				info.segment[i].memsz);
			return -1;
		}
	}
	/* Sort the segments and verify we don't have overlaps */
	if (sort_segments(&info) < 0) {
		return -1;
	}
#if 1
	fprintf(stderr, "kexec_load: entry = %p flags = %lx\n", 
		info.entry, info.kexec_flags);
	print_segments(stderr, &info);
#endif
#if 0
	result = kexec_load(
		info.entry, info.nr_segments, info.segment, info.kexec_flags);
#else
	{
		void (*barebox)(int a0, int a1, int a2, int a3);

		barebox = info.entry;

		printf("\njumping to %p\n\n", barebox);
		barebox(2,              /* number of arguments? */
			0x80002000,
			0x80002008,
			0x10000000      /* no matter */
                );
	}
#endif
	if (result != 0) {
		/* The load failed, print some debugging information */
		fprintf(stderr, "kexec_load failed: %s\n", 
			strerror(errno));
		fprintf(stderr, "entry       = %p flags = %lx\n", 
			info.entry, info.kexec_flags);
		print_segments(stderr, &info);
	}
	return result;
}

#include <boot.h>
#include <init.h>
#include <binfmt.h>

static int do_bootm_elf(struct image_data *data)
{
	printf("\ndo_bootm_elf()\n\n");

	my_load(data->os_file, 0);

	/* unreachable(); */
	return -1;
}

static struct image_handler elf_handler = {
	.name = "ELF",
	.bootm = do_bootm_elf,
	.filetype = filetype_elf,
};

static struct binfmt_hook binfmt_elf_hook = {
	.type = filetype_elf,
	.exec = "bootm",
};

static int elf_register_image_handler(void)
{
	register_image_handler(&elf_handler);
	binfmt_register(&binfmt_elf_hook);

	return 0;
}
late_initcall(elf_register_image_handler);
