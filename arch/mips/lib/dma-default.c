/*
 * Copyright (C) 2015 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <dma.h>

void dma_sync_single_for_cpu(unsigned long address, size_t size,
			     enum dma_data_direction dir)
{
}

void dma_sync_single_for_device(unsigned long address, size_t size,
				enum dma_data_direction dir)
{
}
