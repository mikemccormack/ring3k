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
#include "sdl.h"
#include "win.h"
#include "queue.h"

// shared across all processes (in a window station)
static section_t *gdi_ht_section;
static void *gdi_handle_table = 0;

gdi_handle_table_entry *get_handle_table_entry(HGDIOBJ handle);

class ntgdishm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

#define MAX_GDI_HANDLE 0x4000

void ntgdishm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	if (ofs < MAX_GDI_HANDLE*0x10)
	{
		char unk[16];
		const char *field = "unknown";
		switch (ofs&15)
		{
#define gdifield(ofs,x) case ofs: field = #x; break;
		gdifield(0, kernel_info)
		gdifield(4, ProcessId)
		gdifield(6, Count)
		gdifield(8, Upper)
		gdifield(10,Type)
		gdifield(11,Type_Hi)
		gdifield(12,user_info)
#undef gdifield
		default:
			field = unk;
			sprintf(unk, "unk_%04lx", ofs);
		}
		fprintf(stderr, "%04lx: accessed gdi handle[%04lx]:%s from %08lx\n",
				current->trace_id(), ofs>>4, field, eip);
	}
	else
		fprintf(stderr, "%04lx: accessed gshm[%04lx] from %08lx\n",
				current->trace_id(), ofs, eip);
}

static ntgdishm_tracer ntgdishm_trace;

BOOL gdi_object_t::release()
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	assert( entry );
	assert( entry->kernel_info == this );
	memset( entry, 0, sizeof *entry );
	delete this;
	return TRUE;
}

BOOLEAN NTAPI NtGdiInit()
{
	dprintf("\n");
	return TRUE;
}

win32k_manager_t::win32k_manager_t()
{
	memset( key_state, 0, sizeof key_state );
}

win32k_manager_t::~win32k_manager_t()
{
}

win32k_info_t::win32k_info_t() :
	dc_shared_mem( 0 ),
	user_shared_mem( 0 ),
	user_handles( 0 )
{
	memset( &stock_object, 0, sizeof stock_object );
}

win32k_info_t::~win32k_info_t()
{
}

win32k_info_t *win32k_manager_t::alloc_win32k_info()
{
	return new win32k_info_t;
}

void win32k_manager_t::send_input(INPUT* input)
{
	thread_message_queue_tt *queue = 0;
	ULONG pos;

	if (active_window)
	{
		thread_t *t = active_window->get_win_thread();
		assert(t != NULL);
		queue = t->queue;
	}

	dprintf("active window = %p\n", active_window);

	// keyboard activity
	switch (input->type)
	{
	case INPUT_KEYBOARD:
		// check for dud keycodes
		assert (input->ki.wVk <= 254);

		if (input->ki.dwFlags & KEYEVENTF_KEYUP)
		{
			key_state[input->ki.wVk] = 0;
			if (queue)
				queue->post_message( active_window->handle, WM_KEYUP, input->ki.wVk, 0 );
		}
		else
		{
			key_state[input->ki.wVk] = 0x8000;
			if (queue)
				queue->post_message( active_window->handle, WM_KEYDOWN, input->ki.wVk, 0 );
		}

		break;

	case INPUT_MOUSE:
		// FIXME: need to send a WM_NCHITTEST to figure out whether to send NC messages or not
		pos = MAKELPARAM(input->mi.dx, input->mi.dy);
		if (input->mi.dwFlags & MOUSEEVENTF_LEFTDOWN)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_LBUTTONDOWN, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_LEFTUP)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_LBUTTONUP, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_RIGHTDOWN)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_RBUTTONDOWN, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_RIGHTUP)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_RBUTTONUP, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_MIDDLEDOWN)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_MBUTTONDOWN, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_MIDDLEUP)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_MBUTTONUP, 0, pos );
		}

		if (input->mi.dwFlags & MOUSEEVENTF_MOVE)
		{
			if (queue)
				queue->post_message( active_window->handle, WM_MOUSEMOVE, 0, pos );
		}

		break;
	default:
		dprintf("unknown input %ld\n", input->type);
	}

}

ULONG win32k_manager_t::get_async_key_state( ULONG Key )
{
	if (Key > 254)
		return 0;
	return key_state[ Key ];
}

class win32k_null_t : public win32k_manager_t
{
public:
	virtual BOOL init();
	virtual void fini();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual BOOL bitblt( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop );
	virtual BOOL polypatblt( ULONG Rop, PRECT rect );
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

BOOL win32k_null_t::bitblt( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop )
{
	return TRUE;
}

BOOL win32k_null_t::polypatblt( ULONG Rop, PRECT rect )
{
	return TRUE;
}

win32k_null_t win32k_manager_null;

win32k_manager_t* init_null_win32k_manager()
{
	return &win32k_manager_null;
}

win32k_manager_t *win32k_manager;

void ntgdi_fini()
{
	if (win32k_manager)
		win32k_manager->fini();
}

NTSTATUS win32k_process_init(process_t *process)
{
	NTSTATUS r;

	if (process->win32k_info)
		return STATUS_SUCCESS;

	dprintf("\n");

	// 16bpp by default for now
	if (!win32k_manager)
		win32k_manager = init_sdl_win32k_manager();

	if (!win32k_manager)
		win32k_manager = init_null_win32k_manager();

	if (!win32k_manager->init())
		die("unable to allocate screen\n");

	process->win32k_info = win32k_manager->alloc_win32k_info();

	PPEB ppeb = (PPEB) process->peb_section->get_kernel_address();

	// only do this once per process
	if (ppeb->GdiSharedHandleTable)
		return TRUE;

	if (!gdi_handle_table)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = GDI_SHARED_HANDLE_TABLE_SIZE;
		r = create_section( &gdi_ht_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return r;

		gdi_handle_table = (BYTE*) gdi_ht_section->get_kernel_address();
	}

	// read/write for the kernel and read only for processes
	BYTE *p = GDI_SHARED_HANDLE_TABLE_ADDRESS;

	// unreserve memory so mapit doesn't get a conflicting address
	process->vm->free_virtual_memory( p, GDI_SHARED_HANDLE_TABLE_SIZE, MEM_FREE );

	r = gdi_ht_section->mapit( process->vm, p, 0, MEM_COMMIT, PAGE_READWRITE );
	if (r < STATUS_SUCCESS)
	{
		dprintf("r = %08lx\n", r);
		assert(0);
		return FALSE;
	}

	ppeb->GdiSharedHandleTable = (void*) p;

	if (option_trace)
		process->vm->set_tracer( p, ntgdishm_trace );

	return r;
}

NTSTATUS win32k_thread_init(thread_t *thread)
{
	NTSTATUS r;

	if (thread->win32k_init_complete())
		return STATUS_SUCCESS;

	r = win32k_process_init( thread->process );
	if (r < STATUS_SUCCESS)
		return r;

	ULONG size = 0;
	PVOID buffer = 0;
	r = thread->do_user_callback( NTWIN32_THREAD_INIT_CALLBACK, size, buffer );

	return r;
}

ULONG NTAPI NtGdiQueryFontAssocInfo( HANDLE hdc )
{
	dprintf("%p\n", hdc);
	return 0;
}

// returns same as AddFontResourceW
BOOLEAN NTAPI NtGdiAddFontResourceW(
	PVOID Filename,
	ULONG FilenameLength,
	ULONG u_arg3,
	ULONG u_arg4,
	PVOID p_arg5,
	ULONG u_arg6)
{
	WCHAR buf[0x100];

	FilenameLength *= 2;
	if (FilenameLength > sizeof buf)
		return FALSE;
	NTSTATUS r = copy_from_user( buf, Filename, FilenameLength );
	if (r < STATUS_SUCCESS)
		return FALSE;
	dprintf("filename = %pws\n", buf);
	return TRUE;
}

int find_free_gdi_handle(void)
{
	gdi_handle_table_entry *table = (gdi_handle_table_entry*) gdi_handle_table;

	for (int i=0; i<MAX_GDI_HANDLE; i++)
	{
		if (!table[i].ProcessId)
			return i;
	}
	return -1;
}

HGDIOBJ alloc_gdi_handle( BOOL stock, ULONG type, void *user_info, gdi_object_t* obj )
{
	int index = find_free_gdi_handle();
	if (index < 0)
		return 0;

	gdi_handle_table_entry *table = (gdi_handle_table_entry*) gdi_handle_table;
	table[index].ProcessId = current->process->id;
	table[index].Type = type;
	HGDIOBJ handle = makeHGDIOBJ(0,stock,type,index);
	table[index].Upper = (ULONG)handle >> 16;
	table[index].user_info = user_info;
	table[index].kernel_info = (void*) obj;

	return handle;
}


HGDIOBJ gdi_object_t::alloc( BOOL stock, ULONG type )
{
	gdi_object_t *obj = new gdi_object_t();
	HGDIOBJ handle = alloc_gdi_handle( stock, type, 0, obj );
	if (handle)
		obj->handle = handle;
	else
		delete obj;
	return obj->handle;
}

HGDIOBJ alloc_gdi_object( BOOL stock, ULONG type )
{
	return gdi_object_t::alloc( stock, type );
}

class dcshm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void dcshm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG dc_size = device_context_t::dc_size;
	ULONG ofs = address - mb->get_base_address();
	fprintf(stderr, "%04lx: accessed dcshm[%02lx][%02lx] from %08lx\n",
				current->trace_id(), ofs/dc_size, ofs%dc_size, eip);
}

static dcshm_tracer dcshm_trace;

section_t *device_context_t::g_dc_section;
BYTE *device_context_t::g_dc_shared_mem = 0;
bool device_context_t::g_dc_bitmap[max_device_contexts];

class memory_device_context_factory_t : public device_context_factory_t {
public:
	device_context_t* alloc(ULONG n) { return new memory_device_context_t(n); }
};

HGDIOBJ win32k_manager_t::alloc_compatible_dc()
{
	memory_device_context_factory_t factory;
	device_context_t* dc = device_context_t::alloc( &factory );
	if (!dc)
		return NULL;
	return dc->get_handle();
}

class screen_device_context_factory_t : public device_context_factory_t {
public:
	device_context_t* alloc(ULONG n) { return new screen_device_context_t(n); }
};

device_context_t* win32k_manager_t::alloc_screen_dc_ptr()
{
	static screen_device_context_factory_t factory;
	return device_context_t::alloc( &factory );
}

HGDIOBJ win32k_manager_t::alloc_screen_dc()
{
	device_context_t* dc = alloc_screen_dc_ptr();
	if (!dc)
		return NULL;
	return dc->get_handle();
}

BYTE *device_context_t::get_dc_shared_mem_base()
{
	NTSTATUS r;

	if (!g_dc_shared_mem)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = 0x10000;
		r = create_section( &g_dc_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return NULL;

		g_dc_shared_mem = (BYTE*) g_dc_section->get_kernel_address();
	}

	BYTE*& dc_shared_mem = current->process->win32k_info->dc_shared_mem;
	if (!dc_shared_mem)
	{
		r = g_dc_section->mapit( current->process->vm, dc_shared_mem, 0, MEM_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
		{
			dprintf("failed to map shared memory\n");
			return NULL;
		}

		if (option_trace)
			current->process->vm->set_tracer( dc_shared_mem, dcshm_trace );
	}
	return dc_shared_mem;
}

int device_context_t::get_free_index()
{
	// find a free device context area
	ULONG n;
	for (n=0; n<max_device_contexts; n++)
		if (!g_dc_bitmap[n])
			break;
	if (n >= max_device_contexts)
	{
		dprintf("no device contexts left\n");
		return -1;
	}
	g_dc_bitmap[n] = true;
	return n;
}

device_context_t::device_context_t( ULONG n ) :
	dc_index( n ),
	selected_bitmap( 0 )
{
}

DEVICE_CONTEXT_SHARED_MEMORY* device_context_t::get_dc_shared_mem()
{
	return (DEVICE_CONTEXT_SHARED_MEMORY*) (g_dc_shared_mem + dc_index * dc_size);
}

device_context_t* device_context_t::alloc( device_context_factory_t *factory )
{
	BYTE* dc_shared_mem = get_dc_shared_mem_base();
	if (!dc_shared_mem)
		return NULL;

	int n = get_free_index();
	if (n < 0)
		return NULL;

	device_context_t* dc = factory->alloc( n );
	if (!dc)
		return NULL;

	// calculate user side pointer to the chunk
	BYTE *u_shm = dc_shared_mem + n*dc_size;
	dprintf("dc number %02x address %p\n", n, u_shm );
	dc->handle = alloc_gdi_handle( FALSE, GDI_OBJECT_DC, u_shm, dc );

	DEVICE_CONTEXT_SHARED_MEMORY *dcshm = dc->get_dc_shared_mem();
	dcshm->Brush = win32k_manager->get_stock_object( WHITE_BRUSH );

	return dc;
}

BOOL device_context_t::release()
{
	assert( dc_index <= max_device_contexts );
	assert( g_dc_bitmap[dc_index] );
	g_dc_bitmap[dc_index] = false;
	gdi_object_t::release();
	return TRUE;
}

HANDLE device_context_t::select_bitmap( bitmap_t *bitmap )
{
	bitmap_t* old = selected_bitmap;
	selected_bitmap = bitmap;
	if (!old)
		return NULL;
	return old->get_handle();
}

bitmap_t* device_context_t::get_selected_bitmap()
{
	return selected_bitmap;
}

BOOL device_context_t::bitblt(
	INT xDest, INT yDest,
	INT cx, INT cy,
	device_context_t *src,
	INT xSrc, INT ySrc, ULONG rop )
{
	// FIXME translate coordinates
	return win32k_manager->bitblt( xDest, yDest, cx, cy, src, xSrc, ySrc, rop );
}

BOOL screen_device_context_t::rectangle(INT left, INT top, INT right, INT bottom )
{
	brush_t *brush = get_selected_brush();
	if (!brush)
		return FALSE;
	dprintf("drawing with brush %p with color %08lx\n", brush->get_handle(), brush->get_color() );
	return win32k_manager->rectangle( left, top, right, bottom, brush );
}

BOOL screen_device_context_t::polypatblt( ULONG Rop, PRECT rect )
{
	return win32k_manager->polypatblt( Rop, rect );
}

screen_device_context_t::screen_device_context_t( ULONG n ) :
	device_context_t( n ),
	win( 0 )
{
}

BOOL screen_device_context_t::set_pixel( INT x, INT y, COLORREF color )
{
	return win32k_manager->set_pixel( x, y, color );
}

BOOL screen_device_context_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return win32k_manager->exttextout( x, y, options, rect, text );
}

COLORREF screen_device_context_t::get_pixel( INT x, INT y )
{
	return 0;
}

memory_device_context_t::memory_device_context_t( ULONG n ) :
	device_context_t( n )
{
}

BOOL memory_device_context_t::rectangle(INT left, INT top, INT right, INT bottom )
{
	brush_t *brush = get_selected_brush();
	if (!brush)
		return FALSE;
	dprintf("\n");
	return TRUE;
}

BOOL memory_device_context_t::set_pixel( INT x, INT y, COLORREF color )
{
	dprintf("\n");
	return TRUE;
}

COLORREF memory_device_context_t::get_pixel( INT x, INT y )
{
	bitmap_t* bitmap = get_selected_bitmap();
	if (bitmap)
		return bitmap->get_pixel( x, y );
	return 0;
}

BOOL memory_device_context_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return TRUE;
}

BOOL memory_device_context_t::polypatblt( ULONG Rop, PRECT rect )
{
	return TRUE;
}

brush_t::brush_t( UINT _style, COLORREF _color, ULONG _hatch ) :
	style( _style ),
	color( _color ),
	hatch( _hatch )
{
}

HANDLE brush_t::alloc( UINT style, COLORREF color, ULONG hatch, BOOL stock )
{
	brush_t* brush = new brush_t( style, color, hatch );
	if (!brush)
		return NULL;
	brush->handle = alloc_gdi_handle( stock, GDI_OBJECT_BRUSH, NULL, brush );
	dprintf("created brush %p with color %08lx\n", brush->handle, color );
	return brush->handle;
}

brush_t* brush_from_handle( HGDIOBJ handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_BRUSH)
		return FALSE;
	assert( entry->kernel_info );
	return (brush_t*) entry->kernel_info;
}

brush_t* device_context_t::get_selected_brush()
{
	DEVICE_CONTEXT_SHARED_MEMORY *dcshm = get_dc_shared_mem();
	if (!dcshm)
		return NULL;
	return brush_from_handle( dcshm->Brush );
}

device_context_t* dc_from_handle( HGDIOBJ handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_DC)
		return FALSE;
	assert( entry->kernel_info );
	return (device_context_t*) entry->kernel_info;
}

BOOL win32k_manager_t::release_dc( HGDIOBJ handle )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;
	return dc->release();
}

HGDIOBJ NTAPI NtGdiGetStockObject(ULONG Index)
{
	return win32k_manager->get_stock_object( Index );
}

HANDLE win32k_manager_t::create_solid_brush( COLORREF color )
{
	return brush_t::alloc( BS_SOLID, color, 0 );
}

HANDLE win32k_manager_t::get_stock_object( ULONG Index )
{
	if (Index > STOCK_LAST)
		return 0;
	HANDLE& handle = current->process->win32k_info->stock_object[Index];
	if (handle)
		return handle;

	switch (Index)
	{
	case WHITE_BRUSH:
		handle = brush_t::alloc( 0, RGB(255,255,255), 0, TRUE);
		break;
	case BLACK_BRUSH:
		handle = brush_t::alloc( 0, RGB(0,0,0), 0, TRUE);
		break;
	case LTGRAY_BRUSH:
	case GRAY_BRUSH:
	case DKGRAY_BRUSH:
	case NULL_BRUSH: //case HOLLOW_BRUSH:
	case DC_BRUSH: // FIXME: probably per DC
		handle = alloc_gdi_object( TRUE, GDI_OBJECT_BRUSH );
		break;
	case WHITE_PEN:
	case BLACK_PEN:
	case NULL_PEN:
	case DC_PEN: // FIXME: probably per DC
		handle = alloc_gdi_object( TRUE, GDI_OBJECT_PEN );
		break;
	case OEM_FIXED_FONT:
	case ANSI_FIXED_FONT:
	case ANSI_VAR_FONT:
	case SYSTEM_FONT:
	case DEVICE_DEFAULT_FONT:
	case SYSTEM_FIXED_FONT:
	case DEFAULT_GUI_FONT:
		handle = alloc_gdi_object( TRUE, GDI_OBJECT_FONT );
		break;
	case DEFAULT_PALETTE:
		handle = alloc_gdi_object( TRUE, GDI_OBJECT_PALETTE );
		break;
	}
	return handle;
}

bitmap_t::bitmap_t( int _width, int _height, int _planes, int _bpp ) :
	bits( 0 ),
	width( _width ),
	height( _height ),
	planes( _planes ),
	bpp( _bpp )
{
}

bitmap_t::~bitmap_t()
{
	delete bits;
}

ULONG bitmap_t::get_rowsize()
{
	ULONG row_size = (width*bpp)/8;
	return (row_size + 1)& ~1;
}

ULONG bitmap_t::bitmap_size()
{
	return height * get_rowsize();
}

void bitmap_t::dump()
{
	for (int j=0; j<height; j++)
	{
		for (int i=0; i<width; i++)
			fprintf(stderr,"%c", get_pixel(i, j)? 'X' : ' ');
		fprintf(stderr, "\n");
	}
}

HANDLE bitmap_t::alloc( int width, int height, int planes, int bpp, void *pixels )
{
	bitmap_t* bitmap = new bitmap_t( width, height, planes, bpp );
	if (!bitmap)
		return NULL;
	bitmap->handle = alloc_gdi_handle( FALSE, GDI_OBJECT_BITMAP, 0, bitmap );
	bitmap->bits = new unsigned char [bitmap->bitmap_size()];
	copy_from_user( bitmap->bits, pixels, bitmap->bitmap_size() );
	return bitmap->handle;
}

COLORREF bitmap_t::get_pixel( int x, int y )
{
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	ULONG row_size = get_rowsize();
	switch (bpp)
	{
	case 1:
		if ((bits[row_size * y + x*bpp/8 ]>> (7 - (x%8))) & 1)
			return RGB( 255, 255, 255 );
		else
			return RGB( 0, 0, 0 );
	default:
		dprintf("%d bpp not implemented\n", bpp);
	}
	return 0;
}

bitmap_t* bitmap_from_handle( HANDLE handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_BITMAP)
		return FALSE;
	assert( entry->kernel_info );
	return (bitmap_t*) entry->kernel_info;
}

// parameters look the same as gdi32.CreateBitmap
HGDIOBJ NTAPI NtGdiCreateBitmap(int Width, int Height, UINT Planes, UINT BitsPerPixel, VOID* Pixels)
{
	dprintf("(%dx%d) %d %d %p\n", Width, Height, Planes, BitsPerPixel, Pixels);
	// FIXME: handle negative heights
	assert(Height >=0);
	assert(Width >=0);
	return bitmap_t::alloc( Width, Height, Planes, BitsPerPixel, Pixels );
}

// gdi32.CreateComptabibleDC
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ hdc)
{
	dprintf("%p\n", hdc);
	return win32k_manager->alloc_compatible_dc();
}

// has one more parameter than gdi32.CreateSolidBrush
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF Color, ULONG u_arg2)
{
	return win32k_manager->create_solid_brush( Color );
}

// looks like CreateDIBitmap, with BITMAPINFO unpacked
HGDIOBJ NTAPI NtGdiCreateDIBitmapInternal(
	HDC hdc,
	ULONG Width,
	ULONG Height,
	ULONG Bpp,
	ULONG u_arg5,
	PVOID u_arg6,
	ULONG u_arg7,
	ULONG u_arg8,
	ULONG u_arg9,
	ULONG u_arg10,
	ULONG u_arg11 )
{
	dprintf("%p %08lx %08lx %08lx %08lx %p %08lx %08lx %08lx %08lx %08lx\n",
			hdc, Width, Height, Bpp, u_arg5, u_arg6, u_arg7, u_arg8, u_arg9, u_arg10, u_arg11 );
	return alloc_gdi_object( FALSE, GDI_OBJECT_BITMAP );
}

HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ Bitmap)
{
	dprintf("%p\n", Bitmap);
	return win32k_manager->alloc_screen_dc();
}

gdi_handle_table_entry *get_handle_table_entry(HGDIOBJ handle)
{
	gdi_handle_table_entry *table = (gdi_handle_table_entry*) gdi_handle_table;
	ULONG index = (ULONG)handle&0xffff;
	ULONG upper = (ULONG)handle>>16;
	if (index >= MAX_GDI_HANDLE)
		return 0;
	if (upper != table[index].Upper)
		return 0;
	return &table[index];
}

BOOLEAN NTAPI NtGdiDeleteObjectApp(HGDIOBJ Object)
{
	gdi_handle_table_entry *entry = get_handle_table_entry(Object);
	if (!entry)
		return FALSE;
	if (entry->ProcessId != current->process->id)
	{
		dprintf("pirate deletion! %p\n", Object);
		return FALSE;
	}

	gdi_object_t *obj = (gdi_object_t*) entry->kernel_info;
	assert( obj );

	return obj->release();
}

const char *get_object_type_name( HGDIOBJ object )
{
	switch (get_handle_type( object ))
	{
	case GDI_OBJECT_BRUSH: return "brush";
	case GDI_OBJECT_PEN: return "pen";
	case GDI_OBJECT_PALETTE: return "palette";
	case GDI_OBJECT_FONT: return "font";
	case GDI_OBJECT_BITMAP: return "bitmap";
	}
	return "unknown";
}

HGDIOBJ NTAPI NtGdiSelectBitmap( HGDIOBJ hdc, HGDIOBJ hbm )
{
	dprintf("%p %p\n", hdc, hbm );

	device_context_t* dc = dc_from_handle( hdc );
	if (!dc)
		return FALSE;

	bitmap_t* bitmap = bitmap_from_handle( hbm );
	if (!bitmap)
		return FALSE;

	return dc->select_bitmap( bitmap );
}

// Info
//  1 Long font name
//  2 LOGFONTW
//  4 Full path name
BOOLEAN NTAPI NtGdiGetFontResourceInfoInternalW(
	LPWSTR Files,
	ULONG cwc,
	ULONG cFiles,
	UINT cjIn,
	PULONG BufferSize,
	PVOID Buffer,
	ULONG Info)
{
	dprintf("\n");
	return FALSE;
}

BOOLEAN NTAPI NtGdiFlush(void)
{
	dprintf("\n");
	return FALSE;
}

int NTAPI NtGdiSaveDC(HGDIOBJ hdc)
{
	static int count = 1;
	dprintf("%p\n", hdc);
	return count++;
}

BOOLEAN NTAPI NtGdiRestoreDC(HGDIOBJ hdc, int saved_dc)
{
	dprintf("%p %d\n", hdc, saved_dc);
	return TRUE;
}

HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ hdc, ULONG object_type)
{
	dprintf("%p %08lx\n", hdc, object_type);
	return win32k_manager->alloc_screen_dc();
}

// fun...
ULONG NTAPI NtGdiSetDIBitsToDeviceInternal(
	HGDIOBJ hdc, int xDest, int yDest, ULONG cx, ULONG cy,
	int xSrc, int ySrc, ULONG StartScan, ULONG ScanLines,
	PVOID Bits, PVOID bmi, ULONG Color, ULONG, ULONG, ULONG, ULONG)
{
	dprintf("%p %d %d %ld %ld %d %d...\n", hdc, xDest, yDest, cx, cy, xSrc, ySrc);
	return cy;
}

ULONG NTAPI NtGdiExtGetObjectW(HGDIOBJ Object, ULONG Size, PVOID Buffer)
{
	union {
		BITMAP bm;
	} info;
	ULONG len = 0;

	dprintf("%p %ld %p\n", Object, Size, Buffer);

	memset( &info, 0, sizeof info );
	switch (get_handle_type(Object))
	{
	case GDI_OBJECT_BITMAP:
		dprintf("GDI_OBJECT_BITMAP\n");
		len = sizeof info.bm;
		info.bm.bmType = 0;
		info.bm.bmWidth = 0x10;
		info.bm.bmHeight = 0x10;
		info.bm.bmWidthBytes = 2;
		info.bm.bmPlanes = 1;  // monocrome
		info.bm.bmBits = (PBYTE) 0xbbbb0001;
		break;
	default:
		dprintf("should return data for ?\n");
	}

	if (Size < len)
		return 0;

	NTSTATUS r = copy_to_user( Buffer, &info, len );
	if (r < STATUS_SUCCESS)
		return 0;

	return len;
}

BOOLEAN NTAPI NtGdiBitBlt(HGDIOBJ hdcDest, INT xDest, INT yDest, INT cx, INT cy, HGDIOBJ hdcSrc, INT xSrc, INT ySrc, ULONG rop, ULONG, ULONG)
{
	dprintf("\n");

	device_context_t* src = dc_from_handle( hdcSrc );
	if (!src)
		return FALSE;

	device_context_t* dest = dc_from_handle( hdcSrc );
	if (!dest)
		return FALSE;

	return dest->bitblt( xDest, yDest, cx, cy, dest, xSrc, ySrc, rop );
}

HANDLE NTAPI NtGdiCreateDIBSection(
		HDC DeviceContext,
		HANDLE SectionHandle,
		ULONG Offset,
		PBITMAPINFO bmi,
		ULONG Usage,
		ULONG HeaderSize,
		ULONG Unknown,
		ULONG_PTR ColorSpace,
		PVOID Bits)
{
	return alloc_gdi_object( FALSE, GDI_OBJECT_BITMAP );
}

ULONG NTAPI NtGdiSetFontEnumeration(ULONG Unknown)
{
	return 0;
}

HANDLE NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID)
{
	return win32k_manager->alloc_screen_dc();
}

typedef struct _font_enum_entry {
	ULONG size;
	ULONG offset;
	ULONG fonttype;
	ENUMLOGFONTEXW elfew;
	ULONG pad1[2];
	ULONG pad2;
	ULONG flags;
	NEWTEXTMETRICEXW ntme;
	ULONG pad3[2];
} font_enum_entry;

void fill_font( font_enum_entry* fee, LPWSTR name, ULONG height, ULONG width, ULONG paf, ULONG weight, ULONG flags, ULONG charset )
{
	memset( fee, 0, sizeof *fee );
	fee->size = sizeof *fee;
	fee->fonttype = RASTER_FONTTYPE;
	fee->offset = FIELD_OFFSET( font_enum_entry, pad2 );
	fee->elfew.elfLogFont.lfHeight = height;
	fee->elfew.elfLogFont.lfWidth = width;
	fee->elfew.elfLogFont.lfWeight = weight;
	fee->elfew.elfLogFont.lfPitchAndFamily = paf;
	memcpy( fee->elfew.elfLogFont.lfFaceName, name, strlenW(name)*2 );
	memcpy( fee->elfew.elfFullName, name, strlenW(name)*2 );
	fee->flags = flags;

	fee->ntme.ntmTm.tmHeight = height;
	fee->ntme.ntmTm.tmAveCharWidth = width;
	fee->ntme.ntmTm.tmMaxCharWidth = width;
	fee->ntme.ntmTm.tmWeight = weight;
	fee->ntme.ntmTm.tmPitchAndFamily = paf;
	fee->ntme.ntmTm.tmCharSet = charset;
}

void fill_system( font_enum_entry* fee )
{
	WCHAR sys[] = { 'S','y','s','t','e','m',0 };
	fill_font( fee, sys, 16, 7, FF_SWISS | VARIABLE_PITCH, FW_BOLD, 0x2080ff20, ANSI_CHARSET );
}

void fill_terminal( font_enum_entry* fee )
{
	WCHAR trm[] = { 'T','e','r','m','i','n','a','l',0 };
	fill_font( fee, trm, 12, 8, FF_MODERN | FIXED_PITCH, FW_REGULAR, 0x2020fe01, OEM_CHARSET );
}

HANDLE NTAPI NtGdiEnumFontOpen(
	HANDLE DeviceContext,
	ULONG,
	ULONG,
	ULONG,
	ULONG,
	ULONG,
	PULONG DataLength)
{
	ULONG len = sizeof (font_enum_entry)*2;
	NTSTATUS r = copy_to_user( DataLength, &len, sizeof len );
	if (r < STATUS_SUCCESS)
		return 0;

	return alloc_gdi_object( FALSE, 0x3f );
}

BOOLEAN NTAPI NtGdiEnumFontChunk(
	HANDLE DeviceContext,
	HANDLE FontEnumeration,
	ULONG BufferLength,
	PULONG ReturnLength,
	PVOID Buffer)
{
	font_enum_entry fee[2];
	ULONG len = sizeof fee;

	if (BufferLength < len)
		return FALSE;

	fill_system( &fee[0] );
	fill_terminal( &fee[1] );

	NTSTATUS r = copy_to_user( Buffer, &fee, len );
	if (r < STATUS_SUCCESS)
		return FALSE;

	r = copy_to_user( ReturnLength, &len, sizeof len );
	if (r < STATUS_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE FontEnumeration)
{
	return TRUE;
}

BOOLEAN NTAPI NtGdiGetTextMetricsW(HANDLE DeviceContext, PVOID Buffer, ULONG Length)
{
	font_enum_entry fee;
	NTSTATUS r;

	fill_system( &fee );

	if (Length < sizeof (TEXTMETRICW))
		return FALSE;

	r = copy_to_user( Buffer, &fee.ntme, sizeof (TEXTMETRICW) );
	if (r < STATUS_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOLEAN NTAPI NtGdiSetIcmMode(HANDLE DeviceContext, ULONG, ULONG)
{
	return TRUE;
}

BOOLEAN NTAPI NtGdiComputeXformCoefficients( HANDLE DeviceContext )
{
	if (get_handle_type( DeviceContext ) != GDI_OBJECT_DC)
		return FALSE;
	return TRUE;
}

BOOLEAN NTAPI NtGdiSetPixel( HANDLE handle, INT x, INT y, COLORREF color )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;
	return dc->set_pixel( x, y, color );
}

BOOLEAN NTAPI NtGdiRectangle( HANDLE handle, INT left, INT top, INT right, INT bottom )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	return dc->rectangle( left, top, right, bottom );
}

BOOLEAN NTAPI NtGdiExtTextOutW( HANDLE handle, INT x, INT y, UINT options,
		 LPRECT rect, WCHAR* string, UINT length, INT *dx, UINT )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;
	RECT rectangle;
	NTSTATUS r;
	if (rect)
	{
		r = copy_from_user( &rectangle, rect, sizeof *rect );
		if (r < STATUS_SUCCESS)
			return FALSE;
		rect = &rectangle;
	}

	unicode_string_t text;
	r = text.copy_wstr_from_user( string, length*2 );
	if (r < STATUS_SUCCESS)
		return FALSE;

	if (dx)
		dprintf("character spacing provided but ignored\n");

	return dc->exttextout( x, y, options, rect, text );
}

HANDLE NTAPI NtGdiCreateCompatibleBitmap(HANDLE DeviceContext, int width, int height)
{
	return alloc_gdi_object( FALSE, GDI_OBJECT_BITMAP );
}

int NTAPI NtGdiGetAppClipBox( HANDLE handle, RECT* rectangle )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	NTSTATUS r = copy_to_user( rectangle, &dc->get_bounds_rect(), sizeof *rectangle );
	if (r < STATUS_SUCCESS)
		return ERROR;

	return SIMPLEREGION;
}

BOOLEAN NTAPI NtGdiPolyPatBlt( HANDLE handle, ULONG Rop, PRECT Rectangle, ULONG, ULONG)
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	// copy the rectangle
	RECT rect;
	NTSTATUS r;
	r = copy_from_user( &rect, Rectangle, sizeof rect );
	if (r != STATUS_SUCCESS)
		return FALSE;

	return dc->polypatblt( Rop, &rect );
}

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
