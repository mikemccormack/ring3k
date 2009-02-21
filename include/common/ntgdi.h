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

#ifndef __NTGDI_H__
#define __NTGDI_H__

// from FengYuan's Windows Graphics Programming Section 3.2, page 144

#define GDI_OBJECT_DC      0x01
#define GDI_OBJECT_REGION  0x04
#define GDI_OBJECT_BITMAP  0x05
#define GDI_OBJECT_PALETTE 0x08
#define GDI_OBJECT_FONT    0x0a
#define GDI_OBJECT_BRUSH   0x10
#define GDI_OBJECT_EMF     0x21
#define GDI_OBJECT_PEN     0x30
#define GDI_OBJECT_EXTPEN  0x50

static inline HGDIOBJ makeHGDIOBJ(
	ULONG Top,
	BOOLEAN Stock,
	ULONG ObjectType,
	ULONG Index)
{
	return (HGDIOBJ)(((Top&0xff)<<24) | ((Stock&1) << 23) |
			 ((ObjectType&0x7f)<<16) | (Index &0x3fff));
}

typedef struct _gdi_handle_table_entry {
	void *kernel_info;
	USHORT ProcessId;
	USHORT Count;
	USHORT Upper;
	USHORT Type;
	void *user_info;
} gdi_handle_table_entry;

static inline ULONG get_handle_type(HGDIOBJ handle)
{
	return (((ULONG)handle)>>16)&0x7f;
}

static inline ULONG get_handle_index(HANDLE handle)
{
	return (((ULONG)handle)&0x3fff);
}

static inline ULONG get_handle_top(HANDLE handle)
{
	return (((ULONG)handle)>>24)&0xff;
}

static inline ULONG get_handle_upper(HANDLE handle)
{
	return (((ULONG)handle)>>16);
}

#undef GetRValue
static inline BYTE GetRValue(COLORREF rgb)
{
	return rgb&0xff;
}

#undef GetGValue
static inline BYTE GetGValue(COLORREF rgb)
{
	return (rgb>>8)&0xff;
}

#undef GetBValue
static inline BYTE GetBValue(COLORREF rgb)
{
	return (rgb>>16)&0xff;
}

#undef RGB
static inline COLORREF RGB( BYTE red, BYTE green, BYTE blue )
{
	return red | (green << 8) | (blue << 16);
}

typedef struct _GDI_REGION_SHARED {
	ULONG flags;
	ULONG type;
	RECT  rect;
} GDI_REGION_SHARED;

BOOLEAN NTAPI NtGdiAddFontResourceW(PVOID,ULONG,ULONG,ULONG,PVOID,ULONG);
BOOLEAN NTAPI NtGdiBitBlt(HGDIOBJ,INT,INT,INT,INT,HGDIOBJ,INT,INT,ULONG,ULONG,ULONG);
int     NTAPI NtGdiCombineRgn(HRGN,HRGN,HRGN,int);
BOOLEAN NTAPI NtGdiComputeXformCoefficients(HANDLE);
HGDIOBJ NTAPI NtGdiCreateBitmap(int,int,UINT,UINT,VOID*);
HANDLE  NTAPI NtGdiCreateCompatibleBitmap(HANDLE,int,int);
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ);
HRGN    NTAPI NtGdiCreateEllipticRgn(int,int,int,int);
HPEN    NTAPI NtGdiCreatePen(int,int,COLORREF,ULONG);
HRGN    NTAPI NtGdiCreateRectRgn(int,int,int,int);
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF,ULONG);
HGDIOBJ NTAPI NtGdiCreateDIBitmapInternal(HDC,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
HANDLE  NTAPI NtGdiCreateDIBSection(HDC,HANDLE,ULONG,PBITMAPINFO,ULONG,ULONG,ULONG,ULONG_PTR,PVOID);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HGDIOBJ);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,ULONG,PULONG,PVOID);
BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PULONG);
BOOL    NTAPI NtGdiEqualRgn(HRGN,HRGN);
ULONG   NTAPI NtGdiExtGetObjectW(HGDIOBJ,ULONG,PVOID);
BOOLEAN NTAPI NtGdiExtTextOutW(HANDLE,INT,INT,UINT,LPRECT,WCHAR*,UINT,INT*,UINT);
BOOLEAN NTAPI NtGdiFlush(void);
int     NTAPI NtGdiGetAppClipBox(HANDLE,RECT*);
HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ);
HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ,ULONG);
int     NTAPI NtGdiGetDeviceCaps(HDC,int);
BOOLEAN NTAPI NtGdiGetFontResourceInfoInternalW(LPWSTR,ULONG,ULONG,UINT,PULONG,PVOID,ULONG);
ULONG   NTAPI NtGdiGetRegionData(HRGN,ULONG,PRGNDATA);
int     NTAPI NtGdiGetRgnBox(HRGN,PRECT);
HGDIOBJ NTAPI NtGdiGetStockObject(ULONG);
BOOLEAN NTAPI NtGdiGetTextMetricsW(HANDLE,PVOID,ULONG);
BOOLEAN NTAPI NtGdiInit(void);
BOOLEAN NTAPI NtGdiLineTo(HDC,int,int);
int     NTAPI NtGdiOffsetRgn(HRGN,int,int);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
BOOLEAN NTAPI NtGdiPolyPatBlt(HANDLE,ULONG,PRECT,ULONG,ULONG);
BOOLEAN NTAPI NtGdiPtInRegion(HRGN,int,int);
ULONG   NTAPI NtGdiQueryFontAssocInfo(HANDLE);
BOOLEAN NTAPI NtGdiRectangle(HANDLE,INT,INT,INT,INT);
BOOLEAN NTAPI NtGdiRectInRegion(HRGN,const RECT*);
BOOLEAN NTAPI NtGdiRestoreDC(HGDIOBJ,int);
int     NTAPI NtGdiSaveDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiSelectBitmap(HGDIOBJ,HGDIOBJ);
ULONG   NTAPI NtGdiSetDIBitsToDeviceInternal(HGDIOBJ,int,int,ULONG,ULONG,int,int,ULONG,ULONG,PVOID,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
ULONG   NTAPI NtGdiSetFontEnumeration(ULONG);
BOOLEAN NTAPI NtGdiSetIcmMode(HANDLE,ULONG,ULONG);
BOOLEAN NTAPI NtGdiSetPixel(HANDLE,INT,INT,COLORREF);
BOOL    NTAPI NtGdiSetRectRgn(HRGN,int,int,int,int);

#endif // __NTGDI_H__
