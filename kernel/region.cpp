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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "ntwin32.h"
#include "debug.h"
#include "win32mgr.h"

class region_tt : public gdi_object_t
{
public:
	region_tt();
	static region_tt* alloc();
};

region_tt::region_tt()
{
}

region_tt* region_tt::alloc()
{
	region_tt* region = new region_tt;
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

HRGN NTAPI NtGdiCreateRectRgn( int left, int top, int right, int bottom )
{
	region_tt* region = region_tt::alloc();
	if (!region)
		return 0;
	return (HRGN) region->get_handle();
}

HRGN NTAPI NtGdiCreateEllipticRgn( int left, int top, int right, int bottom )
{
	return 0;
}

HRGN NTAPI NtGdiGetRgnBox( HRGN Region, PRECT Rect )
{
	return 0;
}

int NTAPI NtGdiCombineRgn( HRGN Dest, HRGN Source1, HRGN Source2, int CombineMode )
{
	return 0;
}

BOOL NTAPI NtGdiEqualRgn( HRGN Source1, HRGN Source2 )
{
	return 0;
}

int NTAPI NtGdiOffsetRgn( HRGN Region, int x, int y )
{
	return 0;
}

BOOL NTAPI NtGdiSetRectRgn( HRGN Region, int left, int top, int right, int bottom )
{
	return 0;
}
