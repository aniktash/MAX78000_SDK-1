/* *****************************************************************************
 * Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
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
#ifndef __LL_H_INCL__
#define __LL_H_INCL__

#ifndef LL_INLINE
    #define LL_INLINE static inline
#endif

struct _sl_item_t;
typedef struct _sl_item_t
{
    struct _sl_item_t * next;
}
sl_item_t;

LL_INLINE void* sl_next( void *pv_this )
{
    sl_item_t * p_this = (sl_item_t *)pv_this;
    return p_this->next;
}

LL_INLINE void sl_remove_next( void * pv_this )
{
    sl_item_t * p_this = (sl_item_t *)pv_this;
    if( p_this->next )
    {
        p_this->next = p_this->next->next;
    }
}

LL_INLINE void sl_add( void *pv_this, void * pv_add )
{
    sl_item_t * p_this = (sl_item_t*)pv_this;
    sl_item_t * p_add = (sl_item_t *)pv_add;
    sl_item_t * p_next = p_this->next;
    p_this->next = p_add;
    p_add->next = p_next;
}

LL_INLINE void * sl_init( void * pv_this )
{
    sl_item_t * p_this = (sl_item_t *)pv_this;
    if( p_this )
    {
        p_this->next = 0;
    }
    return pv_this;
}

struct _ll_item_t;
typedef struct _ll_item_t
{
	struct _ll_item_t * next;
	struct _ll_item_t * prev;
}
ll_item_t;

typedef struct
{
	ll_item_t * first;
	ll_item_t * last;
}
ll_t;

LL_INLINE void * ll_init( void * pv_this )
{
    ll_item_t * p_this = (ll_item_t *)pv_this;
    if( p_this )
    {
		p_this->prev = p_this->next = 0;
    }
    return pv_this;
}

LL_INLINE void ll_remove( ll_t * p_ll, void * pv_item )
{
	ll_item_t * p_item = (ll_item_t*)pv_item;
	p_item->prev->next = p_item->next;
	p_item->next->prev = p_item->prev;
	if(  p_ll->first == p_item )
		p_ll->first = p_item->next;
	if(  p_ll->last == p_item )
		p_ll->last = p_item->prev;
}

LL_INLINE void ll_insert_after( ll_t *p_ll, void * pv_after, void * pv_item )
{
	ll_item_t * p_item = (ll_item_t*)pv_item;
	ll_item_t * p_after = (ll_item_t*)pv_after;

	p_item->prev = p_after;
	if( (p_item->next = p_after->next) )
		p_after->next->prev = p_item;
	else
		p_ll->last = p_item;
}

LL_INLINE void ll_insert_before( ll_t *p_ll, void * pv_before, void * pv_item )
{
	ll_item_t * p_item = (ll_item_t*)pv_item;
	ll_item_t * p_before = (ll_item_t*)pv_before;

	p_item->next = p_before;
	if( (p_item->prev = p_before->prev) )
		p_before->prev->next = p_item;
	else
		p_ll->first = p_item;
}

LL_INLINE void ll_add( ll_t *p_ll, void * pv_item )
{
	ll_item_t * p_item = (ll_item_t*)pv_item;
	if( p_ll->last )
		p_ll->last->next = p_item;
	else
		p_ll->first = p_item;
	p_item->prev = p_ll->last;
	p_ll->last = p_item;
}

#endif
