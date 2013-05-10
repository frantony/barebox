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

#define JZ_GPIO_UART1_TXD		JZ_GPIO_PORTE(25)
#define JZ_GPIO_UART1_RXD		JZ_GPIO_PORTE(23)

/* PFUN=1 PTRG=0 PSEL=1 */
#define JZ_GPIO_FUNC_UART1_TXD		JZ_GPIO_FUNC2
#define JZ_GPIO_FUNC_UART1_RXD		JZ_GPIO_FUNC2

	jz_gpio_enable_pullup(JZ_GPIO_UART1_TXD);
	jz_gpio_set_function(JZ_GPIO_UART1_TXD, JZ_GPIO_FUNC_UART1_TXD);

	return 0;
}
coredevice_initcall(jz4755_gpio_init);
