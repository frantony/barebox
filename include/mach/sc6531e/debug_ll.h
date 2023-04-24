/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2023 Antony Pavlov <antonynpavlov@gmail.com> */

#ifndef __MACH_SC6531E_DEBUG_LL_H__
#define __MACH_SC6531E_DEBUG_LL_H__

#include <io.h>

void usb_debug_ll_init(void);
int usb_PUTC_LL(char ch);

static inline void PUTC_LL(char ch)
{
	usb_PUTC_LL(ch);
}

#endif /* __MACH_SC6531E_DEBUG_LL_H__ */
