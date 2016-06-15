/*
 * Copyright (C) 2015 Antony Pavlov <antonynpavlov@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __MACH_RALINK_DEBUG_LL_H__
#define __MACH_RALINK_DEBUG_LL_H__

#define DEBUG_LL_UART_ADDR	0xb0000c00
#define DEBUG_LL_UART_SHIFT	2

/* 40 MHz? */
#define DEBUG_LL_UART_CLK      (39628800 / 16)
#define DEBUG_LL_UART_BPS	CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR	(DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#ifdef __ASSEMBLY__

.macro	debug_ll_rt2880_init
#ifdef CONFIG_DEBUG_LL
	la	t0, DEBUG_LL_UART_ADDR

	li	t1, DEBUG_LL_UART_DIVISOR
	sw	t1, 0x28(t0)

	li	t1, UART_LCR_W			/* DLAB off */
	sb	t1, UART_LCR(t0)		/* Write it out */
#endif /* CONFIG_DEBUG_LL */
.endm

#endif /* __ASSEMBLY__ */

#endif /* __MACH_RALINK_DEBUG_LL_H__ */
