/*
 * Copyright (C) 2013, 2015 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <asm/pbl_nmon.h>

	.macro	board_pbl_start
	.set	push
	.set	noreorder

	mips_barebox_10h

	mips_disable_interrupts

	pbl_blt 0xbf000000 skip_pll_ram_config t8

	pbl_ar9331_pll
	pbl_ar9331_ddr2_config

skip_pll_ram_config:
	pbl_ar9331_uart_enable
	debug_ll_ar9331_init
	mips_nmon

	/*
	 * General Purpose I/O Function (GPIO_FUNCTION_1)
	 *
	 *  SPI_EN  (18) enables SPI SPA Interface signals
	 *               in GPIO_2, GPIO_3, GPIO_4 and GPIO_5.
	 *  RES     (15) reserved. This bit must be written with 1.
	 *  UART_EN  (2) enables UART I/O on GPIO_9 (SIN) and GPIO_10 (SOUT).
	 */

#define AR933X_GPIO_FUNC_SPI_EN			BIT(18)
#define AR933X_GPIO_FUNC_RESERVED		BIT(15)
#define AR933X_GPIO_FUNC_UART_EN		BIT(1)

#if 1
	pbl_reg_writel (AR933X_GPIO_FUNC_UART_EN
			| AR933X_GPIO_FUNC_RESERVED
			| AR933X_GPIO_FUNC_SPI_EN), GPIO_FUNC
#else
	pbl_reg_set (1 << 18), 0xb80600ac
#endif

	copy_to_link_location	pbl_start

	.set	pop
	.endm
