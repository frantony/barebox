#include <common.h>
#include <init.h>
#include <mach/jz4750d_regs.h>
#include <mach/pinctrl.h>

static int jz4755_gpio_init(void)
{
	int i;

	jz_pinctrl_init((void *)JZ4755_GPIO_BASE_ADDR);

	for (i = 0; i < 6; i++) {
		add_generic_device("jz-gpio", i, NULL,
				JZ4755_GPIO_BASE_ADDR + 0x100 * i,
				0x100, IORESOURCE_MEM, NULL);
	}

	return 0;
}
coredevice_initcall(jz4755_gpio_init);
