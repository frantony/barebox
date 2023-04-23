// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <clock.h>
#include <linux/err.h>

#include <debug_ll.h>

#define SC6531E_TIMER_CLOCK 1000

#define SYST_VALUE_SHDW 0x0c

static void __iomem *timer_base;

static uint64_t sc6531e_cs_read(void)
{
	return (uint64_t)readl(timer_base + SYST_VALUE_SHDW);
}

static struct clocksource sc6531e_cs = {
	.read	= sc6531e_cs_read,
	.mask   = CLOCKSOURCE_MASK(32),
	.priority = 60,
};

static int sc6531e_timer_probe(struct device *dev)
{
	struct resource *iores;

	/* use only one timer */
	if (timer_base)
		return -EBUSY;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}

	timer_base = IOMEM(iores->start);
	clocks_calc_mult_shift(&sc6531e_cs.mult, &sc6531e_cs.shift,
		SC6531E_TIMER_CLOCK, NSEC_PER_SEC, 1);

	init_clock(&sc6531e_cs);

	return 0;
}

static __maybe_unused struct of_device_id sc6531e_timer_dt_ids[] = {
	{
		.compatible = "sc6531e-timer",
	}, {
		/* sentinel */
	}
};

static struct driver sc6531e_timer_driver = {
	.probe	= sc6531e_timer_probe,
	.name	= "sc6531e-timer",
	.of_compatible = DRV_OF_COMPAT(sc6531e_timer_dt_ids),
};

coredevice_platform_driver(sc6531e_timer_driver);
