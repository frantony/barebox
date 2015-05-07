#include <init.h>
#include <mach/ath79.h>
#include <usb/ehci.h>

#define AR933X_RESET_REG_RESET_MODULE           0x1c
#define AR933X_RESET_USB_HOST           BIT(5)
#define AR933X_RESET_USB_PHY            BIT(4)
#define AR933X_RESET_USBSUS_OVERRIDE    BIT(3)

#define AR933X_EHCI_BASE	0x1b000000
#define AR933X_EHCI_SIZE	0x1000

static void ath79_device_reset_set(u32 mask)
{
	u32 reg;
	u32 t;

	reg = AR933X_RESET_REG_RESET_MODULE;
	t = ath79_reset_rr(reg);
	ath79_reset_wr(reg, t | mask);
}

static void ath79_device_reset_clear(u32 mask)
{
	u32 reg;
	u32 t;

	reg = AR933X_RESET_REG_RESET_MODULE;
	t = ath79_reset_rr(reg);
	ath79_reset_wr(reg, t & ~mask);
}

static struct ehci_platform_data ehci_pdata = {
	.flags = EHCI_HAS_TT | EHCI_BE_MMIO,
};

static int ehci_init(void)
{
	ath79_device_reset_set(AR933X_RESET_USBSUS_OVERRIDE);
	mdelay(10); mdelay(10); mdelay(10);

	ath79_device_reset_clear(AR933X_RESET_USB_HOST);
	mdelay(10); mdelay(10); mdelay(10);

	ath79_device_reset_clear(AR933X_RESET_USB_PHY);
	mdelay(10); mdelay(10); mdelay(10);

	add_generic_usb_ehci_device(DEVICE_ID_DYNAMIC,
			KSEG1ADDR(AR933X_EHCI_BASE),
			&ehci_pdata);

	return 0;
}
device_initcall(ehci_init);
