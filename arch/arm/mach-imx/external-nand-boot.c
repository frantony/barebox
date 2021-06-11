// SPDX-License-Identifier: GPL-2.0-or-later

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/rawnand.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <mach/imx-nand.h>
#include <mach/esdctl.h>
#include <mach/generic.h>
#include <mach/imx21-regs.h>
#include <mach/imx25-regs.h>
#include <mach/imx27-regs.h>
#include <mach/imx31-regs.h>
#include <mach/imx35-regs.h>

#define BARE_INIT_FUNCTION(name)  \
	__section(.text_bare_init_##name) \
		name

static void __bare_init noinline imx_nandboot_wait_op_done(void __iomem *regs)
{
	u32 r;

	while (1) {
		r = readw(regs + NFC_V1_V2_CONFIG2);
		if (r & NFC_V1_V2_CONFIG2_INT)
			break;
	};

	r &= ~NFC_V1_V2_CONFIG2_INT;

	writew(r, regs + NFC_V1_V2_CONFIG2);
}

/*
 * This function issues the specified command to the NAND device and
 * waits for completion.
 *
 * @param       cmd     command for NAND Flash
 */
static void __bare_init imx_nandboot_send_cmd(void *regs, u16 cmd)
{
	writew(cmd, regs + NFC_V1_V2_FLASH_CMD);
	writew(NFC_CMD, regs + NFC_V1_V2_CONFIG2);

	imx_nandboot_wait_op_done(regs);
}

/*
 * This function sends an address (or partial address) to the
 * NAND device.  The address is used to select the source/destination for
 * a NAND command.
 *
 * @param       addr    address to be written to NFC.
 * @param       islast  True if this is the last address cycle for command
 */
static void __bare_init noinline imx_nandboot_send_addr(void *regs, u16 addr)
{
	writew(addr, regs + NFC_V1_V2_FLASH_ADDR);
	writew(NFC_ADDR, regs + NFC_V1_V2_CONFIG2);

	/* Wait for operation to complete */
	imx_nandboot_wait_op_done(regs);
}

static void __bare_init imx_nandboot_nfc_addr(void *regs, u32 offs, int pagesize_2k)
{
	imx_nandboot_send_addr(regs, offs & 0xff);

	if (pagesize_2k) {
		imx_nandboot_send_addr(regs, offs & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 11) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 19) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 27) & 0xff);
		imx_nandboot_send_cmd(regs, NAND_CMD_READSTART);
	} else {
		imx_nandboot_send_addr(regs, (offs >> 9) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 17) & 0xff);
		imx_nandboot_send_addr(regs, (offs >> 25) & 0xff);
	}
}

static void __bare_init imx_nandboot_send_page(void *regs, int v1,
		unsigned int ops, int pagesize_2k)
{
	int bufs, i;

	if (v1 && pagesize_2k)
		bufs = 4;
	else
		bufs = 1;

	for (i = 0; i < bufs; i++) {
		/* NANDFC buffer 0 is used for page read/write */
		writew(i, regs + NFC_V1_V2_BUF_ADDR);

		writew(ops, regs + NFC_V1_V2_CONFIG2);

		/* Wait for operation to complete */
		imx_nandboot_wait_op_done(regs);
	}
}

static void __bare_init __memcpy32(void *trg, const void *src, int size)
{
	int i;
	unsigned int *t = trg;
	unsigned const int *s = src;

	for (i = 0; i < (size >> 2); i++)
		*t++ = *s++;
}

static noinline void __bare_init imx_nandboot_get_page(void *regs, int v1,
		u32 offs, int pagesize_2k)
{
	imx_nandboot_send_cmd(regs, NAND_CMD_READ0);
	imx_nandboot_nfc_addr(regs, offs, pagesize_2k);
	imx_nandboot_send_page(regs, v1, NFC_OUTPUT, pagesize_2k);
}

static void __bare_init imx_nand_load_image(void *dest, int v1,
					    void __iomem *base, int pagesize_2k)
{
	u32 tmp, page, block, blocksize, pagesize, badblocks;
	int bbt = 0;
	void *regs, *spare0;
	int size = *(uint32_t *)(dest + 0x2c);

	if (pagesize_2k) {
		pagesize = 2048;
		blocksize = 128 * 1024;
	} else {
		pagesize = 512;
		blocksize = 16 * 1024;
	}

	if (v1) {
		regs = base + 0xe00;
		spare0 = base + 0x800;
	} else {
		regs = base + 0x1e00;
		spare0 = base + 0x1000;
	}

	imx_nandboot_send_cmd(regs, NAND_CMD_RESET);

	/* preset operation */
	/* Unlock the internal RAM Buffer */
	writew(0x2, regs + NFC_V1_V2_CONFIG);

	/* Unlock Block Command for given address range */
	writew(0x4, regs + NFC_V1_V2_WRPROT);

	tmp = readw(regs + NFC_V1_V2_CONFIG1);
	tmp |= NFC_V1_V2_CONFIG1_ECC_EN;
	if (!v1)
		/* currently no support for 218 byte OOB with stronger ECC */
		tmp |= NFC_V2_CONFIG1_ECC_MODE_4;
	tmp &= ~(NFC_V1_V2_CONFIG1_SP_EN | NFC_V1_V2_CONFIG1_INT_MSK);
	writew(tmp, regs + NFC_V1_V2_CONFIG1);

	if (!v1) {
		if (pagesize_2k)
			writew(NFC_V2_SPAS_SPARESIZE(64), regs + NFC_V2_SPAS);
		else
			writew(NFC_V2_SPAS_SPARESIZE(16), regs + NFC_V2_SPAS);
	}

	/*
	 * Check if this image has a bad block table embedded. See
	 * imx_bbu_external_nand_register_handler for more information
	 */
	badblocks = *(uint32_t *)(base + ARM_HEAD_SPARE_OFS);
	if (badblocks == IMX_NAND_BBT_MAGIC) {
		bbt = 1;
		badblocks = *(uint32_t *)(base + ARM_HEAD_SPARE_OFS + 4);
	}

	block = page = 0;

	while (1) {
		page = 0;

		imx_nandboot_get_page(regs, v1, block * blocksize +
				page * pagesize, pagesize_2k);

		if (bbt) {
			if (badblocks & (1 << block)) {
				block++;
				continue;
			}
		} else if (pagesize_2k) {
			if ((readw(spare0) & 0xff) != 0xff) {
				block++;
				continue;
			}
		} else {
			if ((readw(spare0 + 4) & 0xff00) != 0xff00) {
				block++;
				continue;
			}
		}

		while (page * pagesize < blocksize) {
			debug("page: %d block: %d dest: %p src "
					"0x%08x\n",
					page, block, dest,
					block * blocksize +
					page * pagesize);
			if (page)
				imx_nandboot_get_page(regs, v1, block * blocksize +
					page * pagesize, pagesize_2k);

			page++;

			__memcpy32(dest, base, pagesize);
			dest += pagesize;
			size -= pagesize;

			if (size <= 0)
				return;
		}
		block++;
	}
}

void BARE_INIT_FUNCTION(imx25_nand_load_image)(void)
{
	void *sdram = (void *)MX25_CSD0_BASE_ADDR;
	void __iomem *nfc_base = IOMEM(MX25_NFC_BASE_ADDR);
	bool pagesize_2k;

	if (readl(MX25_CCM_BASE_ADDR + MX25_CCM_RCSR) & (1 << 8))
		pagesize_2k = true;
	else
		pagesize_2k = false;

	imx_nand_load_image(sdram, 0, nfc_base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx27_nand_load_image)(void)
{
	void *sdram = (void *)MX27_CSD0_BASE_ADDR;
	void __iomem *nfc_base = IOMEM(MX27_NFC_BASE_ADDR);
	bool pagesize_2k;

	if (readl(MX27_SYSCTRL_BASE_ADDR + 0x14) & (1 << 5))
		pagesize_2k = true;
	else
		pagesize_2k = false;

	imx_nand_load_image(sdram, 1, nfc_base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx31_nand_load_image)(void)
{
	void *sdram = (void *)MX31_CSD0_BASE_ADDR;
	void __iomem *nfc_base = IOMEM(MX31_NFC_BASE_ADDR);
	bool pagesize_2k;

	if (readl(MX31_CCM_BASE_ADDR + MX31_CCM_RCSR) & MX31_RCSR_NFMS)
		pagesize_2k = true;
	else
		pagesize_2k = false;

	imx_nand_load_image(sdram, 1, nfc_base, pagesize_2k);
}

void BARE_INIT_FUNCTION(imx35_nand_load_image)(void)
{
	void *sdram = (void *)MX35_CSD0_BASE_ADDR;
	void __iomem *nfc_base = IOMEM(MX35_NFC_BASE_ADDR);
	bool pagesize_2k;

	if (readl(MX35_CCM_BASE_ADDR + MX35_CCM_RCSR) & (1 << 8))
		pagesize_2k = true;
	else
		pagesize_2k = false;

	imx_nand_load_image(sdram, 0, nfc_base, pagesize_2k);
}

/*
 * relocate_to_sdram - move ourselves out of NFC SRAM
 *
 * @nfc_base: base address of the NFC controller
 * @sdram: SDRAM base address where we move ourselves to
 * @fn: Function we continue with when running in SDRAM
 *
 * This function moves ourselves out of NFC SRAM to SDRAM. In case we a currently
 * not running in NFC SRAM this function returns. If running in NFC SRAM, this
 * function will not return, but call @fn instead.
 */
static void BARE_INIT_FUNCTION(relocate_to_sdram)(unsigned long nfc_base,
						  unsigned long sdram,
						  void __noreturn (*fn)(void))
{
	unsigned long __fn;
	u32 r;
	u32 *src, *trg;
	int i;

	/* skip NAND boot if not running from NFC space */
	r = get_pc();
	if (r < nfc_base || r > nfc_base + 0x800)
		return;

	src = (unsigned int *)nfc_base;
	trg = (unsigned int *)sdram;

	/*
	 * Copy initial binary portion from NFC SRAM to beginning of
	 * SDRAM
	 */
	for (i = 0; i < 0x800 / sizeof(int); i++)
		*trg++ = *src++;

	/* The next function we jump to */
	__fn = (unsigned long)fn;
	/* mask out TEXT_BASE */
	__fn &= 0x7ff;
	/*
	 * and add sdram base instead where we copied the initial
	 * binary above
	 */
	__fn += sdram;

	fn = (void *)__fn;

	fn();
}

void BARE_INIT_FUNCTION(imx25_nand_relocate_to_sdram)(void __noreturn (*fn)(void))
{
	unsigned long nfc_base = MX25_NFC_BASE_ADDR;
	unsigned long sdram = MX25_CSD0_BASE_ADDR;

	relocate_to_sdram(nfc_base, sdram, fn);
}

static void __noreturn BARE_INIT_FUNCTION(imx25_boot_nand_external_cont)(void)
{
	imx25_nand_load_image();
	imx25_barebox_entry(NULL);
}

void __noreturn BARE_INIT_FUNCTION(imx25_barebox_boot_nand_external)(void)
{
	imx25_nand_relocate_to_sdram(imx25_boot_nand_external_cont);
	imx25_barebox_entry(NULL);
}

void BARE_INIT_FUNCTION(imx27_nand_relocate_to_sdram)(void __noreturn (*fn)(void))
{
	unsigned long nfc_base = MX27_NFC_BASE_ADDR;
	unsigned long sdram = MX27_CSD0_BASE_ADDR;

	relocate_to_sdram(nfc_base, sdram, fn);
}

static void __noreturn BARE_INIT_FUNCTION(imx27_boot_nand_external_cont)(void)
{
	imx27_nand_load_image();
	imx27_barebox_entry(NULL);
}

void __noreturn BARE_INIT_FUNCTION(imx27_barebox_boot_nand_external)(void)
{
	imx27_nand_relocate_to_sdram(imx27_boot_nand_external_cont);
	imx27_barebox_entry(NULL);
}

void BARE_INIT_FUNCTION(imx31_nand_relocate_to_sdram)(void __noreturn (*fn)(void))
{
	unsigned long nfc_base = MX31_NFC_BASE_ADDR;
	unsigned long sdram = MX31_CSD0_BASE_ADDR;

	relocate_to_sdram(nfc_base, sdram, fn);
}

static void __noreturn BARE_INIT_FUNCTION(imx31_boot_nand_external_cont)(void)
{
	imx31_nand_load_image();
	imx31_barebox_entry(NULL);
}

void __noreturn BARE_INIT_FUNCTION(imx31_barebox_boot_nand_external)(void)
{
	imx31_nand_relocate_to_sdram(imx31_boot_nand_external_cont);
	imx31_barebox_entry(NULL);
}

void BARE_INIT_FUNCTION(imx35_nand_relocate_to_sdram)(void __noreturn (*fn)(void))
{
	unsigned long nfc_base = MX35_NFC_BASE_ADDR;
	unsigned long sdram = MX35_CSD0_BASE_ADDR;

	relocate_to_sdram(nfc_base, sdram, fn);
}

static void __noreturn BARE_INIT_FUNCTION(imx35_boot_nand_external_cont)(void)
{
	imx35_nand_load_image();
	imx35_barebox_entry(NULL);
}

void __noreturn BARE_INIT_FUNCTION(imx35_barebox_boot_nand_external)(void)
{
	imx35_nand_relocate_to_sdram(imx35_boot_nand_external_cont);
	imx35_barebox_entry(NULL);
}
