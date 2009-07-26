/*
 * nt loader
 *
 * Copyright 2006-2009 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "ntwin32.h"
#include "mem.h"
#include "section.h"
#include "debug.h"
#include "win32mgr.h"

class win32k_null_t : public win32k_manager_t
{
public:
	virtual BOOL init();
	virtual void fini();
	virtual device_context_t* alloc_screen_dc_ptr();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual BOOL bitblt( INT xDest, INT yDest, INT cx, INT cy, bitmap_t *src, INT xSrc, INT ySrc, ULONG rop );
	virtual BOOL polypatblt( ULONG Rop, PRECT rect );
	virtual int getcaps( int index );
        virtual BOOL lineto( INT x1, INT y1, INT x2, INT y2, pen_t *pen );
};

BOOL win32k_null_t::init()
{
	return TRUE;
}

void win32k_null_t::fini()
{
}

BOOL win32k_null_t::set_pixel( INT x, INT y, COLORREF color )
{
	return TRUE;
}

BOOL win32k_null_t::rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	return TRUE;
}

BOOL win32k_null_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return TRUE;
}

BOOL win32k_null_t::bitblt( INT xDest, INT yDest, INT cx, INT cy, bitmap_t *src, INT xSrc, INT ySrc, ULONG rop )
{
	return TRUE;
}

BOOL win32k_null_t::polypatblt( ULONG Rop, PRECT rect )
{
	return TRUE;
}

int win32k_null_t::getcaps( int index )
{
	dprintf("%d\n", index);
	return 0;
}

BOOL win32k_null_t::lineto( INT x1, INT y1, INT x2, INT y2, pen_t *pen ) {
    return TRUE;
}

device_context_t* win32k_null_t::alloc_screen_dc_ptr()
{
	// FIXME: make graphics functions more generic
	assert( 0 );
	return 0;
	//return new device_context_t;
}

win32k_null_t win32k_manager_null;

win32k_manager_t* init_null_win32k_manager()
{
	return &win32k_manager_null;
}

