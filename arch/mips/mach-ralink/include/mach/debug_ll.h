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

#include <board/debug_ll.h>

#define DEBUG_LL_UART_BPS	CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR	(DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#ifdef __ASSEMBLY__

#endif /* __ASSEMBLY__ */

#endif /* __MACH_RALINK_DEBUG_LL_H__ */
