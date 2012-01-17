#include <common.h>

#include <command.h>
#include <net.h>
#include <linux/phy.h>
#include <malloc.h>
#include <init.h>
#include <xfuncs.h>
#include <errno.h>
#include <clock.h>
#include <asm/io.h>
#include <errno.h>
#include <21143.h>

#define DEC21143_DEBUG
#undef DEC21143_DEBUG

#define NUM_RX_DESC 3
#define NUM_TX_DESC 1
#define RX_BUFF_SZ  1600

struct tx_desc {
	u32 status;
	u32 length;
	u32 buffer1;
	u32 buffer2;
};

struct rx_desc {
	u32 status;
	u32 length;
	u32 buffer1;
	u32 buffer2;
};

/* Descriptor bits. */
#define TULIP_RDES0_OWN   (1 << 31) /* Own Bit: indicates that the descriptor is owned by the 21143 */
#define TULIP_RDES0_LS	0x00000100	/* Last Descriptor */

#define TULIP_RDES0_ES	0x00008000	/* Error Summary */
#define TULIP_RDES1_RER	0x02000000	/* Receive End Of Ring */
#define TULIP_RDES1_RCH	0x01000000	/* Second Address Chained */

#define TULIP_TDES0_OWN   (1 << 31) /* Own Bit: indicates that the descriptor is owned by the 21143 */
#define TULIP_TDES1_LS    (1 << 30) /* Last Segment */
#define TULIP_TDES1_FS    (1 << 29) /* First Segment */
#define TULIP_TDES1_SET   (1 << 27) /* Setup Packet */
#define TD_TER		0x02000000	/* Transmit End Of Ring */
#define TD_ES		0x00008000	/* Error Summary */

struct dec21143_priv {
	void __iomem		*base;

	struct mii_bus		miibus;
	int			phy_addr;

	struct rx_desc		*rx_ring;
	struct tx_desc		*tx_ring;

	int			tx_new;
	int			rx_new;
	int			flags;

	/* EEPROM */
	struct cdev		cdev;
	int eeprom_size;
};

/* FIXME: write 0 to CSR7 */

#define TULIP_CSR0	0x00 /* Setting Register */
 #define TULIP_CSR0_SWR		0x01 /* Software Reset */
 #define TULIP_CSR0_TAP_MASK	(0x03 << 17)

#define TULIP_CSR1	0x08 /* Transmit Poll Demand Register */

#define TULIP_CSR2	0x10 /* Receive Poll Demand Register */
#define POLL_DEMAND	1

#define TULIP_CSR3	0x18 /* Start of Receive List */

#define TULIP_CSR4	0x20 /* Start of Transmit List */

#define TULIP_CSR5	0x28 /* Status Register */
 #define TULIP_CSR5_RI	(1 << 6) /* Receive Interrupt */
 #define TULIP_CSR5_TI	1 /* Transmit Interrupt */
 #define TULIP_CSR5_TS_MASK	(0x07 << 20)
 #define TULIP_CSR5_RS_MASK	(0x07 << 17)

#define TULIP_CSR6	0x30 /* Operation Mode Register */
 #define TULIP_CSR6_TXON	0x2000
 #define TULIP_CSR6_RXON	0x0002

#define TULIP_CSR7	0x38 /* Interrupt Enable Register */

#define TULIP_CSR8	0x40 /* Unused */

#define TULIP_CSR9	0x48 /* Boot ROM, Serial ROM, and MII Management Register */
#define TULIP_MDIO_DATA0 0x00000
#define TULIP_MDIO_DATA1 0x20000
#define MDIO_ENB		0x00000		/* Ignore the 0x02000 databook setting. */
#define MDIO_ENB_IN		0x40000
#define MDIO_DATA_READ	0x80000

 #define TULIP_CSR9_MDI         (1 << 19) /* MDIO Data In */
 #define TULIP_CSR9_MDOM        (1 << 18) /* MDIO Operation Mode */
 #define TULIP_CSR9_MDO         (1 << 17) /* MDIO Data Out */
 #define TULIP_CSR9_MDC         (1 << 16) /* MDIO Clock */
 #define TULIP_CSR9_RD          (1 << 14)
 #define TULIP_CSR9_WR          (1 << 13)
 #define TULIP_CSR9_SR          (1 << 11) /* Serial ROM Select */
 #define TULIP_CSR9_SRDO        (1 << 3) /* Serial ROM Data Out */
 #define TULIP_CSR9_SRDI        (1 << 2) /* Serial ROM Data In */
 #define TULIP_CSR9_SRCK        (1 << 1) /* Serial ROM Clock */
 #define TULIP_CSR9_SRCS        (1) /* Serial ROM Chip Select */

#define TULIP_CSR10	0x50 /* Unused */
#define TULIP_CSR11	0x58 /* Unused */
#define TULIP_CSR12	0x60 /* Unused */
#define TULIP_CSR13	0x68 /* Unused */
#define TULIP_CSR14	0x70 /* Unused: SIA Transmit and Receive Register */
#define TULIP_CSR15	0x78 /* Unused */

/* Register bits. */
/* CSR5 */
#define STS_TS		0x00700000	/* Transmit Process State */
#define STS_RS		0x000e0000	/* Receive Process State */

/* CSR6 */
#define OMR_PR		0x00000040	/* Promiscous Mode */
#define OMR_PS		0x00040000	/* Port Select */
#define OMR_SDP		0x02000000	/* SD Polarity - MUST BE ASSERTED */
#define OMR_PM		0x00000080	/* Pass All Multicast */

#define DC_W32(priv, reg, val)	writel(val, ((char *)(priv->base) + reg))
#define DC_R32(priv, reg)	readl(((char *)(priv->base) + reg))

/* Read and write the MII registers using software-generated serial
   MDIO protocol.  It is just different enough from the EEPROM protocol
   to not share code.  The maxium data clock rate is 2.5 MHz. */

#define mdio_delay() (void)DC_R32(priv, TULIP_CSR9)

static int dec21143_phy_read(struct mii_bus *bus, int phy_addr, int reg)
{
	struct dec21143_priv *priv = bus->priv;
	int val;

	int i;
	int read_cmd = (0xf6 << 10) | (phy_addr << 5) | reg;

	val = 0;

	/* Establish sync by sending at least 32 logic ones. */
	for (i = 32; i >= 0; i--) {
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | TULIP_MDIO_DATA1);
		mdio_delay();
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | TULIP_MDIO_DATA1 | TULIP_CSR9_MDC);
		mdio_delay();
	}

	/* Shift the read command bits out. */
	for (i = 15; i >= 0; i--) {
		int dataval = (read_cmd & (1 << i)) ? TULIP_MDIO_DATA1 : TULIP_MDIO_DATA0;

		DC_W32(priv, TULIP_CSR9, MDIO_ENB | dataval);
		mdio_delay();
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | dataval | TULIP_CSR9_MDC);
		mdio_delay();
	}

	/* Read the two transition, 16 data, and wire-idle bits. */
	for (i = 19; i > 0; i--) {
		DC_W32(priv, TULIP_CSR9, MDIO_ENB_IN);
		mdio_delay();
		val = (val << 1) | ((DC_R32(priv, TULIP_CSR9) & MDIO_DATA_READ) ? 1 : 0);
		DC_W32(priv, TULIP_CSR9, MDIO_ENB_IN | TULIP_CSR9_MDC);
		mdio_delay();
	}

	val = (val >> 1) & 0xffff;

#ifdef DEC21143_DEBUG
	printf("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n", __func__,
			phy_addr, reg, val);
#endif

	return val;
}

static int dec21143_phy_write(struct mii_bus *bus, int phy_addr,
	int reg, u16 val)
{
	struct dec21143_priv *priv = bus->priv;

	int i;
	int cmd = (0x5002 << 16) | (phy_addr << 23) | (reg << 18) | val;

#ifdef DEC21143_DEBUG
	printf("%s: addr: 0x%02x reg: 0x%02x val: 0x%04x\n", __func__,
	      phy_addr, reg, val);
#endif

	/* Establish sync by sending 32 logic ones. */
	for (i = 32; i >= 0; i--) {
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | TULIP_MDIO_DATA1);
		mdio_delay();
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | TULIP_MDIO_DATA1 | TULIP_CSR9_MDC);
		mdio_delay();
	}

	/* Shift the command bits out. */
	for (i = 31; i >= 0; i--) {
		int dataval = (cmd & (1 << i)) ? TULIP_MDIO_DATA1 : TULIP_MDIO_DATA0;

		DC_W32(priv, TULIP_CSR9, MDIO_ENB | dataval);
		mdio_delay();
		DC_W32(priv, TULIP_CSR9, MDIO_ENB | dataval | TULIP_CSR9_MDC);
		mdio_delay();
	}

	/* Clear out extra bits. */
	for (i = 2; i > 0; i--) {
		DC_W32(priv, TULIP_CSR9, MDIO_ENB_IN);
		mdio_delay();
		DC_W32(priv, TULIP_CSR9, MDIO_ENB_IN | TULIP_CSR9_MDC);
		mdio_delay();
	}

	return 0;
}

#define SETUP_FRAME_LEN 192
#define ETH_ALEN	6
#define	TOUT_LOOP	100

/* FIXME */
#define phys_to_bus(x) (0x0fffffff & (x))

#ifdef DEC21143_DEBUG
static void dump_rings(struct dec21143_priv *priv)
{
	int i;

	printf("dump_rings(): tx_new = %d, rx_new = %d\n", priv->tx_new, priv->rx_new);
	printf("dump_rings(): tx_ring = %08x, rx_ring = %08x\n", priv->tx_ring, priv->rx_ring);

	for (i = 0; i < NUM_RX_DESC; i++) {
		printf("rx_ring[%d].status  =%08x\n", i, le32_to_cpu(priv->rx_ring[i].status));
		printf("rx_ring[%d].length  = %08x\n", i, le32_to_cpu(priv->rx_ring[i].length));
		printf("rx_ring[%d].buffer1 = %08x\n", i, le32_to_cpu(priv->rx_ring[i].buffer1));
		printf("rx_ring[%d].buffer2 = %08x\n", i, le32_to_cpu(priv->rx_ring[i].buffer2));
	}
	printf("\n");

	for (i = 0; i < NUM_TX_DESC; i++) {
		printf("tx_ring[%d].status  = %08x\n", i, le32_to_cpu(priv->tx_ring[i].status));
		printf("tx_ring[%d].length  = %08x\n", i, le32_to_cpu(priv->tx_ring[i].length));
		printf("tx_ring[%d].buffer1 = %08x\n", i, le32_to_cpu(priv->tx_ring[i].buffer1));
		printf("tx_ring[%d].buffer2 = %08x\n", i, le32_to_cpu(priv->tx_ring[i].buffer2));
	}
	printf("\n");
	printf("TULIP_CSR0 = 0x%08x\n", DC_R32(priv, TULIP_CSR0));
	printf("TULIP_CSR1 = 0x%08x\n", DC_R32(priv, TULIP_CSR1));
	printf("TULIP_CSR2 = 0x%08x\n", DC_R32(priv, TULIP_CSR2));
	printf("TULIP_CSR3 = 0x%08x\n", DC_R32(priv, TULIP_CSR3));
	printf("TULIP_CSR4 = 0x%08x\n", DC_R32(priv, TULIP_CSR4));
{
	int t;
	t = DC_R32(priv, TULIP_CSR5);
	printf("TULIP_CSR5 = 0x%08x ", t);
	if (t & 1) {
		printf("<tx int>");
	}
	if (t & 2) {
		printf("<tx stop>");
	}
	if (t & 4) {
		printf("<tx bufun>");
	}
	printf("\n");
}
	printf("TULIP_CSR6 = 0x%08x\n", DC_R32(priv, TULIP_CSR6));

}
#endif

static void dec21143_send_setup_frame(struct eth_device *edev)
{
	struct dec21143_priv *priv = edev->priv;
	int i;
	char setup_frame[SETUP_FRAME_LEN];
	char *pa = &setup_frame[15 * 6];
	char eaddrs[6];

	/* add the broadcast address. */
	memset(setup_frame, 0xff, SETUP_FRAME_LEN);

	edev->get_ethaddr(edev, eaddrs);

	/* Fill the final entry of the table with our physical address. */
	*pa++ = eaddrs[0]; *pa++ = eaddrs[0];
	*pa++ = eaddrs[1]; *pa++ = eaddrs[1];
	*pa++ = eaddrs[2]; *pa++ = eaddrs[2];

#ifdef DEC21143_DEBUG
	dump_rings(priv);
#endif

	for (i = 0; priv->tx_ring[priv->tx_new].status & cpu_to_le32(TULIP_TDES0_OWN); i++) {
		if (i >= TOUT_LOOP) {
			printf("%s: tx buffer not ready 0\n", edev->dev.name);
			goto Done;
		}
	}

	priv->tx_ring[priv->tx_new].buffer1 = cpu_to_le32(phys_to_bus((u32) &setup_frame[0]));
	priv->tx_ring[priv->tx_new].length = cpu_to_le32(TD_TER | TULIP_TDES1_SET | SETUP_FRAME_LEN);
	priv->tx_ring[priv->tx_new].status = cpu_to_le32(TULIP_TDES0_OWN);

	DC_W32(priv, TULIP_CSR1, POLL_DEMAND);

#ifdef DEC21143_DEBUG
	dump_rings(priv);
#endif

	for (i = 0; priv->tx_ring[priv->tx_new].status & cpu_to_le32(TULIP_TDES0_OWN); i++) {
		if (i >= TOUT_LOOP) {
			printf("%s: tx buffer not ready 1\n", edev->dev.name);
			goto Done;
		}
	}

	if (le32_to_cpu(priv->tx_ring[priv->tx_new].status) != 0x7FFFFFFF) {
		printf("TX error status2 = 0x%08X\n", le32_to_cpu(priv->tx_ring[priv->tx_new].status));
	}

	priv->tx_new = (priv->tx_new + 1) % NUM_TX_DESC;

	printf("============================== done\n");
Done:
	return;
}

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK	0x02	/* EEPROM shift clock. */
#define EE_CS		0x01	/* EEPROM chip select. */
#define EE_DATA_WRITE	0x04	/* Data from the Tulip to EEPROM. */
#define EE_WRITE_0	0x01
#define EE_WRITE_1	0x05
#define EE_DATA_READ	0x08	/* Data from the EEPROM chip. */
#define EE_ENB		(0x4800 | EE_CS)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_READ_CMD		(6)

#define eeprom_delay()	(void)DC_R32(priv, TULIP_CSR9)

static int dec21143_read_eeprom(struct dec21143_priv *priv, int addr, u_int16_t *dest)
{
	int i;
	unsigned retval = 0;
	int addr_len = 6;
	int read_cmd = addr | (EE_READ_CMD << addr_len);
	int ee_enb = EE_ENB;

	if (priv->flags & DEC21143_VM5_EEPROM_FIX)
		ee_enb = 0x2800 | EE_CS;

	DC_W32(priv, TULIP_CSR9, ee_enb & ~EE_CS);
	DC_W32(priv, TULIP_CSR9, ee_enb);

	/* Shift the read command bits out. */
	for (i = 4 + addr_len; i >= 0; i--) {
		short dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
		DC_W32(priv, TULIP_CSR9, ee_enb | dataval);
		eeprom_delay();
		DC_W32(priv, TULIP_CSR9, ee_enb | dataval | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((DC_R32(priv, TULIP_CSR9) & EE_DATA_READ) ? 1 : 0);
	}
	DC_W32(priv, TULIP_CSR9, ee_enb);
	eeprom_delay();

	for (i = 16; i > 0; i--) {
		DC_W32(priv, TULIP_CSR9, ee_enb | EE_SHIFT_CLK);
		eeprom_delay();
		retval = (retval << 1) | ((DC_R32(priv, TULIP_CSR9) & EE_DATA_READ) ? 1 : 0);
		DC_W32(priv, TULIP_CSR9, ee_enb);
		eeprom_delay();
	}

	/* Terminate the EEPROM access. */
	DC_W32(priv, TULIP_CSR9, ee_enb & ~EE_CS);

	*dest = retval;

	//return (tp->flags & HAS_SWAPPED_SEEPROM) ? swab16(retval) : retval;
	return retval;
}

static ssize_t dec21143_read_eeprom_file(struct cdev *cdev, void *buf,
		size_t count, loff_t offset, ulong flags)
{
	struct dec21143_priv *priv = cdev->priv;
	char m[128];
	u_int16_t val;
	int i;
	int s = count;

	debug("21143_read_eeprom %ld bytes at 0x%08lX\n", (unsigned long)count, offset);

	/* sanity checks */
	if (!count)
		return 0;

	if (offset + count > priv->eeprom_size)
		return -EINVAL;

	memset(buf, 0, count);

	i = 0;
	if (offset % 2) {
		dec21143_read_eeprom(priv, offset / 2, &val);
		m[i] = val >> 8;
		i++;
		offset++;
		s--;
	}

	while (s > 0) {
		dec21143_read_eeprom(priv, offset / 2, &val);
		m[i] = val & 0xff;
		m[i + 1] = val >> 8;
		offset += 2;
		i += 2;
		s -= 2;
	}
	memcpy(buf, m, count);

	return count;
}

static ssize_t dec21143_write_eeprom_file(struct cdev *cdev, const void *buf,
		size_t count, loff_t offset, ulong flags)
{
	struct dec21143_priv *priv = cdev->priv;

	debug("21143_write_eeprom %ld bytes at 0x%08lX\n", (unsigned long)count, offset);

	if (offset + count > priv->eeprom_size)
		return -EINVAL;

	return count;
}

static struct file_operations eeprom_ops = {
	.read   = dec21143_read_eeprom_file,
	.write  = dec21143_write_eeprom_file,
	.lseek  = dev_lseek_default,
};


static int dec21143_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct dec21143_priv *priv = edev->priv;
	int i;

	for (i = 0; i < 6; i += sizeof(u16)) {
		u_int16_t val;
		dec21143_read_eeprom(priv, 10 + i/2, &val);
		m[i] = val & 0xff;
		m[i + 1] = val >> 8;
	}

	return 0;
}

static int dec21143_set_ethaddr(struct eth_device *edev,
					unsigned char *mac_addr)
{
	//struct dec21143_priv *priv = edev->priv;

	return 0;
}

static int dec21143_init_dev(struct eth_device *edev)
{
	struct dec21143_priv *priv = edev->priv;
	int i;

#ifdef DEC21143_DEBUG
	printf("%s\n", __func__);
#endif

	/* RESET_DE4X5(dev); */

	i = DC_R32(priv, TULIP_CSR0);
	udelay(1000);
	DC_W32(priv, TULIP_CSR0, i | TULIP_CSR0_SWR);
	udelay(1000);
	DC_W32(priv, TULIP_CSR0, i);
	udelay(1000);

	for (i = 0; i < 5; i++) {
		(void)DC_R32(priv, TULIP_CSR0);
		udelay(10000);
	}
	udelay(1000);

	if ((DC_R32(priv, TULIP_CSR5) & (STS_TS | STS_RS)) != 0) {
		printf("Error: Cannot reset ethernet controller.\n");
		return -EIO;
	}

	DC_W32(priv, TULIP_CSR6, OMR_SDP | OMR_PS | OMR_PM | OMR_PR);

	for (i = 0; i < NUM_RX_DESC; i++) {
		priv->rx_ring[i].status = cpu_to_le32(TULIP_RDES0_OWN);
		priv->rx_ring[i].length = cpu_to_le32(TULIP_RDES1_RCH | RX_BUFF_SZ);
		/* FIXME: VERY dirty */
		priv->rx_ring[i].buffer1 = cpu_to_le32(phys_to_bus((u32) NetRxPackets[i]));
#if 1
		/* More that one descriptor */
		priv->rx_ring[i].buffer2 = cpu_to_le32(phys_to_bus((u32) &priv->rx_ring[(i+1) % NUM_RX_DESC]));
#else
		priv->rx_ring[i].buffer2 = 0;
#endif
	}

	for (i = 0; i < NUM_TX_DESC; i++) {
		priv->tx_ring[i].status = 0;
		priv->tx_ring[i].length = 0;
		priv->tx_ring[i].buffer1 = 0;

#if 0
		priv->tx_ring[i].buffer2 = cpu_to_le32(phys_to_bus((u32) &priv->tx_ring[(i+1) % NUM_TX_DESC]));
#endif
		priv->tx_ring[i].buffer2 = 0;
	}

	/* Write the end of list marker to the descriptor lists. */
	priv->rx_ring[NUM_RX_DESC - 1].length |= cpu_to_le32(TULIP_RDES1_RER);
	priv->tx_ring[NUM_TX_DESC - 1].length |= cpu_to_le32(TD_TER);

//	priv->tx_ring[NUM_TX_DESC - 1].buffer2 = cpu_to_le32(phys_to_bus((u32) &priv->tx_ring[0]));

	/* Tell the adapter where the TX/RX rings are located. */
	/* FIXME: rx_ring to bus */
	DC_W32(priv, TULIP_CSR3, phys_to_bus((u32) (priv->rx_ring)));
	DC_W32(priv, TULIP_CSR4, phys_to_bus((u32) (priv->tx_ring)));

	priv->rx_new = 0;
	priv->tx_new = 0;

	dec21143_send_setup_frame(edev);

	/* Start the chip's Tx to process setup frame. */
	DC_W32(priv, TULIP_CSR6, DC_R32(priv, TULIP_CSR6)
				| TULIP_CSR6_TXON | TULIP_CSR6_RXON);

	return 0;
}

static int dec21143_eth_open(struct eth_device *edev)
{
	struct dec21143_priv *priv = edev->priv;
	int ret;

	ret = phy_device_connect(edev, &priv->miibus, priv->phy_addr, NULL, 0,
				 PHY_INTERFACE_MODE_MII);
	if (ret)
		return ret;

	return 0;
}

static void dec21143_eth_halt(struct eth_device *edev)
{
	struct dec21143_priv *priv = edev->priv;

#ifdef DEC21143_DEBUG
	printf("%s\n", __func__);
#endif

	/* Stop the Tx and Rx processes. */
	DC_W32(priv, TULIP_CSR6, DC_R32(priv, TULIP_CSR6) &
			~ (TULIP_CSR6_TXON | TULIP_CSR6_RXON));
}

static int dec21143_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct dec21143_priv *priv = edev->priv;

	int status = -EIO;
	int i;

#ifdef DEC21143_DEBUG
	printf("%s\n", __func__);
//	memory_display(packet, 0, 70, 1, 0);
#endif

	if (packet_length <= 0) {
		printf("%s: bad packet size: %d\n", edev->dev.name, packet_length);
		goto Done;
	}

	for (i = 0; priv->tx_ring[priv->tx_new].status & cpu_to_le32(TULIP_TDES0_OWN); i++) {
		if (i >= TOUT_LOOP) {
			printf("%s: tx error buffer not ready\n", edev->dev.name);
			goto Done;
		}
	}

	dma_flush_range((ulong) packet, (ulong)packet + packet_length);

	priv->tx_ring[priv->tx_new].buffer1 = cpu_to_le32(phys_to_bus((u32) packet));
	priv->tx_ring[priv->tx_new].length = cpu_to_le32(TD_TER | TULIP_TDES1_LS | TULIP_TDES1_FS | packet_length);
	priv->tx_ring[priv->tx_new].status = cpu_to_le32(TULIP_TDES0_OWN);

	DC_W32(priv, TULIP_CSR1, POLL_DEMAND);

	for (i = 0; priv->tx_ring[priv->tx_new].status & cpu_to_le32(TULIP_TDES0_OWN); i++) {
		if (i >= TOUT_LOOP) {
			//printf("%s: tx buffer not ready 2\n", edev->dev.name);
			goto Done;
		}
	}

	if (le32_to_cpu(priv->tx_ring[priv->tx_new].status) & TD_ES) {
#if 0 /* test-only */
		printf("TX error status = 0x%08X\n",
			le32_to_cpu(priv->tx_ring[priv->tx_new].status));
#endif
		priv->tx_ring[priv->tx_new].status = 0x0;
		goto Done;
	}

	status = packet_length;

Done:
	priv->tx_new = (priv->tx_new + 1) % NUM_TX_DESC;

	return 0;
	//return status;
}

static int dec21143_eth_rx(struct eth_device *edev)
{
	struct dec21143_priv *priv = edev->priv;

	s32 status;
	int length = 0;

#ifdef DEC21143_DEBUG
	printf("%s rx_new=%d\n", __func__, priv->rx_new);
	dump_rings(priv);
#endif

	DC_W32(priv, TULIP_CSR2, POLL_DEMAND);

	for ( ; ; ) {
		status = (s32)le32_to_cpu(priv->rx_ring[priv->rx_new].status);

		if (status & TULIP_RDES0_OWN) {
			#ifdef DEC21143_DEBUG
				printf("%s TULIP_RDES0_OWN, exiting\n", __func__);
			#endif
			break;
		}
		#ifdef DEC21143_DEBUG
		printf("%s !TULIP_RDES0_OWN, working\n", __func__);
		#endif

		if (status & TULIP_RDES0_LS) {
			/* Valid frame status. */
			if (status & TULIP_RDES0_ES) {
				/* There was an error. */
				printf("RX error status = 0x%08X\n", status);
			} else {
				ulong packet;

				/* A valid frame received. */
				length = (le32_to_cpu(priv->rx_ring[priv->rx_new].status) >> 16);

				/* Pass the packet up to the protocol layers. */
				/* Skip 4 byte CRC */
				//packet = (ulong) priv->rx_ring[priv->rx_new].buffer1;
				//dma_inv_range(packet, packet + length);
				dma_inv_range(NetRxPackets[priv->rx_new], NetRxPackets[priv->rx_new] + length);
				/* dirty */
				net_receive(NetRxPackets[priv->rx_new], length - 4);
			}
		} else {

			printf("Warning! ! status & TULIP_RDES0_LS\n");
		}

		/* Change buffer ownership for this frame, back to the adapter. */
		priv->rx_ring[priv->rx_new].status = cpu_to_le32(TULIP_RDES0_OWN);
		DC_W32(priv, TULIP_CSR2, POLL_DEMAND);

		/* Update entry information. */
		priv->rx_new = (priv->rx_new + 1) % NUM_RX_DESC;
	}

	DC_W32(priv, TULIP_CSR2, POLL_DEMAND);

	return length;
}

static int dec21143_eth_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct dec21143_priv *priv;
	struct dec21143_platform_data *pdata;

	edev = xzalloc(sizeof(struct eth_device) +
			sizeof(struct dec21143_priv));
	dev->type_data = edev;
	priv = (struct dec21143_priv *)(edev + 1);

	priv->base = dev_request_mem_region(dev, 0);

	edev->priv = priv;

	edev->init = dec21143_init_dev;
	edev->open = dec21143_eth_open;
	edev->send = dec21143_eth_send;
	edev->recv = dec21143_eth_rx;
	edev->get_ethaddr = dec21143_get_ethaddr;
	edev->set_ethaddr = dec21143_set_ethaddr;
	edev->halt = dec21143_eth_halt;

	pdata = dev->platform_data;
	if (pdata)
		priv->flags = pdata->flags;
	else
		priv->flags = 0;

	priv->miibus.read = dec21143_phy_read;
	priv->miibus.write = dec21143_phy_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;
	priv->phy_addr = 0;

	priv->rx_ring = dma_alloc_coherent(sizeof(struct rx_desc) * NUM_RX_DESC);
	priv->tx_ring = dma_alloc_coherent(sizeof(struct tx_desc) * NUM_TX_DESC);

	mdiobus_register(&priv->miibus);

	eth_register(edev);

	/* FIXME */
	priv->eeprom_size = 128;
	priv->cdev.size = 128;
	priv->cdev.dev = dev;
	priv->cdev.ops = &eeprom_ops;
	priv->cdev.priv = priv;

	priv->cdev.name = asprintf("%s_eeprom", (char *)dev_name(&edev->dev));
	devfs_create(&priv->cdev);

	return 0;
}

static void dec21143_eth_remove(struct device_d *dev)
{
	struct eth_device *edev = (struct eth_device *)(dev->type_data);
	struct dec21143_priv *priv = edev->priv;

#ifdef DEC21143_DEBUG
	printf("%s\n", __func__);
#endif
}

static struct driver_d dec21143_eth_driver = {
	.name = "dec21143_eth",
	.probe = dec21143_eth_probe,
	.remove = dec21143_eth_remove,
};

static int dec21143_eth_init(void)
{
	platform_driver_register(&dec21143_eth_driver);

	return 0;
}
device_initcall(dec21143_eth_init);
