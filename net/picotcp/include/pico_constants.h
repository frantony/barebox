/*********************************************************************
 * PicoTCP-NG 
 * Copyright (c) 2020 Daniele Lacamera <root@danielinux.net>
 *
 * This file also includes code from:
 * PicoTCP
 * Copyright (c) 2012-2017 Altran Intelligent Systems
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
 *
 * PicoTCP-NG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) version 3.
 *
 * PicoTCP-NG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 *
 *
 *********************************************************************/
#ifndef INCLUDE_PICO_CONST
#define INCLUDE_PICO_CONST
/* Included from pico_config.h */

#include <stdint.h>

/** Non-endian dependant constants */
#define PICO_SIZE_IP4    4
#define PICO_SIZE_IP6   16
#define PICO_SIZE_ETH    6
#define PICO_SIZE_TRANS  8
#define PICO_SIZE_IEEE802154_EXT (8u)
#define PICO_SIZE_IEEE802154_SHORT (2u)

/** Endian-dependant constants **/
typedef uint64_t pico_time;

/*** *** *** *** *** *** ***
 ***     ARP CONFIG      ***
 *** *** *** *** *** *** ***/

#include "pico_addressing.h"

/* Maximum amount of accepted ARP requests per burst interval */
#define PICO_ARP_MAX_RATE 1
/* Duration of the burst interval in milliseconds */
#define PICO_ARP_INTERVAL 1000

/* Add well-known host numbers here. (bigendian constants only beyond this point) */
#define PICO_IP4_ANY (0x00000000U)
#define PICO_IP4_BCAST (0xffffffffU)

#define PICO_IEEE802154_BCAST (0xffffu)

/* defined in modules/pico_ipv6.c */
#ifdef PICO_SUPPORT_IPV6
extern const uint8_t PICO_IPV6_ANY[PICO_SIZE_IP6];
#endif

static inline uint32_t pico_hash(const void *buf, uint32_t size)
{
    uint32_t hash = 5381;
    uint32_t i;
    const uint8_t *ptr = (const uint8_t *)buf;
    for(i = 0; i < size; i++)
        hash = ((hash << 5) + hash) + ptr[i]; /* hash * 33 + char */
    return hash;
}

/* Debug */
/* #define PICO_SUPPORT_DEBUG_MEMORY */
/* #define PICO_SUPPORT_DEBUG_TOOLS */
#endif
