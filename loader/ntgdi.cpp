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

HGDIOBJ alloc_dc();

// shared across all processes
static section_t *gdi_ht_section;
static void *gdi_handle_table = 0;

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

BOOLEAN NTAPI NtGdiInit()
{
	dprintf("\n");
	return TRUE;
}

NTSTATUS win32k_process_init(process_t *process)
{
	NTSTATUS r;

	dprintf("\n");

	win32k_info_t *info = new win32k_info_t;
	process->win32k_info = info;

	PPEB ppeb = (PPEB) current->process->peb_section->get_kernel_address();

	// only do this once per process
	if (ppeb->GdiSharedHandleTable)
		return TRUE;

	if (!gdi_handle_table)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = 0x50000;
		r = create_section( &gdi_ht_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return r;

		gdi_handle_table = (BYTE*) gdi_ht_section->get_kernel_address();
	}

	// read/write for the kernel and read only for processes
	BYTE* p = 0;
	r = gdi_ht_section->mapit( current->process->vm, p, 0,
							   MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return FALSE;

	ppeb->GdiSharedHandleTable = (void*) p;

	if (0) trace_memory( current->process->vm, p, ntgdishm_trace );

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
		if (r != STATUS_SUCCESS)
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
	if (r != STATUS_SUCCESS)
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

HGDIOBJ alloc_gdi_object( BOOL stock, ULONG type, void *user_info )
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
static section_t *dc_section;
static void *dc_shared_mem = 0;

class dcshm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void dcshm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	fprintf(stderr, "%04lx: accessed dcshm[%04lx] from %08lx\n",
				current->trace_id(), ofs, eip);
}

static dcshm_tracer dcshm_trace;

HGDIOBJ alloc_dc()
{
	NTSTATUS r;

	if (!dc_shared_mem)
	{
		LARGE_INTEGER sz;
		sz.QuadPart = 0x10000;
		r = create_section( &dc_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return FALSE;

		dc_shared_mem = (BYTE*) dc_section->get_kernel_address();
	}

	// FIXME: there are be many DCs per shared section
	BYTE *p = 0;
	r = dc_section->mapit( current->process->vm, p, 0, MEM_COMMIT, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return FALSE;

	HGDIOBJ dc = alloc_gdi_object( FALSE, GDI_OBJECT_DC, (BYTE*)p );

	if (0) trace_memory( current->process->vm, p, dcshm_trace );

	return dc;
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
	return alloc_dc();
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
	return alloc_dc();
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
	return alloc_gdi_object(FALSE, get_handle_type((HGDIOBJ)object_type), 0);
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
	if (r != STATUS_SUCCESS)
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
	return alloc_gdi_object(FALSE, GDI_OBJECT_DC, 0);
}

typedef struct _font_enum_entry {
	ULONG size;
	ULONG offset;
	ULONG fonttype;
	ENUMLOGFONTEXW elfew;
	ULONG pad1[2];
	NEWTEXTMETRICEXW ntme;
	ULONG pad2[4];
} font_enum_entry;

void fill_font( font_enum_entry* fee, LPWSTR name, ULONG height, ULONG width, ULONG paf )
{
	memset( fee, 0, sizeof *fee );
	fee->size = sizeof *fee;
	fee->fonttype = RASTER_FONTTYPE;
	fee->offset = FIELD_OFFSET( font_enum_entry, ntme );
	fee->elfew.elfLogFont.lfHeight = height;
	fee->elfew.elfLogFont.lfWidth = width;
	fee->elfew.elfLogFont.lfPitchAndFamily = paf;
	memcpy( fee->elfew.elfLogFont.lfFaceName, name, strlenW(name)*2 );
	memcpy( fee->elfew.elfFullName, name, strlenW(name)*2 );
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
	if (r != STATUS_SUCCESS)
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
	WCHAR sys[] = { 'S','y','s','t','e','m',0 };
	WCHAR trm[] = { 'T','e','r','m','i','n','a','l',0 };

	if (BufferLength < len)
		return FALSE;

	fill_font( &fee[0], sys, 16, 7, FF_SWISS | VARIABLE_PITCH );
	fill_font( &fee[1], trm, 12, 8, FF_MODERN | FIXED_PITCH );

	NTSTATUS r = copy_to_user( Buffer, &fee, len );
	if (r != STATUS_SUCCESS)
		return FALSE;

	r = copy_to_user( ReturnLength, &len, sizeof len );
	if (r != STATUS_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE FontEnumeration)
{
	return TRUE;
}
