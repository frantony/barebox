// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2015 Altera Corporation. All rights reserved
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/clkdev.h>

#include "clk.h"

#define CLK_MGR_FREE_SHIFT		16
#define CLK_MGR_FREE_MASK		0x7

#define SOCFPGA_MPU_FREE_CLK		"mpu_free_clk"
#define SOCFPGA_NOC_FREE_CLK		"noc_free_clk"
#define SOCFPGA_SDMMC_FREE_CLK		"sdmmc_free_clk"
#define to_socfpga_periph_clk(p) container_of(p, struct socfpga_periph_clk, hw)

static unsigned long clk_periclk_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct socfpga_periph_clk *socfpgaclk = to_socfpga_periph_clk(hw);
	u32 div;

	if (socfpgaclk->fixed_div) {
		div = socfpgaclk->fixed_div;
	} else if (socfpgaclk->div_reg) {
		div = readl(socfpgaclk->div_reg) >> socfpgaclk->shift;
		div &= GENMASK(socfpgaclk->width - 1, 0);
		div += 1;
	} else {
		div = ((readl(socfpgaclk->reg) & 0x7ff) + 1);
	}

	return parent_rate / div;
}

static int clk_periclk_get_parent(struct clk_hw *hw)
{
	struct socfpga_periph_clk *socfpgaclk = to_socfpga_periph_clk(hw);
	u32 clk_src;

	clk_src = readl(socfpgaclk->reg);
	if (streq(clk_hw_get_name(hw), SOCFPGA_MPU_FREE_CLK) ||
	    streq(clk_hw_get_name(hw), SOCFPGA_NOC_FREE_CLK) ||
	    streq(clk_hw_get_name(hw), SOCFPGA_SDMMC_FREE_CLK))
		return (clk_src >> CLK_MGR_FREE_SHIFT) &
			CLK_MGR_FREE_MASK;
	else
		return 0;
}

static const struct clk_ops periclk_ops = {
	.recalc_rate = clk_periclk_recalc_rate,
	.get_parent = clk_periclk_get_parent,
};

static struct clk *__socfpga_periph_init(struct device_node *node,
	const struct clk_ops *ops)
{
	u32 reg;
	struct socfpga_periph_clk *periph_clk;
	const char *clk_name = node->name;
	int rc;
	u32 fixed_div;
	u32 div_reg[3];
	int i;

	of_property_read_u32(node, "reg", &reg);

	periph_clk = xzalloc(sizeof(*periph_clk));

	periph_clk->reg = clk_mgr_base_addr + reg;

	rc = of_property_read_u32_array(node, "div-reg", div_reg, 3);
	if (!rc) {
		periph_clk->div_reg = clk_mgr_base_addr + div_reg[0];
		periph_clk->shift = div_reg[1];
		periph_clk->width = div_reg[2];
	} else {
		periph_clk->div_reg = NULL;
	}

	rc = of_property_read_u32(node, "fixed-divider", &fixed_div);
	if (rc)
		periph_clk->fixed_div = 0;
	else
		periph_clk->fixed_div = fixed_div;

	of_property_read_string(node, "clock-output-names", &clk_name);

	for (i = 0; i < SOCFPGA_MAX_PARENTS; i++) {
		periph_clk->parent_names[i] = of_clk_get_parent_name(node, i);
		if (!periph_clk->parent_names[i])
			break;
	}

	periph_clk->hw.clk.num_parents = i;
	periph_clk->hw.clk.parent_names = periph_clk->parent_names;

	periph_clk->hw.clk.name = xstrdup(clk_name);
	periph_clk->hw.clk.ops = ops;

	rc = bclk_register(&periph_clk->hw.clk);
	if (rc) {
		free(periph_clk);
		return ERR_PTR(rc);
	}

	return &periph_clk->hw.clk;
}

struct clk *socfpga_a10_periph_init(struct device_node *node)
{
	return __socfpga_periph_init(node, &periclk_ops);
}
