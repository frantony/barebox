
#include <common.h>
#include <init.h>
#include <sizes.h>
#include <io.h>
#include <asm/memory.h>

#include <linux/clk.h>
#include <linux/clkdev.h>

#include <mach/ath79.h>
#include <mach/ar71xx_regs.h>

struct clk_ar933x {
	struct clk clk;
	u32 div_shift;
	u32 div_mask;
	const char *parent;
};

static unsigned long clk_ar933x_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	struct clk_ar933x *f = container_of(clk, struct clk_ar933x, clk);
	unsigned long rate;
	unsigned long freq;
	u32 clock_ctrl;
	u32 cpu_config;
	u32 t;

	clock_ctrl = ath79_pll_rr(AR933X_PLL_CLOCK_CTRL_REG);

	if (clock_ctrl & AR933X_PLL_CLOCK_CTRL_BYPASS) {
		rate = parent_rate;
	} else {
		cpu_config = ath79_pll_rr(AR933X_PLL_CPU_CONFIG_REG);

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_REFDIV_MASK;
		freq = parent_rate / t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_NINT_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_NINT_MASK;
		freq *= t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_OUTDIV_MASK;
		if (t == 0)
			t = 1;

		freq >>= t;

		t = ((clock_ctrl >> f->div_shift) & f->div_mask) + 1;
		rate = freq / t;
	}

	return rate;
}

struct clk_ops clk_ar933x_ops = {
	.recalc_rate = clk_ar933x_recalc_rate,
};

static void clk_ar933x(const char *name, const char *parent,
	u32 div_shift, u32 div_mask)
{
	struct clk_ar933x *f = xzalloc(sizeof(*f));

	f->parent = parent;
	f->div_shift = div_shift;
	f->div_mask = div_mask;

	f->clk.ops = &clk_ar933x_ops;
	f->clk.name = name;
	f->clk.parent_names = &f->parent;
	f->clk.num_parents = 1;

	clk_register(&f->clk);
}

static void __init ath79_add_ref_clkdev(const char *id)
{
	u32 t;
	unsigned long ref_rate;
	struct clk *clk;
	int err;

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	if (t & AR933X_BOOTSTRAP_REF_CLK_40)
		ref_rate = (40 * 1000 * 1000);
	else
		ref_rate = (25 * 1000 * 1000);

	clk = clk_fixed(id, ref_rate);
	if (!clk)
		panic("failed to allocate '%s' clock structure", id);

	err = clk_register_clkdev(clk, id, NULL);
	if (err)
		panic("unable to register '%s' clock device", id);
}

static int __init ar933x_clocks_init(void)
{
	extern unsigned int mips_hpt_frequency;

	ath79_add_ref_clkdev("ref");
	clk_add_alias("uart", NULL, "ref", NULL);

	clk_ar933x("cpu", "ref",
		AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_CPU_DIV_MASK);

	mips_hpt_frequency = clk_get_rate(clk_lookup("cpu")) / 2;

	clk_ar933x("ddr", "ref",
		AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_DDR_DIV_MASK);

	clk_ar933x("ahb", "ref",
		AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_AHB_DIV_MASK);
	clk_add_alias("wdt", NULL, "ahb", NULL);

	return 0;
}
pure_initcall(ar933x_clocks_init);

static int ar933x_console_init(void)
{
	/* FIXME: make it more readable */
	/* FIXME: use AR71XX_GPIO_BASE */
	__raw_writel(0x48002, (void *)0xb8040028);

	add_generic_device("ar933x_serial", DEVICE_ID_DYNAMIC, NULL,
				KSEG1ADDR(AR71XX_UART_BASE), AR71XX_UART_SIZE,
				IORESOURCE_MEM | IORESOURCE_MEM_32BIT, NULL);
	return 0;
}
console_initcall(ar933x_console_init);
