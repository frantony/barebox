/*
 *  Atheros AR71xx built-in ethernet mac driver
 *
 *  Copyright (C) 2008-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Based on Atheros' AG7100 driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#ifndef __AG71XX_H
#define __AG71XX_H

#include <linux/types.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <of.h>
#include <of_net.h>
#include <of_address.h>

#include <linux/bitops.h>

#include <mach/ar71xx_regs.h>
#include <mach/ath79.h>

#define AG71XX_DRV_NAME		"ag71xx-gmac"
#define AG71XX_DRV_VERSION	"0.5.35"

#define MAX_WAIT        1000

#define AG71XX_NAPI_WEIGHT	64
#define AG71XX_OOM_REFILL	(1 + HZ/10)

#define AG71XX_INT_ERR	(AG71XX_INT_RX_BE | AG71XX_INT_TX_BE)
#define AG71XX_INT_TX	(AG71XX_INT_TX_PS)
#define AG71XX_INT_RX	(AG71XX_INT_RX_PR | AG71XX_INT_RX_OF)

#define AG71XX_INT_POLL	(AG71XX_INT_RX | AG71XX_INT_TX)
#define AG71XX_INT_INIT	(AG71XX_INT_ERR | AG71XX_INT_POLL)

#define AG71XX_TX_MTU_LEN	1540

#define AG71XX_TX_RING_SPLIT		512
#define AG71XX_TX_RING_DS_PER_PKT	DIV_ROUND_UP(AG71XX_TX_MTU_LEN, \
						     AG71XX_TX_RING_SPLIT)
#define AG71XX_TX_RING_SIZE_DEFAULT	48
#define AG71XX_RX_RING_SIZE_DEFAULT	128

#define AG71XX_TX_RING_SIZE_MAX		48
#define AG71XX_RX_RING_SIZE_MAX		128

#ifdef CONFIG_AG71XX_DEBUG
#define DBG(fmt, args...)	pr_debug(fmt, ## args)
#else
#define DBG(fmt, args...)	do {} while (0)
#endif

#define ag71xx_assert(_cond)						\
do {									\
	if (_cond)							\
		break;							\
	printk("%s,%d: assertion failed\n", __FILE__, __LINE__);	\
	BUG();								\
} while (0)

/*
 * h/w descriptor
 */
typedef struct {
    uint32_t    pkt_start_addr;

    uint32_t    is_empty       :  1;
    uint32_t    res1           : 10;
    uint32_t    ftpp_override  :  5;
    uint32_t    res2           :  4;
    uint32_t    pkt_size       : 12;

    uint32_t    next_desc      ;
}ag7240_desc_t;

#define AG71XX_RX_BUFFER_SIZE  128
#define NO_OF_TX_FIFOS  8
#define NO_OF_RX_FIFOS  8
#define TX_RING_SZ (NO_OF_TX_FIFOS * sizeof(ag7240_desc_t))
#define AG71XX_RB_WRAP	(1<<1)
#define AG71XX_RB_OWNER	(1<<0)
#define MAX_RBUFF_SZ	0x600		/* 1518 rounded up */

struct ag71xx {

	struct ag71xx_platform_data	data;
	struct device_d		*dev;
	struct eth_device netdev;
	void __iomem		*regs;
	void __iomem		*regs_gmac;
	struct mii_bus		miibus;

	void *rx_buffer;
	ag7240_desc_t *fifo_tx;
	ag7240_desc_t *fifo_rx;

	int			rx_buffer_size;
	int			rx_ring_size;

	u32            next_tx;
	u32            next_rx;
	u32            link;
	u32            duplex;
	u32            speed;
	u32            mac_unit;
	u32            mac_base;
};

/* Register offsets */
#define AG71XX_REG_MAC_CFG1	0x0000
#define AG71XX_REG_MAC_CFG2	0x0004
#define AG71XX_REG_MAC_IPG	0x0008
#define AG71XX_REG_MAC_HDX	0x000c
#define AG71XX_REG_MAC_MFL	0x0010
#define AG71XX_REG_MII_CFG	0x0020
#define AG71XX_REG_MII_CMD	0x0024
#define AG71XX_REG_MII_ADDR	0x0028
#define AG71XX_REG_MII_CTRL	0x002c
#define AG71XX_REG_MII_STATUS	0x0030
#define AG71XX_REG_MII_IND	0x0034
#define AG71XX_REG_MAC_IFCTL	0x0038
#define AG71XX_REG_MAC_ADDR1	0x0040
#define AG71XX_REG_MAC_ADDR2	0x0044
#define AG71XX_REG_FIFO_CFG0	0x0048
#define AG71XX_REG_FIFO_CFG1	0x004c
#define AG71XX_REG_FIFO_CFG2	0x0050
#define AG71XX_REG_FIFO_CFG3	0x0054
#define AG71XX_REG_FIFO_CFG4	0x0058
#define AG71XX_REG_FIFO_CFG5	0x005c
#define AG71XX_REG_FIFO_RAM0	0x0060
#define AG71XX_REG_FIFO_RAM1	0x0064
#define AG71XX_REG_FIFO_RAM2	0x0068
#define AG71XX_REG_FIFO_RAM3	0x006c
#define AG71XX_REG_FIFO_RAM4	0x0070
#define AG71XX_REG_FIFO_RAM5	0x0074
#define AG71XX_REG_FIFO_RAM6	0x0078
#define AG71XX_REG_FIFO_RAM7	0x007c

#define AG71XX_REG_TX_CTRL	0x0180
#define AG71XX_REG_TX_DESC	0x0184
#define AG71XX_REG_TX_STATUS	0x0188
#define AG71XX_REG_RX_CTRL	0x018c
#define AG71XX_REG_RX_DESC	0x0190
#define AG71XX_REG_RX_STATUS	0x0194
#define AG71XX_REG_INT_ENABLE	0x0198
#define AG71XX_REG_INT_STATUS	0x019c

#define AG71XX_REG_FIFO_DEPTH	0x01a8
#define AG71XX_REG_RX_SM	0x01b0
#define AG71XX_REG_TX_SM	0x01b4

#define MAC_CFG1_TXE		BIT(0)	/* Tx Enable */
#define MAC_CFG1_STX		BIT(1)	/* Synchronize Tx Enable */
#define MAC_CFG1_RXE		BIT(2)	/* Rx Enable */
#define MAC_CFG1_SRX		BIT(3)	/* Synchronize Rx Enable */
#define MAC_CFG1_TFC		BIT(4)	/* Tx Flow Control Enable */
#define MAC_CFG1_RFC		BIT(5)	/* Rx Flow Control Enable */
#define MAC_CFG1_LB		BIT(8)	/* Loopback mode */
#define MAC_CFG1_TX_RST		BIT(18)	/* Tx Reset */
#define MAC_CFG1_RX_RST		BIT(19)	/* Rx Reset */
#define MAC_CFG1_SR		BIT(31)	/* Soft Reset */

#define MAC_CFG2_FDX		BIT(0)
#define MAC_CFG2_CRC_EN		BIT(1)
#define MAC_CFG2_PAD_CRC_EN	BIT(2)
#define MAC_CFG2_LEN_CHECK	BIT(4)
#define MAC_CFG2_HUGE_FRAME_EN	BIT(5)
#define MAC_CFG2_IF_1000	BIT(9)
#define MAC_CFG2_IF_10_100	BIT(8)

#define FIFO_CFG0_WTM		BIT(0)	/* Watermark Module */
#define FIFO_CFG0_RXS		BIT(1)	/* Rx System Module */
#define FIFO_CFG0_RXF		BIT(2)	/* Rx Fabric Module */
#define FIFO_CFG0_TXS		BIT(3)	/* Tx System Module */
#define FIFO_CFG0_TXF		BIT(4)	/* Tx Fabric Module */
#define FIFO_CFG0_ALL	(FIFO_CFG0_WTM | FIFO_CFG0_RXS | FIFO_CFG0_RXF \
			| FIFO_CFG0_TXS | FIFO_CFG0_TXF)

#define FIFO_CFG0_ENABLE_SHIFT	8

#define FIFO_CFG4_DE		BIT(0)	/* Drop Event */
#define FIFO_CFG4_DV		BIT(1)	/* RX_DV Event */
#define FIFO_CFG4_FC		BIT(2)	/* False Carrier */
#define FIFO_CFG4_CE		BIT(3)	/* Code Error */
#define FIFO_CFG4_CR		BIT(4)	/* CRC error */
#define FIFO_CFG4_LM		BIT(5)	/* Length Mismatch */
#define FIFO_CFG4_LO		BIT(6)	/* Length out of range */
#define FIFO_CFG4_OK		BIT(7)	/* Packet is OK */
#define FIFO_CFG4_MC		BIT(8)	/* Multicast Packet */
#define FIFO_CFG4_BC		BIT(9)	/* Broadcast Packet */
#define FIFO_CFG4_DR		BIT(10)	/* Dribble */
#define FIFO_CFG4_LE		BIT(11)	/* Long Event */
#define FIFO_CFG4_CF		BIT(12)	/* Control Frame */
#define FIFO_CFG4_PF		BIT(13)	/* Pause Frame */
#define FIFO_CFG4_UO		BIT(14)	/* Unsupported Opcode */
#define FIFO_CFG4_VT		BIT(15)	/* VLAN tag detected */
#define FIFO_CFG4_FT		BIT(16)	/* Frame Truncated */
#define FIFO_CFG4_UC		BIT(17)	/* Unicast Packet */

#define FIFO_CFG5_DE		BIT(0)	/* Drop Event */
#define FIFO_CFG5_DV		BIT(1)	/* RX_DV Event */
#define FIFO_CFG5_FC		BIT(2)	/* False Carrier */
#define FIFO_CFG5_CE		BIT(3)	/* Code Error */
#define FIFO_CFG5_LM		BIT(4)	/* Length Mismatch */
#define FIFO_CFG5_LO		BIT(5)	/* Length Out of Range */
#define FIFO_CFG5_OK		BIT(6)	/* Packet is OK */
#define FIFO_CFG5_MC		BIT(7)	/* Multicast Packet */
#define FIFO_CFG5_BC		BIT(8)	/* Broadcast Packet */
#define FIFO_CFG5_DR		BIT(9)	/* Dribble */
#define FIFO_CFG5_CF		BIT(10)	/* Control Frame */
#define FIFO_CFG5_PF		BIT(11)	/* Pause Frame */
#define FIFO_CFG5_UO		BIT(12)	/* Unsupported Opcode */
#define FIFO_CFG5_VT		BIT(13)	/* VLAN tag detected */
#define FIFO_CFG5_LE		BIT(14)	/* Long Event */
#define FIFO_CFG5_FT		BIT(15)	/* Frame Truncated */
#define FIFO_CFG5_16		BIT(16)	/* unknown */
#define FIFO_CFG5_17		BIT(17)	/* unknown */
#define FIFO_CFG5_SF		BIT(18)	/* Short Frame */
#define FIFO_CFG5_BM		BIT(19)	/* Byte Mode */

#define AG71XX_INT_TX_PS	BIT(0)
#define AG71XX_INT_TX_UR	BIT(1)
#define AG71XX_INT_TX_BE	BIT(3)
#define AG71XX_INT_RX_PR	BIT(4)
#define AG71XX_INT_RX_OF	BIT(6)
#define AG71XX_INT_RX_BE	BIT(7)

#define MAC_IFCTL_SPEED		BIT(16)

#define MII_CFG_CLK_DIV_4	0
#define MII_CFG_CLK_DIV_6	2
#define MII_CFG_CLK_DIV_8	3
#define MII_CFG_CLK_DIV_10	4
#define MII_CFG_CLK_DIV_14	5
#define MII_CFG_CLK_DIV_20	6
#define MII_CFG_CLK_DIV_28	7
#define MII_CFG_CLK_DIV_34	8
#define MII_CFG_CLK_DIV_42	9
#define MII_CFG_CLK_DIV_50	10
#define MII_CFG_CLK_DIV_58	11
#define MII_CFG_CLK_DIV_66	12
#define MII_CFG_CLK_DIV_74	13
#define MII_CFG_CLK_DIV_82	14
#define MII_CFG_CLK_DIV_98	15
#define MII_CFG_RESET		BIT(31)

#define MII_CMD_WRITE		0x0
#define MII_CMD_READ		0x1
#define MII_ADDR_SHIFT		8
#define MII_IND_BUSY		BIT(0)
#define MII_IND_INVALID		BIT(2)

#define TX_CTRL_TXE		BIT(0)	/* Tx Enable */

#define TX_STATUS_PS		BIT(0)	/* Packet Sent */
#define TX_STATUS_UR		BIT(1)	/* Tx Underrun */
#define TX_STATUS_BE		BIT(3)	/* Bus Error */

#define RX_CTRL_RXE		BIT(0)	/* Rx Enable */

#define RX_STATUS_PR		BIT(0)	/* Packet Received */
#define RX_STATUS_OF		BIT(2)	/* Rx Overflow */
#define RX_STATUS_BE		BIT(3)	/* Bus Error */

/*
 * GMAC register macros
 */
#define AG71XX_ETH_CFG_RGMII_GE0        (1<<0)
#define AG71XX_ETH_CFG_MII_GE0_SLAVE    (1<<4)

static inline void ag71xx_check_reg_offset(struct ag71xx *ag, unsigned reg)
{
	switch (reg) {
	case AG71XX_REG_MAC_CFG1 ... AG71XX_REG_MAC_MFL:
	case AG71XX_REG_MAC_IFCTL ... AG71XX_REG_TX_SM:
	case AG71XX_REG_MII_CFG:
		break;

	default:
		BUG();
	}
}

#if 0
static inline void ag71xx_wr(struct ag71xx *ag, unsigned reg, u32 value)
{
	ag71xx_check_reg_offset(ag, reg);

	__raw_writel(value, ag->mac_base + reg);
	/* flush write */
	(void) __raw_readl(ag->mac_base + reg);
}

static inline u32 ag71xx_rr(struct ag71xx *ag, unsigned reg)
{
	ag71xx_check_reg_offset(ag, reg);

	return __raw_readl(ag->mac_base + reg);
}
#endif

#ifdef CONFIG_AG71XX_AR8216_SUPPORT
void ag71xx_add_ar8216_header(struct ag71xx *ag, struct sk_buff *skb);
int ag71xx_remove_ar8216_header(struct ag71xx *ag, struct sk_buff *skb,
				int pktlen);
static inline int ag71xx_has_ar8216(struct ag71xx *ag)
{
	return ag71xx_get_pdata(ag)->has_ar8216;
}
#else
static inline void ag71xx_add_ar8216_header(struct ag71xx *ag,
					   struct sk_buff *skb)
{
}

static inline int ag71xx_remove_ar8216_header(struct ag71xx *ag,
					      struct sk_buff *skb,
					      int pktlen)
{
	return 0;
}
static inline int ag71xx_has_ar8216(struct ag71xx *ag)
{
	return 0;
}
#endif

#ifdef CONFIG_AG71XX_DEBUG_FS
int ag71xx_debugfs_root_init(void);
void ag71xx_debugfs_root_exit(void);
int ag71xx_debugfs_init(struct ag71xx *ag);
void ag71xx_debugfs_exit(struct ag71xx *ag);
void ag71xx_debugfs_update_int_stats(struct ag71xx *ag, u32 status);
void ag71xx_debugfs_update_napi_stats(struct ag71xx *ag, int rx, int tx);
#else
static inline int ag71xx_debugfs_root_init(void) { return 0; }
static inline void ag71xx_debugfs_root_exit(void) {}
static inline int ag71xx_debugfs_init(struct ag71xx *ag) { return 0; }
static inline void ag71xx_debugfs_exit(struct ag71xx *ag) {}
static inline void ag71xx_debugfs_update_int_stats(struct ag71xx *ag,
						   u32 status) {}
static inline void ag71xx_debugfs_update_napi_stats(struct ag71xx *ag,
						    int rx, int tx) {}
#endif /* CONFIG_AG71XX_DEBUG_FS */

#endif /* _AG71XX_H */
