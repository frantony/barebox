/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <debug_ll.h>

void main_entry(void);

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void main_entry(void)
{
//	debug_ll_ns16550_init();
	puts_ll("main_entry()\n");

	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));

	puts_ll("start_barebox\n");
	start_barebox();
}
