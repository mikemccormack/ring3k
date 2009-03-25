/*
 * nt loader
 *
 * Copyright 2006-2008 Mike McCormack
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
#include "debug.h"
#include "win32mgr.h"

template<>
COLORREF bitmap_impl_t<1>::get_pixel( int x, int y )
{
	ULONG row_size = get_rowsize();
	if ((bits[row_size * y + x/8 ]>> (7 - (x%8))) & 1)
		return RGB( 255, 255, 255 );
	else
		return RGB( 0, 0, 0 );
}

template<>
COLORREF bitmap_impl_t<2>::get_pixel( int x, int y )
{
	assert(0);
	return RGB( 0, 0, 0 );
}

template<>
COLORREF bitmap_impl_t<16>::get_pixel( int x, int y )
{
	ULONG row_size = get_rowsize();
	USHORT val = *(USHORT*) &bits[row_size * y + x*2 ];
	return RGB( (val & 0xf800) >> 8, (val & 0x07e0) >> 3, (val & 0x1f) << 3 );
}

template<>
COLORREF bitmap_impl_t<24>::get_pixel( int x, int y )
{
	ULONG row_size = get_rowsize();
	ULONG val = *(ULONG*) &bits[row_size * y + x*3 ];
	return val&0xffffff;
}

bitmap_t::~bitmap_t()
{
	assert( magic == magic_val );
	delete bits;
}

ULONG bitmap_t::get_rowsize()
{
	assert( magic == magic_val );
	ULONG row_size = (width*bpp)/8;
	return (row_size + 1)& ~1;
}

ULONG bitmap_t::bitmap_size()
{
	assert( magic == magic_val );
	return height * get_rowsize();
}

void bitmap_t::dump()
{
	assert( magic == magic_val );
	for (int j=0; j<height; j++)
	{
		for (int i=0; i<width; i++)
			fprintf(stderr,"%c", get_pixel(i, j)? 'X' : ' ');
		fprintf(stderr, "\n");
	}
}

bitmap_t::bitmap_t( int _width, int _height, int _planes, int _bpp ) :
	magic( magic_val ),
	bits( 0 ),
	width( _width ),
	height( _height ),
	planes( _planes ),
	bpp( _bpp )
{
}

void bitmap_t::lock()
{
}

void bitmap_t::unlock()
{
}

/*
COLORREF bitmap_t::get_pixel( int x, int y )
{
	assert( magic == magic_val );
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	ULONG row_size = get_rowsize();
	switch (bpp)
	{
	case 1:
		if ((bits[row_size * y + x/8 ]>> (7 - (x%8))) & 1)
			return RGB( 255, 255, 255 );
		else
			return RGB( 0, 0, 0 );
	case 16:
		{
		USHORT val = *(USHORT*) &bits[row_size * y + x*2 ];
		return RGB( (val & 0xf800) >> 8, (val & 0x07e0) >> 3, (val & 0x1f) << 3 );
		}
	default:
		dprintf("%d bpp not implemented\n", bpp);
	}
	return 0;
}
*/

BOOL bitmap_t::set_pixel( int x, int y, COLORREF color )
{
	assert( magic == magic_val );
	assert( width != 0 );
	assert( height != 0 );
	if (x < 0 || x >= width)
		return FALSE;
	if (y < 0 || y >= height)
		return FALSE;
	ULONG row_size = get_rowsize();
	switch (bpp)
	{
	case 1:
		if (color == RGB( 0, 0, 0 ))
			bits[row_size * y + x/8 ] &= ~ (1 << (7 - (x%8)));
		else if (color == RGB( 255, 255, 255 ))
			bits[row_size * y + x/8 ] |= (1 << (7 - (x%8)));
		else
			// implement color translation
			assert(0);
		break;
	case 16:
		*((USHORT*) &bits[row_size * y + x*2 ]) =
			((GetRValue(color)&0xf8) << 8) |
			((GetGValue(color)&0xfc) << 3) |
			((GetBValue(color)&0xf8) >> 3);
		break;
	default:
		dprintf("%d bpp not implemented\n", bpp);
	}
	return TRUE;
}

NTSTATUS bitmap_t::copy_pixels( void *pixels )
{
	return copy_from_user( bits, pixels, bitmap_size() );
}

bitmap_t* bitmap_from_handle( HANDLE handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_BITMAP)
		return FALSE;
	assert( entry->kernel_info );
	gdi_object_t* obj = reinterpret_cast<gdi_object_t*>( entry->kernel_info );
	return static_cast<bitmap_t*>( obj );
}

bitmap_t* alloc_bitmap( int width, int height, int depth )
{
	bitmap_t *bm = NULL;
	switch (depth)
	{
	case 1:
		bm = new bitmap_impl_t<1>( width, height );
		break;
	case 2:
		bm = new bitmap_impl_t<2>( width, height );
		break;
/*
	case 4:
		bm = new bitmap_impl_t<4>( width, height );
		break;
	case 8:
		bm = new bitmap_impl_t<8>( width, height );
		break;
*/
	case 16:
		bm = new bitmap_impl_t<16>( width, height );
		break;
	case 24:
		bm = new bitmap_impl_t<24>( width, height );
		break;
	default:
		fprintf(stderr, "%d bpp not supported\n", depth);
		assert( 0 );
	}

	bm->bits = new unsigned char [bm->bitmap_size()];
	if (!bm->bits)
		throw;
	bm->handle = alloc_gdi_handle( FALSE, GDI_OBJECT_BITMAP, 0, bm );
	if (!bm->handle)
		throw;

	return bm;
}

// parameters look the same as gdi32.CreateBitmap
HGDIOBJ NTAPI NtGdiCreateBitmap(int Width, int Height, UINT Planes, UINT BitsPerPixel, VOID* Pixels)
{
	// FIXME: handle negative heights
	assert(Height >=0);
	assert(Width >=0);
	bitmap_t *bm = NULL;
	bm = alloc_bitmap( Width, Height, BitsPerPixel );
	if (!bm)
		return NULL;
	NTSTATUS r = bm->copy_pixels( Pixels );
	if (r < STATUS_SUCCESS)
	{
		delete bm;
		return 0;
	}
	return bm->get_handle();
}

