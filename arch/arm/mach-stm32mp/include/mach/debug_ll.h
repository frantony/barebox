#ifndef __MACH_STM32MP1_DEBUG_LL_H
#define __MACH_STM32MP1_DEBUG_LL_H

#include <io.h>
#include <mach/stm32.h>

#define DEBUG_LL_UART_ADDR	STM32_UART4_BASE

#define CR1_OFFSET	0x00
#define CR3_OFFSET	0x08
#define BRR_OFFSET	0x0c
#define ISR_OFFSET	0x1c
#define ICR_OFFSET	0x20
#define RDR_OFFSET	0x24
#define TDR_OFFSET	0x28

#define USART_ISR_TXE	BIT(7)


#define PERIPH_BASE	(0x40000000U)
#define PERIPH_BASE_APB2		(PERIPH_BASE + 0x10000)
#define USART1_BASE			(PERIPH_BASE_APB2 + 0x1000)
#define USART1				USART1_BASE

#define MMIO32(addr)		(*(volatile uint32_t *)(addr))
#define USART_SR(usart_base)		MMIO32((usart_base) + 0x00)
#define USART_SR_TXE			(1 << 7)
#define USART_DR(usart_base)		MMIO32((usart_base) + 0x04)
#define USART_DR_MASK                   0x1FF

static inline void PUTC_LL(int c)
{
	void __iomem *usart = (void *)USART1;

	while ((USART_SR(usart) & USART_SR_TXE) == 0);

	/* Send data. */
	USART_DR(usart) = (c & USART_DR_MASK);

#if 0
	void __iomem *base = IOMEM(DEBUG_LL_UART_ADDR);

	writel(c, base + TDR_OFFSET);

	while ((readl(base + ISR_OFFSET) & USART_ISR_TXE) == 0);
#endif
}

#endif /* __MACH_STM32MP1_DEBUG_LL_H */
