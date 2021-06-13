/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __ASM_DEBUG_LL__
#define __ASM_DEBUG_LL__

/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>

#ifdef CONFIG_DEBUG_ERIZO

#define DEBUG_LL_UART_ADDR	0x90000000
#define DEBUG_LL_UART_SHIFT	2
#define DEBUG_LL_UART_IOSIZE32

#define DEBUG_LL_UART_CLK       (24000000 / 16)
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#elif defined(CONFIG_BOARD_BEAGLEV)

#define DEBUG_LL_UART_ADDR	0x12440000
#define DEBUG_LL_UART_SHIFT	2
#define DEBUG_LL_UART_IOSIZE32

#define DEBUG_LL_UART_CLK	(100000000 / 16)
#define DEBUG_LL_UART_BPS	CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR	(DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#elif defined CONFIG_DEBUG_SIFIVE

#include <io.h>

static inline void PUTC_LL(char ch)
{
	void __iomem *uart0 = IOMEM(0x10010000);

	while (readl(uart0) & 0x80000000)
		;

	writel(ch, uart0);
}

#endif

#ifndef debug_ll_init
#define debug_ll_init() (void)0
#endif

#endif /* __ASM_DEBUG_LL__ */
