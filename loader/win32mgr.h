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

#ifndef __WIN32K_MANAGER__
#define __WIN32K_MANAGER__

class win32k_info_t
{
public:
	win32k_info_t();
	// address that device context shared memory is mapped to
	BYTE* dc_shared_mem;
	BYTE* user_shared_mem;
};

class brush_t;

class win32k_manager_t
{
	HANDLE stock_object[STOCK_LAST];
public:
	win32k_manager_t();
	void init_stock_objects();
	HANDLE get_stock_object( ULONG Index );
	HANDLE create_solid_brush( COLORREF color );
	virtual ~win32k_manager_t();
	virtual BOOL init() = 0;
	virtual void fini() = 0;
	HGDIOBJ alloc_dc();
	BOOL release_dc( HGDIOBJ dc );
	virtual BOOL set_pixel( INT x, INT y, COLORREF color ) = 0;
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t *brush ) = 0;
	virtual BOOL exttextout( INT x, INT y, UINT options, LPRECT rect, UNICODE_STRING& text ) = 0;
	win32k_info_t* alloc_win32k_info();
};

extern win32k_manager_t* win32k_manager;

class gdi_object_t
{
protected:
	HGDIOBJ handle;
public:
	HGDIOBJ get_handle() {return handle;}
	virtual ~gdi_object_t() {};
	virtual BOOL release();
	static HGDIOBJ alloc( BOOL stock, ULONG type );
	void *get_shared_mem();
};

class brush_t : public gdi_object_t
{
	ULONG style;
	COLORREF color;
	ULONG hatch;
protected:
	brush_t( UINT style, COLORREF color, ULONG hatch );
public:
	static brush_t* alloc( UINT style, COLORREF color, ULONG hatch, BOOL stock = FALSE );
	COLORREF get_color() {return color;}
};

typedef struct _DEVICE_CONTEXT_SHARED_MEMORY {
	HANDLE unk;
	ULONG Flags;
	HGDIOBJ Brush;
} DEVICE_CONTEXT_SHARED_MEMORY;

class device_context_t : public gdi_object_t
{
	ULONG dc_index;
public:
	static const ULONG max_device_contexts = 0x100;
	static const ULONG dc_size = 0x100;

protected:
	// shared across all processes
	static section_t *g_dc_section;
	static BYTE *g_dc_shared_mem;
	static bool g_dc_bitmap[max_device_contexts];

protected:
	device_context_t( ULONG n );
public:
	static device_context_t* alloc();
	static int get_free_index();
	static BYTE *get_dc_shared_mem_ptr(int n);
	static BYTE* get_dc_shared_mem_base();
	DEVICE_CONTEXT_SHARED_MEMORY* get_dc_shared_mem();
	virtual BOOL release();
	brush_t *get_selected_brush();
	BOOL set_pixel( INT x, INT y, COLORREF color );
	BOOL rectangle( INT x, INT y, INT width, INT height );
	BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
};

#endif // __WIN32K_MANAGER__
