#include <init.h>
#include <linux/compiler.h>
#include <debug_ll.h>
#include <asm/io.h>
#include <mach/jz4770_regs.h>
#include <mach/xburst-flash-header.h>

void static inline init_sdram(void);

#define CFG_EXTAL		(12 * 1000000)		        /* EXTAL freq: 12MHz */
#define CFG_CPU_SPEED		(1008 * 1000000)		    /* CPU clock */

#ifndef __JZ4770_COMMON_H__
#define __JZ4770_COMMON_H__

#define PLL_OUT_MAX 1200		/* 1200MHz. */

#define __CFG_EXTAL     (CFG_EXTAL / 1000000)
#define __CFG_PLL_OUT   (CFG_CPU_SPEED / 1000000)

#ifdef CFG_PLL1_FRQ
    #define __CFG_PLL1_OUT  ((CFG_PLL1_FRQ)/1000000)    /* Set PLL1 default: 240MHz */
#else
    #define __CFG_PLL1_OUT  (432)                       /* Set PLL1 default: 432MHZ  UHC:48MHZ TVE:27MHZ */
#endif

/*pll_0*/
#if (__CFG_PLL_OUT > PLL_OUT_MAX)
	#error "PLL output can NOT more than 1000MHZ"
#elif (__CFG_PLL_OUT > 600)
	#define __PLL_BS          1
	#define __PLL_OD          0
#elif (__CFG_PLL_OUT > 300)
	#define __PLL_BS          0
	#define __PLL_OD          0
#elif (__CFG_PLL_OUT > 155)
	#define __PLL_BS          0
	#define __PLL_OD          1
#elif (__CFG_PLL_OUT > 76)
	#define __PLL_BS          0
	#define __PLL_OD          2
#elif (__CFG_PLL_OUT > 47)
	#define __PLL_BS          0
	#define __PLL_OD          3
#else
	#error "PLL ouptput can NOT less than 48"
#endif

#define __PLL_NO		0
#define NR 			(__PLL_NO + 1)
#define NO 			(0x1 << __PLL_OD)
#define __PLL_MO		(((__CFG_PLL_OUT / __CFG_EXTAL) * NR * NO) - 1)
#define NF 			(__PLL_MO + 1)
#define FOUT			(__CFG_EXTAL * NF / NR / NO)

#if ((__CFG_EXTAL / NR > 50) || (__CFG_EXTAL / NR < 10))
	#error "Can NOT set the value to PLL_N"
#endif

#if ((__PLL_MO > 127) || (__PLL_MO < 1))
	#error "Can NOT set the value to PLL_M"
#endif

#if (__PLL_BS == 1)
	#if (((FOUT * NO) > PLL_OUT_MAX) || ((FOUT * NO) < 500))
		#error "FVCO check failed : BS = 1"
	#endif
#elif (__PLL_BS == 0)
	#if (((FOUT * NO) > 600) || ((FOUT * NO) < 300))
		#error "FVCO check failed : BS = 0"
	#endif
#endif

#define CPCCR_M_N_OD	((__PLL_MO << 24) | (__PLL_NO << 18) | (__PLL_OD << 16) | (__PLL_BS << 31))

/**************************************************************************************************************/

#if (__CFG_PLL1_OUT > PLL_OUT_MAX)
	#error "PLL1 output can NO1T more than 1000MHZ"
#elif (__CFG_PLL1_OUT > 600)
	#define __PLL1_BS          1
	#define __PLL1_OD          0
#elif (__CFG_PLL1_OUT > 300)
	#define __PLL1_BS          0
	#define __PLL1_OD          0
#elif (__CFG_PLL1_OUT > 155)
	#define __PLL1_BS          0
	#define __PLL1_OD          1
#elif (__CFG_PLL1_OUT > 76)
	#define __PLL1_BS          0
	#define __PLL1_OD          2
#elif (__CFG_PLL1_OUT > 47)
	#define __PLL1_BS          0
	#define __PLL1_OD          3
#else
	#error "PLL1 ouptput can NOT less than 48"
#endif

#define __PLL1_NO1		0
#define NR1 			(__PLL1_NO1 + 1)
#define NO1 			(0x1 << __PLL1_OD)
#define __PLL1_MO		(((__CFG_PLL1_OUT / __CFG_EXTAL) * NR1 * NO1) - 1)
#define NF1 			(__PLL1_MO + 1)
#define FOUT1			(__CFG_EXTAL * NF1 / NR1 / NO1)

#if ((__CFG_EXTAL / NR1 > 50) || (__CFG_EXTAL / NR1 < 10))
	#error "Can NOT set the value to PLL1_N"
#endif

#if ((__PLL1_MO > 127) || (__PLL1_MO < 1))
	#error "Can NOT set the value to PLL1_M"
#endif

#if (__PLL1_BS == 1)
	#if (((FOUT1 * NO1) > 1000) || ((FOUT1 * NO1) < 500))
		#error "FVCO1 check failed : BS1 = 1"
	#endif
#elif (__PLL1_BS == 0)
	#if (((FOUT1 * NO1) > 600) || ((FOUT1 * NO1) < 300))
		#error "FVCO1 check failed : BS1 = 0"
	#endif
#endif

#define CPCCR1_M_N_OD	((__PLL1_MO << 24) | (__PLL1_NO1 << 18) | (__PLL1_OD << 16) | (__PLL1_BS << 31))

#endif /* __JZ4770_COMMON_H__ */


#define CPCCR_M_N_OD	((__PLL_MO << 24) | (__PLL_NO << 18) | (__PLL_OD << 16) | (__PLL_BS << 31))

#define CFG_UART_BASE UART2_BASE
//#define DEBUG_LL_UART_SHIFT	2

/* Register Offset */
#define OFF_RDR		(0x00)	/* R  8b H'xx */
#define OFF_TDR		(0x00)	/* W  8b H'xx */
#define OFF_DLLR	(0x00)	/* RW 8b H'00 */
#define OFF_DLHR	(0x04)	/* RW 8b H'00 */
#define OFF_IER		(0x04)	/* RW 8b H'00 */
#define OFF_ISR		(0x08)	/* R  8b H'01 */
#define OFF_FCR		(0x08)	/* W  8b H'00 */
#define OFF_LCR		(0x0C)	/* RW 8b H'00 */
#define OFF_MCR		(0x10)	/* RW 8b H'00 */
#define OFF_LSR		(0x14)	/* R  8b H'00 */
#define OFF_MSR		(0x18)	/* R  8b H'00 */
#define OFF_SPR		(0x1C)	/* RW 8b H'00 */
#define OFF_SIRCR	(0x20)	/* RW 8b H'00, UART0 */
#define OFF_UMR		(0x24)	/* RW 8b H'00, UART M Register */
#define OFF_UACR	(0x28)	/* RW 8b H'00, UART Add Cycle Register */

/*
 * Define macros for UART_FCR
 * UART FIFO Control Register
 */
#define UART_FCR_FE	(1 << 0)	/* 0: non-FIFO mode  1: FIFO mode */
#define UART_FCR_RFLS	(1 << 1)	/* write 1 to flush receive FIFO */
#define UART_FCR_TFLS	(1 << 2)	/* write 1 to flush transmit FIFO */
#define UART_FCR_DMS	(1 << 3)	/* 0: disable DMA mode */
#define UART_FCR_UUE	(1 << 4)	/* 0: disable UART */

/*
 * Define macros for UART_LCR
 * UART Line Control Register
 */
#define UART_LCR_WLEN_8	(3 << 0)
#define UART_LCR_STOP	(1 << 2)	/* 0: 1 stop bit when word length is 5,6,7,8
					   1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */
#define UART_LCR_STOP_1	(0 << 2)	/* 0: 1 stop bit when word length is 5,6,7,8
					   1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */
#define UART_LCR_STOP_2	(1 << 2)	/* 0: 1 stop bit when word length is 5,6,7,8
					   1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */

#define UART_LCR_PE	(1 << 3)	/* 0: parity disable */
#define UART_LCR_PROE	(1 << 4)	/* 0: even parity  1: odd parity */
#define UART_LCR_SPAR	(1 << 5)	/* 0: sticky parity disable */
#define UART_LCR_SBRK	(1 << 6)	/* write 0 normal, write 1 send break */
#define UART_LCR_DLAB	(1 << 7)	/* 0: access UART_RDR/TDR/IER  1: access UART_DLLR/DLHR */

/*
 * Define macros for UART_LSR
 * UART Line Status Register
 */
#define UART_LSR_TDRQ	(1 << 5)	/* 1: transmit FIFO half "empty" */
#define UART_LSR_TEMT	(1 << 6)	/* 1: transmit FIFO and shift registers empty */

/*
 * Define macros for SIRCR
 * Slow IrDA Control Register
 */
#define SIRCR_TSIRE	(1 << 0)	/* 0: transmitter is in UART mode  1: IrDA mode */
#define SIRCR_RSIRE	(1 << 1)	/* 0: receiver is in UART mode  1: IrDA mode */

/* PLL output frequency */
static unsigned int __flash_header_section __cpm_get_pllout(void)
{
	unsigned long m, n, no, pllout;
	unsigned long cppcr = __raw_readl(CPM_CPPCR);
	static unsigned long od[4] = {1, 2, 4, 8};

	if ((cppcr & CPM_CPPCR_PLLEN) && (!(cppcr & CPM_CPPCR_PLLBP))) {
		int a;
		m = __cpm_get_pllm() + 1;
		n = __cpm_get_plln() + 1;
		no = od[__cpm_get_pllod()];
	//	no = 1 << (__cpm_get_pllod() & 3);

		pllout = ((JZ_EXTAL) * m / (n * no));
	} else
		pllout = JZ_EXTAL;

	return pllout;
}

/* PLL output frequency for MSC/I2S/LCD/USB */
static unsigned int __flash_header_section __cpm_get_pllout2(void)
{
	if (__raw_readl(CPM_CPCCR) & CPM_CPCCR_PCS)
		return __cpm_get_pllout()/2;
	else
		return __cpm_get_pllout();
}

/* CPU core clock */
static inline unsigned int __cpm_get_cclk(void)
{
	static int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};
	unsigned int a, b;

	PUTC_LL('y');
	PUTC_LL('y');
	a = __cpm_get_pllout();
	PUTC_LL('Y');
	PUTC_LL('Y');
	b = div[__cpm_get_cdiv()];
	PUTC_LL('Z');
	PUTC_LL('Z');

	return a / b;
}

/* AHB system bus clock */
#if 0
static inline unsigned int __cpm_get_hclk(void)
{
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	return __cpm_get_pllout() / div[__cpm_get_hdiv()];
}
#endif

/* Memory bus clock */
unsigned int __flash_header_section __cpm_get_mclk(void)
{
	static int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	return __cpm_get_pllout() / div[__cpm_get_mdiv()];
}

/*
 * Output 24MHz for SD and 16MHz for MMC.
 */
static inline void __cpm_select_msc_clk(int n, int sd)
{
	unsigned int pllout2 = __cpm_get_pllout2();
	unsigned int div = 0;

	if (sd) {
		div = pllout2 / 24000000;
	}
	else {
		div = pllout2 / 16000000;
	}

	__raw_writel(div - 1, CPM_MSCCDR);
	__raw_writel(__raw_readl(CPM_CPCCR) | CPM_CPCCR_CE, CPM_CPCCR);
}

void spl_main(void);

static inline void serial_setbrg (void)
{
	volatile u8 *uart_lcr = (volatile u8 *)(CFG_UART_BASE + OFF_LCR);
	volatile u8 *uart_dlhr = (volatile u8 *)(CFG_UART_BASE + OFF_DLHR);
	volatile u8 *uart_dllr = (volatile u8 *)(CFG_UART_BASE + OFF_DLLR);
	volatile u8 *uart_umr = (volatile u8 *)(CFG_UART_BASE + OFF_UMR);
	volatile u8 *uart_uacr = (volatile u8 *)(CFG_UART_BASE + OFF_UACR);
	u16 baud_div, tmp;

	*uart_umr = 16;
	*uart_uacr = 0;
	baud_div = 13;

	tmp = *uart_lcr;
	tmp |= UART_LCR_DLAB;
	*uart_lcr = tmp;

	*uart_dlhr = (baud_div >> 8) & 0xff;
	*uart_dllr = baud_div & 0xff;

	tmp &= ~UART_LCR_DLAB;
	*uart_lcr = tmp;
}

static inline int init_serial(void)
{
	volatile u8 *uart_fcr = (volatile u8 *)(CFG_UART_BASE + OFF_FCR);
	volatile u8 *uart_lcr = (volatile u8 *)(CFG_UART_BASE + OFF_LCR);
	volatile u8 *uart_sircr = (volatile u8 *)(CFG_UART_BASE + OFF_SIRCR);

	/* Disable port interrupts while changing hardware */
	writeb(0, (volatile u8 *)(CFG_UART_BASE + OFF_IER));

	/* Disable UART unit function */
	*uart_fcr = ~UART_FCR_UUE;

	/* Set both receiver and transmitter in UART mode (not SIR) */
	*uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

	/* Set databits, stopbits and parity. (8-bit data, 1 stopbit, no parity) */
	*uart_lcr = UART_LCR_WLEN_8 | UART_LCR_STOP_1;

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	*uart_fcr = UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS;

	return 0;
}

#define __gpio_as_uart2() \
do { \
	unsigned int bits = (1 << 30) | (1 << 28); \
	__raw_writel(bits, (void *)GPIO_PXINTC(2)); \
	__raw_writel(bits, (void *)GPIO_PXMASKC(2)); \
	__raw_writel(bits, (void *)GPIO_PXPAT1C(2)); \
	__raw_writel(bits, (void *)GPIO_PXPAT0C(2)); \
} while (0)

/*
 * Init PLL.
 *
 * PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
#define DIV_1 0
#define DIV_2 1
#define DIV_3 2
#define DIV_4 3
#define DIV_6 4
#define DIV_8 5
#define DIV_12 6
#define DIV_16 7
#define DIV_24 8
#define DIV_32 9

#define DIV(a,b,c,d,e,f)					\
({								\
	unsigned int retval;					\
	retval = (DIV_##a << CPM_CPCCR_CDIV_BIT)   |		\
		 (DIV_##b << CPM_CPCCR_H0DIV_BIT)  |		\
		 (DIV_##c << CPM_CPCCR_PDIV_BIT)   |		\
		 (DIV_##d << CPM_CPCCR_C1DIV_BIT)  |		\
		 (DIV_##e << CPM_CPCCR_H2DIV_BIT)  |		\
		 (DIV_##f << CPM_CPCCR_H1DIV_BIT);		\
	retval;							\
})

static void __flash_header_section init_pll(void)
{
	register unsigned int cfcr, plcr1, plcr2;

	/** divisors,
	 *  for jz4770 ,C:H0:P:C1:H2:H1.
	 *  DIV should be one of [1, 2, 3, 4, 6, 8, 12, 16, 24, 32]
	 */
	int pllout2;
	unsigned int div = DIV(1,4,8,2,4,4);
	cfcr = div;

	// write DDRC_CTRL 8 times to clear ddr fifo
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);
	__raw_writel(0, (void *)DDRC_CTRL);

	/* set CPM_CPCCR_MEM only for ddr1 or ddr2 */
	cfcr |= CPM_CPCCR_MEM;

	cfcr |= CPM_CPCCR_CE;

	pllout2 = (cfcr & CPM_CPCCR_PCS) ? (CFG_CPU_SPEED / 2) : CFG_CPU_SPEED;

	plcr1 = CPCCR_M_N_OD;
	plcr1 |= (0x20 << CPM_CPPCR_PLLST_BIT)	/* PLL stable time */
		| CPM_CPPCR_PLLEN;             /* enable PLL */

	/*
	 * Init USB Host clock, pllout2 must be n*48MHz
	 * For JZ4760 UHC - River.
	 */
	__raw_writel(pllout2 / 48000000 - 1, (void *)CPM_UHCCDR);
	/* init PLL */
	__raw_writel(cfcr, (void *)CPM_CPCCR);
	__raw_writel(plcr1, (void *)CPM_CPPCR);

	/* wait for pll output stable ...*/
	while (!(__raw_readl((void *)CPM_CPPCR) & CPM_CPPCR_PLLS));

	/* set CPM_CPCCR_MEM only for ddr1 or ddr2 */
	plcr2 = CPCCR1_M_N_OD | CPM_CPPCR1_PLL1EN;

	/* init PLL_1 , source clock is extal clock */
	__raw_writel(plcr2, (void *)CPM_CPPCR1);

	/* __cpm_enable_pll_change() */
	__raw_writel(__raw_readl((void *)CPM_CPCCR) | CPM_CPCCR_CE,
		(void *)CPM_CPCCR);

	/* wait for pll_1 output stable ... */
	while (!(__raw_readl((void *)CPM_CPPCR1) & CPM_CPPCR1_PLL1S));
}

#include <asm/cacheops.h>

#define CFG_DCACHE_SIZE		16384
#define CFG_ICACHE_SIZE		16384
#define CFG_CACHELINE_SIZE	32

#define K0BASE			0x80000000

static inline void __flush_icache_all(void)
{
	unsigned int addr, t = 0;

	asm volatile ("mtc0 $0, $28"); /* Clear Taglo */
	asm volatile ("mtc0 $0, $29"); /* Clear TagHi */

	for (addr = K0BASE; addr < K0BASE + CFG_ICACHE_SIZE;
	     addr += CFG_CACHELINE_SIZE) {
		asm volatile (
			".set mips3\n\t"
			" cache %0, 0(%1)\n\t"
//			".set mips2\n\t"
			:
			: "I" (Index_Store_Tag_I), "r"(addr));
	}

	/* invalicate btb */
	asm volatile (
		".set mips32\n\t"
		"mfc0 %0, $16, 7\n\t"
		"nop\n\t"
		"ori %0,2\n\t"
		"mtc0 %0, $16, 7\n\t"
//		".set mips2\n\t"
		:
		: "r" (t));
}

static inline void __flush_dcache_all(void)
{
	unsigned int addr;

	for (addr = K0BASE; addr < K0BASE + CFG_DCACHE_SIZE; 
	     addr += CFG_CACHELINE_SIZE) {
		asm volatile (
			".set mips3\n\t"
			" cache %0, 0(%1)\n\t"
//			".set mips2\n\t"
			:
			: "I" (Index_Writeback_Inv_D), "r"(addr));
	}

	asm volatile ("sync");
}

static inline void __flush_cache_all(void)
{
	__flush_dcache_all();
	__flush_icache_all();
}

void __flash_header_section spl_main(void)
{
	u32 a;

	/* __cpm_start_dmac(); etc */
	/* __cpm_start_ddr(); */
	a = __raw_readl((void *)CPM_CLKGR0);
	a &= ~CPM_CLKGR0_DMAC;
	a &= ~CPM_CLKGR0_DDR;
	__raw_writel(a, (void *)CPM_CLKGR0);

	__raw_writel(0x03, (void *)MDMAC_DMACKES);

//	FIXME
//        __gpio_as_nand_16bit(1);

	__gpio_as_uart2();

	/* start UART2 */
	a = __raw_readl((void *)CPM_CLKGR0);
	a &= ~CPM_CLKGR0_UART2;
	__raw_writel(a, (void *)CPM_CLKGR0);

	init_serial();
	PUTC_LL('r');
	PUTC_LL('4');
	PUTC_LL('2');
	PUTC_LL(' ');
	PUTC_LL('\r');
	PUTC_LL('\n');

#if 0
	PUTC_LL(' ');
	PUTC_LL('i');
	PUTC_LL('n');
	PUTC_LL('i');
	PUTC_LL('t');
	PUTC_LL('_');
	PUTC_LL('s');
	PUTC_LL('e');
	PUTC_LL('r');
	PUTC_LL('i');
	PUTC_LL('a');
	PUTC_LL('l');
	PUTC_LL('(');
	PUTC_LL(')');
	PUTC_LL('\r');
	PUTC_LL('\n');
#endif

	init_pll();
#if 0
	PUTC_LL(' ');
	PUTC_LL('i');
	PUTC_LL('n');
	PUTC_LL('i');
	PUTC_LL('t');
	PUTC_LL('_');
	PUTC_LL('p');
	PUTC_LL('l');
	PUTC_LL('l');
	PUTC_LL('(');
	PUTC_LL(')');
	PUTC_LL('\r');
	PUTC_LL('\n');
	PUTC_LL(' ');
#endif

	init_sdram();

	__flush_cache_all();
}

#include <board/DDR2_H5PS1G831CFP-S6.h>

void __flash_header_section ddr_mem_init(int msel, int hl, int tsel, int arg)
{
	volatile int tmp_cnt;
	register unsigned int cpu_clk, ddr_twr;
	register unsigned int ddrc_cfg_reg = 0, init_ddrc_mdelay = 0;

	cpu_clk = CFG_CPU_SPEED;

	ddrc_cfg_reg = DDRC_CFG_TYPE_DDR2 | (DDR_ROW - 12) << 10
		| (DDR_COL - 8) << 8 | DDR_CS1EN << 7 | DDR_CS0EN << 6
		| ((DDR_CL - 1) | 0x8) << 2 | DDR_BANK8 << 1 | DDR_DW32;

	ddrc_cfg_reg |= DDRC_CFG_MPRT;

	init_ddrc_mdelay = tsel << 18 | msel << 16 | hl << 15 | arg << 14;
	ddr_twr = ((__raw_readl(DDRC_TIMING1) & DDRC_TIMING1_TWR_MASK)
		>> DDRC_TIMING1_TWR_BIT) + 1;
	__raw_writel(ddrc_cfg_reg, DDRC_CFG);
	__raw_writel(init_ddrc_mdelay | DDRC_MDELAY_MAUTO, DDRC_MDELAY);

	/***** init ddrc registers & ddr memory regs ****/
	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 10;
	while (tmp_cnt--);

	/* Set CKE High */
	__raw_writel(DDRC_CTRL_CKE, DDRC_CTRL);

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* PREA */
	__raw_writel(DDRC_LMR_CMD_PREC | DDRC_LMR_START, DDRC_LMR);

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* EMR2: extend mode register2 */
	__raw_writel(DDRC_LMR_BA_EMRS2 |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START, DDRC_LMR);

	/* EMR3: extend mode register3 */
	__raw_writel( DDRC_LMR_BA_EMRS3 |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START, DDRC_LMR);

	/* EMR1: extend mode register1 */
#if defined(CONFIG_DDR2_DIFFERENTIAL)
#if defined(CONFIG_DDR2_DIC_NORMAL)
	__raw_writel(((DDR_EMRS1_DIC_NORMAL) << 16) | DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START, DDRC_LMR);
#else
	__raw_writel(((DDR_EMRS1_DIC_HALF) << 16) | DDRC_LMR_BA_EMRS1 |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START, DDRC_LMR);
#endif
#else
	__raw_writel(((DDR_EMRS1_DIC_HALF | DDR_EMRS1_DQS_DIS) << 16) |
		DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#endif

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* MR - DLL Reset A1A0 burst 2 */
	__raw_writel(((ddr_twr - 1) << 9 | DDR2_MRS_DLL_RST |
		DDR_CL << 4 | DDR_MRS_BL_4) << 16 | DDRC_LMR_BA_MRS |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START, DDRC_LMR);

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* PREA */
	__raw_writel(DDRC_LMR_CMD_PREC | DDRC_LMR_START, DDRC_LMR);

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* AR: auto refresh */
	__raw_writel(DDRC_LMR_CMD_AUREF | DDRC_LMR_START, DDRC_LMR);

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	__raw_writel(DDRC_LMR_CMD_AUREF | DDRC_LMR_START, DDRC_LMR);

	/* Wait for DDR_tRP */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);

	/* MR - DLL Reset End */
	__raw_writel(((ddr_twr-1) << 9 | DDR_CL << 4 | DDR_MRS_BL_4) << 16
		| DDRC_LMR_BA_MRS | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
		DDRC_LMR);

	/* wait 200 tCK */
	tmp_cnt = (cpu_clk / 1000000) * 2;
	while (tmp_cnt--);

	/* EMR1 - OCD Default */
#if defined(CONFIG_DDR2_DIFFERENTIAL)
#if defined(CONFIG_DDR2_DIC_NORMAL)
	__raw_writel((DDR_EMRS1_DIC_NORMAL | DDR_EMRS1_OCD_DFLT) << 16 |
		DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#else
	__raw_writel((DDR_EMRS1_DIC_HALF | DDR_EMRS1_OCD_DFLT) << 16 |
		DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#endif

#else
	__raw_writel((DDR_EMRS1_DIC_HALF | DDR_EMRS1_DQS_DIS | DDR_EMRS1_OCD_DFLT) << 16
		| DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#endif

	/* EMR1 - OCD Exit */
#if defined(CONFIG_DDR2_DIFFERENTIAL)
#if defined(CONFIG_DDR2_DIC_NORMAL)
	__raw_writel(((DDR_EMRS1_DIC_NORMAL) << 16) | DDRC_LMR_BA_EMRS1 |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#else
	__raw_writel(((DDR_EMRS1_DIC_HALF) << 16) | DDRC_LMR_BA_EMRS1 |
		DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#endif
#else
	__raw_writel(((DDR_EMRS1_DIC_HALF | DDR_EMRS1_DQS_DIS) << 16) |
		DDRC_LMR_BA_EMRS1 | DDRC_LMR_CMD_LMR | DDRC_LMR_START,
			DDRC_LMR);
#endif

	/* wait DDR_tMRD */
	tmp_cnt = (cpu_clk / 1000000) * 1;
	while (tmp_cnt--);
}

//extern void ddr_mem_init(int msel, int hl, int tsel, int arg);

static void __flash_header_section jzmemset(void *dest,int ch,int len)
{
	unsigned int *d = (unsigned int *)dest;
	int i;
	int wd;

	wd = (ch << 24) | (ch << 16) | (ch << 8) | ch;

	for(i = 0;i < len / 32;i++)
	{
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
		*d++ = wd;
	}
		PUTC_LL('3');
}

long int __flash_header_section initdram(int board_type)
{
	u32 ddr_cfg;
	u32 rows, cols, dw, banks;
	ulong size;
	ddr_cfg = __raw_readl(DDRC_CFG);
	rows = 12 + ((ddr_cfg & DDRC_CFG_ROW_MASK) >> DDRC_CFG_ROW_BIT);
	cols = 8 + ((ddr_cfg & DDRC_CFG_COL_MASK) >> DDRC_CFG_COL_BIT);

	dw = (ddr_cfg & DDRC_CFG_DW) ? 4 : 2;
	banks = (ddr_cfg & DDRC_CFG_BA) ? 8 : 4;

	size = (1 << (rows + cols)) * dw * banks;
	size *= (DDR_CS1EN + DDR_CS0EN);

	return size;
}

static unsigned int __flash_header_section gen_verify_data(unsigned int i)
{
	int data = i/4;

	if (data & 0x1)
	i = data*0x5a5a5a5a;
	else
	i = data*0xa5a5a5a5;
	return i;
}

static int __flash_header_section dma_check_result(void *src, void *dst, int size,int print_flag)
{
	unsigned int addr1, addr2, i, err = 0;
	unsigned int data_expect,dsrc,ddst;

	addr1 = (unsigned int)src;
	addr2 = (unsigned int)dst;

	for (i = 0; i < size; i += 4) {
		data_expect = gen_verify_data(i);
		dsrc = __raw_readl(addr1);
		ddst = __raw_readl(addr2);
		if ((dsrc != data_expect)
		    || (ddst != data_expect)) {
			err = 1;
			if(!print_flag)
				return 1;
		}

		addr1 += 4;
		addr2 += 4;
	}

	return err;
}

static void __flash_header_section dma_nodesc_test(int dma_chan, int dma_src_addr, int dma_dst_addr, int size)
{
	int dma_src_phys_addr, dma_dst_phys_addr;

	/* Allocate DMA buffers */
	dma_src_phys_addr = dma_src_addr & ~0xa0000000;
	dma_dst_phys_addr = dma_dst_addr & ~0xa0000000;

	/* Init DMA module */
	__raw_writel(0, DMAC_DCCSR(dma_chan));
	__raw_writel(DMAC_DRSR_RS_AUTO, DMAC_DRSR(dma_chan));
	__raw_writel(dma_src_phys_addr, DMAC_DSAR(dma_chan));
	__raw_writel(dma_dst_phys_addr, DMAC_DTAR(dma_chan));
	__raw_writel(size / 32, DMAC_DTCR(dma_chan));
	__raw_writel(DMAC_DCMD_SAI | DMAC_DCMD_DAI |
		DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
		DMAC_DCMD_DS_32BYTE | DMAC_DCMD_TIE,
		DMAC_DCMD(dma_chan));
	__raw_writel(DMAC_DCCSR_NDES | DMAC_DCCSR_EN, DMAC_DCCSR(dma_chan));
}

#define MAX_TSEL_VALUE 4
#define MAX_DELAY_VALUES 16 /* quars (2) * hls (2) * msels (4) */
typedef struct ddrc_common_regs {
	unsigned int ctrl;
	unsigned int timing1;
	unsigned int timing2;
	unsigned int refcnt;
	unsigned int dqs;
	unsigned int dqs_adj;
} ddrc_common_regs_t;

typedef struct delay_sel {
	int tsel;
	int msel;
	int hl;
	int quar;
} delay_sel_t;

typedef struct mdelay_array {
	int tsel;
	int num;
	int index[MAX_DELAY_VALUES];
} mdelay_array_t;

#define DDR_DMA_BASE  (0xa0000000)		/*un-cached*/

static int __flash_header_section ddr_dma_test(int print_flag)
{
	int i, err = 0, banks, blocks;
	int times;
	unsigned int addr, DDR_DMA0_SRC, DDR_DMA0_DST;
	volatile unsigned int tmp;
	register unsigned int cpu_clk;
	long int memsize, banksize, testsize;

	banks = (DDR_BANK8 ? 8 : 4) *(DDR_CS0EN + DDR_CS1EN);
	memsize = initdram(0);
	if (memsize > EMC_LOW_SDRAM_SPACE_SIZE)
		memsize = EMC_LOW_SDRAM_SPACE_SIZE;
	//dprintf("memsize = 0x%08x\n", memsize);
	banksize = memsize/banks;
	testsize = 4096;
	blocks = memsize / testsize;
	cpu_clk = CFG_CPU_SPEED;
	//for(times = 0; times < blocks; times++) {
	for(times = 0; times < banks; times++) {
		DDR_DMA0_SRC = DDR_DMA_BASE + banksize * times;
		DDR_DMA0_DST = DDR_DMA_BASE + banksize * (times + 1) - testsize;
		addr = DDR_DMA0_SRC;

		for (i = 0; i < testsize; i += 4) {
			*(volatile unsigned int *)(addr + i) = gen_verify_data(i);
		}

		__raw_writel(0, DMAC_DMACR(0));

		/* Init target buffer */
		jzmemset((void *)DDR_DMA0_DST, 0, testsize);
		dma_nodesc_test(0, DDR_DMA0_SRC, DDR_DMA0_DST, testsize);

		/* global DMA enable bit */
		__raw_writel(DMAC_DMACR_DMAE, DMAC_DMACR(0));

		while(__raw_readl(DMAC_DTCR(0)));

		tmp = (cpu_clk / 1000000) * 1;
		while (tmp--);

		err = dma_check_result((void *)DDR_DMA0_SRC, (void *)DDR_DMA0_DST, testsize,print_flag);

		//REG_DMAC_DCCSR(0) &= ~DMAC_DCCSR_EN; /* disable DMA */
		__raw_writel(0, DMAC_DMACR(0));
		__raw_writel(0, DMAC_DCCSR(0));
		__raw_writel(0, DMAC_DCMD(0));
		__raw_writel(0, DMAC_DRSR(0));

		if (err != 0) {
			return err;
		}
	}
	return err;
}

void __flash_header_section ddr_controller_init(ddrc_common_regs_t *ddrc_common_regs, int msel, int hl, int tsel, int arg)
{
	volatile unsigned int tmp_cnt;
	register unsigned int cpu_clk, us;
	register unsigned int memsize, ddrc_mmap0_reg, ddrc_mmap1_reg, mem_base0, mem_base1, mem_mask0, mem_mask1;
	cpu_clk = CFG_CPU_SPEED;
	us = cpu_clk / 1000000;

	/* reset ddrc_controller */
	__raw_writel(DDRC_CTRL_RESET, DDRC_CTRL);

	/* Wait for precharge, > 200us */
	tmp_cnt = us * 300;
	while (tmp_cnt--);

	__raw_writel(0, DDRC_CTRL);

#if defined(CONFIG_SDRAM_DDR2)
	__raw_writel(0xaaaa, DDRC_PMEMCTRL0); /* FIXME: ODT registers not configed */

#if defined(CONFIG_DDR2_DIFFERENTIAL)
#ifdef CONFIG_DDR2_DRV_CK_CS_FULL
	__raw_writel(0xaaaa, DDRC_PMEMCTRL2);
#else
	__raw_writel(0xaaaaa, DDRC_PMEMCTRL2);
#endif //CONFIG_DDR2_DRV_CK_CS_FULL
#else //CONFIG_DDR2_DIFFERENTIAL
	__raw_writel(0, DDRC_PMEMCTRL2);
#endif //CONFIG_DDR2_DIFFERENTIAL
	__raw_writel(~(1 << 16) & __raw_readl(DDRC_PMEMCTRL3), DDRC_PMEMCTRL3);
	__raw_writel((1 << 17) | __raw_readl(DDRC_PMEMCTRL3), DDRC_PMEMCTRL3);
#if defined(CONFIG_DDR2_DIFFERENTIAL)
	__raw_writel(~(1 << 15) & __raw_readl(DDRC_PMEMCTRL3), DDRC_PMEMCTRL3);
#else
	__raw_writel((1 << 15) | __raw_readl(DDRC_PMEMCTRL3), DDRC_PMEMCTRL3);
#endif
#elif defined(CONFIG_SDRAM_MDDR)
	__raw_writel((1 << 16) | __raw_readl(DDRC_PMEMCTRL3), DDRC_PMEMCTRL3);
#endif
	ddrc_common_regs->timing2 &= ~DDRC_TIMING2_RWCOV_MASK;
	ddrc_common_regs->timing2 |= ((tsel == 0) ? 0 : (tsel-1)) << DDRC_TIMING2_RWCOV_BIT;

	__raw_writel(ddrc_common_regs->timing1, DDRC_TIMING1);
	__raw_writel(ddrc_common_regs->timing2, DDRC_TIMING2);
	__raw_writel(ddrc_common_regs->dqs_adj, DDRC_DQS_ADJ);

	ddr_mem_init(msel, hl, tsel, arg);

	memsize = initdram(0);
	mem_base0 = DDR_MEM_PHY_BASE >> 24;
	mem_base1 = (DDR_MEM_PHY_BASE + memsize / (DDR_CS1EN + DDR_CS0EN)) >> 24;
	mem_mask1 = mem_mask0 = 0xff &
		~(((memsize / (DDR_CS1EN + DDR_CS0EN) >> 24) - 1) &
		 DDRC_MMAP_MASK_MASK);

	ddrc_mmap0_reg = mem_base0 << DDRC_MMAP_BASE_BIT | mem_mask0;
	ddrc_mmap1_reg = mem_base1 << DDRC_MMAP_BASE_BIT | mem_mask1;

	__raw_writel(ddrc_mmap0_reg, DDRC_MMAP0);
	__raw_writel(ddrc_mmap1_reg, DDRC_MMAP1);
	__raw_writel(ddrc_common_regs->refcnt, DDRC_REFCNT);

	/* Enable DLL Detect */
	__raw_writel(ddrc_common_regs->dqs, DDRC_DQS);

	/* Set CKE High */
	__raw_writel(ddrc_common_regs->ctrl, DDRC_CTRL);

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = us * 10;
	while (tmp_cnt--);

	/* Auto Refresh */
	__raw_writel(DDRC_LMR_CMD_AUREF | DDRC_LMR_START, DDRC_LMR);

	/* Wait for number of auto-refresh cycles */
	tmp_cnt = us * 10;
	while (tmp_cnt--);
}

/* DDR sdram init */
void static __flash_header_section init_sdram(void)
{
	int i, num = 0, tsel = 0, msel, hl;
	int j = 0, k = 0, index = 0, quar = 0, sum = 0, tsel_index=0;
	register unsigned int tmp, cpu_clk, mem_clk, ddr_twr, ns_int;
	register unsigned long ps;
	register unsigned int dqs_adj = 0x2325;
	ddrc_common_regs_t ddrc_common_regs;

	cpu_clk = __cpm_get_cclk();;
	mem_clk = __cpm_get_mclk();
//	cpu_clk = 0x23C34600;
//	mem_clk = 0x08F0D180;
	ps = 1000000000 / (mem_clk / 1000); /* ns per tck ns <= real value */
	//ns = 1000000000 / mem_clk; /* ns per tck ns <= real value */

	//dprintf("mem_clk = %d, cpu_clk = %d\n", mem_clk, cpu_clk);

	/* ACTIVE to PRECHARGE command period */
	tmp = DDR_GET_VALUE(DDR_tRAS, ps);
	//tmp = (DDR_tRAS % ns == 0) ? (DDR_tRAS / ps) : (DDR_tRAS / ps + 1);
	if (tmp < 1) tmp = 1;
	if (tmp > 31) tmp = 31;
	ddrc_common_regs.timing1 = (((tmp) / 2) << DDRC_TIMING1_TRAS_BIT);

	/* READ to PRECHARGE command period. */
	tmp = DDR_GET_VALUE(DDR_tRTP, ps);
	//tmp = (DDR_tRTP % ns == 0) ? (DDR_tRTP / ns) : (DDR_tRTP / ns + 1);
	if (tmp < 1) tmp = 1;
	if (tmp > 4) tmp = 4;
	ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TRTP_BIT);

	/* PRECHARGE command period. */
	tmp = DDR_GET_VALUE(DDR_tRP, ps);
	//tmp = (DDR_tRP % ns == 0) ? DDR_tRP / ns : (DDR_tRP / ns + 1);
	if (tmp < 1) tmp = 1;
	if (tmp > 8) tmp = 8;
	ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TRP_BIT);

	/* ACTIVE to READ or WRITE command period. */
	tmp = DDR_GET_VALUE(DDR_tRCD, ps);
	//tmp = (DDR_tRCD % ns == 0) ? DDR_tRCD / ns : (DDR_tRCD / ns + 1);
	if (tmp < 1) tmp = 1;
	if (tmp > 8) tmp = 8;
	ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TRCD_BIT);

	/* ACTIVE to ACTIVE command period. */
	tmp = DDR_GET_VALUE(DDR_tRC, ps);
	//tmp = (DDR_tRC % ns == 0) ? DDR_tRC / ns : (DDR_tRC / ns + 1);
	if (tmp < 3) tmp = 3;
	if (tmp > 31) tmp = 31;
	ddrc_common_regs.timing1 |= ((tmp / 2) << DDRC_TIMING1_TRC_BIT);

	/* ACTIVE bank A to ACTIVE bank B command period. */
	tmp = DDR_GET_VALUE(DDR_tRRD, ps);
	//tmp = (DDR_tRRD % ns == 0) ? DDR_tRRD / ns : (DDR_tRRD / ns + 1);
	if (tmp < 2) tmp = 2;
	if (tmp > 4) tmp = 4;
	ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TRRD_BIT);

	/* WRITE Recovery Time defined by register MR of DDR2 memory */
	tmp = DDR_GET_VALUE(DDR_tWR, ps);
	//tmp = (DDR_tWR % ns == 0) ? DDR_tWR / ns : (DDR_tWR / ns + 1);
	tmp = (tmp < 1) ? 1 : tmp;
	tmp = (tmp < 2) ? 2 : tmp;
	tmp = (tmp > 6) ? 6 : tmp;
	ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TWR_BIT);
	ddr_twr = tmp;

	// Unit is ns
	if(DDR_tWTR > 5) {
		/* WRITE to READ command delay. */
		tmp = DDR_GET_VALUE(DDR_tWTR, ps);
		//tmp = (DDR_tWTR % ns == 0) ? DDR_tWTR / ns : (DDR_tWTR / ns + 1);
		if (tmp > 4) tmp = 4;
		ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TWTR_BIT);
	// Unit is tCK
	} else {
		/* WRITE to READ command delay. */
		tmp = DDR_tWTR;
		if (tmp > 4) tmp = 4;
		ddrc_common_regs.timing1 |= ((tmp - 1) << DDRC_TIMING1_TWTR_BIT);
	}

	/* AUTO-REFRESH command period. */
	tmp = DDR_GET_VALUE(DDR_tRFC, ps);
	//tmp = (DDR_tRFC % ns == 0) ? DDR_tRFC / ns : (DDR_tRFC / ns + 1);
	if (tmp > 31) tmp = 31;
	ddrc_common_regs.timing2 = ((tmp / 2) << DDRC_TIMING2_TRFC_BIT);

	/* Minimum Self-Refresh / Deep-Power-Down time */
	tmp = DDR_tMINSR;
	if (tmp < 9) tmp = 9;
	if (tmp > 129) tmp = 129;
	tmp = ((tmp - 1)%8 == 0) ? ((tmp - 1)/8-1) : ((tmp - 1)/8);
	ddrc_common_regs.timing2 |= (tmp << DDRC_TIMING2_TMINSR_BIT);
	ddrc_common_regs.timing2 |= (DDR_tXP << 4) | (DDR_tMRD - 1);

	ddrc_common_regs.refcnt = DDR_CLK_DIV << 1 | DDRC_REFCNT_REF_EN;

	ns_int = (1000000000 % mem_clk == 0) ?
		(1000000000 / mem_clk) : (1000000000 / mem_clk + 1);
	tmp = DDR_tREFI/ns_int;
	tmp = tmp / (16 * (1 << DDR_CLK_DIV)) - 1;
	if (tmp > 0xfff)
		tmp = 0xfff;
	if (tmp < 1)
		tmp = 1;

	ddrc_common_regs.refcnt |= tmp << DDRC_REFCNT_CON_BIT;
	ddrc_common_regs.dqs = DDRC_DQS_AUTO | DDRC_DQS_DET | DDRC_DQS_SRDET;

	/* precharge power down, disable power down */
	/* precharge power down, if set active power down, |= DDRC_CTRL_ACTPD */
	ddrc_common_regs.ctrl = DDRC_CTRL_PDT_DIS | DDRC_CTRL_PRET_8 | DDRC_CTRL_UNALIGN | DDRC_CTRL_CKE;
	ddrc_common_regs.dqs_adj = dqs_adj;

	/* Add Jz4760 chip here. Jz4760 chip have no cvt */

		PUTC_LL('2');
	mdelay_array_t mdelay_pass_array[MAX_TSEL_VALUE];
	jzmemset(mdelay_pass_array, 0, sizeof(mdelay_pass_array));

		PUTC_LL('4');
	for (i = 0; i < MAX_TSEL_VALUE; i ++) {
		tsel = i;
		num =0;
		for (j = 0; j < MAX_DELAY_VALUES; j++) {
			msel = j / 4;
			hl = ((j / 2) & 1) ^ 1;
			quar = j & 1;
			ddr_controller_init(&ddrc_common_regs, msel, hl, tsel, quar);

			if(ddr_dma_test(0) != 0) {
				if (num > 0) {
					mdelay_pass_array[k].tsel = tsel;
					mdelay_pass_array[k].num = num;
					k++;
					break;
				}
				else
					continue;
			}
			else {
				mdelay_pass_array[k].index[num] = j;
				num++;
			}
		}
		if (k > 0 && num == 0)
			break;
	}
		PUTC_LL('5');
	if (k == 0) {
		//serial_puts("\n\nDDR INIT ERROR: can't find a suitable mask delay.\n");
		PUTC_LL('E');
		while(1);
	}
	tsel_index = (k-1)/2;
	tsel = mdelay_pass_array[tsel_index].tsel;
	num = mdelay_pass_array[tsel_index].num;
	index = (mdelay_pass_array[tsel_index].index[0] + mdelay_pass_array[tsel_index].index[num-1])/2;
//	serial_puts("X");
//	serial_put_hex(index);
//	serial_put_hex(tsel);
	msel = index/4;
	hl = ((index / 2) & 1) ^ 1;
	quar = index & 1;

	int max_adj = (__raw_readl(DDRC_DQS) & DDRC_DQS_RDQS_MASK) >> DDRC_DQS_RDQS_BIT;
	max_adj = (__raw_readl(DDRC_DQS_ADJ) & DDRC_DQS_ADJRSIGN)
		? (max_adj + ((__raw_readl(DDRC_DQS_ADJ) & DDRC_DQS_ADJRDQS_MASK) >> DDRC_DQS_ADJRDQS_BIT))
		: (max_adj - ((__raw_readl(DDRC_DQS_ADJ) & DDRC_DQS_ADJRDQS_MASK) >> DDRC_DQS_ADJRDQS_BIT));
	if (max_adj > (DDRC_DQS_ADJRDQS_MASK >> DDRC_DQS_ADJRDQS_BIT))
		max_adj = DDRC_DQS_ADJRDQS_MASK >> DDRC_DQS_ADJRDQS_BIT;

	int index_min=-max_adj, index_max = max_adj;

	num = 0;

	for (i = -max_adj; i < max_adj; i++) {
		dqs_adj = (i < 0) ? (0x2320 | -i) : (0x2300 | i);
		ddrc_common_regs.dqs_adj = dqs_adj;
		ddr_controller_init(&ddrc_common_regs, msel, hl, tsel, quar);
		if (ddr_dma_test(0) != 0) {
			if (num > 0) {
				index_max = i;
				break;
			}
		}
		else {
			if (num == 0)
				index_min = i;
			num++;
		}
	}
	index = (index_min + index_max - 1) / 2;

	dqs_adj = (index < 0) ? (0x2320 | (-index)) : (0x2300 | index);
	ddrc_common_regs. dqs_adj = dqs_adj;
	ddr_controller_init(&ddrc_common_regs, msel, hl, tsel, quar);
}
