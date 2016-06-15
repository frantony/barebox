/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 */

#define BOARD_PBL_START start_wrtnode2r

#include <asm/pbl_macros.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	/* Initialize caches... */
	//mips_cache_reset

	/* ... and enable them */
	//dcache_enable

	//debug_ll_init

	.set	pop

// FIMXE: wrtnode2r has 256M
ENTRY_FUNCTION_END(BOARD_PBL_START, wrtnode2r, SZ_128M)
