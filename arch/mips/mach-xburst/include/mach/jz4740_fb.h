/*
 * Framebuffer driver for Ingenic JZ4740 SoC
 *
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MACH_JZ4740_FB_H__
#define __MACH_JZ4740_FB_H__

#include <fb.h>

struct jz4740_fb_platform_data {
	struct fb_videomode	*mode;
};

#endif /* __MACH_JZ4740_FB_H__ */
