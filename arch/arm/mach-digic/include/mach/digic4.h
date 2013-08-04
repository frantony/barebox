/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DIGIC4_H__
#define __DIGIC4_H__

#define DIGIC4_UART	0xc0800000

#define DIGIC4_TIMER0	0xc0210000
#define DIGIC4_TIMER1	0xc0210100
#define DIGIC4_TIMER2	0xc0210200

#define DIGIC4_GPIO(n)	(0xc0220000 + 4*n)

#endif /* __DIGIC4_H__ */
