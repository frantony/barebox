#ifndef _ASM_RISCV_BYTEORDER_H
#define _ASM_RISCV_BYTEORDER_H

#if defined(__RISCVEB__)
#include <linux/byteorder/big_endian.h>
#else
#include <linux/byteorder/little_endian.h>
#endif

#endif /* _ASM_RISCV_BYTEORDER_H */
