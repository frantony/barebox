/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020 Antony Pavlov <antonynpavlov@gmail.com>
 */

#define BOARD_PBL_START start_wrtnode

#include <asm/pbl_macros.h>
#include <asm/pbl_nmon.h>
#include <linux/sizes.h>

ENTRY_FUNCTION(BOARD_PBL_START)

	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_blt 0x9c000000 skip_init t8

	/* SoC specific setup ... */
	/* ... absent */

skip_init:

	debug_ll_rt2880_init

	.set	pop

ENTRY_FUNCTION_END(BOARD_PBL_START, wrtnode, SZ_64M)
