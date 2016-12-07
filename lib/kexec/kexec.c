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

#include <libfile.h>

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

static int kexec_load_one_file(struct kexec_info *info, char *fname, unsigned long kexec_flags)
{
	char *buf;
	size_t fsize;
	int i = 0;

	buf = read_file(fname, &fsize);

	/* FIXME: check buf */

	for (i = 0; i < kexec_file_types; i++) {
		if (kexec_file_type[i].probe(buf, fsize) >= 0)
			break;
	}

	if (i == kexec_file_types) {
		printf("Cannot determine the file type "
				"of %s\n", fname);
		return -1;
	}

	return kexec_file_type[i].load(buf, fsize, info);
}

static int kexec_load_binary_file(struct kexec_info *info, char *fname, unsigned long base)
{
	char *buf;
	size_t fsize;

	buf = read_file(fname, &fsize);

	/* FIXME: check buf */

	add_segment(info, buf, fsize, base, fsize);

	return 0;
}

static int print_segments(struct kexec_info *info)
{
	int i;

	printf("print_segments\n");
	for (i = 0; i < info->nr_segments; i++) {
		struct kexec_segment *seg = &info->segment[i];

		printf("  %d. buf=%#08p bufsz=%#lx mem=%#08p memsz=%#lx\n", i,
			seg->buf, seg->bufsz, seg->mem, seg->memsz);
	}

	return 0;
}

int kexec_load_bootm_data(struct image_data *data)
{
	int result;
	struct kexec_info info;

	memset(&info, 0, sizeof(info));

	result = kexec_load_one_file(&info, data->os_file, 0);
	if (result < 0) {
		printf("Cannot load %s\n", data->os_file);
		return result;
	}

	if (data->oftree_file) {
		int i;
		unsigned long base = 0;

		for (i = 0; i < info.nr_segments; i++) {
			struct kexec_segment *seg = &info.segment[i];

			base = seg->mem + seg->memsz;
		}

		kexec_load_binary_file(&info, data->oftree_file, base);
		data->oftree_address = base;
	}

	print_segments(&info);

	/* Verify all of the segments load to a valid location in memory */

	/* Sort the segments and verify we don't have overlaps */
	if (sort_segments(&info) < 0) {
		return -1;
	}

	return kexec_load(info.entry,
		info.nr_segments, info.segment, info.kexec_flags);
}
