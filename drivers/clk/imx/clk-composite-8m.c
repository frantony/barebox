// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2018 NXP
 */

#include <common.h>
#include <errno.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include "clk.h"

#define PCG_PREDIV_SHIFT	16
#define PCG_PREDIV_WIDTH	3
#define PCG_PREDIV_MAX		8

#define PCG_DIV_SHIFT		0
#define PCG_CORE_DIV_WIDTH	3
#define PCG_DIV_WIDTH		6
#define PCG_DIV_MAX		64

#define PCG_PCS_SHIFT		24
#define PCG_PCS_WIDTH		3

#define PCG_CGC_SHIFT		28

#define clk_div_mask(width)	((1 << (width)) - 1)

static unsigned long imx8m_clk_composite_divider_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	struct clk_divider *divider = container_of(hw, struct clk_divider, hw);
	struct clk *clk = clk_hw_to_clk(hw);
	unsigned long prediv_rate;
	unsigned int prediv_value;
	unsigned int div_value;

	prediv_value = readl(divider->reg) >> divider->shift;
	prediv_value &= clk_div_mask(divider->width);

	prediv_rate = divider_recalc_rate(clk, parent_rate, prediv_value,
						NULL, divider->flags,
						divider->width);

	div_value = readl(divider->reg) >> PCG_DIV_SHIFT;
	div_value &= clk_div_mask(PCG_DIV_WIDTH);

	return divider_recalc_rate(clk, prediv_rate, div_value, NULL,
				   divider->flags, PCG_DIV_WIDTH);
}

static int imx8m_clk_composite_compute_dividers(unsigned long rate,
						unsigned long parent_rate,
						int *prediv, int *postdiv)
{
	int div1, div2;
	int error = INT_MAX;
	int ret = -EINVAL;

	*prediv = 1;
	*postdiv = 1;

	for (div1 = 1; div1 <= PCG_PREDIV_MAX; div1++) {
		for (div2 = 1; div2 <= PCG_DIV_MAX; div2++) {
			int new_error = ((parent_rate / div1) / div2) - rate;

			if (abs(new_error) < abs(error)) {
				*prediv = div1;
				*postdiv = div2;
				error = new_error;
				ret = 0;
			}
		}
	}
	return ret;
}

static long imx8m_clk_composite_divider_round_rate(struct clk_hw *hw,
						unsigned long rate,
						unsigned long *prate)
{
	int prediv_value;
	int div_value;

	imx8m_clk_composite_compute_dividers(rate, *prate,
						&prediv_value, &div_value);
	rate = DIV_ROUND_UP(*prate, prediv_value);

	return DIV_ROUND_UP(rate, div_value);

}

static int imx8m_clk_composite_divider_set_rate(struct clk_hw *hw,
					unsigned long rate,
					unsigned long parent_rate)
{
	struct clk_divider *divider = container_of(hw, struct clk_divider, hw);
	int prediv_value;
	int div_value;
	int ret;
	u32 val;

	ret = imx8m_clk_composite_compute_dividers(rate, parent_rate,
						&prediv_value, &div_value);
	if (ret)
		return -EINVAL;

	val = readl(divider->reg);
	val &= ~((clk_div_mask(divider->width) << divider->shift) |
			(clk_div_mask(PCG_DIV_WIDTH) << PCG_DIV_SHIFT));

	val |= (u32)(prediv_value  - 1) << divider->shift;
	val |= (u32)(div_value - 1) << PCG_DIV_SHIFT;
	writel(val, divider->reg);

	return ret;
}
static int imx8m_clk_composite_mux_get_parent(struct clk_hw *hw)
{
	return clk_mux_ops.get_parent(hw);
}

static int imx8m_clk_composite_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_mux *m = container_of(hw, struct clk_mux, hw);
	u32 val;

	val = readl(m->reg);
	val &= ~(((1 << m->width) - 1) << m->shift);
	val |= index << m->shift;

	/*
	 * write twice to make sure non-target interface
	 * SEL_A/B point the same clk input.
	 */
	writel(val, m->reg);
	writel(val, m->reg);

	return 0;
}

static const struct clk_ops imx8m_clk_composite_divider_ops = {
	.recalc_rate = imx8m_clk_composite_divider_recalc_rate,
	.round_rate = imx8m_clk_composite_divider_round_rate,
	.set_rate = imx8m_clk_composite_divider_set_rate,
};

static const struct clk_ops imx8m_clk_composite_mux_ops = {
	.get_parent = imx8m_clk_composite_mux_get_parent,
	.set_parent = imx8m_clk_composite_mux_set_parent,
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
};

struct clk *imx8m_clk_composite_flags(const char *name,
					const char * const *parent_names,
					int num_parents, void __iomem *reg,
					u32 composite_flags,
					unsigned long flags)
{
	struct clk *comp = ERR_PTR(-ENOMEM);
	struct clk_divider *div = NULL;
	struct clk_gate *gate = NULL;
	struct clk_mux *mux = NULL;
	const struct clk_ops *mux_ops;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		goto fail;

	mux->reg = reg;
	mux->shift = PCG_PCS_SHIFT;
	mux->width = PCG_PCS_WIDTH;
	mux->hw.clk.ops = &clk_mux_ops;

	div = kzalloc(sizeof(*div), GFP_KERNEL);
	if (!div)
		goto fail;

	div->reg = reg;
	if (composite_flags & IMX_COMPOSITE_CORE) {
		div->shift = PCG_DIV_SHIFT;
		div->width = PCG_CORE_DIV_WIDTH;
		div->hw.clk.ops = &clk_divider_ops;
		mux_ops = &imx8m_clk_composite_mux_ops;
	} else if (composite_flags & IMX_COMPOSITE_BUS) {
		div->shift = PCG_PREDIV_SHIFT;
		div->width = PCG_PREDIV_WIDTH;
		div->hw.clk.ops = &imx8m_clk_composite_divider_ops;
		mux_ops = &imx8m_clk_composite_mux_ops;
	} else {
		div->shift = PCG_PREDIV_SHIFT;
		div->width = PCG_PREDIV_WIDTH;
		div->hw.clk.ops = &imx8m_clk_composite_divider_ops;
		mux_ops = &clk_mux_ops;
	}

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		goto fail;

	gate->reg = reg;
	gate->shift = PCG_CGC_SHIFT;
	gate->hw.clk.ops = &clk_gate_ops;

	comp = clk_register_composite(name, parent_names, num_parents,
				      &mux->hw.clk, &div->hw.clk, &gate->hw.clk, flags);
	if (IS_ERR(comp))
		goto fail;

	return comp;

fail:
	kfree(gate);
	kfree(div);
	kfree(mux);
	return ERR_CAST(comp);
}
