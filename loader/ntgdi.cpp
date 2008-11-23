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

#include <SDL/SDL.h>

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

BOOLEAN delete_handle_table_entry( HANDLE Object )
{
	dprintf("%p\n", Object);

	gdi_handle_table_entry *entry = get_handle_table_entry(Object);
	if (!entry)
		return FALSE;
	if (entry->ProcessId != current->process->id)
	{
		dprintf("pirate deletion! %p\n", Object);
		return FALSE;
	}

	memset( entry, 0, sizeof *entry );

	return TRUE;
}

BOOLEAN NTAPI NtGdiInit()
{
	dprintf("\n");
	return TRUE;
}

class win32k_sdl_t : public win32k_manager_t
{
	SDL_Surface *screen;
public:
	win32k_sdl_t( SDL_Surface *_screen );
	BOOL set_pixel( INT x, INT y, COLORREF color );
private:
	void set_pixel_32bpp( INT x, INT y, COLORREF color );
	void set_pixel_16bpp( INT x, INT y, COLORREF color );
};

win32k_manager_t::~win32k_manager_t()
{
}

win32k_sdl_t::win32k_sdl_t( SDL_Surface *_screen ) :
	screen( _screen )
{
}

void win32k_sdl_t::set_pixel_16bpp( INT x, INT y, COLORREF color )
{
	Uint16 *bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
	*bufp = SDL_MapRGB(screen->format, GetRValue(color), GetGValue(color), GetBValue(color));
}

void win32k_sdl_t::set_pixel_32bpp( INT x, INT y, COLORREF color )
{
	Uint32 *bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
	*bufp = SDL_MapRGB(screen->format, GetRValue(color), GetGValue(color), GetBValue(color));
}

BOOL win32k_sdl_t::set_pixel( INT x, INT y, COLORREF color )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	switch (screen->format->BytesPerPixel)
	{
	case 2:
		set_pixel_16bpp( x, y, color );
		break;
	case 4:
		set_pixel_32bpp( x, y, color );
		break;
	default:
		dprintf("%d bpp not supported\n", screen->format->BytesPerPixel);
	}

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, x, y, 1, 1);

	return TRUE;
}

win32k_manager_t* alloc_win32k_SDL()
{
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
		return NULL;
	atexit(SDL_Quit);

	SDL_Surface *screen = SDL_SetVideoMode(640, 480, 16, SDL_SWSURFACE);
	if ( screen == NULL )
		return NULL;

	return new win32k_sdl_t( screen );
}

NTSTATUS win32k_process_init(process_t *process)
{
	NTSTATUS r;

	dprintf("\n");

	win32k_manager_t *info = alloc_win32k_SDL();
	if (!info)
		die("unable to allocate screen\n");
	process->win32k_info = info;

	PPEB ppeb = (PPEB) current->process->peb_section->get_kernel_address();

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
	current->process->vm->free_virtual_memory( p, GDI_SHARED_HANDLE_TABLE_SIZE, MEM_FREE );

	r = gdi_ht_section->mapit( current->process->vm, p, 0,
				   MEM_COMMIT, PAGE_READWRITE );
	if (r < STATUS_SUCCESS)
	{
		dprintf("r = %08lx\n", r);
		assert(0);
		return FALSE;
	}

	ppeb->GdiSharedHandleTable = (void*) p;

	if (option_trace)
		current->process->vm->set_tracer( p, ntgdishm_trace );

	return r;
}

NTSTATUS win32k_thread_init(thread_t *thread)
{
	NTSTATUS r;

	if (thread->win32k_init_complete())
		return STATUS_SUCCESS;

	if (!thread->process->win32k_info)
	{
		r = win32k_process_init( thread->process );
		if (r < STATUS_SUCCESS)
			return r;
	}

	ULONG size = 0;
	PVOID buffer = 0;
	r = current->do_user_callback( 75, size, buffer );

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

HGDIOBJ alloc_gdi_object( BOOL stock, ULONG type, void *user_info, void *kernel_info )
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

	return handle;
}

// shared across all processes
static section_t *g_dc_section;
static BYTE *g_dc_shared_mem = 0;
static const ULONG max_device_contexts = 0x100;
static const ULONG dc_size = 0x100;
static bool g_dc_bitmap[max_device_contexts];

class dcshm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void dcshm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	fprintf(stderr, "%04lx: accessed dcshm[%02lx][%02lx] from %08lx\n",
				current->trace_id(), ofs/dc_size, ofs%dc_size, eip);
}

static dcshm_tracer dcshm_trace;

win32k_manager_t::win32k_manager_t() :
	dc_shared_mem(0)
{
}

HGDIOBJ win32k_manager_t::alloc_dc()
{
	NTSTATUS r;

	if (!g_dc_shared_mem)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = 0x10000;
		r = create_section( &g_dc_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return FALSE;

		g_dc_shared_mem = (BYTE*) g_dc_section->get_kernel_address();
	}

	if (!dc_shared_mem)
	{
		r = g_dc_section->mapit( current->process->vm, dc_shared_mem, 0, MEM_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
		{
			dprintf("failed to map shared memory\n");
			return FALSE;
		}

		if (option_trace)
			current->process->vm->set_tracer( dc_shared_mem, dcshm_trace );
	}

	// find a free device context area
	ULONG n;
	for (n=0; n<max_device_contexts; n++)
		if (!g_dc_bitmap[n])
			break;
	if (n >= max_device_contexts)
	{
		dprintf("no device contexts left\n");
		return FALSE;
	}
	g_dc_bitmap[n] = true;

	// calculate pointers to it
	//BYTE* k_dcu = dc_shared_mem + n * dc_size;
	BYTE* u_dcu = dc_shared_mem + n * dc_size;
	dprintf("dc number %02lx address %p\n", n, u_dcu);

	HGDIOBJ dc = alloc_gdi_object( FALSE, GDI_OBJECT_DC, (BYTE*)u_dcu, (void*) n );

	return dc;
}

BOOL win32k_manager_t::release_dc( HGDIOBJ dc )
{
	gdi_handle_table_entry *entry = get_handle_table_entry( dc );
	if (!entry)
		return FALSE;
	if (entry->Type != GDI_OBJECT_DC)
		return FALSE;
	ULONG index = (ULONG) (entry->kernel_info);
	assert( index <= max_device_contexts );
	assert( g_dc_bitmap[index] );
	g_dc_bitmap[index] = false;
	return delete_handle_table_entry( dc );
}

HGDIOBJ NTAPI NtGdiGetStockObject(ULONG Index)
{
	dprintf("%ld\n", Index);

	switch (Index)
	{
	case WHITE_BRUSH:
	case LTGRAY_BRUSH:
	case GRAY_BRUSH:
	case DKGRAY_BRUSH:
	case BLACK_BRUSH:
	case NULL_BRUSH: //case HOLLOW_BRUSH:
		return alloc_gdi_object(TRUE,GDI_OBJECT_BRUSH, 0);
	case WHITE_PEN:
	case BLACK_PEN:
	case NULL_PEN:
		return alloc_gdi_object(TRUE,GDI_OBJECT_PEN, 0);
	case OEM_FIXED_FONT:
	case ANSI_FIXED_FONT:
	case ANSI_VAR_FONT:
	case SYSTEM_FONT:
	case DEVICE_DEFAULT_FONT:
	case SYSTEM_FIXED_FONT:
	case DEFAULT_GUI_FONT:
	case 18: // 18 and 19 are used by csrss.exe when starting
	case 19:
		return alloc_gdi_object(TRUE,GDI_OBJECT_FONT, 0);
	case DEFAULT_PALETTE:
		return alloc_gdi_object(TRUE,GDI_OBJECT_PALETTE, 0);
	default:
		return 0;
	}
}

// parameters look the same as gdi32.CreateBitmap
HGDIOBJ NTAPI NtGdiCreateBitmap(int Width, int Height, UINT Planes, UINT BitsPerPixel, VOID**Out)
{
	dprintf("(%dx%d) %d %d %p\n", Width, Height, Planes, BitsPerPixel, Out);
	return alloc_gdi_object(FALSE, GDI_OBJECT_BITMAP, 0);
}

// gdi32.CreateComptabibleDC
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ hdc)
{
	dprintf("%p\n", hdc);
	return current->process->win32k_info->alloc_dc();
}

// has one more parameter than gdi32.CreateSolidBrush
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF Color, ULONG u_arg2)
{
	dprintf("%08lx %08lx\n", Color, u_arg2);
	return alloc_gdi_object(FALSE, GDI_OBJECT_BRUSH, 0);
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
	return alloc_gdi_object(FALSE, GDI_OBJECT_BITMAP, 0);
}

HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ Bitmap)
{
	dprintf("%p\n", Bitmap);
	return current->process->win32k_info->alloc_dc();
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
	return delete_handle_table_entry( Object );
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

	//FIXME: validate handle
	if (get_handle_type(bitmap) != GDI_OBJECT_BITMAP)
		return 0;

	return alloc_gdi_object(FALSE, GDI_OBJECT_BITMAP, 0);
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
	return current->process->win32k_info->alloc_dc();
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
	return alloc_gdi_object(FALSE, GDI_OBJECT_BITMAP, 0);
}

BOOL NTAPI NtGdiSetFontEnumeration(PVOID Unknown)
{
	return 0;
}

HANDLE NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID)
{
	return current->process->win32k_info->alloc_dc();
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

	return alloc_gdi_object(FALSE, 0x3f, 0);
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

BOOLEAN NTAPI NtGdiSetPixel( HANDLE dc, INT x, INT y, COLORREF color )
{
	return current->process->win32k_info->set_pixel( x, y, color );
}

BOOLEAN NTAPI NtGdiRectangle( HANDLE dc, INT x, INT y, INT width, INT height )
{
	return TRUE;
}
