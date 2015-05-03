/*
 * Copyright (C) 2012 by Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __ASM_DMA_H
#define __ASM_DMA_H

#include "asm/dma-mapping.h"

void dma_sync_single_for_cpu(unsigned long address, size_t size,
					   enum dma_data_direction dir);
void dma_sync_single_for_device(unsigned long address, size_t size,
					      enum dma_data_direction dir);

#endif /* __ASM_DMA_H */
