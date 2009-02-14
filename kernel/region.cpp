/*
 * GDI region objects. Shamelessly ripped out from the X11 distribution
 * Thanks for the nice licence.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *					  1999 Alex Korobka
 *                              Copyright 2009 Mike McCormack
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

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/
/*
 * The functions in this file implement the Region abstraction, similar to one
 * used in the X11 sample server. A Region is simply an area, as the name
 * implies, and is implemented as a "y-x-banded" array of rectangles. To
 * explain: Each Region is made up of a certain number of rectangles sorted
 * by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that every rectangle with a
 * given upper-left y coordinate (y1) will have the same lower-right y
 * coordinate (y2) and vice versa. If a rectangle has scanlines in a band, it
 * will span the entire vertical distance of the band. This means that some
 * areas that could be merged into a taller rectangle will be represented as
 * several shorter rectangles to account for shorter rectangles to its left
 * or right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. E.g. no two rectangles in a band are allowed
 * to touch.
 *
 * Whenever possible, bands will be merged together to cover a greater vertical
 * distance (and thus reduce the number of rectangles). Two bands can be merged
 * only if the bottom of one touches the top of the other and they have
 * rectangles in the same places (of the same width, of course). This maintains
 * the y-x-banding that's so nice to have...
 */


#include "config.h"

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "ntwin32.h"
#include "debug.h"
#include "win32mgr.h"

#define TRACE dprintf

#define RGN_DEFAULT_RECTS	2

class region_tt : public gdi_object_t
{
	INT size;
	INT numRects;
	RECT *rects;
	RECT extents;

public:
	region_tt( INT n );
	~region_tt();
	static region_tt* alloc( INT n );
	void set_rect( int left, int top, int right, int bottom );
	INT get_region_box( RECT* rect );
	INT get_region_type() const;
	BOOL rect_equal( PRECT r1, PRECT r2 );
	BOOL equal( region_tt *other );
	void offset_rect( RECT& rect, INT x, INT y );
	INT offset( INT x, INT y );
	INT get_num_rects() const;
	PRECT get_rects() const;
	void get_bounds_rect( RECT& rcBounds ) const;
	BOOL point_in_rect( const RECT& rect, int x, int y );
	BOOL contains_point( int x, int y );
	BOOL overlaps_rect( const RECT& overlap );
	BOOL rects_overlap( const RECT& r1, const RECT& r2 );
	static void dump_rect( const RECT& rect )
	{
		dprintf("%ld,%ld-%ld,%ld\n", rect.left, rect.top, rect.right, rect.bottom);
	}
	void empty_region();
};

region_tt::region_tt( INT n )
{
	size = n;
	rects = new RECT[n];
	empty_region();
}

region_tt::~region_tt()
{
	delete[] rects;
}

void region_tt::empty_region()
{
	numRects = 0;
	extents.left = 0;
	extents.top = 0;
	extents.right = 0;
	extents.bottom = 0;
}

region_tt* region_tt::alloc( INT n )
{
	region_tt* region = new region_tt( n );
	if (!region)
		return NULL;
	region->handle = alloc_gdi_handle( FALSE, GDI_OBJECT_REGION, 0, region );
	if (!region->handle)
	{
		delete region;
		return 0;
	}
	return region;
}

region_tt* region_from_handle( HGDIOBJ handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_REGION)
		return FALSE;
	assert( entry->kernel_info );
	return (region_tt*) entry->kernel_info;
}

template<typename T> static inline void swap( T& a, T& b )
{
	T x = a;
	a = b;
	b = x;
}

static inline void fix_rectangle( RECT& rect )
{
	if (rect.left > rect.right)
		swap( rect.left, rect.right );
	if (rect.top > rect.bottom)
		swap( rect.top, rect.bottom );
}

void region_tt::set_rect( int left, int top, int right, int bottom )
{
	if (left > right) { INT tmp = left; left = right; right = tmp; }
	if (top > bottom) { INT tmp = top; top = bottom; bottom = tmp; }

	if ((left != right) && (top != bottom))
	{
		extents.left = left;
		extents.top = top;
		extents.right = right;
		extents.bottom = bottom;

		fix_rectangle( extents );

		rects[0] = extents;
		numRects = 1;
	}
	else
		empty_region();
}

INT region_tt::get_region_type() const
{
	switch(numRects)
	{
	case 0:  return NULLREGION;
	case 1:  return SIMPLEREGION;
	default: return COMPLEXREGION;
	}
}

INT region_tt::get_num_rects() const
{
	return numRects;
}

PRECT region_tt::get_rects() const
{
	return rects;
}

void region_tt::get_bounds_rect( RECT& rcBounds ) const
{
	rcBounds = extents;
}

INT region_tt::get_region_box( RECT* rect )
{
	rect->left = extents.left;
	rect->top = extents.top;
	rect->right = extents.right;
	rect->bottom = extents.bottom;
	TRACE("%p (%ld,%ld-%ld,%ld)\n", get_handle(),
		rect->left, rect->top, rect->right, rect->bottom);
	return get_region_type();
}

BOOL region_tt::rect_equal( PRECT r1, PRECT r2 )
{
	return (r1->left == r2->left) &&
		(r1->right == r2->right) &&
		(r1->top == r2->top) &&
		(r1->bottom == r2->bottom);
}

BOOL region_tt::equal( region_tt *other )
{
	if (numRects != other->numRects)
		return FALSE;

	if (numRects == 0)
		return TRUE;

	if (!rect_equal( &extents, &other->extents ))
		return FALSE;

	for (int i = 0; i < numRects; i++ )
		if (!rect_equal(&rects[i], &other->rects[i]))
			return FALSE;

	return TRUE;
}

void region_tt::offset_rect( RECT& rect, INT x, INT y )
{
	rect.left += x;
	rect.right += x;
	rect.top += y;
	rect.bottom += y;
}

INT region_tt::offset( INT x, INT y )
{
	int nbox = numRects;
	RECT *pbox = rects;

	while (nbox--)
	{
		offset_rect( *pbox, x, y );
		pbox++;
	}
	offset_rect( extents, x, y );
	return get_region_type();
}

BOOL region_tt::point_in_rect( const RECT& rect, int x, int y )
{
	return (rect.top <= y) && (rect.bottom > y) &&
		(rect.left <= x) && (rect.right > x);
}

BOOL region_tt::contains_point( int x, int y )
{
	if (numRects == 0)
		return FALSE;

	if (!point_in_rect( extents, x, y ))
		return FALSE;

	for (int i = 0; i < numRects; i++)
		if (point_in_rect(rects[i], x, y))
			return TRUE;

	return FALSE;
}

BOOL region_tt::rects_overlap( const RECT& r1, const RECT& r2 )
{
	return (r1.right > r2.left) && (r1.left < r2.right) &&
		(r1.bottom > r2.top) && (r1.top < r2.bottom);
}

BOOL region_tt::overlaps_rect( const RECT& rect )
{
	if (numRects == 0)
		return FALSE;

	if (!rects_overlap( extents, rect ))
		return FALSE;

	for (int i = 0; i < numRects; i++)
		if (rects_overlap(rects[i], rect))
			return TRUE;

	return FALSE;
}

HRGN NTAPI NtGdiCreateRectRgn( int left, int top, int right, int bottom )
{
	region_tt* region = region_tt::alloc( RGN_DEFAULT_RECTS );
	if (!region)
		return 0;
	region->set_rect( left, top, right, bottom );
	return (HRGN) region->get_handle();
}

HRGN NTAPI NtGdiCreateEllipticRgn( int left, int top, int right, int bottom )
{
	return 0;
}

int NTAPI NtGdiGetRgnBox( HRGN Region, PRECT Rect )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;

	RECT box;
	int region_type = region->get_region_box( &box );

	NTSTATUS r;
	r = copy_to_user( Rect, &box, sizeof box );
	if (r < STATUS_SUCCESS)
		return ERROR;

	return region_type;
}

int NTAPI NtGdiCombineRgn( HRGN Dest, HRGN Source1, HRGN Source2, int CombineMode )
{
	return 0;
}

BOOL NTAPI NtGdiEqualRgn( HRGN Source1, HRGN Source2 )
{
	region_tt* rgn1 = region_from_handle( Source1 );
	if (!rgn1)
		return ERROR;

	region_tt* rgn2 = region_from_handle( Source2 );
	if (!rgn2)
		return ERROR;

	return rgn1->equal( rgn2 );
}

int NTAPI NtGdiOffsetRgn( HRGN Region, int x, int y )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;
	return region->offset( x, y );
}

BOOL NTAPI NtGdiSetRectRgn( HRGN Region, int left, int top, int right, int bottom )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;
	region->set_rect( left, top, right, bottom );
	return TRUE;
}

ULONG NTAPI NtGdiGetRegionData( HRGN Region, ULONG Count, PRGNDATA Data )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;

	ULONG size = region->get_num_rects() * sizeof(RECT);
	if (Count < (size + sizeof(RGNDATAHEADER)) || Data == NULL)
	{
		if (Data)	/* buffer is too small, signal it by return 0 */
			return 0;
		else		/* user requested buffer size with rgndata NULL */
			return size + sizeof(RGNDATAHEADER);
	}

	RGNDATAHEADER rdh;

	rdh.dwSize = sizeof(RGNDATAHEADER);
	rdh.iType = RDH_RECTANGLES;
	rdh.nCount = region->get_num_rects();
	rdh.nRgnSize = size;
	region->get_bounds_rect( rdh.rcBound );

	NTSTATUS r;
	r = copy_to_user( Data, &rdh, sizeof rdh );
	if (r < STATUS_SUCCESS)
		return ERROR;

	r = copy_to_user( Data->Buffer, region->get_rects(), size );
	if (r < STATUS_SUCCESS)
		return ERROR;

	return size + sizeof(RGNDATAHEADER);
}

BOOLEAN NTAPI NtGdiPtInRegion( HRGN Region, int x, int y )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;

	return region->contains_point( x, y );
}

BOOLEAN NTAPI NtGdiRectInRegion( HRGN Region, const RECT *rect )
{
	region_tt* region = region_from_handle( Region );
	if (!region)
		return ERROR;

	RECT overlap;
	NTSTATUS r;
	r = copy_from_user( &overlap, rect, sizeof *rect );
	if (r < STATUS_SUCCESS)
		return ERROR;

	fix_rectangle( overlap );

	return region->overlaps_rect( overlap );
}
