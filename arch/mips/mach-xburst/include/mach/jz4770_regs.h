/*
 *  based on linux/include/asm-mips/mach-jz4770/regs.h
 *
 *  Ingenic's JZ4770 common include.
 *
 *  Copyright (C) 2006 - 2007 Ingenic Semiconductor Inc.
 *
 *  Author: <yliu@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JZ4770_REGS_H__
#define __JZ4770_REGS_H__

#include <linux/bitops.h>

#define CPM_BASE	0xb0000000
#define OST_BASE	0xb0002000
#define TCU_BASE	0xb0002000
#define WDT_BASE	0xb0002000
#define UART2_BASE	0xb0032000

/*
 * OST registers address definition
 */
#define OST_OSTDR	(OST_BASE + 0xe0)
#define OST_OSTCNT	(OST_BASE + 0xe4)
#define OST_OSTCNTL	(OST_BASE + 0xe4)
#define OST_OSTCNTH	(OST_BASE + 0xe8)
#define OST_OSTCNTHBUF	(OST_BASE + 0xfc)
#define OST_OSTCSR	(OST_BASE + 0xec)

/*
 * OST registers common define
 */

/* Operating system control register(OSTCSR) */
#define OSTCSR_CNT_MD		BIT(15)
#define OSTCSR_SD		BIT(9)
#define OSTCSR_EXT_EN		BIT(2)
#define OSTCSR_RTC_EN		BIT(1)
#define OSTCSR_PCK_EN		BIT(0)

#define OSTCSR_PRESCALE_LSB	3
#define OSTCSR_PRESCALE_MASK	BITS_H2L(5, OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE1	(0x0 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE4	(0x1 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE16	(0x2 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE64	(0x3 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE256	(0x4 << OSTCSR_PRESCALE_LSB)
#define OSTCSR_PRESCALE1024	(0x5 << OSTCSR_PRESCALE_LSB)

/*
 * TCU registers address definition
 */
#define TCU_TESR	(TCU_BASE + 0x14)
#define TCU_TSCR	(TCU_BASE + 0x3c)

/*
 * TCU registers bit field common define
 */

/* Timer counter enable set register(TESR) */
#define TESR_OST	BIT(15)

/* Timer counter enable clear register(TECR) */
#define TECR_OST	BIT(15)

/* Timer stop register(TSR) */
#define TSR_WDT_STOP		BIT(16)
#define TSR_OST_STOP		BIT(15)

/* Timer stop set register(TSSR) */
#define TSSR_WDT		BIT(16)
#define TSSR_OST		BIT(15)

/* Timer stop clear register(TSCR) */
#define TSCR_WDT		BIT(16)
#define TSCR_OST		BIT(15)

/* Timer control register(TCSR) */
#define TCSR_BYPASS		BIT(11)
#define TCSR_CLRZ		BIT(10)
#define TCSR_SD_ABRUPT		BIT(9)
#define TCSR_INITL_HIGH		BIT(8)
#define TCSR_PWM_EN		BIT(7)
#define TCSR_PWM_IN_EN		BIT(6)
#define TCSR_EXT_EN		BIT(2)
#define TCSR_RTC_EN		BIT(1)
#define TCSR_PCK_EN		BIT(0)

#define TCSR_PRESCALE_LSB	3
#define TCSR_PRESCALE_MASK	BITS_H2L(5, TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE1		(0x0 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE4		(0x1 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE16		(0x2 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE64		(0x3 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE256	(0x4 << TCSR_PRESCALE_LSB)
#define TCSR_PRESCALE1024	(0x5 << TCSR_PRESCALE_LSB)

#endif /* __JZ4770_REGS_H__ */
