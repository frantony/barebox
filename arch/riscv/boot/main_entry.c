// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

#include <linux/types.h>
#include <linux/sizes.h>
#include <asm-generic/div64.h>


#define HASHES_PER_LINE	65

#define FILESIZE_MAX	((loff_t)-1)

static int printed;
static int progress_max;
static int spin;

static void show_progress(int now)
{
	char spinchr[] = "\\|/-";

	if (now < 0) {
		printf("%c\b", spinchr[spin++ % (sizeof(spinchr) - 1)]);
		return;
	}

	if (progress_max && progress_max != FILESIZE_MAX) {
		uint64_t tmp = (int64_t)now * HASHES_PER_LINE;
		do_div(tmp, progress_max);
		now = tmp;
	}

	while (printed < now) {
		if (!(printed % HASHES_PER_LINE) && printed)
			printf("\n\t");
		printf("#");
		printed++;
	}
}

static void init_progression_bar(int max)
{
	printed = 0;
	progress_max = max;
	spin = 0;
	if (progress_max && progress_max != FILESIZE_MAX)
		printf("\t[%*s]\r\t[", HASHES_PER_LINE, "");
	else
		printf("\t");
}

static int update_progress(resource_size_t offset)
{
	/* Only check every 4k to reduce overhead */
	if (offset & (SZ_4K - 1))
		return 0;

	show_progress(offset);

	return 0;
}

static void mem_test_report_failure(const char *failure_description,
				    resource_size_t expected_value,
				    resource_size_t actual_value,
				    volatile resource_size_t *address)
{
	printf("FAILURE (%s): "
	       "expected 0x%08x, actual 0x%08x at address 0x%08x.\n",
	       failure_description, expected_value, actual_value,
	       (resource_size_t)address);
}


static int mem_test_moving_inversions(resource_size_t _start, resource_size_t _end)
{
	volatile resource_size_t *start, num_words, offset, temp, anti_pattern;
	int ret;

	_start = ALIGN(_start, sizeof(resource_size_t));
	_end = ALIGN_DOWN(_end, sizeof(resource_size_t)) - 1;


	if (_end <= _start)
		return -EINVAL;

	start = (resource_size_t *)_start;
	num_words = (_end - _start + 1)/sizeof(resource_size_t);

	printf("Starting moving inversions test of RAM [0x%08x - 0x%08x]:\n"
	       "Fill with address, compare, fill with inverted address, compare again\n",
		_start, _end);

	/*
	 * Description: Test the integrity of a physical
	 *		memory device by performing an
	 *		increment/decrement test over the
	 *		entire region. In the process every
	 *		storage bit in the device is tested
	 *		as a zero and a one. The base address
	 *		and the size of the region are
	 *		selected by the caller.
	 */

	init_progression_bar(3 * num_words);

	/* Fill memory with a known pattern */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(offset);
		if (ret)
			return ret;
		start[offset] = offset + 1;
	}

	/* Check each location and invert it for the second pass */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(num_words + offset);
		if (ret)
			return ret;

		temp = start[offset];
		if (temp != (offset + 1)) {
			printf("\n");
			mem_test_report_failure("read/write",
						(offset + 1),
						temp, &start[offset]);
			return -EIO;
		}

		anti_pattern = ~(offset + 1);
		start[offset] = anti_pattern;
	}

	/* Check each location for the inverted pattern and zero it */
	for (offset = 0; offset < num_words; offset++) {
		ret = update_progress(2 * num_words + offset);
		if (ret)
			return ret;

		anti_pattern = ~(offset + 1);
		temp = start[offset];

		if (temp != anti_pattern) {
			printf("\n");
			mem_test_report_failure("read/write",
						anti_pattern,
						temp, &start[offset]);
			return -EIO;
		}

		start[offset] = 0;
	}
	show_progress(3 * num_words);

	/* end of progressbar */
	printf("\n");

	return 0;
}

void main_entry(void);

#include <debug_ll.h>

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void main_entry(void)
{
	int i;

//	debug_ll_ns16550_init();
	puts_ll("main_entry()\n");

	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);

	puts_ll("bss is clear\n");
	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));

	puts_ll("mem_malloc_init done\n\n");
	i = 0;
	while (1) {
		malloc_stats();
		printf("%d: ", i);
		mem_test_moving_inversions(0x80100000, 0x80800000);
		i++;
	}
}
