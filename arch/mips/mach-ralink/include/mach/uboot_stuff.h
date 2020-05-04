#ifndef __UBOOT_IO_H__
#define __UBOOT_IO_H__

#include <asm/mipsregs.h>

static inline void __udelay(int us)
{
	u32 end = us * 1000; /* FIXME */

	write_c0_count(0);
	while (read_c0_count() < end)
		;
}

#define sync() asm volatile ("sync" ::: "memory")

#define mb()						\
__asm__ __volatile__(					\
	"# prevent instructions being moved around\n\t"	\
	".set\tnoreorder\n\t"				\
	"# 8 nops to fool the R4400 pipeline\n\t"	\
	"nop;nop;nop;nop;nop;nop;nop;nop\n\t"		\
	".set\treorder"					\
	: /* no output */				\
	: /* no input */				\
	: "memory")
#define rmb() mb()
#define wmb() mb()

static inline void mips_cache(int op, const volatile void *addr)
{
#ifdef __GCC_HAVE_BUILTIN_MIPS_CACHE
	__builtin_mips_cache(op, addr);
#else
	__asm__ __volatile__("cache %0, 0(%1)" : : "i"(op), "r"(addr));
#endif
}

#define __BUILD_CLRBITS(bwlq, sfx, end, type)				\
									\
static inline void clrbits_##sfx(volatile void __iomem *mem, type clr)	\
{									\
	type __val = __raw_read##bwlq(mem);				\
	__val = end##_to_cpu(__val);					\
	__val &= ~clr;							\
	__val = cpu_to_##end(__val);					\
	__raw_write##bwlq(__val, mem);					\
}

#define __BUILD_SETBITS(bwlq, sfx, end, type)				\
									\
static inline void setbits_##sfx(volatile void __iomem *mem, type set)	\
{									\
	type __val = __raw_read##bwlq(mem);				\
	__val = end##_to_cpu(__val);					\
	__val |= set;							\
	__val = cpu_to_##end(__val);					\
	__raw_write##bwlq(__val, mem);					\
}

#define __BUILD_CLRSETBITS(bwlq, sfx, end, type)			\
									\
static inline void clrsetbits_##sfx(volatile void __iomem *mem,		\
					type clr, type set)		\
{									\
	type __val = __raw_read##bwlq(mem);				\
	__val = end##_to_cpu(__val);					\
	__val &= ~clr;							\
	__val |= set;							\
	__val = cpu_to_##end(__val);					\
	__raw_write##bwlq(__val, mem);					\
}

#define BUILD_CLRSETBITS(bwlq, sfx, end, type)				\
									\
__BUILD_CLRBITS(bwlq, sfx, end, type)					\
__BUILD_SETBITS(bwlq, sfx, end, type)					\
__BUILD_CLRSETBITS(bwlq, sfx, end, type)

#define __to_cpu(v)		(v)
#define cpu_to__(v)		(v)

BUILD_CLRSETBITS(l, 32, _, u32)

#endif /* __UBOOT_IO_H__ */
