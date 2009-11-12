/*
 * Copyright (C) 2007, Guennadi Liakhovetski <lg@denx.de>
 *
 * (C) Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the MX31ADS Freescale board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/mx35.h>

/* High Level Configuration Options */
#define CONFIG_ARM1136		1	/* This is an arm1136 CPU core */
#define CONFIG_MXC		1
#define CONFIG_MX35		1	/* in a mx31 */
#define CONFIG_MX35_HCLK_FREQ	24000000	/* RedBoot says 26MHz */
#define CONFIG_MX35_CLK32	32768

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define BOARD_LATE_INIT
/*
 * Disabled for now due to build problems under Debian and a significant increase
 * in the final file size: 144260 vs. 109536 Bytes.
 */
#if 0
#define CONFIG_OF_LIBFDT		1
#define CONFIG_FIT			1
#define CONFIG_FIT_VERBOSE		1
#endif

#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_REVISION_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1

/*
 * Size of malloc() pool
 */
#define CFG_MALLOC_LEN		(CFG_ENV_SIZE + 512 * 1024)
#define CFG_GBL_DATA_SIZE	128/* size in bytes reserved for initial data */

/*
 * Hardware drivers
 */
#define CONFIG_HARD_I2C		1
#define CONFIG_I2C_MXC		1
#define CFG_I2C_PORT		I2C_BASE_ADDR
#define CFG_I2C_SPEED		100000
#define CFG_I2C_SLAVE		0xfe

#define CONFIG_MX35_UART	1
#define CFG_MX35_UART1		1

#define CONFIG_MMC		1
#define CONFIG_DOS_PARTITION	1
#define CONFIG_CMD_FAT		1

#ifdef CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_FLASH_HEADER
#define FHEADER_OFFSET	0x400
#endif

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX	1
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{9600, 19200, 38400, 57600, 115200}

#define CFG_MMC_BASE		0x0

/***********************************************************
 * Command definition
 ***********************************************************/

#include <config_cmd_default.h>

#define CONFIG_CMD_PING
#define CONFIG_CMD_DHCP
/*#define CONFIG_CMD_SPI*/
/*#define CONFIG_CMD_DATE*/
#define CONFIG_CMD_NAND

#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII

#define CONFIG_BOOTDELAY	3

#define CONFIG_LOADADDR		0x80800000	/* loadaddr env var */

#define	CONFIG_EXTRA_ENV_SETTINGS					\
		"netdev=eth0\0"						\
		"ethprime=smc911x\0"					\
		"uboot_addr=0xa0000000\0"				\
		"uboot=u-boot.bin\0"			\
		"kernel=uImage\0"				\
		"nfsroot=/opt/eldk/arm\0"				\
		"bootargs_base=setenv bootargs console=ttymxc0,115200\0"\
		"bootargs_nfs=setenv bootargs ${bootargs} root=/dev/nfs "\
			"ip=dhcp nfsroot=${serverip}:${nfsroot},v3,tcp\0"\
		"bootcmd=run bootcmd_net\0"				\
		"bootcmd_net=run bootargs_base bootargs_nfs; "		\
			"tftpboot ${loadaddr} ${kernel}; bootm\0"	\
		"prg_uboot=tftpboot ${loadaddr} ${uboot}; "		\
			"protect off ${uboot_addr} 0xa003ffff; "	\
			"erase ${uboot_addr} 0xa003ffff; "		\
			"cp.b ${loadaddr} ${uboot_addr} ${filesize}; "	\
			"setenv filesize; saveenv\0"

/*Support LAN9217*/
#define CONFIG_DRIVER_SMC911X	1
#define CONFIG_DRIVER_SMC911X_16_BIT 1
#define CONFIG_DRIVER_SMC911X_BASE CS5_BASE_ADDR

#define CONFIG_HAS_ETH1
#define CONFIG_NET_MULTI 1
#define CONFIG_MXC_FEC
#define CONFIG_MII
#define CFG_DISCOVER_PHY

#define CFG_FEC0_IOBASE FEC_BASE_ADDR
#define CFG_FEC0_PINMUX	-1
#define CFG_FEC0_PHY_ADDR	0x1F
#define CFG_FEC0_MIIBASE -1

/*
#define NAND_MAX_CHIPS                1
#define CFG_NAND_BASE         (NFC_BASE_ADDR + 0x1E00)
#define CFG_MAX_NAND_DEVICE 1
*/

/*
 * The MX31ADS board seems to have a hardware "peculiarity" confirmed under
 * U-Boot, RedBoot and Linux: the ethernet Rx signal is reaching the CS8900A
 * controller inverted. The controller is capable of detecting and correcting
 * this, but it needs 4 network packets for that. Which means, at startup, you
 * will not receive answers to the first 4 packest, unless there have been some
 * broadcasts on the network, or your board is on a hub. Reducing the ARP
 * timeout from default 5 seconds to 200ms we speed up the initial TFTP
 * transfer, should the user wish one, significantly.
 */
#define CONFIG_ARP_TIMEOUT	200UL

/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP		/* undef to save memory */
#define CFG_PROMPT		"=> "
#define CFG_CBSIZE		256	/* Console I/O Buffer Size */
/* Print Buffer Size */
#define CFG_PBSIZE		(CFG_CBSIZE + sizeof(CFG_PROMPT) + 16)
#define CFG_MAXARGS		16	/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size */

#define CFG_MEMTEST_START	0	/* memtest works on */
#define CFG_MEMTEST_END		0x10000

#undef	CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CFG_LOAD_ADDR		CONFIG_LOADADDR

#define CFG_HZ			CONFIG_MX35_CLK32/* use 32kHz clock as source */

#define CONFIG_CMDLINE_EDITING	1

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 * 1024)	/* regular stack */

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CSD0_BASE_ADDR
#define PHYS_SDRAM_1_SIZE	(128 * 1024 * 1024)

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
#define CFG_FLASH_BASE		CS0_BASE_ADDR
#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CFG_MAX_FLASH_SECT	512	/* max number of sectors on one chip */
/* Monitor at beginning of flash */
#define CFG_MONITOR_BASE	CFG_FLASH_BASE
#define CFG_MONITOR_LEN		(512 * 1024)	/* Reserve 256KiB */

#define	CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_SECT_SIZE	(128 * 1024)
#define CFG_ENV_SIZE		CFG_ENV_SECT_SIZE

/* Address and size of Redundant Environment Sector	*/
#define CFG_ENV_OFFSET_REDUND	(CFG_ENV_OFFSET + CFG_ENV_SIZE)
#define CFG_ENV_SIZE_REDUND	CFG_ENV_SIZE

/*
 * S29WS256N NOR flash has 4 32KiB small sectors at the beginning and at the
 * end. The rest of 32MiB is in 128KiB big sectors. U-Boot occupies the low
 * 4 sectors, if we put environment next to it, we will have to occupy 128KiB
 * for it. Putting it at the top of flash we use only 32KiB.
 */
#define CFG_ENV_ADDR		(CFG_MONITOR_BASE + CFG_ENV_SECT_SIZE)

/*-----------------------------------------------------------------------
 * CFI FLASH driver setup
 */
#define CFG_FLASH_CFI			1/* Flash memory is CFI compliant */
#define CFG_FLASH_CFI_DRIVER		1/* Use drivers/cfi_flash.c */
/* A non-standard buffered write algorithm */
#define CONFIG_FLASH_SPANSION_S29WS_N	1
#define CFG_FLASH_USE_BUFFER_WRITE	1/* Use buffered writes (~10x faster) */
#define CFG_FLASH_PROTECTION		1/* Use hardware sector protection */

/*-----------------------------------------------------------------------
 * NAND FLASH driver setup
 */
#define NAND_MAX_CHIPS         1
#define CFG_MAX_NAND_DEVICE    1
#define CFG_NAND_BASE          0x40000000
/*
 * JFFS2 partitions
 */
#undef CONFIG_JFFS2_CMDLINE
#define CONFIG_JFFS2_DEV	"nor0"

#endif				/* __CONFIG_H */
