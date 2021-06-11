#include <debug_ll.h>
#include <common.h>
#include <linux/sizes.h>
#include <mach/generic.h>
#include <mach/iomux-mx6.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>

static inline void setup_uart(void)
{
	void __iomem *iomuxbase = IOMEM(MX6_IOMUXC_BASE_ADDR);

	imx6_ungate_all_peripherals();

	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT10__UART1_TXD);
	imx_setup_pad(iomuxbase, MX6Q_PAD_CSI0_DAT11__UART1_RXD);

	imx6_uart_setup_ll();

	putc_ll('>');
}

extern char __dtb_imx6q_sabresd_start[];

ENTRY_FUNCTION(start_imx6q_sabresd, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6q_sabresd_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}

extern char __dtb_imx6qp_sabresd_start[];

ENTRY_FUNCTION(start_imx6qp_sabresd, r0, r1, r2)
{
	void *fdt;

	imx6_cpu_lowlevel_init();

	if (IS_ENABLED(CONFIG_DEBUG_LL))
		setup_uart();

	fdt = __dtb_imx6qp_sabresd_start + get_runtime_offset();

	barebox_arm_entry(0x10000000, SZ_1G, fdt);
}
