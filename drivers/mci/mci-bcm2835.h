/* SPDX-License-Identifier: GPL-2.0-only */
#define BCM2835_MCI_SLOTISR_VER			0xfc

#define MIN_FREQ 400000
#define BLOCK_SHIFT			16

#define SDHCI_SPEC_100	0
#define SDHCI_SPEC_200	1
#define SDHCI_SPEC_300	2

#define CONTROL0_HISPEED	(1 << 2)
#define CONTROL0_4DATA		(1 << 1)
#define CONTROL0_8DATA		(1 << 5)

#define CONTROL1_DATARST	(1 << 26)
#define CONTROL1_CMDRST		(1 << 25)
#define CONTROL1_HOSTRST	(1 << 24)
#define CONTROL1_CLKSELPROG	(1 << 5)
#define CONTROL1_CLKENA		(1 << 2)
#define CONTROL1_CLK_STABLE	(1 << 1)
#define CONTROL1_INTCLKENA	(1 << 0)
#define CONTROL1_CLKMSB		6
#define CONTROL1_CLKLSB		8
#define CONTROL1_TIMEOUT	(0x0E << 16)

#define MAX_CLK_DIVIDER_V3	2046
#define MAX_CLK_DIVIDER_V2	256
