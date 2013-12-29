/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
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

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	/* Do addres test, if we on flash, it is safe to
	 * do PLL and RAM config. If we in RAM,
	 * then it is already configured. */
ar933x_addres_test:
	la t9, ar933x_addres_test
	and t9, 0xfff00000
	bne t9, 0xbfc00000, ar933x_pbl_end
	 nop

	pbl_ar9331_pll
	pbl_ar9331_ram

ar933x_pbl_end:
	copy_to_link_location	pbl_start

	.set	pop
	.endm
