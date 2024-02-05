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
#ifndef INCLUDE_PICO_QUEUE
#define INCLUDE_PICO_QUEUE
#include "pico_config.h"
#include "pico_frame.h"

#define Q_LIMIT 0

#ifndef NULL
#define NULL ((void *)0)
#endif

void *pico_mutex_init(void);
void pico_mutex_deinit(void *mutex);
void pico_mutex_lock(void *mutex);
int pico_mutex_lock_timeout(void *mutex, int timeout);
void pico_mutex_unlock(void *mutex);
void pico_mutex_unlock_ISR(void *mutex);

struct pico_stack;
struct pico_queue {
    uint32_t frames;
    uint32_t size;
    uint32_t max_frames;
    uint32_t max_size;
    struct pico_frame *head;
    struct pico_frame *tail;
#ifdef PICO_SUPPORT_MUTEX
    void *mutex;
#endif
#ifdef PICO_SUPPORT_TICKLESS
    void (*listener)(struct pico_stack *, void *);
    void *listener_arg;
    void *listener_stack;
#endif
    uint8_t shared;
    uint16_t overhead;
};

#ifdef PICO_SUPPORT_MUTEX
#define PICOTCP_MUTEX_LOCK(x) { \
        if (x == NULL) \
            x = pico_mutex_init(); \
        pico_mutex_lock(x); \
}
#define PICOTCP_MUTEX_UNLOCK(x) pico_mutex_unlock(x)
#define PICOTCP_MUTEX_DEL(x) pico_mutex_deinit(x)

#else
#define PICOTCP_MUTEX_LOCK(x) do {} while(0)
#define PICOTCP_MUTEX_UNLOCK(x) do {} while(0)
#define PICOTCP_MUTEX_DEL(x) do {} while(0)
#endif

#ifdef PICO_SUPPORT_TICKLESS

static inline void pico_queue_register_listener(struct pico_stack *S, struct pico_queue *q, void (*fn)(struct pico_stack *, void *), void *arg)
{
    q->listener = fn;
    q->listener_arg = arg;
    q->listener_stack = S;
}

void pico_schedule_job(struct pico_stack *S, void (*exe)(struct pico_stack *, void*), void *arg);

static inline void pico_queue_wakeup(struct pico_queue *q)
{
    if (q->listener)
        pico_schedule_job(q->listener_stack, q->listener, q->listener_arg);
}

#else
#define pico_queue_register_listener(S, q, fn, arg) do{}while(0)
#define pico_queue_wakeup(q) do{}while(0)
#endif

#ifdef PICO_SUPPORT_DEBUG_TOOLS
static void debug_q(struct pico_queue *q)
{
    struct pico_frame *p = q->head;
    dbg("%d: ", q->frames);
    while(p) {
        dbg("(%p)-->", p);
        p = p->next;
    }
    dbg("X\n");
}

#else

#define debug_q(x) do {} while(0)
#endif

static inline int32_t pico_enqueue(struct pico_queue *q, struct pico_frame *p)
{
    if ((q->max_frames) && (q->max_frames <= q->frames))
        return -1;

#if (Q_LIMIT != 0)
    if ((Q_LIMIT < p->buffer_len + q->size))
        return -1;

#endif

    if ((q->max_size) && (q->max_size < (p->buffer_len + q->size)))
        return -1;

    if (q->shared)
        PICOTCP_MUTEX_LOCK(q->mutex);

    p->next = NULL;
    if (!q->head) {
        q->head = p;
        q->tail = p;
        q->size = 0;
        q->frames = 0;
    } else {
        q->tail->next = p;
        q->tail = p;
    }

    q->size += p->buffer_len + q->overhead;
    q->frames++;
    debug_q(q);

    if (q->shared)
        PICOTCP_MUTEX_UNLOCK(q->mutex);

    if (q->frames == 1)
        pico_queue_wakeup(q);

    return (int32_t)q->size;
}

static inline struct pico_frame *pico_dequeue(struct pico_queue *q)
{
    struct pico_frame *p = q->head;
    if (!p)
        return NULL;

    if (q->frames < 1)
        return NULL;

    if (q->shared)
        PICOTCP_MUTEX_LOCK(q->mutex);

    q->head = p->next;
    q->frames--;
    q->size -= p->buffer_len - q->overhead;
    if (q->head == NULL)
        q->tail = NULL;

    debug_q(q);

    p->next = NULL;
    if (q->shared)
        PICOTCP_MUTEX_UNLOCK(q->mutex);

    return p;
}

static inline struct pico_frame *pico_queue_peek(struct pico_queue *q)
{
    struct pico_frame *p = q->head;
    if (q->frames < 1)
        return NULL;

    debug_q(q);
    return p;
}

static inline void pico_queue_deinit(struct pico_queue *q)
{
    if (q->shared) {
        PICOTCP_MUTEX_DEL(q->mutex);
    }
}

static inline void pico_queue_empty(struct pico_queue *q)
{
    struct pico_frame *p = pico_dequeue(q);
    while(p) {
        pico_frame_discard(p);
        p = pico_dequeue(q);
    }
}

static inline void pico_queue_protect(struct pico_queue *q)
{
    q->shared = 1;
}

#endif
