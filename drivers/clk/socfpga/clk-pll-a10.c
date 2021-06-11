// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 Altera Corporation. All rights reserved
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/math64.h>

#include "clk.h"

/* Clock Manager offsets */
#define CLK_MGR_PLL_CLK_SRC_SHIFT	8
#define CLK_MGR_PLL_CLK_SRC_MASK	0x3

/* Clock bypass bits */
#define SOCFPGA_PLL_BG_PWRDWN		0
#define SOCFPGA_PLL_PWR_DOWN		1
#define SOCFPGA_PLL_EXT_ENA		2
#define SOCFPGA_PLL_DIVF_MASK		0x00001FFF
#define SOCFPGA_PLL_DIVF_SHIFT	0
#define SOCFPGA_PLL_DIVQ_MASK		0x003F0000
#define SOCFPGA_PLL_DIVQ_SHIFT	16
#define SOCFGPA_MAX_PARENTS	5

#define SOCFPGA_MAIN_PLL_CLK		"main_pll"
#define SOCFPGA_PERIP_PLL_CLK		"periph_pll"

#define to_socfpga_clk(p) container_of(p, struct socfpga_pll, hw)

static unsigned long clk_pll_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hw);
	unsigned long divf, divq, reg;
	unsigned long long vco_freq;

	/* read VCO1 reg for numerator and denominator */
	reg = readl(socfpgaclk->reg + 0x4);
	divf = (reg & SOCFPGA_PLL_DIVF_MASK) >> SOCFPGA_PLL_DIVF_SHIFT;
	divq = (reg & SOCFPGA_PLL_DIVQ_MASK) >> SOCFPGA_PLL_DIVQ_SHIFT;
	vco_freq = (unsigned long long)parent_rate * (divf + 1);
	do_div(vco_freq, (1 + divq));
	return (unsigned long)vco_freq;
}

static int clk_pll_get_parent(struct clk_hw *hw)
{
	struct socfpga_pll *socfpgaclk = to_socfpga_clk(hw);
	u32 pll_src;

	pll_src = readl(socfpgaclk->reg);

	return (pll_src >> CLK_MGR_PLL_CLK_SRC_SHIFT) &
		CLK_MGR_PLL_CLK_SRC_MASK;
}

static int clk_socfpga_enable(struct clk_hw *hw)
{
	struct socfpga_pll *socfpga_clk = to_socfpga_clk(hw);
	u32 val;

	val = readl(socfpga_clk->reg);
	val |= 1 << socfpga_clk->bit_idx;
	writel(val, socfpga_clk->reg);

	return 0;
}

static void clk_socfpga_disable(struct clk_hw *hw)
{
	struct socfpga_pll *socfpga_clk = to_socfpga_clk(hw);
	u32 val;

	val = readl(socfpga_clk->reg);
	val &= ~(1 << socfpga_clk->bit_idx);
	writel(val, socfpga_clk->reg);
}

static struct clk_ops clk_pll_ops = {
	.recalc_rate = clk_pll_recalc_rate,
	.get_parent = clk_pll_get_parent,
};

static struct clk *__socfpga_pll_init(struct device_node *node,
	const struct clk_ops *ops)
{
	u32 reg;
	struct socfpga_pll *pll_clk;
	const char *clk_name = node->name;
	int rc;
	int i;

	of_property_read_u32(node, "reg", &reg);

	pll_clk = xzalloc(sizeof(*pll_clk));

	pll_clk->reg = clk_mgr_base_addr + reg;

	of_property_read_string(node, "clock-output-names", &clk_name);

	pll_clk->hw.clk.name = xstrdup(clk_name);
	pll_clk->hw.clk.ops = ops;

	for (i = 0; i < SOCFPGA_MAX_PARENTS; i++) {
		pll_clk->parent_names[i] = of_clk_get_parent_name(node, i);
		if (!pll_clk->parent_names[i])
			break;
	}

	pll_clk->bit_idx = SOCFPGA_PLL_EXT_ENA;
	pll_clk->hw.clk.num_parents = i;
	pll_clk->hw.clk.parent_names = pll_clk->parent_names;

	clk_pll_ops.enable = clk_socfpga_enable;
	clk_pll_ops.disable = clk_socfpga_disable;

	rc = bclk_register(&pll_clk->hw.clk);
	if (rc) {
		free(pll_clk);
		return NULL;
	}

	return &pll_clk->hw.clk;
}

struct clk *socfpga_a10_pll_init(struct device_node *node)
{
	return __socfpga_pll_init(node, &clk_pll_ops);
}
