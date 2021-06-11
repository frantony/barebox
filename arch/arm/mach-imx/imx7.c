// SPDX-License-Identifier: GPL-2.0-or-later

#include <init.h>
#include <common.h>
#include <io.h>
#include <linux/sizes.h>
#include <asm/psci.h>
#include <mach/imx7.h>
#include <mach/generic.h>
#include <mach/revision.h>
#include <mach/reset-reason.h>
#include <mach/imx7-regs.h>

void imx7_init_lowlevel(void)
{
	void __iomem *aips1 = IOMEM(MX7_AIPS1_CONFIG_BASE_ADDR);
	void __iomem *aips2 = IOMEM(MX7_AIPS2_CONFIG_BASE_ADDR);
	void __iomem *aips3 = IOMEM(MX7_AIPS3_CONFIG_BASE_ADDR);

	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, aips1);
	writel(0x77777777, aips1 + 0x4);
	writel(0, aips1 + 0x40);
	writel(0, aips1 + 0x44);
	writel(0, aips1 + 0x48);
	writel(0, aips1 + 0x4c);
	writel(0, aips1 + 0x50);

	writel(0x77777777, aips2);
	writel(0x77777777, aips2 + 0x4);
	writel(0, aips2 + 0x40);
	writel(0, aips2 + 0x44);
	writel(0, aips2 + 0x48);
	writel(0, aips2 + 0x4c);
	writel(0, aips2 + 0x50);

	writel(0x77777777, aips3);
	writel(0x77777777, aips3 + 0x4);
	writel(0, aips3 + 0x40);
	writel(0, aips3 + 0x44);
	writel(0, aips3 + 0x48);
	writel(0, aips3 + 0x4c);
	writel(0, aips3 + 0x50);
}

#define CSU_NUM_REGS               64
#define CSU_INIT_SEC_LEVEL0        0x00FF00FF

static void imx7_init_csu(void)
{
	void __iomem *csu = IOMEM(MX7_CSU_BASE_ADDR);
	int i = 0;

	for (i = 0; i < CSU_NUM_REGS; i++)
		writel(CSU_INIT_SEC_LEVEL0, csu + i * 4);
}

#define GPC_CPU_PGC_SW_PDN_REQ	0xfc
#define GPC_CPU_PGC_SW_PUP_REQ	0xf0
#define GPC_PGC_C1		0x840
#define GPC_PGC(n)		(0x800 + (n) * 0x40)

#define BM_CPU_PGC_SW_PDN_PUP_REQ_CORE1_A7	0x2

#define PGC_CTRL		0x0

/* below is for i.MX7D */
#define SRC_GPR1_MX7D		0x074
#define SRC_A7RCR1		0x008

static void imx_gpcv2_set_core_power(int core, bool pdn)
{
	void __iomem *gpc = IOMEM(MX7_GPC_BASE_ADDR);
	void __iomem *pgc = gpc + GPC_PGC(core);

	u32 reg = pdn ? GPC_CPU_PGC_SW_PUP_REQ : GPC_CPU_PGC_SW_PDN_REQ;
	u32 val;

	writel(1, pgc + PGC_CTRL);

	val = readl(gpc + reg);
	val |= 1 << core;
	writel(val, gpc + reg);

	while (readl(gpc + reg) & (1 << core));

	writel(0, pgc + PGC_CTRL);
}

static int imx7_cpu_on(u32 cpu_id)
{
	void __iomem *src = IOMEM(MX7_SRC_BASE_ADDR);
	u32 val;

	writel(psci_cpu_entry, src + cpu_id * 8 + SRC_GPR1_MX7D);
	imx_gpcv2_set_core_power(cpu_id, true);

	val = readl(src + SRC_A7RCR1);
	val |= 1 << cpu_id;
	writel(val, src + SRC_A7RCR1);

	return 0;
}

static int imx7_cpu_off(void)
{
	void __iomem *src = IOMEM(MX7_SRC_BASE_ADDR);
	u32 val;
	int cpu_id = psci_get_cpu_id();

	val = readl(src + SRC_A7RCR1);
	val &= ~(1 << cpu_id);
	writel(val, src + SRC_A7RCR1);

	/*
	 * FIXME: This reads nice and symmetrically to cpu_on above,
	 * but of course this will never be reached as we have just
	 * put the CPU we are currently running on into reset.
	 */

	imx_gpcv2_set_core_power(cpu_id, false);

	while (1);

	return 0;
}

static struct psci_ops imx7_psci_ops = {
	.cpu_on = imx7_cpu_on,
	.cpu_off = imx7_cpu_off,
};

int imx7_init(void)
{
	const char *cputypestr;
	void __iomem *src = IOMEM(MX7_SRC_BASE_ADDR);

	imx7_init_lowlevel();

	imx7_init_csu();

	imx7_boot_save_loc();

	psci_set_ops(&imx7_psci_ops);

	switch (imx7_cpu_type()) {
	case IMX7_CPUTYPE_IMX7D:
		cputypestr = "i.MX7d";
		break;
	case IMX7_CPUTYPE_IMX7S:
		cputypestr = "i.MX7s";
		break;
	default:
		cputypestr = "unknown i.MX7";
		break;
	}

	imx_set_silicon_revision(cputypestr, imx7_cpu_revision());
	imx_set_reset_reason(src + IMX7_SRC_SRSR, imx7_reset_reasons);

	return 0;
}
