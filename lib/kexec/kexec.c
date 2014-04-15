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
 */

#define _GNU_SOURCE
#include <common.h>
#include <fs.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <asm/io.h>

#include "kexec.h"
#include "kexec-elf.h"

static int sort_segments(struct kexec_info *info)
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
			printf("Overlapping memory segments at %p\n",
				end);
			return -1;
		}
		end = ((char *)info->segment[i].mem) + info->segment[i].memsz;
	}

	return 0;
}

void add_segment_phys_virt(struct kexec_info *info,
	const void *buf, size_t bufsz,
	unsigned long base, size_t memsz, int phys)
{
	size_t size;
	int pagesize;

	if (bufsz > memsz) {
		bufsz = memsz;
	}

	/* Forget empty segments */
	if (memsz == 0) {
		return;
	}

	/* Round memsz up to a multiple of pagesize */
	pagesize = 4096;
	memsz = (memsz + (pagesize - 1)) & ~(pagesize - 1);

	if (phys)
		base = virt_to_phys((void *)base);

	size = (info->nr_segments + 1) * sizeof(info->segment[0]);
	info->segment = xrealloc(info->segment, size);
	info->segment[info->nr_segments].buf   = buf;
	info->segment[info->nr_segments].bufsz = bufsz;
	info->segment[info->nr_segments].mem   = (void *)base;
	info->segment[info->nr_segments].memsz = memsz;
	info->nr_segments++;
	if (info->nr_segments > KEXEC_MAX_SEGMENTS) {
		printf("Warning: kernel segment limit reached. "
			"This will likely fail\n");
	}
}

/*
 *	Load the new kernel
 */
int kexec_load_file(char *kernel, unsigned long kexec_flags)
{
	char *kernel_buf;
	off_t kernel_size;
	int i = 0;
	int result;
	struct kexec_info info;

	memset(&info, 0, sizeof(info));
	info.segment = NULL;
	info.nr_segments = 0;
	info.entry = NULL;
	info.kexec_flags = kexec_flags;

	kernel_buf = read_file(kernel, &kernel_size);

	for (i = 0; i < kexec_file_types; i++) {
		if (kexec_file_type[i].probe(kernel_buf, kernel_size) >= 0)
			break;
	}

	if (i == kexec_file_types) {
		printf("Cannot determine the file type "
				"of %s\n", kernel);
		return -1;
	}

	result = kexec_file_type[i].load(kernel_buf, kernel_size, &info);
	if (result < 0) {
		printf("Cannot load %s\n", kernel);
		return result;
	}

	/* Verify all of the segments load to a valid location in memory */

	/* Sort the segments and verify we don't have overlaps */
	if (sort_segments(&info) < 0) {
		return -1;
	}

	result = kexec_load(info.entry,
		info.nr_segments, info.segment, info.kexec_flags);

	return result;
}
