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
#define MAX_BLOCK_SIZE 1600
#define MAX_BLOCK_COUNT 16

#define DECLARE_HEAP(type, orderby) \
    struct heap_ ## type {   \
        uint32_t size;    \
        uint32_t n;       \
        type *top[MAX_BLOCK_COUNT];        \
    }; \
    typedef struct heap_ ## type heap_ ## type; \
    static inline type* heap_get_element(struct heap_ ## type *heap, uint32_t idx) \
    { \
        uint32_t elements_per_block = MAX_BLOCK_SIZE/sizeof(type); \
        return &heap->top[idx/elements_per_block][idx%elements_per_block];\
    } \
    static inline int8_t heap_increase_size(struct heap_ ## type *heap) \
    {\
        type *newTop; \
        uint32_t elements_per_block = MAX_BLOCK_SIZE/sizeof(type); \
        uint32_t elements = (heap->n + 1)%elements_per_block;\
        elements = elements?elements:elements_per_block;\
        if (heap->n+1 > elements_per_block * MAX_BLOCK_COUNT){\
            return -1;\
        }\
        newTop = PICO_ZALLOC(elements*sizeof(type)); \
        if(!newTop) { \
            return -1; \
        } \
        if (heap->top[heap->n/elements_per_block])  { \
            memcpy(newTop, heap->top[heap->n/elements_per_block], (elements - 1) * sizeof(type)); \
            PICO_FREE(heap->top[heap->n/elements_per_block]); \
        } \
        heap->top[heap->n/elements_per_block] = newTop;             \
        heap->size++;                                                               \
        return 0;                                                               \
    }\
    static inline int heap_insert(struct heap_ ## type *heap, type * el) \
    { \
        type *half;                                                                 \
        uint32_t i; \
        if (++heap->n >= heap->size) {                                                \
            if (heap_increase_size(heap)){                                                    \
                heap->n--;                                                           \
                return -1;                                                           \
            }                                                                       \
        }                                                                             \
        if (heap->n == 1) {                                                       \
            memcpy(heap_get_element(heap, 1), el, sizeof(type));                                    \
            return 0;                                                                   \
        }                                                                             \
        i = heap->n;                                                                    \
        half = heap_get_element(heap, i/2);                                                   \
        while ( (i > 1) && (half->orderby > el->orderby) ) {        \
            memcpy(heap_get_element(heap, i), heap_get_element(heap, i / 2), sizeof(type));                     \
            i /= 2;                                                                     \
            half = heap_get_element(heap, i/2);                                                   \
        }             \
        memcpy(heap_get_element(heap, i), el, sizeof(type));                                      \
        return 0;                                                                     \
    } \
    static inline int heap_peek(struct heap_ ## type *heap, type * first) \
    { \
        type *last;           \
        type *left_child;           \
        type *right_child;           \
        uint32_t i, child;        \
        if(heap->n == 0) {    \
            return -1;          \
        }                     \
        memcpy(first, heap_get_element(heap, 1), sizeof(type));   \
        last = heap_get_element(heap, heap->n--);                 \
        for(i = 1; (i * 2u) <= heap->n; i = child) {   \
            child = 2u * i;                              \
            right_child = heap_get_element(heap, child+1);     \
            left_child = heap_get_element(heap, child);      \
            if ((child != heap->n) &&                   \
                (right_child->orderby          \
                < left_child->orderby))           \
                child++;                                \
            left_child = heap_get_element(heap, child);      \
            if (last->orderby >                         \
                left_child->orderby)               \
                memcpy(heap_get_element(heap,i), heap_get_element(heap,child), \
                       sizeof(type));                  \
            else                                        \
                break;                                  \
        }                                             \
        memcpy(heap_get_element(heap, i), last, sizeof(type));    \
        return 0;                                     \
    } \
    static inline type *heap_first(heap_ ## type * heap)  \
    { \
        if (heap->n == 0)     \
            return NULL;        \
        return heap_get_element(heap, 1);  \
    } \
    static inline heap_ ## type *heap_init(void) \
    { \
        heap_ ## type * p = (heap_ ## type *)PICO_ZALLOC(sizeof(heap_ ## type));  \
        return p;     \
    } \

