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
#include "ntgdi.h"
#include "mem.h"
#include "section.h"
#include "debug.h"
#include "win32mgr.h"

#if defined (HAVE_SDL) && defined (HAVE_SDL_SDL_H)
#include <SDL/SDL.h>
#endif

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
		char *field = "unknown";
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

win32k_manager_t::~win32k_manager_t()
{
}

win32k_info_t::win32k_info_t() :
	dc_shared_mem( 0 ),
	user_shared_mem( 0 )
{
	memset( &stock_object, 0, sizeof stock_object );
}

win32k_info_t *win32k_manager_t::alloc_win32k_info()
{
	return new win32k_info_t;
}

#if defined (HAVE_SDL) && defined (HAVE_SDL_SDL_H)

class win32k_sdl_t : public win32k_manager_t, public sleeper_t
{
protected:
	SDL_Surface *screen;
public:
	virtual BOOL init();
	virtual void fini();
	win32k_sdl_t();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
protected:
	Uint16 map_colorref( COLORREF );
	virtual SDL_Surface* set_mode() = 0;
	virtual void set_pixel_l( INT x, INT y, COLORREF color ) = 0;
	virtual void rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush ) = 0;
	virtual bool sleep_timeout( LARGE_INTEGER& timeout );
	static Uint32 timeout_callback( Uint32 interval, void *arg );
};

win32k_sdl_t::win32k_sdl_t()
{
}

BOOL win32k_sdl_t::set_pixel( INT x, INT y, COLORREF color )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	set_pixel_l( x, y, color );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, x, y, 1, 1);

	return TRUE;
}

template<typename T> void swap( T& A, T& B )
{
	T x = A;
	A = B;
	B = x;
}

BOOL win32k_sdl_t::rectangle(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	if (left > right)
		swap( left, right );
	if (top > bottom)
		swap( top, bottom );

	top = max( 0, top );
	left = max( 0, left );
	right = min( screen->w - 1, right );
	bottom = min( screen->h - 1, bottom );

	rectangle_l( left, top, right, bottom, brush );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, left, top, right - left, bottom - top );

	return TRUE;
}

BOOL win32k_sdl_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	dprintf("text: %pus\n", &text );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	//SDL_UpdateRect( screen, left, top, right - left, bottom - top );

	return TRUE;
}

Uint32 win32k_sdl_t::timeout_callback( Uint32 interval, void *arg )
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = 0;
	event.user.data1 = 0;
	event.user.data2 = 0;
	SDL_PushEvent( &event );
	return 0;
}

bool win32k_sdl_t::sleep_timeout( LARGE_INTEGER& timeout )
{
	Uint32 interval = get_int_timeout( timeout );
	SDL_TimerID id = SDL_AddTimer( interval, win32k_sdl_t::timeout_callback, 0 );

	SDL_Event event;
	bool quit = false;
	while (SDL_WaitEvent( &event ))
	{
		if (event.type == SDL_QUIT)
		{
			quit = true;
			break;
		}

		if (event.type == SDL_USEREVENT && event.user.code == 0)
		{
			// timer has expired, no need to cancel it
			id = NULL;
			break;
		}
	}

	if (id != NULL)
		SDL_RemoveTimer( id );
	return quit;
}

BOOL win32k_sdl_t::init()
{
	if ( SDL_WasInit(SDL_INIT_VIDEO) )
		return TRUE;

	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 )
		return FALSE;

	screen = set_mode();

	brush_t light_blue(0, RGB(0x3b, 0x72, 0xa9), 0);
	rectangle( 0, 0, screen->w, screen->h, &light_blue );

	sleeper = this;

	return TRUE;
}

void win32k_sdl_t::fini()
{
	if ( !SDL_WasInit(SDL_INIT_VIDEO) )
		return;
	SDL_Quit();
}

class win32k_sdl_16bpp_t : public win32k_sdl_t
{
public:
	virtual SDL_Surface* set_mode();
	virtual void set_pixel_l( INT x, INT y, COLORREF color );
	virtual void rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush );
	Uint16 map_colorref( COLORREF color );
};

SDL_Surface* win32k_sdl_16bpp_t::set_mode()
{
	return SDL_SetVideoMode( 640, 480, 16, SDL_SWSURFACE );
}

Uint16 win32k_sdl_16bpp_t::map_colorref( COLORREF color )
{
	return SDL_MapRGB(screen->format, GetRValue(color), GetGValue(color), GetBValue(color));
}

void win32k_sdl_16bpp_t::set_pixel_l( INT x, INT y, COLORREF color )
{
	Uint16 *bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
	*bufp = map_colorref( color );
}

void win32k_sdl_16bpp_t::rectangle_l(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	COLORREF brush_val, pen_val;

	// FIXME: use correct pen color
	pen_val = map_colorref( RGB( 0, 0, 0 ) );
	brush_val = map_colorref( brush->get_color() );
	dprintf("brush color = %08lx\n", brush->get_color());

	Uint16 *ptr = (Uint16 *)screen->pixels + top*screen->pitch/2;

	// top line
	for (INT count = left; count <= right; count++)
		ptr[count] = pen_val;
	ptr += screen->pitch/2;
	top++;

	while (top < (bottom -1))
	{
		// left border drawn by pen
		ptr[ left ] = pen_val;

		// filled by brush
		INT count;
		for (count = left+1; count < (right - 1); count++)
			ptr[count] = brush_val;

		// right border drawn by pen
		ptr[ count ] = pen_val;

		//next line
		top++;
		ptr += screen->pitch/2;
	}

	// bottom line
	for (INT count = left; count <= right; count++)
		ptr[count] = pen_val;
}

win32k_sdl_16bpp_t win32k_manager_sdl_16bpp;

win32k_manager_t* init_sdl_win32k_manager()
{
	return &win32k_manager_sdl_16bpp;
}

#else

win32k_manager_t* init_sdl_win32k_manager()
{
	return NULL;
}

#endif

class win32k_null_t : public win32k_manager_t
{
public:
	virtual BOOL init();
	virtual void fini();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
protected:
	virtual void set_pixel_l( INT x, INT y, COLORREF color );
	virtual void rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush );
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

void win32k_null_t::set_pixel_l( INT x, INT y, COLORREF color )
{
}

void win32k_null_t::rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush )
{
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

static const ULONG NTWIN32_THREAD_INIT_CALLBACK = 74;

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

win32k_manager_t::win32k_manager_t()
{
}

section_t *device_context_t::g_dc_section;
BYTE *device_context_t::g_dc_shared_mem = 0;
bool device_context_t::g_dc_bitmap[max_device_contexts];

HGDIOBJ win32k_manager_t::alloc_dc()
{
	device_context_t* dc = device_context_t::alloc();
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
	dc_index( n )
{
}

DEVICE_CONTEXT_SHARED_MEMORY* device_context_t::get_dc_shared_mem()
{
	return (DEVICE_CONTEXT_SHARED_MEMORY*) (g_dc_shared_mem + dc_index * dc_size);
}

device_context_t* device_context_t::alloc()
{
	BYTE* dc_shared_mem = get_dc_shared_mem_base();
	if (!dc_shared_mem)
		return NULL;

	int n = get_free_index();
	if (n < 0)
		return NULL;

	device_context_t* dc = new device_context_t( n );
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

BOOL device_context_t::rectangle(INT left, INT top, INT right, INT bottom )
{
	brush_t *brush = get_selected_brush();
	if (!brush)
		return FALSE;
	dprintf("drawing with brush %p with color %08lx\n", brush->get_handle(), brush->get_color() );
	return win32k_manager->rectangle( left, top, right, bottom, brush );
}

BOOL device_context_t::set_pixel( INT x, INT y, COLORREF color )
{
	return win32k_manager->set_pixel( x, y, color );
}

BOOL device_context_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return win32k_manager->exttextout( x, y, options, rect, text );
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

// parameters look the same as gdi32.CreateBitmap
HGDIOBJ NTAPI NtGdiCreateBitmap(int Width, int Height, UINT Planes, UINT BitsPerPixel, VOID**Out)
{
	dprintf("(%dx%d) %d %d %p\n", Width, Height, Planes, BitsPerPixel, Out);
	return alloc_gdi_object( FALSE, GDI_OBJECT_BITMAP );
}

// gdi32.CreateComptabibleDC
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ hdc)
{
	dprintf("%p\n", hdc);
	return win32k_manager->alloc_dc();
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
	return win32k_manager->alloc_dc();
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

char *get_object_type_name( HGDIOBJ object )
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

HGDIOBJ NTAPI NtGdiSelectBitmap(HGDIOBJ hdc, HGDIOBJ bitmap)
{
	dprintf("%p %p\n", hdc, bitmap);

	device_context_t* dc = dc_from_handle( hdc );
	if (!dc)
		return FALSE;

	if (get_handle_type(bitmap) != GDI_OBJECT_BITMAP)
		return 0;

	return alloc_gdi_object( FALSE, GDI_OBJECT_BITMAP );
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
	return win32k_manager->alloc_dc();
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
	return TRUE;
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

BOOL NTAPI NtGdiSetFontEnumeration(PVOID Unknown)
{
	return 0;
}

HANDLE NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID)
{
	return win32k_manager->alloc_dc();
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
