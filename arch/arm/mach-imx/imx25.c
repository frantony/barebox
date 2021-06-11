// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <mach/imx25-regs.h>
#include <mach/iim.h>
#include <io.h>
#include <mach/weim.h>
#include <mach/generic.h>
#include <linux/sizes.h>

#define MX25_BOOTROM_HAB_MAGIC		0x3c95cac6
#define MX25_DRYICE_GPR			0x3c

/* IIM fuse definitions */
#define IIM_BANK0_BASE	(MX25_IIM_BASE_ADDR + 0x800)
#define IIM_BANK1_BASE	(MX25_IIM_BASE_ADDR + 0xc00)
#define IIM_BANK2_BASE	(MX25_IIM_BASE_ADDR + 0x1000)

#define IIM_UID		(IIM_BANK0_BASE + 0x20)
#define IIM_MAC_ADDR	(IIM_BANK0_BASE + 0x68)

u64 imx_uid(void)
{
	u64 uid = 0;
	int i;

	/*
	 * This code assumes that the UID is stored little-endian. The
	 * Freescale AN3682 document is silent about the endianess, but
	 * experimentation shows that this is the case. All other multi-byte
	 * values in IIM are big-endian as per AN3682.
	 */
	for (i = 0; i < 8; i++)
		uid |= (u64)readb(IIM_UID + i*4) << (i*8);

	return uid;
}

int imx25_init(void)
{
	int val;

	imx25_boot_save_loc();
	add_generic_device("imx25-esdctl", 0, NULL, MX25_ESDCTL_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);

	/*
	 * When the i.MX25 is used with internal boot, the boot ROM always
	 * performs some HAB actions. These will copy the value from DryIce
	 * GPR (0x53ffc03c) to a location in SRAM (0x78001734) and then overwrites
	 * the GPR with 0x3c95cac6.
	 * After the HAB routine is done, the boot ROM should copy the previously
	 * saved value from SRAM back to the GPR. The last step is not done.
	 * The boot ROM is officially broken in this regard.
	 * This renders the Non-volatile memory to a Non-Non-volatile memory.
	 * To still be able to use the GPR for its intended purpose, copy the
	 * saved SRAM value back manually.
	 */

	/*
	 * When the value in the GPR matches the value the ROM has written there then
	 * we have to copy back the value from the SRAM back to the GPR.
	 */
	val = readl(MX25_DRYICE_BASE_ADDR + MX25_DRYICE_GPR);
	if (val == MX25_BOOTROM_HAB_MAGIC) {
		val = readl(MX25_IRAM_BASE_ADDR + 0x1734);
		writel(val, MX25_DRYICE_BASE_ADDR + MX25_DRYICE_GPR);
	}

	return 0;
}

int imx25_devices_init(void)
{
	add_generic_device("imx_iim", 0, NULL, MX25_IIM_BASE_ADDR, SZ_4K,
			IORESOURCE_MEM, NULL);

	add_generic_device("imx-iomuxv3", 0, NULL, MX25_IOMUXC_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx25-ccm", 0, NULL, MX25_CCM_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpt", 0, NULL, MX25_GPT1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 0, NULL, MX25_GPIO1_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 1, NULL, MX25_GPIO2_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 2, NULL, MX25_GPIO3_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx31-gpio", 3, NULL, MX25_GPIO4_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx21-wdt", 0, NULL, MX25_WDOG_BASE_ADDR, 0x1000, IORESOURCE_MEM, NULL);
	add_generic_device("imx25-usb-misc", 0, NULL, MX25_USB_OTG_BASE_ADDR + 0x600, 0x100, IORESOURCE_MEM, NULL);

	return 0;
}
