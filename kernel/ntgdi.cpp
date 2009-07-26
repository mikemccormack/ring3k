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
#include "sdl.h"
#include "null_display.h"
#include "win.h"
#include "queue.h"
#include "alloc_bitmap.h"

// shared across all processes (in a window station)
static section_t *gdi_ht_section;
static void *gdi_handle_table = 0;

win32k_manager_t* (*win32k_manager_create)();

struct graphics_driver_list {
	const char *name;
	win32k_manager_t* (*create)();
};

struct graphics_driver_list graphics_drivers[] = {
	{ "sdl", &init_sdl_win32k_manager, },
	{ "null", &init_null_win32k_manager, },
	{ NULL, NULL, },
};

bool set_graphics_driver( const char *driver )
{
	int i;

	for (i=0; graphics_drivers[i].name; i++)
	{
		if (!strcmp(graphics_drivers[i].name, driver))
		{
			win32k_manager_create = graphics_drivers[i].create;
			return true;
		}
	}

	return false;
}

void list_graphics_drivers()
{
	int i;

	for (i=0; graphics_drivers[i].name; i++)
		printf("%s ", graphics_drivers[i].name);
}

win32k_manager_t *win32k_manager;

class ntgdishm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
	virtual bool enabled() const;
};

bool ntgdishm_tracer::enabled() const
{
	return trace_is_enabled( "gdishm" );
}

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

gdi_object_t::gdi_object_t() :
	handle( 0 ),
	refcount( 0 )
{
}

BOOL gdi_object_t::release()
{
	if (refcount)
		return FALSE;
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	assert( entry );
	assert( reinterpret_cast<gdi_object_t*>( entry->kernel_info ) == this );
	memset( entry, 0, sizeof *entry );
	delete this;
	return TRUE;
}

BOOLEAN NTAPI NtGdiInit()
{
	return do_gdi_init();
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

	if (!win32k_manager)
	{
		if (win32k_manager_create)
			win32k_manager = win32k_manager_create();
		else
		{
			for (int i=0; graphics_drivers[i].name && !win32k_manager; i++)
				win32k_manager = graphics_drivers[i].create();
		}
	}

	if (!win32k_manager)
		die("failed to allocate graphics driver\n");

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
        table[index].Count = 0;
	table[index].Upper = (ULONG)handle >> 16;
	table[index].user_info = user_info;
	table[index].kernel_info = reinterpret_cast<void*>( obj );

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

class gdishm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
	virtual bool enabled() const;
};

static inline ULONG get_gdi_type_size( ULONG type )
{
	switch (type)
	{
	case GDI_OBJECT_DC:
		return sizeof (GDI_DEVICE_CONTEXT_SHARED);
	default:
		return 0;
	}
}

ULONG object_from_memory( BYTE *address )
{
	gdi_handle_table_entry *table = (gdi_handle_table_entry*) gdi_handle_table;
	for (ULONG i=0; i<MAX_GDI_HANDLE; i++)
	{
		ULONG sz = get_gdi_type_size( table[i].Type );
		if (!sz)
			continue;
		BYTE* ptr = (BYTE*) table[i].user_info;
		if (ptr > address)
			continue;
		if ((ptr+sz) > address)
			return i;
	}
	return 0;
}

void gdishm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG n = object_from_memory( address );
	if (n)
	{
		gdi_handle_table_entry *table = (gdi_handle_table_entry*) gdi_handle_table;
		ULONG ofs = address - (BYTE*) table[n].user_info;
		fprintf(stderr, "%04lx: accessed gdishm[%04lx][%04lx] from %08lx\n",
				current->trace_id(), n, ofs, eip);
	}
	else
	{
		ULONG ofs = address - mb->get_base_address();
		fprintf(stderr, "%04lx: accessed gdishm[%04lx] from %08lx\n",
				current->trace_id(), ofs, eip);
	}
}

bool gdishm_tracer::enabled() const
{
	return trace_is_enabled( "gdishm" );
}

static gdishm_tracer gdishm_trace;

section_t *gdi_object_t::g_gdi_section;
BYTE *gdi_object_t::g_gdi_shared_memory;
allocation_bitmap_t* gdi_object_t::g_gdi_shared_bitmap;

HGDIOBJ win32k_manager_t::alloc_compatible_dc()
{
	device_context_t* dc = new memory_device_context_t;
	if (!dc)
		return NULL;
	return dc->get_handle();
}

HGDIOBJ win32k_manager_t::alloc_screen_dc()
{
	device_context_t* dc = alloc_screen_dc_ptr();
	if (!dc)
		return NULL;
	return dc->get_handle();
}

void gdi_object_t::init_gdi_shared_mem()
{
	NTSTATUS r;
	int dc_shared_memory_size = 0x10000;

	if (!g_gdi_shared_memory)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = dc_shared_memory_size;
		r = create_section( &g_gdi_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		assert (r >= STATUS_SUCCESS);

		g_gdi_shared_memory = (BYTE*) g_gdi_section->get_kernel_address();

		assert( g_gdi_shared_bitmap == NULL );
		g_gdi_shared_bitmap = new allocation_bitmap_t;
		g_gdi_shared_bitmap->set_area( g_gdi_shared_memory, dc_shared_memory_size );
	}

	BYTE*& dc_shared_mem = current->process->win32k_info->dc_shared_mem;
	if (!dc_shared_mem)
	{
		r = g_gdi_section->mapit( current->process->vm, dc_shared_mem, 0, MEM_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
		{
			dprintf("failed to map shared memory\n");
			assert( 0 );
		}

		current->process->vm->set_tracer( dc_shared_mem, gdishm_trace );
	}
}

// see SaveDC in wine/dlls/gdi32/dc.c
int device_context_t::save_dc()
{
	dc_state_tt *dcs = new dc_state_tt;
	dcs->next = saved_dc;
	saved_dc = dcs;

	// FIXME: actually copy the state

	return ++saveLevel;
}

// see RestoreDC in wine/dlls/gdi32/dc.c
BOOL device_context_t::restore_dc( int level )
{
	if (level == 0)
		return FALSE;

	if (abs(level) > saveLevel)
		return FALSE;

	if (level < 0)
		level = saveLevel + level + 1;

	BOOL success=TRUE;
	while (saveLevel >= level)
	{
		dc_state_tt *dcs = saved_dc;
		saved_dc = dcs->next;
		dcs->next = 0;
		if (--saveLevel < level)
		{
			// FIXME: actually restore the state
			//set_dc_state( hdc, hdcs );
		}
		delete dcs;
	}
	return success;
}

BYTE* gdi_object_t::get_shared_mem() const
{
	return user_to_kernel( get_user_shared_mem() );
}

BYTE* gdi_object_t::get_user_shared_mem() const
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	assert( entry != NULL );
	return (BYTE*) entry->user_info;
}

GDI_DEVICE_CONTEXT_SHARED* device_context_t::get_dc_shared_mem() const
{
	return (GDI_DEVICE_CONTEXT_SHARED*) get_shared_mem();
}

BYTE *gdi_object_t::alloc_gdi_shared_memory( size_t len, BYTE** kernel_shm )
{
	init_gdi_shared_mem();
	return g_gdi_shared_bitmap->alloc( len );
}

void gdi_object_t::free_gdi_shared_memory( BYTE *shm )
{
	g_gdi_shared_bitmap->free( shm );
}

device_context_t::device_context_t() :
	selected_bitmap( 0 ),
	saved_dc( 0 ),
	saveLevel( 0 )
{
	// calculate user side pointer to the chunk
	BYTE *shm = alloc_gdi_shared_memory( sizeof (GDI_DEVICE_CONTEXT_SHARED) );
	if (!shm)
		throw;

	dprintf("dc offset %08x\n", shm - g_gdi_shared_memory );
	BYTE *user_shm = gdi_object_t::kernel_to_user( shm );

	handle = alloc_gdi_handle( FALSE, GDI_OBJECT_DC, user_shm, this );
	if (!handle)
		throw;

	GDI_DEVICE_CONTEXT_SHARED *dcshm = get_dc_shared_mem();
	dcshm->Brush = (HBRUSH) win32k_manager->get_stock_object( WHITE_BRUSH );
	dcshm->Pen = (HPEN) win32k_manager->get_stock_object( WHITE_PEN );
	dcshm->TextColor = RGB( 0, 0, 0 );
	dcshm->BackgroundColor = RGB( 255, 255, 255 );
}

BOOL device_context_t::release()
{
	GDI_DEVICE_CONTEXT_SHARED *shm = get_dc_shared_mem();
	g_gdi_shared_bitmap->free( (unsigned char*) shm, sizeof *shm );
	gdi_object_t::release();
	return TRUE;
}

HANDLE device_context_t::select_bitmap( bitmap_t *bitmap )
{
	assert( bitmap->is_valid() );
	bitmap_t* old = selected_bitmap;
	selected_bitmap = bitmap;
	bitmap->select();
	if (!old)
		return NULL;
	assert( old->is_valid() );
	old->deselect();
	return old->get_handle();
}

bitmap_t* device_context_t::get_bitmap()
{
	if (selected_bitmap)
		assert( selected_bitmap->is_valid() );
	return selected_bitmap;
}

BOOL device_context_t::bitblt(
	INT xDest, INT yDest,
	INT cx, INT cy,
	bitmap_t *src,
	INT xSrc, INT ySrc, ULONG rop )
{
	// FIXME translate coordinates
	return win32k_manager->bitblt( xDest, yDest, cx, cy, src, xSrc, ySrc, rop );
}

memory_device_context_t::memory_device_context_t()
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

BOOL memory_device_context_t::lineto(INT left, INT top)
{
    return TRUE;
}

BOOL memory_device_context_t::set_pixel( INT x, INT y, COLORREF color )
{
	bitmap_t* bitmap = get_bitmap();
	if (bitmap)
		return bitmap->set_pixel( x, y, color );
	return TRUE;
}

COLORREF memory_device_context_t::get_pixel( INT x, INT y )
{
	bitmap_t* bitmap = get_bitmap();
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

int memory_device_context_t::getcaps( int index )
{
	dprintf("%d\n", index );
	return 0;
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
	gdi_object_t* obj = reinterpret_cast<gdi_object_t*>( entry->kernel_info );
	return static_cast<brush_t*>( obj );
}

pen_t::pen_t( UINT _style, UINT _width, COLORREF _color ) :
	style( _style ),
    width( _width ),
	color( _color )
{
}

HANDLE pen_t::alloc( UINT style, UINT width, COLORREF color, BOOL stock )
{
	pen_t* pen = new pen_t( style, width, color );
	if (!pen)
		return NULL;
	pen->handle = alloc_gdi_handle( stock, GDI_OBJECT_PEN, NULL, pen );
	dprintf("created pen %p with color %08lx\n", pen->handle, color );
	return pen->handle;
}

pen_t* pen_from_handle( HGDIOBJ handle )
{
  gdi_handle_table_entry *entry = get_handle_table_entry( handle );
  if (!entry) 
    return NULL;

  if (entry->Type != GDI_OBJECT_PEN)
    return NULL;


  gdi_object_t* obj = reinterpret_cast<gdi_object_t*>( entry->kernel_info );
  return static_cast<pen_t*>( obj );
}

brush_t* device_context_t::get_selected_brush()
{
	GDI_DEVICE_CONTEXT_SHARED *dcshm = get_dc_shared_mem();
	if (!dcshm)
		return NULL;
	return brush_from_handle( dcshm->Brush );
}

pen_t* device_context_t::get_selected_pen()
{
  GDI_DEVICE_CONTEXT_SHARED *dcshm = get_dc_shared_mem();
  if (!dcshm)
    return NULL;

  return pen_from_handle( dcshm->Pen );;
}

LPPOINT device_context_t::get_current_pen_pos()
{
  GDI_DEVICE_CONTEXT_SHARED *dcshm = get_dc_shared_mem();
  if (!dcshm)
    return NULL;

  return &dcshm->CurrentPenPos;
}

device_context_t* dc_from_handle( HGDIOBJ handle )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( handle );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_DC)
		return FALSE;
	gdi_object_t* obj = reinterpret_cast<gdi_object_t*>( entry->kernel_info );
	return static_cast<device_context_t*>( obj );
}

COLORREF get_di_pixel_4bpp( stretch_di_bits_args& args, int x, int y )
{
	int bytes_per_line = ((args.info->biWidth+3)&~3)>>1;
	int ofs = (args.info->biHeight - y - 1) * bytes_per_line + (x>>1);

	// slow!
	BYTE pixel = 0;
	NTSTATUS r;
	r = copy_from_user( &pixel, (BYTE*) args.bits + ofs, 1 );
	if ( r < STATUS_SUCCESS)
	{
		dprintf("copy failed\n");
		return 0;
	}

	BYTE val = (pixel >> (x&1?0:4)) & 0x0f;

	assert( val < 16);

	return RGB( args.colors[val].rgbRed,
		args.colors[val].rgbGreen,
		args.colors[val].rgbBlue );
}

COLORREF get_di_pixel( stretch_di_bits_args& args, int x, int y )
{
	switch (args.info->biBitCount)
	{
	case 4:
		return get_di_pixel_4bpp( args, x, y );
	default:
		dprintf("%d bpp\n", args.info->biBitCount);
	}
	return 0;
}

BOOL device_context_t::stretch_di_bits( stretch_di_bits_args& args )
{
	bitmap_t* bitmap = get_bitmap();
	if (!bitmap)
		return FALSE;

	args.src_x = max( args.src_x, 0 );
	args.src_y = max( args.src_y, 0 );
	args.src_x = min( args.src_x, args.info->biWidth );
	args.src_y = min( args.src_y, args.info->biHeight );

	args.src_width = max( args.src_width, 0 );
	args.src_height = max( args.src_height, 0 );
	args.src_width = min( args.src_width, args.info->biWidth - args.src_x );
	args.src_height = min( args.src_height, args.info->biHeight - args.src_y );

	dprintf("w,h %ld,%ld\n", args.info->biWidth, args.info->biHeight);
	dprintf("bits, planes %d,%d\n", args.info->biBitCount, args.info->biPlanes);
	dprintf("compression %08lx\n", args.info->biCompression );
	dprintf("size %08lx\n", args.info->biSize );

	// copy the pixels
	COLORREF pixel;
	for (int i=0; i<args.src_height; i++)
	{
		for (int j=0; j<args.src_width; j++)
		{
			pixel = get_di_pixel( args, args.src_x+j, args.src_y+i );
			set_pixel( args.dest_x+j, args.dest_y+i, pixel );
		}
	}

	return TRUE;
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

HANDLE win32k_manager_t::create_pen( UINT style, UINT width, COLORREF color )
{
	return pen_t::alloc( style, width, color );
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
        handle = pen_t::alloc( PS_SOLID, 1, RGB(255, 255, 255), TRUE );
        break;
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

// gdi32.CreateComptabibleDC
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ hdc)
{
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
	ULONG,
	PVOID,
	ULONG,
	ULONG,
	ULONG,
	ULONG,
	ULONG)
{
	bitmap_t *bm = alloc_bitmap( Width, Height, Bpp );
	if (!bm)
		return NULL;
	return bm->get_handle();
}

HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ Bitmap)
{
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

	gdi_object_t *obj = reinterpret_cast<gdi_object_t*>( entry->kernel_info );
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
	device_context_t* dc = dc_from_handle( hdc );
	if (!dc)
		return FALSE;

	bitmap_t* bitmap = bitmap_from_handle( hbm );
	if (!bitmap)
		return FALSE;

	assert( bitmap->is_valid() );

	return dc->select_bitmap( bitmap );
}

HGDIOBJ NTAPI NtGdiSelectPen( HGDIOBJ hdc, HGDIOBJ hbm )
{
 
        return NULL;
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
	return FALSE;
}

BOOLEAN NTAPI NtGdiFlush(void)
{
	return 0x93;
}

int NTAPI NtGdiSaveDC(HGDIOBJ hdc)
{
	device_context_t* dc = dc_from_handle( hdc );
	if (!dc)
		return 0;

	return dc->save_dc();
}

BOOLEAN NTAPI NtGdiRestoreDC( HGDIOBJ hdc, int level )
{
	device_context_t* dc = dc_from_handle( hdc );
	if (!dc)
		return FALSE;

	return dc->restore_dc( level );
}

HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ hdc, ULONG object_type)
{
	return win32k_manager->alloc_screen_dc();
}

// fun...
ULONG NTAPI NtGdiSetDIBitsToDeviceInternal(
	HGDIOBJ hdc, int xDest, int yDest, ULONG cx, ULONG cy,
	int xSrc, int ySrc, ULONG StartScan, ULONG ScanLines,
	PVOID Bits, PVOID bmi, ULONG Color, ULONG, ULONG, ULONG, ULONG)
{
	return cy;
}

ULONG NTAPI NtGdiExtGetObjectW(HGDIOBJ Object, ULONG Size, PVOID Buffer)
{
	union {
		BITMAP bm;
	} info;
	ULONG len = 0;

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
	device_context_t* dest = dc_from_handle( hdcSrc );
	if (!dest)
		return FALSE;

	device_context_t* src = dc_from_handle( hdcSrc );
	if (!src)
		return FALSE;

	return dest->bitblt( xDest, yDest, cx, cy, src->get_bitmap(), xSrc, ySrc, rop );
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

HANDLE NTAPI NtGdiCreateCompatibleBitmap( HANDLE DeviceContext, int width, int height )
{
	device_context_t* dc = dc_from_handle( DeviceContext );
	if (!dc)
		return FALSE;

	int bpp = dc->getcaps( BITSPIXEL );
	if (!bpp)
		return FALSE;

	bitmap_t *bm = alloc_bitmap( width, height, bpp );
	return bm->get_handle();
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

BOOLEAN NTAPI NtGdiMoveTo( HDC handle, int xpos, int ypos, LPPOINT pptOut)
{
	return TRUE;
}

BOOLEAN NTAPI NtGdiLineTo( HDC handle, int xpos, int ypos )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	return dc->lineto( xpos, ypos );
}

int NTAPI NtGdiGetDeviceCaps( HDC handle, int index )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	return dc->getcaps( index );
}

HPEN NTAPI NtGdiCreatePen(int style, int width, COLORREF color, ULONG)
{
    return (HPEN) win32k_manager->create_pen( style, width, color );
}

BOOLEAN NTAPI NtGdiStretchDIBitsInternal(
	HDC handle,
	int dest_x, int dest_y, int dest_width, int dest_height,
	int src_x, int src_y, int src_width, int src_height,
	const VOID *bits, const BITMAPINFO *info, UINT usage, DWORD rop,
	ULONG, ULONG, ULONG )
{
	device_context_t* dc = dc_from_handle( handle );
	if (!dc)
		return FALSE;

	BITMAPINFOHEADER bmi;
	NTSTATUS r;
	RGBQUAD colors[0x100];

	r = copy_from_user( &bmi, &info->bmiHeader );
	if (r < STATUS_SUCCESS)
		return FALSE;

	stretch_di_bits_args args;
	args.dest_x = dest_x;
	args.dest_y = dest_y;
	args.dest_width = dest_width;
	args.dest_height = dest_height;
	args.src_x = src_x;
	args.src_y = src_y;
	args.src_width = src_width;
	args.src_height = src_height;
	args.bits = bits;
	args.info = &bmi;
	args.usage = usage;
	args.rop = rop;

	if (bmi.biBitCount <= 8)
	{
		dprintf("copying %d colors\n",  bmi.biBitCount);
		r = copy_from_user( colors, &info->bmiColors, (1 << bmi.biBitCount) * sizeof (RGBQUAD));
		if (r < STATUS_SUCCESS)
			return FALSE;
		args.colors = colors;
	}
	else
		args.colors = NULL;

	return dc->stretch_di_bits( args );
}
