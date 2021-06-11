// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>

#include "clk.h"

#define SET	0x4
#define CLR	0x8

/**
 * struct clk_pll - mxs pll clock
 * @hw: clk_hw for the pll
 * @base: base address of the pll
 * @power: the shift of power bit
 * @rate: the clock rate of the pll
 *
 * The mxs pll is a fixed rate clock with power and gate control,
 * and the shift of gate bit is always 31.
 */
struct clk_pll {
	struct clk_hw hw;
	const char *parent;
	void __iomem *base;
	u8 power;
	unsigned long rate;
};

#define to_clk_pll(_hw) container_of(_hw, struct clk_pll, hw)

static int clk_pll_enable(struct clk_hw *hw)
{
	struct clk_pll *pll = to_clk_pll(hw);

	writel(1 << pll->power, pll->base + SET);

	udelay(10);

	writel(1 << 31, pll->base + CLR);

	return 0;
}

static void clk_pll_disable(struct clk_hw *hw)
{
	struct clk_pll *pll = to_clk_pll(hw);

	writel(1 << 31, pll->base + SET);

	writel(1 << pll->power, pll->base + CLR);
}

static int clk_pll_is_enabled(struct clk_hw *hw)
{
	struct clk_pll *pll = to_clk_pll(hw);
	u32 val;

	val = readl(pll->base);

	if (val & (1 << 31))
		return 0;
	else
		return 1;
}

static unsigned long clk_pll_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct clk_pll *pll = to_clk_pll(hw);

	return pll->rate;
}

static const struct clk_ops clk_pll_ops = {
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_pll_recalc_rate,
	.is_enabled = clk_pll_is_enabled,
};

struct clk *mxs_clk_pll(const char *name, const char *parent_name,
			void __iomem *base, u8 power, unsigned long rate)
{
	struct clk_pll *pll;
	int ret;

	pll = xzalloc(sizeof(*pll));

	pll->parent = parent_name;
	pll->hw.clk.name = name;
	pll->hw.clk.ops = &clk_pll_ops;
	pll->hw.clk.parent_names = &pll->parent;
	pll->hw.clk.num_parents = 1;

	pll->base = base;
	pll->rate = rate;
	pll->power = power;

	ret = bclk_register(&pll->hw.clk);
	if (ret)
		ERR_PTR(ret);

	return &pll->hw.clk;
}
