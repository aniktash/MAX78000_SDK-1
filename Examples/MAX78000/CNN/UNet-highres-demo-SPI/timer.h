/* *****************************************************************************
 * Copyright (C) 2020 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 **************************************************************************** */
#ifndef __TIMER_H__
#define __TIMER_H__

#include "ll.h"

#define TIMER_SECONDS(freq,s)	((s)*(freq))
#define TIMER_MS(freq,ms)	(((uint64_t)(freq)*(ms)/1000))
#define TIMER_US(freq,us)	(((uint64_t)(freq)*(us)/1000000.0))

#ifndef ASSERT
#define ASSERT(x) if(!(x)) { while(1); }
#endif

struct _timer_t;
struct _timer_hw_t;
typedef void (*timer_cb_t)( struct _timer_t * );

typedef struct _timer_t
{
    sl_item_t   sl;   // must be first struct member
    uint64_t    time;
    uint64_t    period;
	timer_cb_t  cb;
	void *		context;
	struct _timer_hw_t*	hw;
}
timer0_t;

typedef struct _timer_hw_t
{
	sl_item_t       head;
	uint32_t		new_timer : 1;
	uint32_t		recursed : 1;
	timer0_t			rollover;
}
timer_hw_t;



void timer_hw_init( timer_hw_t * timer_hw, uint64_t rollover_period );

static inline void timer_init( timer0_t * timer, timer_hw_t * timer_hw, uint64_t expire_time, uint64_t period, timer_cb_t cb, void * context )
{
    sl_init( timer );
    timer->time = expire_time;
    timer->period = period;
	timer->cb = cb;
	timer->context = context;
	timer->hw = timer_hw;
}


#pragma pack(1)
    typedef union
    {
        uint64_t uint64;
        uint32_t uint32[2];
    }
    long_time_t;
#pragma pack()

   
static inline void expire_timers( timer_hw_t * timer_hw, uint64_t time, sl_item_t * p_expired )
{
    // expires all timers <= time and returns a list of expired timers
    void * exp = p_expired;
    timer0_t *p;
    while( (p = sl_next(&timer_hw->head)) && p->time <= time )
    {
        sl_remove_next( &timer_hw->head );
        sl_add( exp, p );
        exp = p;
    }
}

static inline void callback_timers( sl_item_t * p_expired, sl_item_t * p_periodic_expired )
{
    void * pe = p_periodic_expired;
    timer0_t *p;
    while( (p = sl_next(p_expired)) )
    {
        sl_remove_next( p_expired );
		p->cb( p );
        if( p->period )
        {
            p->time += p->period;
            sl_add( pe, p );
            pe = p;
        }
    }
}

static inline sl_item_t * find_last_le( timer_hw_t * timer_hw, uint64_t time )
{
    timer0_t * p = (timer0_t*)&timer_hw->head;
    void * last = p;
    while( (p = sl_next(p)) && p->time <= time )
    {
        last = p;
    }
    return last;
}

static inline void readd_periodic_timers( timer_hw_t * timer_hw, sl_item_t * p_periodic_expired )
{
    timer0_t *p;
    sl_item_t * add;
    while( (p = sl_next(p_periodic_expired)) )
    {
        add = find_last_le( timer_hw, p->time );
        sl_remove_next( p_periodic_expired );
        sl_add( add, p );
    }
}

static inline uint64_t timer_dispatch_pending( timer_hw_t * timer_hw, uint64_t time )
{
    sl_item_t expired, periodic_expired;
	timer_hw->recursed = 1;
	do
	{
		sl_init( &expired );
		sl_init( &periodic_expired );
		timer_hw->new_timer = 0;
		expire_timers( timer_hw, time, &expired );
		callback_timers( &expired, &periodic_expired );
		readd_periodic_timers( timer_hw, &periodic_expired );
	}
	while( timer_hw->new_timer );
	timer_hw->recursed = 0;
	return ((timer0_t*)sl_next(&timer_hw->head))->time;
}

static inline bool timer_start( timer0_t * timer )
{
    sl_add( find_last_le( timer->hw, timer->time ), timer );
	timer->hw->new_timer = 1;
    return !timer->hw->recursed;
}

static inline void timer_stop( timer0_t * timer )
{
    timer_hw_t *hw = timer->hw;
    if( hw )
    {
        void * p = &timer->hw->head;
        void * prev;
        do
        {
            prev = p;
            p = sl_next( p );
        }
        while(p && p != timer);
        ASSERT(p); // assert on missing timer
        sl_remove_next(prev);
    }
}
#endif
