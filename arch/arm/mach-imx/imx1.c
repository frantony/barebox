// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/imx1-regs.h>
#include <mach/weim.h>
#include <mach/iomux-v1.h>
#include <mach/generic.h>
#include <reset_source.h>

#define MX1_RSR MX1_SCM_BASE_ADDR
#define RSR_EXR	(1 << 0)
#define RSR_WDR	(1 << 1)

static void imx1_detect_reset_source(void)
{
	u32 val = readl((void *)MX1_RSR) & 0x3;

	switch (val) {
	case RSR_EXR:
		reset_source_set(RESET_RST);
		return;
	case RSR_WDR:
		reset_source_set(RESET_WDG);
		return;
	case 0:
		reset_source_set(RESET_POR);
		return;
	default:
		/* else keep the default 'unknown' state */
		return;
	}
}

void imx1_setup_eimcs(size_t cs, unsigned upper, unsigned lower)
{
	writel(upper, MX1_EIM_BASE_ADDR + cs * 8);
	writel(lower, MX1_EIM_BASE_ADDR + 4 + cs * 8);
}

#include <mach/esdctl.h>

int imx1_init(void)
{
	imx1_detect_reset_source();
	add_generic_device("imx1-sdramc", 0, NULL, MX1_SDRAMC_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}

int imx1_devices_init(void)
{
	add_generic_device("imx1-ccm", 0, NULL, MX1_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpt", 0, NULL, MX1_TIM1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 0, NULL, MX1_GPIO1_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 1, NULL, MX1_GPIO2_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 2, NULL, MX1_GPIO3_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-gpio", 3, NULL, MX1_GPIO4_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);
	add_generic_device("imx1-wdt", 0, NULL, MX1_WDT_BASE_ADDR, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
