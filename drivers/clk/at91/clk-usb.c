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

#define USB_SOURCE_MAX		2

#define SAM9X5_USB_DIV_SHIFT	8
#define SAM9X5_USB_MAX_DIV	0xf

#define RM9200_USB_DIV_SHIFT	28
#define RM9200_USB_DIV_TAB_SIZE	4

#define SAM9X5_USBS_MASK	GENMASK(0, 0)
#define SAM9X60_USBS_MASK	GENMASK(1, 0)

struct at91sam9x5_clk_usb {
	struct clk_hw hw;
	struct regmap *regmap;
	u32 usbs_mask;
	u8 num_parents;
	const char *parent_names[];
};

#define to_at91sam9x5_clk_usb(_hw) \
	container_of(_hw, struct at91sam9x5_clk_usb, hw)

struct at91rm9200_clk_usb {
	struct clk_hw hw;
	struct regmap *regmap;
	u32 divisors[4];
	const char *parent_name;
};

#define to_at91rm9200_clk_usb(_hw) \
	container_of(_hw, struct at91rm9200_clk_usb, hw)

static unsigned long at91sam9x5_clk_usb_recalc_rate(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);
	unsigned int usbr;
	u8 usbdiv;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);
	usbdiv = (usbr & AT91_PMC_OHCIUSBDIV) >> SAM9X5_USB_DIV_SHIFT;

	return DIV_ROUND_CLOSEST(parent_rate, (usbdiv + 1));
}

static int at91sam9x5_clk_usb_set_parent(struct clk_hw *hw, u8 index)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);

	if (index >= usb->num_parents)
		return -EINVAL;

	regmap_write_bits(usb->regmap, AT91_PMC_USB, usb->usbs_mask, index);

	return 0;
}

static int at91sam9x5_clk_usb_get_parent(struct clk_hw *hw)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);
	unsigned int usbr;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);

	return usbr & usb->usbs_mask;
}

static int at91sam9x5_clk_usb_set_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);
	unsigned long div;

	if (!rate)
		return -EINVAL;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);
	if (div > SAM9X5_USB_MAX_DIV + 1 || !div)
		return -EINVAL;

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_OHCIUSBDIV,
			  (div - 1) << SAM9X5_USB_DIV_SHIFT);

	return 0;
}

static const struct clk_ops at91sam9x5_usb_ops = {
	.recalc_rate = at91sam9x5_clk_usb_recalc_rate,
	.get_parent = at91sam9x5_clk_usb_get_parent,
	.set_parent = at91sam9x5_clk_usb_set_parent,
	.set_rate = at91sam9x5_clk_usb_set_rate,
};

static int at91sam9n12_clk_usb_enable(struct clk_hw *hw)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_USBS,
			  AT91_PMC_USBS);

	return 0;
}

static void at91sam9n12_clk_usb_disable(struct clk_hw *hw)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_USBS, 0);
}

static int at91sam9n12_clk_usb_is_enabled(struct clk_hw *hw)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(hw);
	unsigned int usbr;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);

	return usbr & AT91_PMC_USBS;
}

static const struct clk_ops at91sam9n12_usb_ops = {
	.enable = at91sam9n12_clk_usb_enable,
	.disable = at91sam9n12_clk_usb_disable,
	.is_enabled = at91sam9n12_clk_usb_is_enabled,
	.recalc_rate = at91sam9x5_clk_usb_recalc_rate,
	.set_rate = at91sam9x5_clk_usb_set_rate,
};

static struct clk * __init
_at91sam9x5_clk_register_usb(struct regmap *regmap, const char *name,
			     const char **parent_names, u8 num_parents,
			     u32 usbs_mask)
{
	struct at91sam9x5_clk_usb *usb;
	int ret;

	usb = kzalloc(struct_size(usb, parent_names, num_parents), GFP_KERNEL);
	usb->hw.clk.name = name;
	usb->hw.clk.ops = &at91sam9x5_usb_ops;
	memcpy(usb->parent_names, parent_names,
	       num_parents * sizeof(usb->parent_names[0]));
	usb->hw.clk.parent_names = usb->parent_names;
	usb->hw.clk.num_parents = num_parents;
	usb->hw.clk.flags = CLK_SET_RATE_PARENT;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE | */
	/* 	     CLK_SET_RATE_PARENT; */
	usb->regmap = regmap;
	usb->usbs_mask = usbs_mask;
	usb->num_parents = num_parents;

	ret = bclk_register(&usb->hw.clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->hw.clk;
}

struct clk * __init
at91sam9x5_clk_register_usb(struct regmap *regmap, const char *name,
			    const char **parent_names, u8 num_parents)
{
	return _at91sam9x5_clk_register_usb(regmap, name, parent_names,
					    num_parents, SAM9X5_USBS_MASK);
}

struct clk * __init
sam9x60_clk_register_usb(struct regmap *regmap, const char *name,
			 const char **parent_names, u8 num_parents)
{
	return _at91sam9x5_clk_register_usb(regmap, name, parent_names,
					    num_parents, SAM9X60_USBS_MASK);
}

struct clk * __init
at91sam9n12_clk_register_usb(struct regmap *regmap, const char *name,
			     const char *parent_name)
{
	struct at91sam9x5_clk_usb *usb;
	int ret;

	usb = xzalloc(sizeof(*usb));
	usb->hw.clk.name = name;
	usb->hw.clk.ops = &at91sam9n12_usb_ops;
	usb->parent_names[0] = parent_name;
	usb->hw.clk.parent_names = &usb->parent_names[0];
	usb->hw.clk.num_parents = 1;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT; */
	usb->regmap = regmap;

	ret = bclk_register(&usb->hw.clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->hw.clk;
}

static unsigned long at91rm9200_clk_usb_recalc_rate(struct clk_hw *hw,
						    unsigned long parent_rate)
{
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(hw);
	unsigned int pllbr;
	u8 usbdiv;

	regmap_read(usb->regmap, AT91_CKGR_PLLBR, &pllbr);

	usbdiv = (pllbr & AT91_PMC_USBDIV) >> RM9200_USB_DIV_SHIFT;
	if (usb->divisors[usbdiv])
		return parent_rate / usb->divisors[usbdiv];

	return 0;
}

static long at91rm9200_clk_usb_round_rate(struct clk_hw *hw, unsigned long rate,
					  unsigned long *parent_rate)
{
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(hw);
	struct clk *parent = clk_get_parent(clk_hw_to_clk(hw));
	unsigned long bestrate = 0;
	int bestdiff = -1;
	unsigned long tmprate;
	int tmpdiff;
	int i = 0;

	for (i = 0; i < RM9200_USB_DIV_TAB_SIZE; i++) {
		unsigned long tmp_parent_rate;

		if (!usb->divisors[i])
			continue;

		tmp_parent_rate = rate * usb->divisors[i];
		tmp_parent_rate = clk_round_rate(parent, tmp_parent_rate);
		if (!tmp_parent_rate)
			continue;

		tmprate = DIV_ROUND_CLOSEST(tmp_parent_rate, usb->divisors[i]);
		if (tmprate < rate)
			tmpdiff = rate - tmprate;
		else
			tmpdiff = tmprate - rate;

		if (bestdiff < 0 || bestdiff > tmpdiff) {
			bestrate = tmprate;
			bestdiff = tmpdiff;
			*parent_rate = tmp_parent_rate;
		}

		if (!bestdiff)
			break;
	}

	return bestrate;
}

static int at91rm9200_clk_usb_set_rate(struct clk_hw *hw, unsigned long rate,
				       unsigned long parent_rate)
{
	int i;
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(hw);
	unsigned long div;

	if (!rate)
		return -EINVAL;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);

	for (i = 0; i < RM9200_USB_DIV_TAB_SIZE; i++) {
		if (usb->divisors[i] == div) {
			regmap_write_bits(usb->regmap, AT91_CKGR_PLLBR,
					  AT91_PMC_USBDIV,
					  i << RM9200_USB_DIV_SHIFT);

			return 0;
		}
	}

	return -EINVAL;
}

static const struct clk_ops at91rm9200_usb_ops = {
	.recalc_rate = at91rm9200_clk_usb_recalc_rate,
	.round_rate = at91rm9200_clk_usb_round_rate,
	.set_rate = at91rm9200_clk_usb_set_rate,
};

struct clk * __init
at91rm9200_clk_register_usb(struct regmap *regmap, const char *name,
			    const char *parent_name, const u32 *divisors)
{
	struct at91rm9200_clk_usb *usb;
	int ret;

	usb = xzalloc(sizeof(*usb));
	usb->hw.clk.name = name;
	usb->hw.clk.ops = &at91rm9200_usb_ops;
	usb->parent_name = parent_name;
	usb->hw.clk.parent_names = &usb->parent_name;
	usb->hw.clk.num_parents = 1;
	/* init.flags = CLK_SET_RATE_PARENT; */

	usb->regmap = regmap;
	memcpy(usb->divisors, divisors, sizeof(usb->divisors));

	ret = bclk_register(&usb->hw.clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->hw.clk;
}
