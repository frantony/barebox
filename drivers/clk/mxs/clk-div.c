// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

/**
 * struct clk_div - mxs integer divider clock
 * @divider: the parent class
 * @ops: pointer to clk_ops of parent class
 * @reg: register address
 * @busy: busy bit shift
 *
 * The mxs divider clock is a subclass of basic clk_divider with an
 * addtional busy bit.
 */
struct clk_div {
	struct clk_divider divider;
	const char *parent;
	const struct clk_ops *ops;
	void __iomem *reg;
	u8 busy;
};

static inline struct clk_div *to_clk_div(struct clk_hw *hw)
{
	struct clk_divider *divider = to_clk_divider(hw);

	return container_of(divider, struct clk_div, divider);
}

static unsigned long clk_div_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct clk_div *div = to_clk_div(hw);

	return div->ops->recalc_rate(&div->divider.hw, parent_rate);
}

static long clk_div_round_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long *prate)
{
	struct clk_div *div = to_clk_div(hw);

	return div->ops->round_rate(&div->divider.hw, rate, prate);
}

static int clk_div_set_rate(struct clk_hw *hw, unsigned long rate,
			    unsigned long parent_rate)
{
	struct clk_div *div = to_clk_div(hw);
	int ret;

	ret = div->ops->set_rate(&div->divider.hw, rate, parent_rate);
	if (ret)
		return ret;

	if (clk_hw_is_enabled(hw))
		while (readl(div->reg) & 1 << div->busy);

	return 0;
}

static struct clk_ops clk_div_ops = {
	.recalc_rate = clk_div_recalc_rate,
	.round_rate = clk_div_round_rate,
	.set_rate = clk_div_set_rate,
};

struct clk *mxs_clk_div(const char *name, const char *parent_name,
			void __iomem *reg, u8 shift, u8 width, u8 busy)
{
	struct clk_div *div;
	int ret;

	div = xzalloc(sizeof(*div));

	div->parent = parent_name;
	div->divider.hw.clk.name = name;
	div->divider.hw.clk.ops = &clk_div_ops;
	div->divider.hw.clk.parent_names = &div->parent;
	div->divider.hw.clk.num_parents = 1;

	div->reg = reg;
	div->busy = busy;

	div->divider.reg = reg;
	div->divider.shift = shift;
	div->divider.width = width;
	div->divider.flags = CLK_DIVIDER_ONE_BASED;
	div->ops = &clk_divider_ops;

	ret = bclk_register(&div->divider.hw.clk);
	if (ret)
		return ERR_PTR(ret);

	return &div->divider.hw.clk;
}
