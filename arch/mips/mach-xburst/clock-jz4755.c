#include <common.h>
#include <init.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <io.h>
#include <mach/jz4750d_regs.h>

#define JZ4755_RTC_FREQ	32768
#define JZ4755_EXT_FREQ	24000000

#define JZ_REG_CLOCK_CTRL	0x00
#define JZ_CLOCK_CTRL_ECS	BIT(31)
#define JZ_CLOCK_CTRL_PCS	BIT(21)

#define JZ_REG_CLOCK_PLL	0x10
#define JZ_CLOCK_PLL_BYPASS	BIT(9)

#define JZ_REG_CLOCK_MMC	0x68
#define JZ_CLOCK_MMC_DIV_MASK	0x001f

enum jz4755_clks {
	dummy, rtc, ext, ext_half, uart, pll, pll_half, msc, tcu,
	clk_max
};

struct divided_clk {
	struct clk clk;
	u32 reg;
	u32 mask;
	const char *parent;
};

static struct {
	const char	*name;
	struct clk	*clk;
} clks[clk_max] = {
	{ "dummy", },
	{ "rtc", },
	{ "ext", },
	{ "ext_half", },
	{ "uart", },
	{ "pll", },
	{ "pll_half", },
	{ "msc", },
	{ "tcu", },
};

static u32 jz_clk_reg_read(int reg)
{
	#define jz_clock_base JZ4755_CPM_BASE_ADDR
	return readl((void *)jz_clock_base + reg);
}

static inline void jz4755_clk_fixed_factor(enum jz4755_clks id,
		enum jz4755_clks parent, unsigned int mult, unsigned int div)
{
	clks[id].clk =
		clk_fixed_factor(clks[id].name, clks[parent].name, mult, div);
}

static unsigned long jz_clk_divided_recalc_rate(struct clk *clk, unsigned long parent_rate)
{
	struct divided_clk *dclk = container_of(clk, struct divided_clk, clk);
	int div;

// FIXME
	if (!strncmp(clk->parent_names[0], clks[ext].name, 16))
		return parent_rate;

	div = (jz_clk_reg_read(dclk->reg) & dclk->mask) + 1;

	return parent_rate / div;
}

static struct clk_ops jz_clk_divided_ops = {
	.recalc_rate = jz_clk_divided_recalc_rate,
};

static struct divided_clk jz4755_clock_divided_clks[] = {
	[0] = {
		.reg = JZ_REG_CLOCK_MMC,
		.mask = JZ_CLOCK_MMC_DIV_MASK,
	}
};

static void jz_clk_divided(enum jz4755_clks id, int divided_clk_id, const char *parent_name)
{
	struct clk *clk = &jz4755_clock_divided_clks[divided_clk_id].clk;

	memset(clk, 0, sizeof(*clk));

	clks[id].clk = clk;
	clk->name = clks[id].name;
	clk->parent_names = &jz4755_clock_divided_clks[divided_clk_id].parent;
	clk->parent_names[0] = parent_name;
	clk->num_parents = 1;
	clk->ops = &jz_clk_divided_ops;

	clk_register(clk);
}

static const int pllno[] = {1, 2, 2, 4};

static unsigned long jz_clk_pll_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	uint32_t val;
	int m;
	int n;
	int od;

	val = jz_clk_reg_read(JZ_REG_CLOCK_PLL);

	if (val & JZ_CLOCK_PLL_BYPASS)
		return parent_rate;

	m = ((val >> 23) & 0x1ff) + 2;
	n = ((val >> 18) & 0x1f) + 2;
	od = (val >> 16) & 0x3;

	return ((parent_rate / n) * m) / pllno[od];
}

static struct clk_ops jz_clk_pll_ops = {
	.recalc_rate = jz_clk_pll_recalc_rate,
};

static __init void jz4755_clk_register(enum jz4755_clks id)
{
	clk_register_clkdev(clks[id].clk, clks[id].name, NULL);
}

static void jz_clk_pll(void)
{
	int ret;
	struct clk *clk;

	clk = xzalloc(sizeof(struct clk));
	clk->name = "pll";
	clk->parent_names[0] = (const char*)clks[ext].name;
	clk->num_parents = 1;
	clk->ops = &jz_clk_pll_ops;

	ret = clk_register(clk);
	if (ret) {
		free(clk);
		//return ERR_PTR(ret);
	}
	clks[pll].clk = clk;
}

static __init int clk_init(void)
{
	u32 val;

	clks[dummy].clk = clk_fixed(clks[dummy].name, 0);
	clks[rtc].clk = clk_fixed(clks[rtc].name, JZ4755_RTC_FREQ);
	clks[ext].clk = clk_fixed(clks[ext].name, JZ4755_EXT_FREQ);

	jz4755_clk_fixed_factor(ext_half, ext, 1, 2);

	val = jz_clk_reg_read(JZ_REG_CLOCK_CTRL);
	if (val & JZ_CLOCK_CTRL_ECS) {
		jz4755_clk_fixed_factor(uart, ext_half, 1, 1);
	} else {
		jz4755_clk_fixed_factor(uart, ext, 1, 1);
	}

	jz_clk_pll();
	jz4755_clk_fixed_factor(pll_half, pll, 1, 2);

	if (val & JZ_CLOCK_CTRL_PCS) {
		jz_clk_divided(msc, 0, "pll");
	} else {
		jz_clk_divided(msc, 0, "pll_half");
	}

	jz4755_clk_fixed_factor(tcu, ext, 1, 1);

	jz4755_clk_register(dummy);
	jz4755_clk_register(rtc);
	jz4755_clk_register(ext);
	jz4755_clk_register(ext_half);
	jz4755_clk_register(uart);
	jz4755_clk_register(pll);
	jz4755_clk_register(pll_half);
	jz4755_clk_register(msc);
	jz4755_clk_register(tcu);

	return 0;
}
postcore_initcall(clk_init);
