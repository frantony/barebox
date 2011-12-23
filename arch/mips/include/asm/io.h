/*
 * Stolen from the linux-2.6/include/asm-generic/io.h
 */

/**
 * @file
 * @brief mips IO access functions
 */

#ifndef __ASM_MIPS_IO_H
#define __ASM_MIPS_IO_H

#include <linux/compiler.h>
#include <asm/types.h>
#include <asm/addrspace.h>
#include <asm/byteorder.h>
#include <linux/const.h>

void *dma_alloc_coherent(size_t size);
void dma_free_coherent(void *mem, size_t size);

void dma_clean_range(unsigned long, unsigned long);
void dma_flush_range(unsigned long, unsigned long);
void dma_inv_range(unsigned long, unsigned long);

#ifdef CONFIG_32BIT
#define CAC_BASE		_AC(0x80000000, UL)
#define UNCAC_BASE		_AC(0xa0000000, UL)
#endif

/*
 * This gives the physical RAM offset.
 */
#ifndef PHYS_OFFSET
#define PHYS_OFFSET		_AC(0, UL)
#endif

/*
 * This handles the memory map.
 */
#ifndef PAGE_OFFSET
#define PAGE_OFFSET		(CAC_BASE + PHYS_OFFSET)
#endif

#define UNCAC_ADDR(addr)	((addr) - PAGE_OFFSET + UNCAC_BASE + 	\
								PHYS_OFFSET)
#define CAC_ADDR(addr)		((addr) - UNCAC_BASE + PAGE_OFFSET -	\
								PHYS_OFFSET)
/*
 *     virt_to_phys    -       map virtual addresses to physical
 *     @address: address to remap
 *
 *     The returned physical address is the physical (CPU) mapping for
 *     the memory address given. It is only valid to use this function on
 *     addresses directly mapped or allocated via kmalloc.
 *
 *     This function does not give bus mappings for DMA transfers. In
 *     almost all conceivable cases a device driver should not be using
 *     this function
 */
static inline unsigned long virt_to_phys(volatile const void *address)
{
	return (unsigned long)address - PAGE_OFFSET + PHYS_OFFSET;
}

/*
 *     phys_to_virt    -       map physical address to virtual
 *     @address: address to remap
 *
 *     The returned virtual address is a current CPU mapping for
 *     the memory address given. It is only valid to use this function on
 *     addresses that have a kernel mapping
 *
 *     This function does not handle bus mappings for DMA transfers. In
 *     almost all conceivable cases a device driver should not be using
 *     this function
 */
static inline void * phys_to_virt(unsigned long address)
{
	return (void *)(address + PAGE_OFFSET - PHYS_OFFSET);
}

/*****************************************************************************/
/*
 * readX/writeX() are used to access memory mapped devices. On some
 * architectures the memory mapped IO stuff needs to be accessed
 * differently. On the simple architectures, we just read/write the
 * memory location directly.
 */
#ifndef __raw_readb
static inline u8 __raw_readb(const volatile void __iomem *addr)
{
	return *(const volatile u8 __force *) addr;
}
#endif

#ifndef __raw_readw
static inline u16 __raw_readw(const volatile void __iomem *addr)
{
	return *(const volatile u16 __force *) addr;
}
#endif

#ifndef __raw_readl
static inline u32 __raw_readl(const volatile void __iomem *addr)
{
	return *(const volatile u32 __force *) addr;
}
#endif

#define readb __raw_readb
#define readw(addr) __le16_to_cpu(__raw_readw(addr))
#define readl(addr) __le32_to_cpu(__raw_readl(addr))

#ifndef __raw_writeb
static inline void __raw_writeb(u8 b, volatile void __iomem *addr)
{
	*(volatile u8 __force *) addr = b;
}
#endif

#ifndef __raw_writew
static inline void __raw_writew(u16 b, volatile void __iomem *addr)
{
	*(volatile u16 __force *) addr = b;
}
#endif

#ifndef __raw_writel
static inline void __raw_writel(u32 b, volatile void __iomem *addr)
{
	*(volatile u32 __force *) addr = b;
}
#endif

#define writeb __raw_writeb
#define writew(b,addr) __raw_writew(__cpu_to_le16(b),addr)
#define writel(b,addr) __raw_writel(__cpu_to_le32(b),addr)

#define in_be16(a)	__be16_to_cpu(__raw_readw(a))
#define in_be32(a)	__be32_to_cpu(__raw_readl(a))
#define out_be16(a, v)	__raw_writew(__cpu_to_be16(v), a)
#define out_be32(a, v)	__raw_writel(__cpu_to_be32(v), a)

#endif	/* __ASM_MIPS_IO_H */
