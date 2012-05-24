/*
 * Startup Code for MIPS CPU
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	b       __start
	 nop

	/*
	   MIPS_REVISION_REG located at 0x1fc00010
	   see the MIPS_REVISION_CORID macro in linux kernel sources
	   set up it to 0x420 (Malta Board with CoreLV) as qemu does
	*/
	.org    0x10
	.word   0x00000420

	.align 4
__start:

	mips_disable_interrupts

	/* cpu specific setup ... */
	/* ... absent */

	copy_to_link_location	pbl_start

	.set	pop
	.endm
