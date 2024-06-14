/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 */

#ifndef _MACH_STM32_H_
#define _MACH_STM32_H_

/*
 * Peripheral memory map
 */
#define STM32_DBGMCU_BASE		0x50081000
#define STM32_DDRCTL_BASE		0x5A003000
#define STM32_TAMP_BASE			0x5C00A000

#define STM32_USART1_BASE		0x5C000000
#define STM32_USART2_BASE		0x4000E000
#define STM32_USART3_BASE		0x4000F000
#define STM32_UART4_BASE		0x40010000
#define STM32_UART5_BASE		0x40011000
#define STM32_USART6_BASE		0x44003000
#define STM32_UART7_BASE		0x40018000
#define STM32_UART8_BASE		0x40019000

#define STM32_SYSRAM_BASE		0x2FFC0000
#define STM32_SYSRAM_SIZE		SZ_256K

#define STM32_DDR_BASE			0xC0000000
#define STM32_DDR_SIZE			SZ_1G

int stm32mp_soc(void);

#endif /* _MACH_STM32_H_ */
