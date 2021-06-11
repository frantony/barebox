// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <malloc.h>
#include <linux/math64.h>

#include "clk.h"

#define MFN_BITS	(10)
#define MFN_SIGN	(BIT(MFN_BITS - 1))
#define MFN_MASK	(MFN_SIGN - 1)

struct clk_pllv1 {
	struct clk_hw hw;
	void __iomem *reg;
	const char *parent;
};

static inline bool mfn_is_negative(unsigned int mfn)
{
    return mfn & MFN_SIGN;
}

static unsigned long clk_pllv1_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct clk_pllv1 *pll = container_of(hw, struct clk_pllv1, hw);
	unsigned long long ll;
	int mfn_abs;
	unsigned int mfi, mfn, mfd, pd;
	u32 reg_val = readl(pll->reg);
	unsigned long freq = parent_rate;

	mfi = (reg_val >> 10) & 0xf;
	mfn = reg_val & 0x3ff;
	mfd = (reg_val >> 16) & 0x3ff;
	pd =  (reg_val >> 26) & 0xf;

	mfi = mfi <= 5 ? 5 : mfi;

	mfn_abs = mfn;

#if !defined CONFIG_ARCH_MX1 && !defined CONFIG_ARCH_MX21
	if (mfn >= 0x200) {
		mfn |= 0xFFFFFE00;
		mfn_abs = -mfn;
	}
#endif

	freq *= 2;
	freq /= pd + 1;

	ll = (unsigned long long)freq * mfn_abs;

	do_div(ll, mfd + 1);
	if (mfn_is_negative(mfn))
		ll = (freq * mfi) - ll;
	else
		ll = (freq * mfi) + ll;

	return ll;
}

struct clk_ops clk_pllv1_ops = {
	.recalc_rate = clk_pllv1_recalc_rate,
};

struct clk *imx_clk_pllv1(const char *name, const char *parent,
		void __iomem *base)
{
	struct clk_pllv1 *pll = xzalloc(sizeof(*pll));
	int ret;

	pll->parent = parent;
	pll->reg = base;
	pll->hw.clk.ops = &clk_pllv1_ops;
	pll->hw.clk.name = name;
	pll->hw.clk.parent_names = &pll->parent;
	pll->hw.clk.num_parents = 1;

	ret = bclk_register(&pll->hw.clk);
	if (ret) {
		free(pll);
		return ERR_PTR(ret);
	}

	return &pll->hw.clk;
}
