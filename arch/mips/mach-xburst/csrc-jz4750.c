/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Based on barebox' tegra20-timer.c:
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
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

/**
 * @file
 * @brief Clocksource based on JZ475x OS timer
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <mach/jz4750d_regs.h>

#define TCU_OSTDR	0x00
#define TCU_OSTCNT  0x08
#define TCU_OSTCSR	0x0c
#define TCU_OSTCSR_PRESCALE_BIT		3
#define TCU_OSTCSR_PRESCALE_MASK	(0x7 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE1		(0x0 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE4		(0x1 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE16		(0x2 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE64		(0x3 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE256		(0x4 << TCU_OSTCSR_PRESCALE_BIT)
 #define TCU_OSTCSR_PRESCALE1024	(0x5 << TCU_OSTCSR_PRESCALE_BIT)
#define TCU_OSTCSR_EXT_EN		BIT(2) /* select extal as the timer clock input */
#define TCU_OSTCSR_RTC_EN		BIT(1) /* select rtcclk as the timer clock input */
#define TCU_OSTCSR_PCK_EN		BIT(0) /* select pclk as the timer clock input */

static void __iomem *ostimer_base;

static inline void jz4750_ostimer_reg_writel(u32 val, int reg)
{
	writel(val, ostimer_base + reg);
}

static uint64_t jz4750_ostimer_cs_read(void)
{
	return (uint64_t)readl(ostimer_base + TCU_OSTCNT);
}

static struct clocksource jz4750_ostimer_cs = {
	.read	= jz4750_ostimer_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
};

static int jz4750_ostimer_cs_probe(struct device_d *dev)
{
	struct clk *timer_clk;
	unsigned long rate;

	/* use only one timer */
	if (ostimer_base)
		return -EBUSY;

	ostimer_base = dev_request_mem_region(dev, 0);
	if (!ostimer_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	timer_clk = clk_get(dev, NULL);
	if (!timer_clk) {
		dev_err(dev, "could not get clock\n");
		return -ENODEV;
	}

	clk_enable(timer_clk);

	rate = clk_get_rate(timer_clk);

	clocks_calc_mult_shift(&jz4750_ostimer_cs.mult,
		&jz4750_ostimer_cs.shift, rate, NSEC_PER_SEC, 10);

	init_clock(&jz4750_ostimer_cs);

	jz4750_ostimer_reg_writel(TCU_OSTCSR_PRESCALE1 | TCU_OSTCSR_EXT_EN,
		TCU_OSTCSR);
	jz4750_ostimer_reg_writel(0, TCU_OSTCNT);
	jz4750_ostimer_reg_writel(0xffffffff, TCU_OSTDR);

	/* enable timer clock */
	writel(TCU_TSCR_OSTSC, (void *)TCU_TSCR);
	/* start counting up */
	writel(TCU_TESR_OSTST, (void *)TCU_TESR);

	return 0;
}

static __maybe_unused struct of_device_id jz4750_ostimer_dt_ids[] = {
	{
		.compatible = "ingenic,jz4750-ostimer",
	}, {
		/* sentinel */
	}
};

static struct driver_d jz4750_ostimer_cs_driver = {
	.probe	= jz4750_ostimer_cs_probe,
	.name	= "jz4750-ostimer",
	.of_compatible = DRV_OF_COMPAT(jz4750_ostimer_dt_ids),
};

static int jz4750_ostimer_cs_init(void)
{
	return platform_driver_register(&jz4750_ostimer_cs_driver);
}
coredevice_initcall(jz4750_ostimer_cs_init);
