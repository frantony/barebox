/*
 * Copyright (C) 2000, 2004, 2005  MIPS Technologies, Inc.
 *	All rights reserved.
 *	Authors: Carsten Langgaard <carstenl@mips.com>
 *		 Maciej W. Rozycki <macro@mips.com>
 * Copyright (C) 2005 Ralf Baechle (ralf@linux-mips.org)
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 */
#ifndef _ASM_GT64120_H
#define _ASM_GT64120_H

#define GT_DEF_BASE		0x14000000

#define MSK(n)			((1 << (n)) - 1)

/*
 *  Register offset addresses
 */

/* CPU Address Decode.  */
#define GT_PCI0IOLD_OFS		0x048
#define GT_PCI0IOHD_OFS		0x050
#define GT_PCI0M0LD_OFS		0x058
#define GT_PCI0M0HD_OFS		0x060
#define GT_ISD_OFS		0x068

#define GT_PCI0M1LD_OFS		0x080
#define GT_PCI0M1HD_OFS		0x088
#define GT_PCI1IOLD_OFS		0x090
#define GT_PCI1IOHD_OFS		0x098
#define GT_PCI1M0LD_OFS		0x0a0
#define GT_PCI1M0HD_OFS		0x0a8
#define GT_PCI1M1LD_OFS		0x0b0
#define GT_PCI1M1HD_OFS		0x0b8
#define GT_PCI1M1LD_OFS		0x0b0
#define GT_PCI1M1HD_OFS		0x0b8

#define GT_PCI0IOREMAP_OFS	0x0f0
#define GT_PCI0M0REMAP_OFS	0x0f8
#define GT_PCI0M1REMAP_OFS	0x100
#define GT_PCI1IOREMAP_OFS	0x108
#define GT_PCI1M0REMAP_OFS	0x110
#define GT_PCI1M1REMAP_OFS	0x118

/* PCI Internal.  */
#define GT_PCI0_CMD_OFS		0xc00
#define GT_PCI0_TOR_OFS		0xc04
#define GT_PCI0_BS_SCS10_OFS	0xc08
#define GT_PCI0_BS_SCS32_OFS	0xc0c
#define GT_PCI0_BS_CS20_OFS	0xc10
#define GT_PCI0_BS_CS3BT_OFS	0xc14

#define GT_PCI1_IACK_OFS	0xc30
#define GT_PCI0_IACK_OFS	0xc34

#define GT_PCI0_BARE_OFS	0xc3c
#define GT_PCI0_PREFMBR_OFS	0xc40

#define GT_PCI0_SCS10_BAR_OFS	0xc48
#define GT_PCI0_SCS32_BAR_OFS	0xc4c
#define GT_PCI0_CS20_BAR_OFS	0xc50
#define GT_PCI0_CS3BT_BAR_OFS	0xc54
#define GT_PCI0_SSCS10_BAR_OFS	0xc58
#define GT_PCI0_SSCS32_BAR_OFS	0xc5c

#define GT_PCI0_SCS3BT_BAR_OFS	0xc64

#define GT_PCI1_CMD_OFS		0xc80
#define GT_PCI1_TOR_OFS		0xc84
#define GT_PCI1_BS_SCS10_OFS	0xc88
#define GT_PCI1_BS_SCS32_OFS	0xc8c
#define GT_PCI1_BS_CS20_OFS	0xc90
#define GT_PCI1_BS_CS3BT_OFS	0xc94

#define GT_PCI1_BARE_OFS	0xcbc
#define GT_PCI1_PREFMBR_OFS	0xcc0

#define GT_PCI1_SCS10_BAR_OFS	0xcc8
#define GT_PCI1_SCS32_BAR_OFS	0xccc
#define GT_PCI1_CS20_BAR_OFS	0xcd0
#define GT_PCI1_CS3BT_BAR_OFS	0xcd4
#define GT_PCI1_SSCS10_BAR_OFS	0xcd8
#define GT_PCI1_SSCS32_BAR_OFS	0xcdc

#define GT_PCI1_SCS3BT_BAR_OFS	0xce4

#define GT_PCI1_CFGADDR_OFS	0xcf0
#define GT_PCI1_CFGDATA_OFS	0xcf4
#define GT_PCI0_CFGADDR_OFS	0xcf8
#define GT_PCI0_CFGDATA_OFS	0xcfc

/* Interrupts.  */
#define GT_INTRCAUSE_OFS	0xc18
#define GT_INTRMASK_OFS		0xc1c

#define GT_PCI0_ICMASK_OFS	0xc24
#define GT_PCI0_SERR0MASK_OFS	0xc28

#define GT_CPU_INTSEL_OFS	0xc70
#define GT_PCI0_INTSEL_OFS	0xc74

#define GT_HINTRCAUSE_OFS	0xc98
#define GT_HINTRMASK_OFS	0xc9c

#define GT_PCI0_HICMASK_OFS	0xca4
#define GT_PCI1_SERR1MASK_OFS	0xca8

/*
 *  Register encodings
 */
#define GT_CPU_ENDIAN_SHF	12
#define GT_CPU_ENDIAN_MSK	(MSK(1) << GT_CPU_ENDIAN_SHF)
#define GT_CPU_ENDIAN_BIT	GT_CPU_ENDIAN_MSK
#define GT_CPU_WR_SHF		16
#define GT_CPU_WR_MSK		(MSK(1) << GT_CPU_WR_SHF)
#define GT_CPU_WR_BIT		GT_CPU_WR_MSK
#define GT_CPU_WR_DXDXDXDX	0
#define GT_CPU_WR_DDDD		1

#define GT_PCI_DCRM_SHF		21
#define GT_PCI_LD_SHF		0
#define GT_PCI_LD_MSK		(MSK(15) << GT_PCI_LD_SHF)
#define GT_PCI_HD_SHF		0
#define GT_PCI_HD_MSK		(MSK(7) << GT_PCI_HD_SHF)
#define GT_PCI_REMAP_SHF	0
#define GT_PCI_REMAP_MSK	(MSK(11) << GT_PCI_REMAP_SHF)

#define GT_CFGADDR_CFGEN_SHF	31
#define GT_CFGADDR_CFGEN_MSK	(MSK(1) << GT_CFGADDR_CFGEN_SHF)
#define GT_CFGADDR_CFGEN_BIT	GT_CFGADDR_CFGEN_MSK

#define GT_CFGADDR_BUSNUM_SHF	16
#define GT_CFGADDR_BUSNUM_MSK	(MSK(8) << GT_CFGADDR_BUSNUM_SHF)

#define GT_CFGADDR_DEVNUM_SHF	11
#define GT_CFGADDR_DEVNUM_MSK	(MSK(5) << GT_CFGADDR_DEVNUM_SHF)

#define GT_CFGADDR_FUNCNUM_SHF	8
#define GT_CFGADDR_FUNCNUM_MSK	(MSK(3) << GT_CFGADDR_FUNCNUM_SHF)

#define GT_CFGADDR_REGNUM_SHF	2
#define GT_CFGADDR_REGNUM_MSK	(MSK(6) << GT_CFGADDR_REGNUM_SHF)

#define GT_INTRCAUSE_MASABORT0_SHF	18
#define GT_INTRCAUSE_MASABORT0_MSK	(MSK(1) << GT_INTRCAUSE_MASABORT0_SHF)
#define GT_INTRCAUSE_MASABORT0_BIT	GT_INTRCAUSE_MASABORT0_MSK

#define GT_INTRCAUSE_TARABORT0_SHF	19
#define GT_INTRCAUSE_TARABORT0_MSK	(MSK(1) << GT_INTRCAUSE_TARABORT0_SHF)
#define GT_INTRCAUSE_TARABORT0_BIT	GT_INTRCAUSE_TARABORT0_MSK

#define GT_PCI0_CFGADDR_REGNUM_SHF	2
#define GT_PCI0_CFGADDR_REGNUM_MSK	(MSK(6) << GT_PCI0_CFGADDR_REGNUM_SHF)
#define GT_PCI0_CFGADDR_FUNCTNUM_SHF	8
#define GT_PCI0_CFGADDR_FUNCTNUM_MSK	(MSK(3) << GT_PCI0_CFGADDR_FUNCTNUM_SHF)
#define GT_PCI0_CFGADDR_DEVNUM_SHF	11
#define GT_PCI0_CFGADDR_DEVNUM_MSK	(MSK(5) << GT_PCI0_CFGADDR_DEVNUM_SHF)
#define GT_PCI0_CFGADDR_BUSNUM_SHF	16
#define GT_PCI0_CFGADDR_BUSNUM_MSK	(MSK(8) << GT_PCI0_CFGADDR_BUSNUM_SHF)
#define GT_PCI0_CFGADDR_CONFIGEN_SHF	31
#define GT_PCI0_CFGADDR_CONFIGEN_MSK	(MSK(1) << GT_PCI0_CFGADDR_CONFIGEN_SHF)
#define GT_PCI0_CFGADDR_CONFIGEN_BIT	GT_PCI0_CFGADDR_CONFIGEN_MSK

#define GT_PCI0_CMD_MBYTESWAP_SHF	0
#define GT_PCI0_CMD_MBYTESWAP_MSK	(MSK(1) << GT_PCI0_CMD_MBYTESWAP_SHF)
#define GT_PCI0_CMD_MBYTESWAP_BIT	GT_PCI0_CMD_MBYTESWAP_MSK
#define GT_PCI0_CMD_MWORDSWAP_SHF	10
#define GT_PCI0_CMD_MWORDSWAP_MSK	(MSK(1) << GT_PCI0_CMD_MWORDSWAP_SHF)
#define GT_PCI0_CMD_MWORDSWAP_BIT	GT_PCI0_CMD_MWORDSWAP_MSK
#define GT_PCI0_CMD_SBYTESWAP_SHF	16
#define GT_PCI0_CMD_SBYTESWAP_MSK	(MSK(1) << GT_PCI0_CMD_SBYTESWAP_SHF)
#define GT_PCI0_CMD_SBYTESWAP_BIT	GT_PCI0_CMD_SBYTESWAP_MSK
#define GT_PCI0_CMD_SWORDSWAP_SHF	11
#define GT_PCI0_CMD_SWORDSWAP_MSK	(MSK(1) << GT_PCI0_CMD_SWORDSWAP_SHF)
#define GT_PCI0_CMD_SWORDSWAP_BIT	GT_PCI0_CMD_SWORDSWAP_MSK

/*
 *  Misc
 */
#define GT_DEF_PCI0_IO_BASE	0x10000000UL
#define GT_DEF_PCI0_IO_SIZE	0x02000000UL
#define GT_DEF_PCI0_MEM0_BASE	0x12000000UL
#define GT_DEF_PCI0_MEM0_SIZE	0x02000000UL

#define GT_MAX_BANKSIZE		(256 * 1024 * 1024)	/* Max 256MB bank  */
#define GT_LATTIM_MIN		6			/* Minimum lat  */

/*
 * The gt64120_dep.h file must define the following macros
 *
 *   GT_READ(ofs, data_pointer)
 *   GT_WRITE(ofs, data)           - read/write GT64120 registers in 32bit
 *
 *   TIMER 	- gt64120 timer irq, temporary solution until
 *		  full gt64120 cascade interrupt support is in place
 */

/*
 * Because of an error/peculiarity in the Galileo chip, we need to swap the
 * bytes when running bigendian.  We also provide non-swapping versions.
 */
#define __GT_READ(ofs)							\
	(*(volatile u32 *)(GT64120_BASE+(ofs)))
#define __GT_WRITE(ofs, data)						\
	do { *(volatile u32 *)(GT64120_BASE+(ofs)) = (data); } while (0)
#define GT_READ(ofs)		le32_to_cpu(__GT_READ(ofs))
#define GT_WRITE(ofs, data)	__GT_WRITE(ofs, cpu_to_le32(data))

#endif /* _ASM_GT64120_H */
