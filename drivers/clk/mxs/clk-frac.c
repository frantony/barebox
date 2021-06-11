// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>
#include <linux/math64.h>

#include "clk.h"

/**
 * struct clk_frac - mxs fractional divider clock
 * @hw: clk_hw for the fractional divider clock
 * @reg: register address
 * @shift: the divider bit shift
 * @width: the divider bit width
 * @busy: busy bit shift
 *
 * The clock is an adjustable fractional divider with a busy bit to wait
 * when the divider is adjusted.
 */
struct clk_frac {
	struct clk_hw hw;
	const char *parent;
	void __iomem *reg;
	u8 shift;
	u8 width;
	u8 busy;
};

#define to_clk_frac(_hw) container_of(_hw, struct clk_frac, hw)

static unsigned long clk_frac_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	struct clk_frac *frac = to_clk_frac(hw);
	u32 div;

	div = readl(frac->reg) >> frac->shift;
	div &= (1 << frac->width) - 1;

	return (parent_rate >> frac->width) * div;
}

static long clk_frac_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct clk_frac *frac = to_clk_frac(hw);
	unsigned long parent_rate = *prate;
	u32 div;
	u64 tmp;

	if (rate > parent_rate)
		return -EINVAL;

	tmp = rate;
	tmp <<= frac->width;
	do_div(tmp, parent_rate);
	div = tmp;

	if (!div)
		return -EINVAL;

	return (parent_rate >> frac->width) * div;
}

static int clk_frac_set_rate(struct clk_hw *hw, unsigned long rate,
			     unsigned long parent_rate)
{
	struct clk_frac *frac = to_clk_frac(hw);
	u32 div, val;
	u64 tmp;

	if (rate > parent_rate)
		return -EINVAL;

	tmp = rate;
	tmp <<= frac->width;
	do_div(tmp, parent_rate);
	div = tmp;

	if (!div)
		return -EINVAL;

	val = readl(frac->reg);
	val &= ~(((1 << frac->width) - 1) << frac->shift);
	val |= div << frac->shift;
	writel(val, frac->reg);

	if (clk_hw_is_enabled(hw))
		while (readl(frac->reg) & 1 << frac->busy);

	return 0;
}

static struct clk_ops clk_frac_ops = {
	.recalc_rate = clk_frac_recalc_rate,
	.round_rate = clk_frac_round_rate,
	.set_rate = clk_frac_set_rate,
};

struct clk *mxs_clk_frac(const char *name, const char *parent_name,
			 void __iomem *reg, u8 shift, u8 width, u8 busy)
{
	struct clk_frac *frac;
	int ret;

	frac = kzalloc(sizeof(*frac), GFP_KERNEL);
	if (!frac)
		return ERR_PTR(-ENOMEM);

	frac->parent = parent_name;
	frac->hw.clk.name = name;
	frac->hw.clk.ops = &clk_frac_ops;
	frac->hw.clk.parent_names = &frac->parent;
	frac->hw.clk.num_parents = 1;

	frac->reg = reg;
	frac->shift = shift;
	frac->width = width;

	ret = bclk_register(&frac->hw.clk);
	if (ret)
		return ERR_PTR(ret);

	return &frac->hw.clk;
}
