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
 * struct clk_ref - mxs reference clock
 * @hw: clk_hw for the reference clock
 * @reg: register address
 * @idx: the index of the reference clock within the same register
 *
 * The mxs reference clock sources from pll.  Every 4 reference clocks share
 * one register space, and @idx is used to identify them.  Each reference
 * clock has a gate control and a fractional * divider.  The rate is calculated
 * as pll rate  * (18 / FRAC), where FRAC = 18 ~ 35.
 */
struct clk_ref {
	struct clk_hw hw;
	const char *parent;
	void __iomem *reg;
	u8 idx;
};

#define to_clk_ref(_hw) container_of(_hw, struct clk_ref, hw)

#define SET	0x4
#define CLR	0x8

static int clk_ref_is_enabled(struct clk_hw *hw)
{
	struct clk_ref *ref = to_clk_ref(hw);
	u32 reg = readl(ref->reg);

	if (reg & 1 << ((ref->idx + 1) * 8 - 1))
		return 0;

	return 1;
}

static int clk_ref_enable(struct clk_hw *hw)
{
	struct clk_ref *ref = to_clk_ref(hw);

	writel(1 << ((ref->idx + 1) * 8 - 1), ref->reg + CLR);

	return 0;
}

static void clk_ref_disable(struct clk_hw *hw)
{
	struct clk_ref *ref = to_clk_ref(hw);

	writel(1 << ((ref->idx + 1) * 8 - 1), ref->reg + SET);
}

static unsigned long clk_ref_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct clk_ref *ref = to_clk_ref(hw);
	u64 tmp = parent_rate;
	u8 frac = (readl(ref->reg) >> (ref->idx * 8)) & 0x3f;

	tmp *= 18;
	do_div(tmp, frac);

	return tmp;
}

static long clk_ref_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	unsigned long parent_rate = *prate;
	u64 tmp = parent_rate;
	u32 frac;

	tmp = tmp * 18 + rate / 2;
	do_div(tmp, rate);
	frac = tmp;

	if (frac < 18)
		frac = 18;
	else if (frac > 35)
		frac = 35;

	tmp = parent_rate;
	tmp *= 18;
	do_div(tmp, frac);

	return tmp;
}

static int clk_ref_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct clk_ref *ref = to_clk_ref(hw);
	u64 tmp = parent_rate;
	u32 val;
	u32 frac, shift = ref->idx * 8;

	tmp = tmp * 18 + rate / 2;
	do_div(tmp, rate);
	frac = tmp;

	if (frac < 18)
		frac = 18;
	else if (frac > 35)
		frac = 35;

	val = readl(ref->reg);
	val &= ~(0x3f << shift);
	val |= frac << shift;
	writel(val, ref->reg);

	return 0;
}

static const struct clk_ops clk_ref_ops = {
	.is_enabled	= clk_ref_is_enabled,
	.enable		= clk_ref_enable,
	.disable	= clk_ref_disable,
	.recalc_rate	= clk_ref_recalc_rate,
	.round_rate	= clk_ref_round_rate,
	.set_rate	= clk_ref_set_rate,
};

struct clk *mxs_clk_ref(const char *name, const char *parent_name,
			void __iomem *reg, u8 idx)
{
	struct clk_ref *ref;
	int ret;

	ref = xzalloc(sizeof(*ref));

	ref->parent = parent_name;
	ref->hw.clk.name = name;
	ref->hw.clk.ops = &clk_ref_ops;
	ref->hw.clk.parent_names = &ref->parent;
	ref->hw.clk.num_parents = 1;

	ref->reg = reg;
	ref->idx = idx;

	ret = bclk_register(&ref->hw.clk);
	if (ret)
		return ERR_PTR(ret);

	return &ref->hw.clk;
}
