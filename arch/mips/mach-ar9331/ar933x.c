
#include <common.h>
#include <init.h>
#include <sizes.h>
#include <io.h>
#include <asm/memory.h>

#include <linux/clk.h>
#include <linux/clkdev.h>

#include <mach/ath79.h>
#include <mach/ar71xx_regs.h>

static void __init ath79_add_sys_clkdev(const char *id, unsigned long rate)
{
	struct clk *clk;
	int err;

	clk = clk_fixed(id, rate);
	if (!clk)
		panic("failed to allocate %s clock structure", id);

	err = clk_register_clkdev(clk, id, NULL);
	if (err)
		panic("unable to register %s clock device", id);
}

static int __init ar933x_clocks_init(void)
{
	unsigned long ref_rate;
	unsigned long cpu_rate;
	unsigned long ddr_rate;
	unsigned long ahb_rate;
	u32 clock_ctrl;
	u32 cpu_config;
	u32 freq;
	u32 t;

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	if (t & AR933X_BOOTSTRAP_REF_CLK_40)
		ref_rate = (40 * 1000 * 1000);
	else
		ref_rate = (25 * 1000 * 1000);

	clock_ctrl = ath79_pll_rr(AR933X_PLL_CLOCK_CTRL_REG);
	if (clock_ctrl & AR933X_PLL_CLOCK_CTRL_BYPASS) {
		cpu_rate = ref_rate;
		ahb_rate = ref_rate;
		ddr_rate = ref_rate;
	} else {
		cpu_config = ath79_pll_rr(AR933X_PLL_CPU_CONFIG_REG);

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_REFDIV_MASK;
		freq = ref_rate / t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_NINT_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_NINT_MASK;
		freq *= t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_OUTDIV_MASK;
		if (t == 0)
			t = 1;

		freq >>= t;

		t = ((clock_ctrl >> AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT) &
		     AR933X_PLL_CLOCK_CTRL_CPU_DIV_MASK) + 1;
		cpu_rate = freq / t;

		t = ((clock_ctrl >> AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT) &
		      AR933X_PLL_CLOCK_CTRL_DDR_DIV_MASK) + 1;
		ddr_rate = freq / t;

		t = ((clock_ctrl >> AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT) &
		     AR933X_PLL_CLOCK_CTRL_AHB_DIV_MASK) + 1;
		ahb_rate = freq / t;
	}

	ath79_add_sys_clkdev("ref", ref_rate);
	ath79_add_sys_clkdev("cpu", cpu_rate);
	ath79_add_sys_clkdev("ddr", ddr_rate);
	ath79_add_sys_clkdev("ahb", ahb_rate);

	clk_add_alias("wdt", NULL, "ahb", NULL);
	clk_add_alias("uart", NULL, "ref", NULL);

	return 0;
}
postcore_initcall(ar933x_clocks_init);
