// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <linux/clk.h>

/* USART3 register offsets */
#define USART3_CR				0x0000
#define USART3_MR				0x0004
#define USART3_IER				0x0008
#define USART3_IDR				0x000c
#define USART3_IMR				0x0010
#define USART3_CSR				0x0014
#define USART3_RHR				0x0018
#define USART3_THR				0x001c
#define USART3_BRGR				0x0020
#define USART3_RTOR				0x0024
#define USART3_TTGR				0x0028
#define USART3_FIDI				0x0040
#define USART3_NER				0x0044
#define USART3_XXR				0x0048
#define USART3_IFR				0x004c
#define USART3_RPR				0x0100
#define USART3_RCR				0x0104
#define USART3_TPR				0x0108
#define USART3_TCR				0x010c
#define USART3_RNPR				0x0110
#define USART3_RNCR				0x0114
#define USART3_TNPR				0x0118
#define USART3_TNCR				0x011c
#define USART3_PTCR				0x0120
#define USART3_PTSR				0x0124

/* Bitfields in CR */
#define USART3_RSTRX_OFFSET			2
#define USART3_RSTRX_SIZE			1
#define USART3_RSTTX_OFFSET			3
#define USART3_RSTTX_SIZE			1
#define USART3_RXEN_OFFSET			4
#define USART3_RXEN_SIZE			1
#define USART3_RXDIS_OFFSET			5
#define USART3_RXDIS_SIZE			1
#define USART3_TXEN_OFFSET			6
#define USART3_TXEN_SIZE			1
#define USART3_TXDIS_OFFSET			7
#define USART3_TXDIS_SIZE			1
#define USART3_RSTSTA_OFFSET			8
#define USART3_RSTSTA_SIZE			1
#define USART3_STTBRK_OFFSET			9
#define USART3_STTBRK_SIZE			1
#define USART3_STPBRK_OFFSET			10
#define USART3_STPBRK_SIZE			1
#define USART3_STTTO_OFFSET			11
#define USART3_STTTO_SIZE			1
#define USART3_SENDA_OFFSET			12
#define USART3_SENDA_SIZE			1
#define USART3_RSTIT_OFFSET			13
#define USART3_RSTIT_SIZE			1
#define USART3_RSTNACK_OFFSET			14
#define USART3_RSTNACK_SIZE			1
#define USART3_RETTO_OFFSET			15
#define USART3_RETTO_SIZE			1
#define USART3_DTREN_OFFSET			16
#define USART3_DTREN_SIZE			1
#define USART3_DTRDIS_OFFSET			17
#define USART3_DTRDIS_SIZE			1
#define USART3_RTSEN_OFFSET			18
#define USART3_RTSEN_SIZE			1
#define USART3_RTSDIS_OFFSET			19
#define USART3_RTSDIS_SIZE			1
#define USART3_COMM_TX_OFFSET			30
#define USART3_COMM_TX_SIZE			1
#define USART3_COMM_RX_OFFSET			31
#define USART3_COMM_RX_SIZE			1

/* Bitfields in MR */
#define USART3_USART_MODE_OFFSET		0
#define USART3_USART_MODE_SIZE			4
#define USART3_USCLKS_OFFSET			4
#define USART3_USCLKS_SIZE			2
#define USART3_CHRL_OFFSET			6
#define USART3_CHRL_SIZE			2
#define USART3_SYNC_OFFSET			8
#define USART3_SYNC_SIZE			1
#define USART3_PAR_OFFSET			9
#define USART3_PAR_SIZE				3
#define USART3_NBSTOP_OFFSET			12
#define USART3_NBSTOP_SIZE			2
#define USART3_CHMODE_OFFSET			14
#define USART3_CHMODE_SIZE			2
#define USART3_MSBF_OFFSET			16
#define USART3_MSBF_SIZE			1
#define USART3_MODE9_OFFSET			17
#define USART3_MODE9_SIZE			1
#define USART3_CLKO_OFFSET			18
#define USART3_CLKO_SIZE			1
#define USART3_OVER_OFFSET			19
#define USART3_OVER_SIZE			1
#define USART3_INACK_OFFSET			20
#define USART3_INACK_SIZE			1
#define USART3_DSNACK_OFFSET			21
#define USART3_DSNACK_SIZE			1
#define USART3_MAX_ITERATION_OFFSET		24
#define USART3_MAX_ITERATION_SIZE		3
#define USART3_FILTER_OFFSET			28
#define USART3_FILTER_SIZE			1

/* Bitfields in CSR */
#define USART3_RXRDY_OFFSET			0
#define USART3_RXRDY_SIZE			1
#define USART3_TXRDY_OFFSET			1
#define USART3_TXRDY_SIZE			1
#define USART3_RXBRK_OFFSET			2
#define USART3_RXBRK_SIZE			1
#define USART3_ENDRX_OFFSET			3
#define USART3_ENDRX_SIZE			1
#define USART3_ENDTX_OFFSET			4
#define USART3_ENDTX_SIZE			1
#define USART3_OVRE_OFFSET			5
#define USART3_OVRE_SIZE			1
#define USART3_FRAME_OFFSET			6
#define USART3_FRAME_SIZE			1
#define USART3_PARE_OFFSET			7
#define USART3_PARE_SIZE			1
#define USART3_TIMEOUT_OFFSET			8
#define USART3_TIMEOUT_SIZE			1
#define USART3_TXEMPTY_OFFSET			9
#define USART3_TXEMPTY_SIZE			1
#define USART3_ITERATION_OFFSET			10
#define USART3_ITERATION_SIZE			1
#define USART3_TXBUFE_OFFSET			11
#define USART3_TXBUFE_SIZE			1
#define USART3_RXBUFF_OFFSET			12
#define USART3_RXBUFF_SIZE			1
#define USART3_NACK_OFFSET			13
#define USART3_NACK_SIZE			1
#define USART3_RIIC_OFFSET			16
#define USART3_RIIC_SIZE			1
#define USART3_DSRIC_OFFSET			17
#define USART3_DSRIC_SIZE			1
#define USART3_DCDIC_OFFSET			18
#define USART3_DCDIC_SIZE			1
#define USART3_CTSIC_OFFSET			19
#define USART3_CTSIC_SIZE			1
#define USART3_RI_OFFSET			20
#define USART3_RI_SIZE				1
#define USART3_DSR_OFFSET			21
#define USART3_DSR_SIZE				1
#define USART3_DCD_OFFSET			22
#define USART3_DCD_SIZE				1
#define USART3_CTS_OFFSET			23
#define USART3_CTS_SIZE				1

/* Bitfields in RHR */
#define USART3_RXCHR_OFFSET			0
#define USART3_RXCHR_SIZE			9

/* Bitfields in THR */
#define USART3_TXCHR_OFFSET			0
#define USART3_TXCHR_SIZE			9

/* Bitfields in BRGR */
#define USART3_CD_OFFSET			0
#define USART3_CD_SIZE				16

/* Bitfields in RTOR */
#define USART3_TO_OFFSET			0
#define USART3_TO_SIZE				16

/* Bitfields in TTGR */
#define USART3_TG_OFFSET			0
#define USART3_TG_SIZE				8

/* Bitfields in FIDI */
#define USART3_FI_DI_RATIO_OFFSET		0
#define USART3_FI_DI_RATIO_SIZE			11

/* Bitfields in NER */
#define USART3_NB_ERRORS_OFFSET			0
#define USART3_NB_ERRORS_SIZE			8

/* Bitfields in XXR */
#define USART3_XOFF_OFFSET			0
#define USART3_XOFF_SIZE			8
#define USART3_XON_OFFSET			8
#define USART3_XON_SIZE				8

/* Bitfields in IFR */
#define USART3_IRDA_FILTER_OFFSET		0
#define USART3_IRDA_FILTER_SIZE			8

/* Bitfields in RCR */
#define USART3_RXCTR_OFFSET			0
#define USART3_RXCTR_SIZE			16

/* Bitfields in TCR */
#define USART3_TXCTR_OFFSET			0
#define USART3_TXCTR_SIZE			16

/* Bitfields in RNCR */
#define USART3_RXNCR_OFFSET			0
#define USART3_RXNCR_SIZE			16

/* Bitfields in TNCR */
#define USART3_TXNCR_OFFSET			0
#define USART3_TXNCR_SIZE			16

/* Bitfields in PTCR */
#define USART3_RXTEN_OFFSET			0
#define USART3_RXTEN_SIZE			1
#define USART3_RXTDIS_OFFSET			1
#define USART3_RXTDIS_SIZE			1
#define USART3_TXTEN_OFFSET			8
#define USART3_TXTEN_SIZE			1
#define USART3_TXTDIS_OFFSET			9
#define USART3_TXTDIS_SIZE			1

/* Constants for USART_MODE */
#define USART3_USART_MODE_NORMAL		0
#define USART3_USART_MODE_RS485			1
#define USART3_USART_MODE_HARDWARE		2
#define USART3_USART_MODE_MODEM			3
#define USART3_USART_MODE_ISO7816_T0		4
#define USART3_USART_MODE_ISO7816_T1		6
#define USART3_USART_MODE_IRDA			8

/* Constants for USCLKS */
#define USART3_USCLKS_MCK			0
#define USART3_USCLKS_MCK_DIV			1
#define USART3_USCLKS_SCK			3

/* Constants for CHRL */
#define USART3_CHRL_5				0
#define USART3_CHRL_6				1
#define USART3_CHRL_7				2
#define USART3_CHRL_8				3

/* Constants for PAR */
#define USART3_PAR_EVEN				0
#define USART3_PAR_ODD				1
#define USART3_PAR_SPACE			2
#define USART3_PAR_MARK				3
#define USART3_PAR_NONE				4
#define USART3_PAR_MULTI			6

/* Constants for NBSTOP */
#define USART3_NBSTOP_1				0
#define USART3_NBSTOP_1_5			1
#define USART3_NBSTOP_2				2

/* Constants for CHMODE */
#define USART3_CHMODE_NORMAL			0
#define USART3_CHMODE_ECHO			1
#define USART3_CHMODE_LOCAL_LOOP		2
#define USART3_CHMODE_REMOTE_LOOP		3

/* Constants for MSBF */
#define USART3_MSBF_LSBF			0
#define USART3_MSBF_MSBF			1

/* Constants for OVER */
#define USART3_OVER_X16				0
#define USART3_OVER_X8				1

/* Constants for CD */
#define USART3_CD_DISABLE			0
#define USART3_CD_BYPASS			1

/* Constants for TO */
#define USART3_TO_DISABLE			0

/* Constants for TG */
#define USART3_TG_DISABLE			0

/* Constants for FI_DI_RATIO */
#define USART3_FI_DI_RATIO_DISABLE		0

/* Bit manipulation macros */
#define USART3_BIT(name)				\
	(1 << USART3_##name##_OFFSET)
#define USART3_BF(name,value)				\
	(((value) & ((1 << USART3_##name##_SIZE) - 1))	\
	 << USART3_##name##_OFFSET)
#define USART3_BFEXT(name,value)			\
	(((value) >> USART3_##name##_OFFSET)		\
	 & ((1 << USART3_##name##_SIZE) - 1))
#define USART3_BFINS(name,value,old)			\
	(((old) & ~(((1 << USART3_##name##_SIZE) - 1)	\
		    << USART3_##name##_OFFSET))		\
	 | USART3_BF(name,value))

/*
 * We wrap our port structure around the generic console_device.
 */
struct atmel_uart_port {
	void __iomem		*base;
	struct console_device	uart;		/* uart */
	struct clk		*clk;		/* uart clock */
	u32			uartclk;
};

static inline struct atmel_uart_port *
to_atmel_uart_port(struct console_device *uart)
{
	return container_of(uart, struct atmel_uart_port, uart);
}

static void atmel_serial_putc(struct console_device *cdev, char c)
{
	struct atmel_uart_port *uart = to_atmel_uart_port(cdev);

	while (!(readl(uart->base + USART3_CSR) & USART3_BIT(TXRDY)));

	writel(c, uart->base + USART3_THR);
}

static int atmel_serial_tstc(struct console_device *cdev)
{
	struct atmel_uart_port *uart = to_atmel_uart_port(cdev);

	return (readl(uart->base + USART3_CSR) & USART3_BIT(RXRDY)) != 0;
}

static int atmel_serial_getc(struct console_device *cdev)
{
	struct atmel_uart_port *uart = to_atmel_uart_port(cdev);

	while (!(readl(uart->base + USART3_CSR) & USART3_BIT(RXRDY))) ;
	return readl(uart->base + USART3_RHR);
}

static int atmel_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct atmel_uart_port *uart = to_atmel_uart_port(cdev);
	unsigned long divisor;

	/*
	 *              Master Clock
	 * Baud Rate = --------------
	 *                16 * CD
	 */
	divisor = (uart->uartclk / 16 + baudrate / 2) / baudrate;
	writel(USART3_BF(CD, divisor), uart->base + USART3_BRGR);

	return 0;
}

static int atmel_serial_set_mode(struct console_device *cdev, enum console_mode mode)
{
	struct atmel_uart_port *uart = to_atmel_uart_port(cdev);
	u32 mr;
	u8 m;

	mr = readl(uart->base + USART3_MR);
	mr &= ~0xf;

	switch (mode) {
	case CONSOLE_MODE_NORMAL:
		m = USART3_USART_MODE_NORMAL;
		break;
	case CONSOLE_MODE_RS485:
		m = USART3_USART_MODE_RS485;
		break;
	default:
		return -EINVAL;
	}

	mr |= USART3_BF(USART_MODE, m);
	writel(mr, uart->base + USART3_MR);

	return 0;
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
static int atmel_serial_init_port(struct device *dev,
				  struct atmel_uart_port *uart)
{
	uart->base = dev_request_mem_region_err_null(dev, 0);
	if (!uart->base)
		return -ENOENT;

	uart->clk = clk_get_for_console(dev, "usart");
	if (IS_ERR(uart->clk)) {
		dev_err(dev, "Failed to get 'usart' clock\n");
		return PTR_ERR(uart->clk);
	}

	clk_enable(uart->clk);
	uart->uartclk = clk_get_rate(uart->clk);

	writel(USART3_BIT(RSTRX) | USART3_BIT(RSTTX), uart->base + USART3_CR);

	writel(USART3_BIT(RXEN) | USART3_BIT(TXEN), uart->base + USART3_CR);
	writel((USART3_BF(USART_MODE, USART3_USART_MODE_NORMAL)
			   | USART3_BF(USCLKS, USART3_USCLKS_MCK)
			   | USART3_BF(CHRL, USART3_CHRL_8)
			   | USART3_BF(PAR, USART3_PAR_NONE)
			   | USART3_BF(NBSTOP, USART3_NBSTOP_1)), uart->base + USART3_MR);

	return 0;
}

static int atmel_serial_probe(struct device *dev)
{
	struct atmel_uart_port *uart;
	struct console_device *cdev;
	int ret;

	uart = xzalloc(sizeof(struct atmel_uart_port));

	ret = atmel_serial_init_port(dev, uart);
	if (ret)
		return ret;

	cdev = &uart->uart;
	cdev->dev = dev;
	cdev->tstc = atmel_serial_tstc;
	cdev->putc = atmel_serial_putc;
	cdev->getc = atmel_serial_getc;
	cdev->setbrg = uart->clk ? atmel_serial_setbaudrate : NULL;
	cdev->set_mode = atmel_serial_set_mode;
	cdev->linux_console_name = "ttyAT";
	cdev->linux_earlycon_name = "atmel_serial";

	cdev->phys_base = uart->base;

	/* Enable UART */

	console_register(cdev);

	return 0;
}

static const struct of_device_id __maybe_unused atmel_serial_dt_ids[] = {
	{ .compatible = "atmel,at91rm9200-usart" },
	{ .compatible = "atmel,at91sam9260-usart" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, atmel_serial_dt_ids);

static struct driver atmel_serial_driver = {
        .name  = "atmel_usart",
        .probe = atmel_serial_probe,
	.of_compatible = DRV_OF_COMPAT(atmel_serial_dt_ids),
};
console_platform_driver(atmel_serial_driver);
