// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 * Copyright 2012 Linaro Ltd.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <malloc.h>
#include <linux/math64.h>

#include "clk.h"

/**
 * struct clk_pfd - IMX PFD clock
 * @clk_hw:	clock source
 * @reg:	PFD register address
 * @idx:	the index of PFD encoded in the register
 *
 * PFD clock found on i.MX6 series.  Each register for PFD has 4 clk_pfd
 * data encoded, and member idx is used to specify the one.  And each
 * register has SET, CLR and TOG registers at offset 0x4 0x8 and 0xc.
 */
struct clk_pfd {
	struct clk_hw	hw;
	void __iomem	*reg;
	u8		idx;
	const char	*parent;
};

#define to_clk_pfd(_hw) container_of(_hw, struct clk_pfd, hw)

#define SET	0x4
#define CLR	0x8
#define OTG	0xc

static int clk_pfd_enable(struct clk_hw *hw)
{
	struct clk_pfd *pfd = to_clk_pfd(hw);
	writel(1 << ((pfd->idx + 1) * 8 - 1), pfd->reg + CLR);

	return 0;
}

static void clk_pfd_disable(struct clk_hw *hw)
{
	struct clk_pfd *pfd = to_clk_pfd(hw);

	writel(1 << ((pfd->idx + 1) * 8 - 1), pfd->reg + SET);
}

static unsigned long clk_pfd_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct clk_pfd *pfd = to_clk_pfd(hw);
	u64 tmp = parent_rate;
	u8 frac = (readl(pfd->reg) >> (pfd->idx * 8)) & 0x3f;

	tmp *= 18;
	do_div(tmp, frac);

	return tmp;
}

static long clk_pfd_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	u64 tmp = *prate;
	u8 frac;

	tmp = tmp * 18 + rate / 2;
	do_div(tmp, rate);
	frac = tmp;
	if (frac < 12)
		frac = 12;
	else if (frac > 35)
		frac = 35;
	tmp = *prate;
	tmp *= 18;
	do_div(tmp, frac);

	return tmp;
}

static int clk_pfd_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct clk_pfd *pfd = to_clk_pfd(hw);
	u64 tmp = parent_rate;
	u8 frac;

	tmp = tmp * 18 + rate / 2;
	do_div(tmp, rate);
	frac = tmp;
	if (frac < 12)
		frac = 12;
	else if (frac > 35)
		frac = 35;

	writel(0x3f << (pfd->idx * 8), pfd->reg + CLR);
	writel(frac << (pfd->idx * 8), pfd->reg + SET);

	return 0;
}

static const struct clk_ops clk_pfd_ops = {
	.enable		= clk_pfd_enable,
	.disable	= clk_pfd_disable,
	.recalc_rate	= clk_pfd_recalc_rate,
	.round_rate	= clk_pfd_round_rate,
	.set_rate	= clk_pfd_set_rate,
};

struct clk *imx_clk_pfd(const char *name, const char *parent,
			void __iomem *reg, u8 idx)
{
	struct clk_pfd *pfd;
	int ret;

	pfd = xzalloc(sizeof(*pfd));

	pfd->reg = reg;
	pfd->idx = idx;
	pfd->parent = parent;
	pfd->hw.clk.name = name;
	pfd->hw.clk.ops = &clk_pfd_ops;
	pfd->hw.clk.parent_names = &pfd->parent;
	pfd->hw.clk.num_parents = 1;

	ret = bclk_register(&pfd->hw.clk);
	if (ret) {
		free(pfd);
		return ERR_PTR(ret);
	}

	return &pfd->hw.clk;
}
