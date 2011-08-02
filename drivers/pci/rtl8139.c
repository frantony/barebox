#include <common.h>

#include <command.h>
#include <net.h>
#include <miidev.h>
#include <malloc.h>
#include <init.h>
#include <xfuncs.h>
#include <errno.h>
#include <clock.h>
#include <asm/io.h>

#include <linux/pci.h>

#undef RTL8139_DEBUG
#define RTL8139_DEBUG

/*
 * Receive ring size
 * Warning: 64K ring has hardware issues and may lock up.
 */
#define RX_BUF_IDX	0	/* 8K ring */
#define RX_BUF_LEN	(8192 << RX_BUF_IDX)
#define RX_BUF_PAD	16
#define RX_BUF_WRAP_PAD 2048 /* spare padding to handle lack of packet wrap */

#if RX_BUF_LEN == 65536
#define RX_BUF_TOT_LEN	RX_BUF_LEN
#else
#define RX_BUF_TOT_LEN	(RX_BUF_LEN + RX_BUF_PAD + RX_BUF_WRAP_PAD)
#endif

/* Number of Tx descriptor registers. */
#define NUM_TX_DESC	4

/* max supported ethernet frame size -- must be at least (dev->mtu+14+4).*/
#define MAX_ETH_FRAME_SIZE	1536

/* Size of the Tx bounce buffers -- must be at least (dev->mtu+14+4). */
#define TX_BUF_SIZE	MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_LEN	(TX_BUF_SIZE * NUM_TX_DESC)

/* PCI Tuning Parameters
   Threshold is bytes transferred to chip before transmission starts. */
#define TX_FIFO_THRESH 256	/* In bytes, rounded down to 32 byte units. */

/* The following settings are log_2(bytes)-4:  0 == 16 bytes .. 6==1024, 7==end of packet. */
#define RX_FIFO_THRESH	7	/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	7	/* Maximum PCI burst, '6' is 1024 */
#define TX_DMA_BURST	6	/* Maximum PCI burst, '6' is 1024 */
#define TX_RETRY	8	/* 0-15.  retries = 16 + (TX_RETRY * 16) */

struct rtl8139_priv {
	void __iomem		*base;
	struct pci_dev		*pci_dev;
	unsigned char		*rx_ring;
	unsigned int		cur_rx; /* RX buf index of next pkt */
	dma_addr_t		rx_ring_dma;

	u32			rx_config;
	unsigned int		tx_flag;
	unsigned long		cur_tx;
	unsigned long		dirty_tx;
	unsigned char		*tx_buf[NUM_TX_DESC];   /* Tx bounce buffers */
	unsigned char		*tx_bufs;       /* Tx bounce buffer region. */
	dma_addr_t		tx_bufs_dma;

	struct mii_device	miidev;
};

#define ETH_ZLEN        60              /* Min. octets in frame sans FCS */

/* Registers */
#define MAC0		0x00
#define MAR0		0x08
#define TxStatus0	0x10

enum TxStatusBits {
	TxHostOwns	= 0x2000,
	TxUnderrun	= 0x4000,
	TxStatOK	= 0x8000,
	TxOutOfWindow	= 0x20000000,
	TxAborted	= 0x40000000,
	TxCarrierLost	= 0x80000000,
};

#define TxAddr0		0x20
#define RxBuf		0x30
#define ChipCmd		0x37
#define  CmdReset	0x10
#define  CmdRxEnb	0x08
#define  CmdTxEnb	0x04
#define  RxBufEmpty	0x01
#define RxBufPtr	0x38
#define RxBufAddr	0x3A
#define IntrMask	0x3C
#define IntrStatus	0x3E
#define  PCIErr		0x8000
#define  PCSTimeout	0x4000
#define  RxFIFOOver	0x0040
#define  RxUnderrun	0x0020
#define  RxOverflow	0x0010
#define  TxErr		0x0008
#define  TxOK		0x0004
#define  RxErr		0x0002
#define  RxOK		0x0001
#define    RxAckBits	(RxFIFOOver | RxOverflow | RxOK)

#define TxConfig	0x40
/* Bits in TxConfig. */
enum tx_config_bits {
	/* Interframe Gap Time. Only TxIFG96 doesn't violate IEEE 802.3 */
	TxIFGShift	= 24,
	TxIFG84		= (0 << TxIFGShift), /* 8.4us / 840ns (10 / 100Mbps) */
	TxIFG88		= (1 << TxIFGShift), /* 8.8us / 880ns (10 / 100Mbps) */
	TxIFG92		= (2 << TxIFGShift), /* 9.2us / 920ns (10 / 100Mbps) */
	TxIFG96		= (3 << TxIFGShift), /* 9.6us / 960ns (10 / 100Mbps) */

	TxLoopBack	= (1 << 18) | (1 << 17), /* enable loopback test mode */
	TxCRC		= (1 << 16),	/* DISABLE Tx pkt CRC append */
	TxClearAbt	= (1 << 0),	/* Clear abort (WO) */
	TxDMAShift	= 8, /* DMA burst value (0-7) is shifted X many bits */
	TxRetryShift	= 4, /* TXRR value (0-15) is shifted X many bits */

	TxVersionMask	= 0x7C800000, /* mask out version bits 30-26, 23 */
};

#define RxConfig	0x44
	/* rx fifo threshold */
#define  RxCfgFIFOShift	13
#define	 RxCfgFIFONone	(7 << RxCfgFIFOShift)
	/* Max DMA burst */
#define	 RxCfgDMAShift	8
#define	 RxCfgDMAUnlimited (7 << RxCfgDMAShift)
	/* rx ring buffer length */
#define	 RxCfgRcv8K	0
#define	 RxCfgRcv16K	(1 << 11)
#define	 RxCfgRcv32K	(1 << 12)
#define	 RxCfgRcv64K	((1 << 11) | (1 << 12))
	/* Disable packet wrap at end of Rx buffer. (not possible with 64k) */
#define	 RxNoWrap	(1 << 7)
#define	 AcceptErr		0x20
#define	 AcceptRunt		0x10
#define	 AcceptBroadcast	0x08
#define	 AcceptMulticast	0x04
#define	 AcceptMyPhys		0x02
#define	 AcceptAllPhys		0x01

#define RxMissed	0x4C
#define Cfg9346		0x50
#define  Cfg9346_Lock	0x00
#define  Cfg9346_Unlock	0xC0
#define BasicModeCtrl	0x62
#define BasicModeStatus	0x64
#define NWayAdvert	0x66
#define NWayLPAR	0x68
#define NWayExpansion	0x6a

static const char mii_2_8139_map[8] = {
	BasicModeCtrl,
	BasicModeStatus,
	0,
	0,
	NWayAdvert,
	NWayLPAR,
	NWayExpansion,
	0
};

/* write MMIO register */
#define RTL_W8(priv, reg, val)	writeb(val, ((char *)(priv->base) + reg))
#define RTL_W16(priv, reg, val)	writew(val, ((char *)(priv->base) + reg))
#define RTL_W32(priv, reg, val)	writel(val, ((char *)(priv->base) + reg))

/* read MMIO register */
#define RTL_R8(priv, reg)	readb(((char *)(priv->base) + reg))
#define RTL_R16(priv, reg)	readw(((char *)(priv->base) + reg))
#define RTL_R32(priv, reg)	readl(((char *)(priv->base) + reg))

/* write MMIO register, with flush */
/* Flush avoids rtl8139 bug w/ posted MMIO writes */
static inline void RTL_W8_F(struct rtl8139_priv *priv, int reg, int val)
{
	RTL_W8(priv, reg, val);
	RTL_R8(priv, reg);
}

static inline void RTL_W16_F(struct rtl8139_priv *priv, int reg, int val)
{
	RTL_W16(priv, reg, val);
	RTL_R16(priv, reg);
}

static inline void RTL_W32_F(struct rtl8139_priv *priv, int reg, int val)
{
	RTL_W32(priv, reg, val);
	RTL_R32(priv, reg);
}

static const unsigned int rtl8139_rx_config =
	RxCfgRcv8K | RxNoWrap |
	(RX_FIFO_THRESH << RxCfgFIFOShift) |
	(RX_DMA_BURST << RxCfgDMAShift);

static const unsigned int rtl8139_tx_config =
	TxIFG96 | (TX_DMA_BURST << TxDMAShift) | (TX_RETRY << TxRetryShift);

static void rtl8139_chip_reset(struct rtl8139_priv *priv)
{
	int i;

#ifdef RTL8139_DEBUG
	printf("rtl8139_chip_reset()\n");
#endif

	/* Soft reset the chip. */
	RTL_W8(priv, ChipCmd, CmdReset);

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--) {
		//barrier();
		if ((RTL_R8(priv, ChipCmd) & CmdReset) == 0)
			break;
		udelay(10);
	}
}

static void __set_rx_mode(struct rtl8139_priv *priv)
{
	u32 mc_filter[2];	/* Multicast hash filter */
	int rx_mode;
	u32 tmp;

	rx_mode =
	    AcceptBroadcast | AcceptMulticast | AcceptMyPhys |
	    AcceptAllPhys;
	mc_filter[1] = mc_filter[0] = 0xffffffff;

	/* We can safely update without stopping the chip. */
	tmp = rtl8139_rx_config | rx_mode;
	if (priv->rx_config != tmp) {
		RTL_W32_F(priv, RxConfig, tmp);
		priv->rx_config = tmp;
	}

	RTL_W32_F(priv, MAR0 + 0, mc_filter[0]);
	RTL_W32_F(priv, MAR0 + 4, mc_filter[1]);
}

/* Start the hardware at open or resume. */
static void rtl8139_hw_start(struct rtl8139_priv *priv)
{
	u32 i;
	u8 tmp;

	rtl8139_chip_reset(priv);

	/* unlock Config[01234] and BMCR register writes */
	RTL_W8_F(priv, Cfg9346, Cfg9346_Unlock);

#if 0
	/* Restore our idea of the MAC address. */
	RTL_W32_F(priv, MAC0 + 0, *(__le32 *) (dev->dev_addr + 0));
	RTL_W32_F(priv, MAC0 + 4, *(__le16 *) (dev->dev_addr + 4));
#endif

	priv->cur_rx = 0;

	/* init Rx ring buffer DMA address */
	RTL_W32_F(priv, RxBuf, priv->rx_ring_dma);

	/* Must enable Tx/Rx before setting transfer thresholds! */
	RTL_W8(priv, ChipCmd, CmdRxEnb | CmdTxEnb);

	priv->rx_config = rtl8139_rx_config | AcceptBroadcast | AcceptMyPhys;
	RTL_W32(priv, RxConfig, priv->rx_config);
	RTL_W32(priv, TxConfig, rtl8139_tx_config);

	/* Lock Config[01234] and BMCR register writes */
	RTL_W8(priv, Cfg9346, Cfg9346_Lock);

	/* init Tx buffer DMA addresses */
	for (i = 0; i < NUM_TX_DESC; i++)
		RTL_W32_F(priv, TxAddr0 + (i * 4), priv->tx_bufs_dma + (priv->tx_buf[i] - priv->tx_bufs));

	RTL_W32(priv, RxMissed, 0);

	__set_rx_mode(priv);

	/* Disable interrupts by clearing the interrupt mask. */
	RTL_W16(priv, IntrMask, 0);

	/* make sure RxTx has started */
	tmp = RTL_R8(priv, ChipCmd);
	if ((!(tmp & CmdRxEnb)) || (!(tmp & CmdTxEnb)))
		RTL_W8(priv, ChipCmd, CmdRxEnb | CmdTxEnb);
}

static inline void rtl8139_tx_clear(struct rtl8139_priv *priv)
{
	priv->cur_tx = 0;
	priv->dirty_tx = 0;

	/* XXX account for unsent Tx packets in tp->stats.tx_dropped */
}

/* Initialize the Rx and Tx rings, along with various 'dev' bits. */
static void rtl8139_init_ring(struct rtl8139_priv *priv)
{
	int i;

	priv->cur_rx = 0;
	priv->cur_tx = 0;
	priv->dirty_tx = 0;

	for (i = 0; i < NUM_TX_DESC; i++)
		priv->tx_buf[i] = &priv->tx_bufs[i * TX_BUF_SIZE];
}

static int rtl8139_phy_read(struct mii_device *mdev, int phy_addr, int reg)
{
	struct eth_device *edev = mdev->edev;
	struct rtl8139_priv *priv = edev->priv;
	int val;

	val = 0;

	if (phy_addr == 0) { /* Really a 8139. Use internal registers. */
		val = reg < 8 && mii_2_8139_map[reg] ?
				RTL_R16(priv, mii_2_8139_map[reg]) : 0;
	}

#ifdef RTL8139_DEBUG
	printf("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n", __func__,
			phy_addr, reg, val);
#endif

	return val;
}

static int rtl8139_phy_write(struct mii_device *mdev, int phy_addr,
	int reg, int val)
{
	struct eth_device *edev = mdev->edev;
	struct rtl8139_priv *priv = edev->priv;

#ifdef RTL8139_DEBUG
	printf("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n", __func__,
	      phy_addr, reg, val);
#endif

	if (phy_addr == 0) { /* Really a 8139. Use internal registers. */
		if (reg == 0) {
			RTL_W8(priv, Cfg9346, Cfg9346_Unlock);
			RTL_W16(priv, BasicModeCtrl, val);
			RTL_W8(priv, Cfg9346, Cfg9346_Lock);
		} else if (reg < 8 && mii_2_8139_map[reg]) {
			RTL_W16(priv, mii_2_8139_map[reg], val);
		}
	}

	return 0;
}

static int rtl8139_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct rtl8139_priv *priv = edev->priv;
	int i;

	for (i = 0; i < 6; ++i) {
		m[i] = RTL_R8(priv, (MAC0 + i));
	}

	return 0;
}

static int rtl8139_set_ethaddr(struct eth_device *edev,
					unsigned char *mac_addr)
{
	struct rtl8139_priv *priv = edev->priv;
	int i;

	RTL_W8(priv, Cfg9346, Cfg9346_Unlock);

	for (i = 0; i < 6; i++) {
		RTL_W8(priv, (MAC0 + i), mac_addr[i]);
		RTL_R8(priv, mac_addr[i]);
	}

	RTL_W8(priv, Cfg9346, Cfg9346_Lock);

	return 0;
}

static int rtl8139_init_dev(struct eth_device *edev)
{
	struct rtl8139_priv *priv = edev->priv;

#ifdef RTL8139_DEBUG
	printf("rtl8139_init_dev()\n");
#endif

	rtl8139_chip_reset(priv);

	//miidev_restart_aneg(&priv->miidev);

	return 0;
}

static int rtl8139_eth_open(struct eth_device *edev)
{
	struct rtl8139_priv *priv = edev->priv;

	/* HACK: FIXME */
	priv->tx_bufs = (unsigned char *)0xa1000000;
	priv->tx_bufs_dma = (dma_addr_t)0x01000000;

	priv->rx_ring = (unsigned char *)0xa2000000;
	priv->rx_ring_dma = (dma_addr_t)0x02000000;

	priv->tx_flag = (TX_FIFO_THRESH << 11) & 0x003f0000;

	rtl8139_init_ring(priv);
	rtl8139_hw_start(priv);

	miidev_wait_aneg(&priv->miidev);
	miidev_print_status(&priv->miidev);

	return 0;
}

static void rtl8139_eth_halt(struct eth_device *edev)
{
	struct rtl8139_priv *priv = edev->priv;

	/* Stop the chip's Tx and Rx DMA processes. */
	RTL_W8(priv, ChipCmd, 0);

	/* Disable interrupts by clearing the interrupt mask. */
	RTL_W16(priv, IntrMask, 0);

	/* Green! Put the chip in low-power mode. */
	RTL_W8(priv, Cfg9346, Cfg9346_Unlock);
}

static void rtl8139_tx_interrupt(struct rtl8139_priv *priv)
{
	unsigned long dirty_tx, tx_left;

	dirty_tx = priv->dirty_tx;
	tx_left = priv->cur_tx - dirty_tx;
	while (tx_left > 0) {
		int entry = dirty_tx % NUM_TX_DESC;
		int txstatus;

		txstatus = RTL_R32(priv, TxStatus0 + (entry * sizeof(u32)));

		if (!(txstatus & (TxStatOK | TxUnderrun | TxAborted)))
			break;	/* It still hasn't been Txed */

		/* Note: TxCarrierLost is always asserted at 100mbps. */
		if (txstatus & (TxOutOfWindow | TxAborted)) {
			/* There was an major error, log it. */
			printf("Transmit error, Tx status %08x\n", txstatus);
			if (txstatus & TxAborted) {
				RTL_W32(priv, TxConfig, TxClearAbt);
				RTL_W16(priv, IntrStatus, TxErr);
			}
		} else {
			if (txstatus & TxUnderrun) {
				/* Add 64 to the Tx FIFO threshold. */
				if (priv->tx_flag < 0x00300000)
					priv->tx_flag += 0x00020000;
			}
		}

		dirty_tx++;
		tx_left--;
	}

	if (priv->dirty_tx != dirty_tx) {
		priv->dirty_tx = dirty_tx;
	}
}

static int rtl8139_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct rtl8139_priv *priv = edev->priv;

	unsigned int entry;

#ifdef RTL8139_DEBUG
	printf("rtl8139_eth_send()\n");
	memory_display(packet, 0, 70, 1);
#endif

	rtl8139_tx_interrupt(priv);

	/* Calculate the next Tx descriptor entry. */
	entry = priv->cur_tx % NUM_TX_DESC;

	/* Note: the chip doesn't have auto-pad! */
	if (likely(packet_length < TX_BUF_SIZE)) {
		if (packet_length < ETH_ZLEN)
			memset(priv->tx_buf[entry], 0, ETH_ZLEN);
		memcpy(priv->tx_buf[entry], packet, packet_length);
	} else {
		printf("packet too long\n");
		return 0;
	}

	/*
	 * Writing to TxStatus triggers a DMA transfer of the data
	 * copied to tp->tx_buf[entry] above.
	 */
	if (packet_length < ETH_ZLEN) {
		packet_length = ETH_ZLEN;
	}
	RTL_W32_F(priv, (TxStatus0 + (entry * sizeof(u32))),
			(priv->tx_flag | packet_length));

	priv->cur_tx++;

	return 0;
}

static const u16 rtl8139_intr_mask =
		PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver |
		TxErr | TxOK | RxErr | RxOK;

static int rtl8139_eth_rx(struct eth_device *edev)
{
	struct rtl8139_priv *priv = edev->priv;
	unsigned char *rx_ring = priv->rx_ring;
	unsigned int cur_rx = priv->cur_rx;
	unsigned int rx_size = 0;

	u32 ring_offset = cur_rx % RX_BUF_LEN;
	u32 rx_status;
	unsigned int pkt_size;

#ifdef RTL8139_DEBUG
	printf("rtl8139_eth_rx()\n");

	printf("In %s(), current %04x BufAddr %04x, free to %04x, Cmd %02x\n",
		   __func__, (u16)cur_rx,
		   RTL_R16(priv, RxBufAddr), RTL_R16(priv, RxBufPtr),
		   RTL_R8(priv, ChipCmd));
#endif

	rtl8139_tx_interrupt(priv);

	if (RTL_R8(priv, ChipCmd) & RxBufEmpty) {
		/* no data */
		return 0;
	}

	rx_status = le32_to_cpu(*(__le32 *) (rx_ring + ring_offset));
	rx_size = rx_status >> 16;
	pkt_size = rx_size - 4;

#ifdef RTL8139_DEBUG
	memory_display(&rx_ring[ring_offset], 0, 70, 1);
#endif

	net_receive(&rx_ring[ring_offset + 4], pkt_size);

	cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
	cur_rx = cur_rx & (RX_BUF_LEN - 1); /* FIXME */
	RTL_W16(priv, RxBufPtr, (u16) (cur_rx - 16));

	priv->cur_rx = cur_rx;

	return pkt_size /* size */;
}

static int rtl8139_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct eth_device *edev;
	struct rtl8139_priv *priv;

	u32 bar;
	struct pci_bus *bus = pdev->bus;
	struct device_d *dev = &pdev->dev;

#ifdef RTL8139_DEBUG
	printf("rtl8139_probe()\n");
#endif

	/* enable pci device */
	pci_read_config_dword(pdev, PCI_COMMAND, &bar);
	pci_write_config_dword(pdev, PCI_COMMAND, bar | 0x03);

	edev = xzalloc(sizeof(struct eth_device) +
			sizeof(struct rtl8139_priv));
	dev->type_data = edev;
	priv = (struct rtl8139_priv *)(edev + 1);

	edev->priv = priv;

	priv->pci_dev = pdev;

	priv->miidev.read = rtl8139_phy_read;
	priv->miidev.write = rtl8139_phy_write;
	priv->miidev.address = 0;
	priv->miidev.flags = 0;
	priv->miidev.edev = edev;

	/* FIXME: pci_resource_start() */
	pci_read_config_dword(pdev, PCI_BASE_ADDRESS_1, &bar);
	priv->base = (void *)bus->ops->res_start(bus, bar);

	printf("found rtl8139 (rev %02x) at %02x: %04x (base=%p)\n",
			pdev->revision,
			pdev->devfn,
			(pdev->class >> 8) & 0xffff,
			priv->base);

	edev->init = rtl8139_init_dev;
	edev->open = rtl8139_eth_open;
	edev->send = rtl8139_eth_send;
	edev->recv = rtl8139_eth_rx;
	edev->get_ethaddr = rtl8139_get_ethaddr;
	edev->set_ethaddr = rtl8139_set_ethaddr;
	edev->halt = rtl8139_eth_halt;

	mii_register(&priv->miidev);
	eth_register(edev);

	return 0;
}

static DEFINE_PCI_DEVICE_TABLE(rtl8139_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK,	PCI_DEVICE_ID_REALTEK_8139), },
	{ },
};

static struct pci_driver rtl8139_eth_driver = {
	.name = "rtl8139_eth",
	.id_table = rtl8139_pci_tbl,
	.probe = rtl8139_probe,
};

static int rtl8139_init(void)
{
	pci_register_driver(&rtl8139_eth_driver);

	add_generic_device("rtl8139_eth", -1, NULL, 0xb0000000, 16,
		IORESOURCE_MEM, NULL);

	return 0;
}
device_initcall(rtl8139_init);
