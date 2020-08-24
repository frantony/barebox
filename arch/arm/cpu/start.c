// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix

#define pr_fmt(fmt) "start.c: " fmt

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <of.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>
#include <asm/secure.h>
#include <asm/unaligned.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <memory.h>
#include <uncompress.h>
#include <malloc.h>

#include <debug_ll.h>

#include "entry.h"

unsigned long arm_stack_top;
static unsigned long arm_barebox_size;
static unsigned long arm_endmem;
static void *barebox_boarddata;
static unsigned long barebox_boarddata_size;

static bool blob_is_fdt(const void *blob)
{
	return get_unaligned_be32(blob) == FDT_MAGIC;
}

static bool blob_is_compressed_fdt(const void *blob)
{
	const struct barebox_arm_boarddata_compressed_dtb *dtb = blob;

	return dtb->magic == BAREBOX_ARM_BOARDDATA_COMPRESSED_DTB_MAGIC;
}

static bool blob_is_arm_boarddata(const void *blob)
{
	const struct barebox_arm_boarddata *bd = blob;

	return bd->magic == BAREBOX_ARM_BOARDDATA_MAGIC;
}

u32 barebox_arm_machine(void)
{
	if (barebox_boarddata && blob_is_arm_boarddata(barebox_boarddata)) {
		const struct barebox_arm_boarddata *bd = barebox_boarddata;
		return bd->machine;
	} else {
		return 0;
	}
}

void *barebox_arm_boot_dtb(void)
{
	void *dtb;
	void *data;
	int ret;
	struct barebox_arm_boarddata_compressed_dtb *compressed_dtb;
	static void *boot_dtb;

	if (boot_dtb)
		return boot_dtb;

	if (barebox_boarddata && blob_is_fdt(barebox_boarddata)) {
		pr_debug("%s: using barebox_boarddata\n", __func__);
		return barebox_boarddata;
	}

	if (!IS_ENABLED(CONFIG_ARM_USE_COMPRESSED_DTB) || !barebox_boarddata
			|| !blob_is_compressed_fdt(barebox_boarddata))
		return NULL;

	compressed_dtb = barebox_boarddata;

	pr_debug("%s: using compressed_dtb\n", __func__);

	dtb = malloc(compressed_dtb->datalen_uncompressed);
	if (!dtb)
		return NULL;

	data = compressed_dtb + 1;

	ret = uncompress(data, compressed_dtb->datalen, NULL, NULL,
			dtb, NULL, NULL);
	if (ret) {
		pr_err("uncompressing dtb failed\n");
		free(dtb);
		return NULL;
	}

	boot_dtb = dtb;

	return boot_dtb;
}

static inline unsigned long arm_mem_boarddata(unsigned long membase,
					      unsigned long endmem,
					      unsigned long size)
{
	unsigned long mem;

	mem = arm_mem_barebox_image(membase, endmem, arm_barebox_size);
	mem -= ALIGN(size, 64);

	return mem;
}

unsigned long arm_mem_ramoops_get(void)
{
	return arm_mem_ramoops(0, arm_stack_top);
}
EXPORT_SYMBOL_GPL(arm_mem_ramoops_get);

unsigned long arm_mem_endmem_get(void)
{
	return arm_endmem;
}
EXPORT_SYMBOL_GPL(arm_mem_endmem_get);

static int barebox_memory_areas_init(void)
{
	if(barebox_boarddata)
		request_sdram_region("board data", (unsigned long)barebox_boarddata,
				     barebox_boarddata_size);

	return 0;
}
device_initcall(barebox_memory_areas_init);

__noreturn void barebox_non_pbl_start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{
	unsigned long endmem = membase + memsize;
	unsigned long malloc_start, malloc_end;
	unsigned long barebox_size = barebox_image_size + MAX_BSS_SIZE;
	unsigned long barebox_base = arm_mem_barebox_image(membase,
							   endmem,
							   barebox_size);

//	if (IS_ENABLED(CONFIG_CPU_V7))
//		armv7_hyp_install();

	if (IS_ENABLED(CONFIG_RELOCATABLE))
		relocate_to_adr(barebox_base);

	setup_c();

	barrier();

	pbl_barebox_break();

	pr_debug("memory at 0x%08lx, size 0x%08lx\n", membase, memsize);

	arm_endmem = endmem;
	arm_stack_top = arm_mem_stack_top(membase, endmem);
	arm_barebox_size = barebox_size;
	malloc_end = barebox_base;

	if (IS_ENABLED(CONFIG_MMU_EARLY)) {
		unsigned long ttb = arm_mem_ttb(membase, endmem);

		if (IS_ENABLED(CONFIG_PBL_IMAGE)) {
			arm_set_cache_functions();
		} else {
			pr_debug("enabling MMU, ttb @ 0x%08lx\n", ttb);
			arm_early_mmu_cache_invalidate();
			mmu_early_enable(membase, memsize, ttb);
		}
	}

	if (boarddata) {
		uint32_t totalsize = 0;
		const char *name;

		if (blob_is_fdt(boarddata)) {
			totalsize = get_unaligned_be32(boarddata + 4);
			name = "DTB";
		} else if (blob_is_compressed_fdt(boarddata)) {
			struct barebox_arm_boarddata_compressed_dtb *bd = boarddata;
			totalsize = bd->datalen + sizeof(*bd);
			name = "Compressed DTB";
		} else if (blob_is_arm_boarddata(boarddata)) {
			totalsize = sizeof(struct barebox_arm_boarddata);
			name = "machine type";
		} else if ((unsigned long)boarddata < 8192) {
			struct barebox_arm_boarddata *bd;
			uint32_t machine_type = (unsigned long)boarddata;
			unsigned long mem = arm_mem_boarddata(membase, endmem,
							      sizeof(*bd));
			pr_debug("found machine type %d in boarddata\n",
				 machine_type);
			bd = barebox_boarddata = (void *)mem;
			barebox_boarddata_size = sizeof(*bd);
			bd->magic = BAREBOX_ARM_BOARDDATA_MAGIC;
			bd->machine = machine_type;
			malloc_end = mem;
		}

		if (totalsize) {
			unsigned long mem = arm_mem_boarddata(membase, endmem,
							      totalsize);
			pr_debug("found %s in boarddata, copying to 0x%08lx\n",
				 name, mem);
			barebox_boarddata = memcpy((void *)mem, boarddata,
						   totalsize);
			barebox_boarddata_size = totalsize;
			malloc_end = mem;
		}
	}

	/*
	 * Maximum malloc space is the Kconfig value if given
	 * or 1GB.
	 */
	if (MALLOC_SIZE > 0) {
		malloc_start = malloc_end - MALLOC_SIZE;
		if (malloc_start < membase)
			malloc_start = membase;
	} else {
		malloc_start = malloc_end - (malloc_end - membase) / 2;
		if (malloc_end - malloc_start > SZ_1G)
			malloc_start = malloc_end - SZ_1G;
	}

	pr_debug("initializing malloc pool at 0x%08lx (size 0x%08lx)\n",
			malloc_start, malloc_end - malloc_start);

	mem_malloc_init((void *)malloc_start, (void *)malloc_end - 1);

	if (IS_ENABLED(CONFIG_BOOTM_OPTEE))
		of_add_reserve_entry(endmem - OPTEE_SIZE, endmem - 1);

	pr_debug("starting barebox...\n");

	start_barebox();
}

#ifndef CONFIG_PBL_IMAGE

void start(void);

void NAKED __section(.text_entry) start(void)
{
	barebox_arm_head();
}

#else



/* Set the default clock frequencies after reset. */
uint32_t rcc_apb1_frequency = 16000000;
uint32_t rcc_apb2_frequency = 16000000;

#define MMIO32(addr)		(*(volatile uint32_t *)(addr))

#define PERIPH_BASE	(0x40000000U)
#define PERIPH_BASE_AHB1	(PERIPH_BASE + 0x20000)
#define PERIPH_BASE_APB2		(PERIPH_BASE + 0x10000)
#define RCC_BASE	(PERIPH_BASE_AHB1 + 0x3800)
#define GPIO_PORT_A_BASE		(PERIPH_BASE_AHB1 + 0x0000)
#define GPIO_PORT_G_BASE		(PERIPH_BASE_AHB1 + 0x1800)
#define USART1_BASE			(PERIPH_BASE_APB2 + 0x1000)

#define _REG_BIT(base, bit)		(((base) << 5) + (bit))

enum rcc_periph_clken {
	RCC_GPIOA	= _REG_BIT(0x30, 0),
	RCC_GPIOG	= _REG_BIT(0x30, 6),
	RCC_USART1	= _REG_BIT(0x44, 4),
};

#define RCC_REG(i)	MMIO32(RCC_BASE + ((i) >> 5))
#define RCC_BIT(i)	(1 << ((i) & 0x1f))

static void barebox_rcc_periph_clock_enable(enum rcc_periph_clken clken)
{
	RCC_REG(clken) |= RCC_BIT(clken);
}

static void clock_setup(void)
{
	/* Enable GPIOG clock for LED & USARTs. */
	barebox_rcc_periph_clock_enable(RCC_GPIOG);
	barebox_rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable clocks for USART2. */
	barebox_rcc_periph_clock_enable(RCC_USART1);
}

#define USART1				USART1_BASE

#define USART_SR(usart_base)		MMIO32((usart_base) + 0x00)

#define USART_DR(usart_base)		MMIO32((usart_base) + 0x04)
#define USART_DR_MASK                   0x1FF

#define USART_BRR(usart_base)		MMIO32((usart_base) + 0x08)
#define USART_CR1(usart_base)		MMIO32((usart_base) + 0x0c)
#define USART_CR2(usart_base)		MMIO32((usart_base) + 0x10)
#define USART_CR3(usart_base)		MMIO32((usart_base) + 0x14)


#define USART_CR2_STOPBITS_1		(0x00 << 12)     /* 1 stop bit */
#define USART_CR2_STOPBITS_MASK		(0x03 << 12)

#define USART_STOPBITS_1		USART_CR2_STOPBITS_1   /* 1 stop bit */
#define USART_SR_TXE			(1 << 7)
#define USART_CR1_RE			(1 << 2)
#define USART_CR1_TE			(1 << 3)
#define USART_CR1_M			(1 << 12)
#define USART_CR1_UE			(1 << 13)
#define USART_MODE_TX		        USART_CR1_TE
#define USART_PARITY_NONE		0x00
#define USART_FLOWCONTROL_NONE	        0x00
#define USART_MODE_MASK		        (USART_CR1_RE | USART_CR1_TE)

static void usart_wait_send_ready(uint32_t usart)
{
	/* Wait until the data has been transferred into the shift register. */
	while ((USART_SR(usart) & USART_SR_TXE) == 0);
}

static void usart_send(uint32_t usart, uint16_t data)
{
	/* Send data. */
	USART_DR(usart) = (data & USART_DR_MASK);
}

static void usart_send_blocking(uint32_t usart, uint16_t data)
{
	usart_wait_send_ready(usart);
	usart_send(usart, data);
}

static void usart_set_baudrate(uint32_t usart, uint32_t baud)
{
	uint32_t clock = rcc_apb1_frequency;

#if defined USART1
	if ((usart == USART1)
#if defined USART6
		|| (usart == USART6)
#endif
		) {
		clock = rcc_apb2_frequency;
	}
#endif

	/*
	 * Yes it is as simple as that. The reference manual is
	 * talking about fractional calculation but it seems to be only
	 * marketing babble to sound awesome. It is nothing else but a
	 * simple divider to generate the correct baudrate.
	 *
	 * Note: We round() the value rather than floor()ing it, for more
	 * accurate divisor selection.
	 */
#ifdef LPUART1
	if (usart == LPUART1) {
		USART_BRR(usart) = (clock / baud) * 256
			+ ((clock % baud) * 256 + baud / 2) / baud;
		return;
	}
#endif

	USART_BRR(usart) = (clock + baud / 2) / baud;
}

static void usart_set_databits(uint32_t usart, uint32_t bits)
{
	if (bits == 8) {
		USART_CR1(usart) &= ~USART_CR1_M; /* 8 data bits */
	} else {
		USART_CR1(usart) |= USART_CR1_M;  /* 9 data bits */
	}
}

static void usart_set_stopbits(uint32_t usart, uint32_t stopbits)
{
	uint32_t reg32;

	reg32 = USART_CR2(usart);
	reg32 = (reg32 & ~USART_CR2_STOPBITS_MASK) | stopbits;
	USART_CR2(usart) = reg32;
}

static void usart_set_mode(uint32_t usart, uint32_t mode)
{
	uint32_t reg32;

	reg32 = USART_CR1(usart);
	reg32 = (reg32 & ~USART_MODE_MASK) | mode;
	USART_CR1(usart) = reg32;
}

#define USART_CR3_CTSE			(1 << 9)
#define USART_CR3_RTSE			(1 << 8)

#define USART_FLOWCONTROL_MASK	        (USART_CR3_RTSE | USART_CR3_CTSE)

static void usart_set_flow_control(uint32_t usart, uint32_t flowcontrol)
{
	uint32_t reg32;

	reg32 = USART_CR3(usart);
	reg32 = (reg32 & ~USART_FLOWCONTROL_MASK) | flowcontrol;
	USART_CR3(usart) = reg32;
}

static void usart_enable(uint32_t usart)
{
	USART_CR1(usart) |= USART_CR1_UE;
}

#define USART_CR1_PS			(1 << 9)
#define USART_CR1_PCE			(1 << 10)
#define USART_PARITY_MASK		(USART_CR1_PS | USART_CR1_PCE)

static void usart_set_parity(uint32_t usart, uint32_t parity)
{
	uint32_t reg32;

	reg32 = USART_CR1(usart);
	reg32 = (reg32 & ~USART_PARITY_MASK) | parity;
	USART_CR1(usart) = reg32;
}

static void usart_setup(void)
{
	/* Setup USART2 parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

#define GPIOA				GPIO_PORT_A_BASE
#define GPIOG				GPIO_PORT_G_BASE

#define GPIO_MODER(port)		MMIO32((port) + 0x00)
#define GPIO_MODE(n, mode)		((mode) << (2 * (n)))
#define GPIO_MODE_MASK(n)		(0x3 << (2 * (n)))
#define GPIO_MODE_INPUT			0x0
#define GPIO_MODE_OUTPUT		0x1
#define GPIO_MODE_AF			0x2
#define GPIO_MODE_ANALOG		0x3
#define GPIO_PUPD(n, pupd)		((pupd) << (2 * (n)))
#define GPIO_PUPD_MASK(n)		(0x3 << (2 * (n)))
#define GPIO_PUPD_NONE			0x0
#define GPIO_PUPD_PULLUP		0x1
#define GPIO_PUPD_PULLDOWN		0x2
#define GPIO_PUPDR(port)		MMIO32((port) + 0x0c)

#define GPIO9				(1 << 9)
#define GPIO13				(1 << 13)

static void barebox_gpio_mode_setup(uint32_t gpioport, uint8_t mode, uint8_t pull_up_down,
		     uint16_t gpios)
{
	uint16_t i;
	uint32_t moder, pupd;

	/*
	 * We want to set the config only for the pins mentioned in gpios,
	 * but keeping the others, so read out the actual config first.
	 */
	moder = GPIO_MODER(gpioport);
	pupd = GPIO_PUPDR(gpioport);

	for (i = 0; i < 16; i++) {
		if (!((1 << i) & gpios)) {
			continue;
		}

		moder &= ~GPIO_MODE_MASK(i);
		moder |= GPIO_MODE(i, mode);
		pupd &= ~GPIO_PUPD_MASK(i);
		pupd |= GPIO_PUPD(i, pull_up_down);
	}

	/* Set mode and pull up/down control registers. */
	GPIO_MODER(gpioport) = moder;
	GPIO_PUPDR(gpioport) = pupd;
}

#define GPIO_ODR(port)			MMIO32((port) + 0x14)
#define GPIO_BSRR(port)			MMIO32((port) + 0x18)

static void barebox_gpio_toggle(uint32_t gpioport, uint16_t gpios)
{
	uint32_t port = GPIO_ODR(gpioport);
	GPIO_BSRR(gpioport) = ((port & gpios) << 16) | (~port & gpios);
}

#define GPIO_AFRL(port)			MMIO32((port) + 0x20)
#define GPIO_AFRH(port)			MMIO32((port) + 0x24)
#define GPIO_AFR(n, af)			((af) << ((n) * 4))
#define GPIO_AFR_MASK(n)		(0xf << ((n) * 4))

static void barebox_gpio_set_af(uint32_t gpioport, uint8_t alt_func_num, uint16_t gpios)
{
	uint16_t i;
	uint32_t afrl, afrh;

	afrl = GPIO_AFRL(gpioport);
	afrh = GPIO_AFRH(gpioport);

	for (i = 0; i < 8; i++) {
		if (!((1 << i) & gpios)) {
			continue;
		}
		afrl &= ~GPIO_AFR_MASK(i);
		afrl |= GPIO_AFR(i, alt_func_num);
	}

	for (i = 8; i < 16; i++) {
		if (!((1 << i) & gpios)) {
			continue;
		}
		afrh &= ~GPIO_AFR_MASK(i - 8);
		afrh |= GPIO_AFR(i - 8, alt_func_num);
	}

	GPIO_AFRL(gpioport) = afrl;
	GPIO_AFRH(gpioport) = afrh;
}

#define GPIO_AF7			0x7

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO13 on GPIO port G for LED. */
	barebox_gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	barebox_gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);

	/* Setup GPIO pins for USART1 transmit. */
	barebox_gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);

	/* Setup USART1 TX pin as alternate function. */
	barebox_gpio_set_af(GPIOA, GPIO_AF7, GPIO9);
}

static void stm32_usart_led_demo(void)
{
	int i, j = 0, c = 0;

	clock_setup();
	gpio_setup();
	usart_setup();

	/* Blink the LED (PG13) on the board with every transmitted byte. */
	while (1) {
		barebox_gpio_toggle(GPIOG, GPIO13);	/* LED on/off */
		usart_send_blocking(USART1, c + '0'); /* USART1: Send byte. */
		c = (c == 9) ? 0 : c + 1;	/* Increment c. */
		if ((j++ % 80) == 0) {		/* Newline after line full. */
			usart_send_blocking(USART1, '\r');
			usart_send_blocking(USART1, '\n');
		}
		for (i = 0; i < 300000; i++) {	/* Wait a bit. */
			__asm__("NOP");
		}
	}
}




void start(unsigned long membase, unsigned long memsize, void *boarddata);
/*
 * First function in the uncompressed image. We get here from
 * the pbl. The stack already has been set up by the pbl.
 */
void NAKED __section(.text_entry) start(unsigned long membase,
		unsigned long memsize, void *boarddata)
{

	stm32_usart_led_demo();

	barebox_non_pbl_start(membase, memsize, boarddata);
}
#endif
