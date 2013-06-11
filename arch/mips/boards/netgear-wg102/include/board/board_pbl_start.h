/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <asm/pbl_macros.h>
#include <mach/pbl_macros.h>

#include <mach/debug_ll.h>

#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_ar2312_pll

	pbl_ar2312_rst_uart0
	debug_ll_ns16550_init

	debug_ll_ns16550_outc 'a'
	debug_ll_ns16550_outnl

	mips_nmon

	pbl_ar2312_x16_sdram
	debug_ll_ns16550_outc 'b'
	debug_ll_ns16550_outnl

	li t0, 0xa0000000
	li t1, 0x12345678
	sw t1, 0(t0)
	lw t2, 0(t0)
	beq t1, t2, oki
	 nop
	debug_ll_ns16550_outc 'c'
	debug_ll_ns16550_outnl
oki:
	debug_ll_ns16550_outc 'd'
	debug_ll_ns16550_outnl

	copy_to_link_location	pbl_start

	.set	pop
	.endm
