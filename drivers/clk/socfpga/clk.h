/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2013, Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * based on drivers/clk/tegra/clk.h
 */

#ifndef __SOCFPGA_CLK_H
#define __SOCFPGA_CLK_H

#include <linux/clk.h>

/* Clock Manager offsets */
#define CLKMGR_CTRL		0x0
#define CLKMGR_BYPASS		0x4
#define CLKMGR_DBCTRL		0x10
#define CLKMGR_L4SRC		0x70
#define CLKMGR_PERPLL_SRC	0xAC

#define SOCFPGA_MAX_PARENTS		5

#define streq(a, b) (strcmp((a), (b)) == 0)

extern void __iomem *clk_mgr_base_addr;

void __init socfpga_pll_init(struct device_node *node);
void __init socfpga_periph_init(struct device_node *node);
void __init socfpga_gate_init(struct device_node *node);

#ifdef CONFIG_ARCH_SOCFPGA_ARRIA10
struct clk *socfpga_a10_pll_init(struct device_node *node);
struct clk *socfpga_a10_periph_init(struct device_node *node);
struct clk *socfpga_a10_gate_init(struct device_node *node);
#else
static inline struct clk *socfpga_a10_pll_init(struct device_node *node)
{
	return ERR_PTR(-ENOSYS);
}
static inline struct clk *socfpga_a10_periph_init(struct device_node *node)
{
	return ERR_PTR(-ENOSYS);
}
static inline struct clk *socfpga_a10_gate_init(struct device_node *node)
{
	return ERR_PTR(-ENOSYS);
}
#endif

struct socfpga_pll {
	struct clk_hw hw;
	void __iomem *reg;
	u32 bit_idx;
	const char *parent_names[SOCFPGA_MAX_PARENTS];
};

struct socfpga_gate_clk {
	struct clk_hw hw;
	char *parent_name;
	u32 fixed_div;
	void __iomem *div_reg;
	struct regmap *sys_mgr_base_addr;
	u32 width;	/* only valid if div_reg != 0 */
	u32 shift;	/* only valid if div_reg != 0 */
	u32 bit_idx;
	void __iomem *reg;
	u32 clk_phase[2];
	const char *parent_names[SOCFPGA_MAX_PARENTS];
};

struct socfpga_periph_clk {
	struct clk_hw hw;
	void __iomem *reg;
	char *parent_name;
	u32 fixed_div;
	void __iomem *div_reg;
	u32 width;      /* only valid if div_reg != 0 */
	u32 shift;      /* only valid if div_reg != 0 */
	const char *parent_names[SOCFPGA_MAX_PARENTS];
};

#endif /* SOCFPGA_CLK_H */
