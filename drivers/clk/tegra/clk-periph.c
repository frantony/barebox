// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/log2.h>

#include "clk.h"

#define to_clk_periph(_hw) container_of(_hw, struct tegra_clk_periph, hw)

static int clk_periph_get_parent(struct clk_hw *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->mux->ops->get_parent(clk_to_clk_hw(periph->mux));
}

static int clk_periph_set_parent(struct clk_hw *hw, u8 index)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->mux->ops->set_parent(clk_to_clk_hw(periph->mux), index);
}

static unsigned long clk_periph_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->recalc_rate(clk_to_clk_hw(periph->div), parent_rate);
}

static long clk_periph_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *prate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->round_rate(clk_to_clk_hw(periph->div), rate, prate);
}

static int clk_periph_set_rate(struct clk_hw *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->set_rate(clk_to_clk_hw(periph->div), rate, parent_rate);
}

static int clk_periph_is_enabled(struct clk_hw *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->gate->ops->is_enabled(clk_to_clk_hw(periph->gate));
}

static int clk_periph_enable(struct clk_hw *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	periph->gate->ops->enable(clk_to_clk_hw(periph->gate));

	return 0;
}

static void clk_periph_disable(struct clk_hw *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	periph->gate->ops->disable(clk_to_clk_hw(periph->gate));
}

const struct clk_ops tegra_clk_periph_ops = {
	.get_parent = clk_periph_get_parent,
	.set_parent = clk_periph_set_parent,
	.recalc_rate = clk_periph_recalc_rate,
	.round_rate = clk_periph_round_rate,
	.set_rate = clk_periph_set_rate,
	.is_enabled = clk_periph_is_enabled,
	.enable = clk_periph_enable,
	.disable = clk_periph_disable,
};

const struct clk_ops tegra_clk_periph_nodiv_ops = {
	.get_parent = clk_periph_get_parent,
	.set_parent = clk_periph_set_parent,
	.is_enabled = clk_periph_is_enabled,
	.enable = clk_periph_enable,
	.disable = clk_periph_disable,
};

static struct clk *_tegra_clk_register_periph(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags,
		int div)
{
	struct tegra_clk_periph *periph;
	int ret, gate_offs, rst_offs;
	u8 mux_size = order_base_2(num_parents);

	periph = kzalloc(sizeof(*periph), GFP_KERNEL);
	if (!periph) {
		pr_err("%s: could not allocate peripheral clk\n",
		       __func__);
		goto out_periph;
	}

	periph->mux = clk_mux_alloc(NULL, 0, clk_base + reg_offset, 32 - mux_size,
				    mux_size, parent_names, num_parents, 0);
	if (!periph->mux)
		goto out_mux;

	if (id >= 96)
		gate_offs = 0x360 + (((id - 96) >> 3) & 0xc);
	else
		gate_offs = 0x10 + ((id >> 3) & 0xc);

	periph->gate = clk_gate_alloc(NULL, NULL, clk_base + gate_offs,
				      id & 0x1f, 0, 0);
	if (!periph->gate)
		goto out_gate;

	if (div == 8) {
		periph->div = tegra_clk_divider_alloc(NULL, NULL, clk_base +
		              reg_offset, 0, TEGRA_DIVIDER_ROUND_UP, 0, 8, 1);
		if (!periph->div)
			goto out_div;
	} else if (div == 16) {
		periph->div = tegra_clk_divider_alloc(NULL, NULL, clk_base +
		              reg_offset, 0, TEGRA_DIVIDER_ROUND_UP, 0, 16, 0);
		if (!periph->div)
			goto out_div;
	}

	periph->hw.clk.name = name;
	periph->hw.clk.ops = div ? &tegra_clk_periph_ops :
				   &tegra_clk_periph_nodiv_ops;
	periph->hw.clk.parent_names = parent_names;
	periph->hw.clk.num_parents = num_parents;
	periph->flags = flags;

	if (id >= 96)
		rst_offs = 0x358 + (((id - 96) >> 3) & 0xc);
	else
		rst_offs = 0x4 + ((id >> 3) & 0xc);
	periph->rst_reg = clk_base + rst_offs;
	periph->rst_shift = id & 0x1f;

	ret = bclk_register(&periph->hw.clk);
	if (ret)
		goto out_register;

	return &periph->hw.clk;

out_register:
	tegra_clk_divider_free(periph->div);
out_div:
	clk_gate_free(periph->gate);
out_gate:
	clk_mux_free(periph->mux);
out_mux:
	kfree(periph);
out_periph:
	return NULL;
}

struct clk *tegra_clk_register_periph_nodiv(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags)
{
	return _tegra_clk_register_periph(name, parent_names, num_parents,
					  clk_base, reg_offset, id, flags,
					  0);
}

struct clk *tegra_clk_register_periph(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags)
{
	return _tegra_clk_register_periph(name, parent_names, num_parents,
					  clk_base, reg_offset, id, flags,
					  8);
}

struct clk *tegra_clk_register_periph_div16(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags)
{
	return _tegra_clk_register_periph(name, parent_names, num_parents,
					  clk_base, reg_offset, id, flags,
					  16);
}
