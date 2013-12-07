/*
 * based on linux.git/drivers/tty/serial/ar933x_uart.c
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

#ifndef __MACH_AR9331_DEBUG_LL__
#define __MACH_AR9331_DEBUG_LL__

#include <io.h>
#include <asm/addrspace.h>

#define AR71XX_APB_BASE         0x18000000
#define AR71XX_UART_BASE        (AR71XX_APB_BASE + 0x00020000)

#define AR933X_UART_DATA_REG            0x00
#define AR933X_UART_DATA_TX_RX_MASK     0xff
#define AR933X_UART_DATA_TX_CSR         BIT(9)

#define DEBUG_LL_UART_ADDR	KSEG1ADDR(AR71XX_UART_BASE)

static inline void ar933x_debug_ll_writel(u32 b, int offset)
{
	cpu_writel(b, (u8 *)DEBUG_LL_UART_ADDR + offset);
}

static inline u32 ar933x_debug_ll_readl(int offset)
{
	return cpu_readl((u8 *)DEBUG_LL_UART_ADDR + offset);
}

static __inline__ void PUTC_LL(int ch)
{
	u32 data;

	/* wait transmitter ready */
	data = ar933x_debug_ll_readl(AR933X_UART_DATA_REG);
	while (!(data & AR933X_UART_DATA_TX_CSR)) {
		data = ar933x_debug_ll_readl(AR933X_UART_DATA_REG);
	}

	data = (ch & AR933X_UART_DATA_TX_RX_MASK) | AR933X_UART_DATA_TX_CSR;
	ar933x_debug_ll_writel(data, AR933X_UART_DATA_REG);
}

#endif /* __MACH_AR9331_DEBUG_LL__ */
