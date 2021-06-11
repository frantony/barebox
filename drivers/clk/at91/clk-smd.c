// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/overflow.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define SMD_SOURCE_MAX		2

#define SMD_DIV_SHIFT		8
#define SMD_MAX_DIV		0xf

struct at91sam9x5_clk_smd {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent_names[];
};

#define to_at91sam9x5_clk_smd(_hw) \
	container_of(_hw, struct at91sam9x5_clk_smd, hw)

static unsigned long at91sam9x5_clk_smd_recalc_rate(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	struct at91sam9x5_clk_smd *smd = to_at91sam9x5_clk_smd(hw);
	unsigned int smdr;
	u8 smddiv;

	regmap_read(smd->regmap, AT91_PMC_SMD, &smdr);
	smddiv = (smdr & AT91_PMC_SMD_DIV) >> SMD_DIV_SHIFT;

	return parent_rate / (smddiv + 1);
}

static long at91sam9x5_clk_smd_round_rate(struct clk_hw *hw, unsigned long rate,
					  unsigned long *parent_rate)
{
	unsigned long div;
	unsigned long bestrate;
	unsigned long tmp;

	if (rate >= *parent_rate)
		return *parent_rate;

	div = *parent_rate / rate;
	if (div > SMD_MAX_DIV)
		return *parent_rate / (SMD_MAX_DIV + 1);

	bestrate = *parent_rate / div;
	tmp = *parent_rate / (div + 1);
	if (bestrate - rate > rate - tmp)
		bestrate = tmp;

	return bestrate;
}

static int at91sam9x5_clk_smd_set_parent(struct clk_hw *hw, u8 index)
{
	struct at91sam9x5_clk_smd *smd = to_at91sam9x5_clk_smd(hw);

	if (index > 1)
		return -EINVAL;

	regmap_write_bits(smd->regmap, AT91_PMC_SMD, AT91_PMC_SMDS,
			  index ? AT91_PMC_SMDS : 0);

	return 0;
}

static int at91sam9x5_clk_smd_get_parent(struct clk_hw *hw)
{
	struct at91sam9x5_clk_smd *smd = to_at91sam9x5_clk_smd(hw);
	unsigned int smdr;

	regmap_read(smd->regmap, AT91_PMC_SMD, &smdr);

	return smdr & AT91_PMC_SMDS;
}

static int at91sam9x5_clk_smd_set_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate)
{
	struct at91sam9x5_clk_smd *smd = to_at91sam9x5_clk_smd(hw);
	unsigned long div = parent_rate / rate;

	if (parent_rate % rate || div < 1 || div > (SMD_MAX_DIV + 1))
		return -EINVAL;

	regmap_write_bits(smd->regmap, AT91_PMC_SMD, AT91_PMC_SMD_DIV,
			  (div - 1) << SMD_DIV_SHIFT);

	return 0;
}

static const struct clk_ops at91sam9x5_smd_ops = {
	.recalc_rate = at91sam9x5_clk_smd_recalc_rate,
	.round_rate = at91sam9x5_clk_smd_round_rate,
	.get_parent = at91sam9x5_clk_smd_get_parent,
	.set_parent = at91sam9x5_clk_smd_set_parent,
	.set_rate = at91sam9x5_clk_smd_set_rate,
};

struct clk * __init
at91sam9x5_clk_register_smd(struct regmap *regmap, const char *name,
			    const char **parent_names, u8 num_parents)
{
	struct at91sam9x5_clk_smd *smd;
	int ret;

	smd = xzalloc(struct_size(smd, parent_names, num_parents));
	smd->hw.clk.name = name;
	smd->hw.clk.ops = &at91sam9x5_smd_ops;
	memcpy(smd->parent_names, parent_names,
	       num_parents * sizeof(smd->parent_names[0]));
	smd->hw.clk.parent_names = smd->parent_names;
	smd->hw.clk.num_parents = num_parents;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE; */
	smd->regmap = regmap;

	ret = bclk_register(&smd->hw.clk);
	if (ret) {
		kfree(smd);
		return ERR_PTR(ret);
	}

	return &smd->hw.clk;
}
