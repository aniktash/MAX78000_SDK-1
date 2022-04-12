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

#include "global.h"
#include "timer_systick.h"
#include "mxc_sys.h"

// The timer_systick provides timing services using the ARM SysTick timer
// timer_systick uses non-hardware specific function in primitive/timer.c
// timer_systick uses an interrupt-counting mechanism as opposed to compare-interrupt
// mechanism due to the limited nature of the SysTick timer.

// It's possible to use the MAX32520's peripheral timers in counting mode to
// implement compare-interrupt capability, but those timers continue to count
// during debug halt.  Compares that occur during debug halt do not get latched which
// is problematic during debug.

// The compare-interrupt method is superior to interrupt-counting because it offers much
// finer time granularity (10x to 100x) with less ISR overhead and facilitates time stamping
// useful for software-based execution profiling.

// All timers based on primitives/timer.c can be started/stopped and can execute one-shot and
// periodic callbacks at ISR time.

typedef struct
{
	timer_hw_t			hw;
	uint64_t			count;
}
timer_systick_t;

static timer_systick_t s_timer_systick;

void timer_systick_init( uint32_t ms, uint8_t isr_priority )
{
	// initialize the timer_systick
	// 'ms' is the implementation specific unit of time to be used by the timer.
	// In this implementation, it is 10 milliseconds.  This means that any time specifications must be >= 10ms or it will be considered 0ms.
	// All time specifications are truncated to 10ms.
	timer_hw_init( &s_timer_systick.hw, SysTick_LOAD_RELOAD_Msk + 1 );
	NVIC_SetPriority (SysTick_IRQn, isr_priority );
	SysTick->VAL = 0;
	SysTick->LOAD = (SysTick->CALIB & SysTick_CALIB_TENMS_Msk) * ms / 10;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	NVIC_EnableIRQ(SysTick_IRQn);
}

void timer_systick_stop( timer0_t *timer )
{
	// Stop a timer potentially before it expires.
    NVIC_DisableIRQ(SysTick_IRQn);
    timer_stop( timer );
    NVIC_EnableIRQ(SysTick_IRQn);
}

static inline uint64_t get_time( void )
{
	// Time is stored in a 64-bit format.
	s_timer_systick.count += (SysTick->LOAD - SysTick->VAL);
	return s_timer_systick.count;
}

void SysTick_Handler( void )
{
    s_timer_systick.count++;
	// The callback function of all timers that have expired will be called during timer_dispatch_pending()
	timer_dispatch_pending( &s_timer_systick.hw, s_timer_systick.count );
}

void timer_systick_start( timer0_t *timer, uint64_t delta, uint64_t period, timer_cb_t cb, void * context )
{
	// initializes and starts a timer
	// delta and period are in 1ms units, truncated to 10ms granularity.
	// delta is the time to the first expire callback.
	// period, if not zero, automatically re-init's timer every time it expires.  The callback is called at every expiration.
	// If period is zero, then the timer is called only once.

	// Periodic timers occur every 'period' milliseconds and will only be removed by a call to timer_systick_stop()
	// One-shot timers (period=0) are removed automatically upon expiration.

	// callbacks occur in the context of SysTick_Handler() ISR

	NVIC_DisableIRQ(SysTick_IRQn);
	uint64_t alarm = s_timer_systick.count + delta;
	timer_init( timer, &s_timer_systick.hw, alarm, period, cb, context );
	if( timer_start( timer ) )
	{
		SysTick_Handler();
	}
	NVIC_EnableIRQ(SysTick_IRQn);
}

