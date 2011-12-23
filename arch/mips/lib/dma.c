#include <common.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <malloc.h>

static inline void __iomem *ioremap_nocache(phys_t offset, unsigned long size)
{
	return (void __iomem *) (unsigned long)CKSEG1ADDR(offset);
}

void *dma_alloc_coherent(size_t size)
{
	void *ret;

	size = PAGE_ALIGN(size);
	ret = xmemalign(PAGE_SIZE, size);

	dma_inv_range((unsigned long)ret, (unsigned long)ret + size);

	ret = ioremap_nocache((phys_t)ret, size);

	return ret;
}

void dma_free_coherent(void *mem, size_t size)
{
	free(mem);
}
