/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 */

#define BOARD_PBL_START start_linkit

#include <asm/pbl_macros.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_blt 0x9c000000 skip_init t8

	/* Initialize cache and setup stack in there */
	PTR_LA	t9, mt7628_sram_init
	jalr	t9
	 nop

	/* Initialize any external memory */
	PTR_LA	t9, mt7628_lowlevel_init
	jalr	t9
	 nop

	/* Initialize caches... */
	mips_cache_reset

	/* ... and enable them */
	dcache_enable

skip_init:
	.set	pop

ENTRY_FUNCTION_END(BOARD_PBL_START, linkit_smart, SZ_128M)
