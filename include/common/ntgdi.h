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

#define GDI_OBJECT_DC	  0x01
#define GDI_OBJECT_REGION  0x04
#define GDI_OBJECT_BITMAP  0x05
#define GDI_OBJECT_PALETTE 0x08
#define GDI_OBJECT_FONT	0x0a
#define GDI_OBJECT_BRUSH   0x10
#define GDI_OBJECT_EMF	 0x21
#define GDI_OBJECT_PEN	 0x30
#define GDI_OBJECT_EXTPEN  0x50

inline HGDIOBJ makeHGDIOBJ(
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

inline ULONG get_handle_type(HGDIOBJ handle)
{
	return (((ULONG)handle)>>16)&0x7f;
}

HGDIOBJ alloc_gdi_object( BOOL stock, ULONG type );

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

#if 0
  /* Brush styles */
#define BS_SOLID	0
#define BS_NULL		1
#define BS_HOLLOW	1
#define BS_HATCHED	2
#define BS_PATTERN	3
#define BS_INDEXED	4
#define BS_DIBPATTERN	5
#define BS_DIBPATTERNPT	6
#define BS_PATTERN8X8	7
#define BS_DIBPATTERN8X8 8
#define BS_MONOPATTERN	9

#define SRCCOPY         0xcc0020
#define SRCPAINT        0xee0086
#define SRCAND          0x8800c6
#define SRCINVERT       0x660046
#define SRCERASE        0x440328
#define NOTSRCCOPY      0x330008
#define NOTSRCERASE     0x1100a6
#define MERGECOPY       0xc000ca
#define MERGEPAINT      0xbb0226
#define PATCOPY         0xf00021
#define PATPAINT        0xfb0a09
#define PATINVERT       0x5a0049
#define DSTINVERT       0x550009
#define BLACKNESS       0x000042
#define WHITENESS       0xff0062
#endif

BOOLEAN NTAPI NtGdiAddFontResourceW(PVOID,ULONG,ULONG,ULONG,PVOID,ULONG);
BOOLEAN NTAPI NtGdiBitBlt(HGDIOBJ,INT,INT,INT,INT,HGDIOBJ,INT,INT,ULONG,ULONG,ULONG);
int     NTAPI NtGdiCombineRgn(HRGN,HRGN,HRGN,int);
BOOLEAN NTAPI NtGdiComputeXformCoefficients(HANDLE);
HGDIOBJ NTAPI NtGdiCreateBitmap(int,int,UINT,UINT,VOID*);
HANDLE  NTAPI NtGdiCreateCompatibleBitmap(HANDLE,int,int);
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ);
HRGN    NTAPI NtGdiCreateEllipticRgn(int,int,int,int);
HRGN    NTAPI NtGdiCreateRectRgn(int,int,int,int);
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF,ULONG);
HGDIOBJ NTAPI NtGdiCreateDIBitmapInternal(HDC,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
HANDLE  NTAPI NtGdiCreateDIBSection(HDC,HANDLE,ULONG,PBITMAPINFO,ULONG,ULONG,ULONG,ULONG_PTR,PVOID);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HGDIOBJ);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,ULONG,PULONG,PVOID);
BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PULONG);
BOOL    NTAPI NtGdiEqualRgn(HRGN,HRGN);
ULONG NTAPI   NtGdiExtGetObjectW(HGDIOBJ,ULONG,PVOID);
BOOLEAN NTAPI NtGdiExtTextOutW(HANDLE,INT,INT,UINT,LPRECT,WCHAR*,UINT,INT*,UINT);
BOOLEAN NTAPI NtGdiFlush(void);
int     NTAPI NtGdiGetAppClipBox(HANDLE,RECT*);
HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ);
HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ,ULONG);
BOOLEAN NTAPI NtGdiGetFontResourceInfoInternalW(LPWSTR,ULONG,ULONG,UINT,PULONG,PVOID,ULONG);
int     NTAPI NtGdiGetRgnBox(HRGN,PRECT);
HGDIOBJ NTAPI NtGdiGetStockObject(ULONG);
BOOLEAN NTAPI NtGdiGetTextMetricsW(HANDLE,PVOID,ULONG);
BOOLEAN NTAPI NtGdiInit(void);
int     NTAPI NtGdiOffsetRgn(HRGN,int,int);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
BOOLEAN NTAPI NtGdiPolyPatBlt(HANDLE,ULONG,PRECT,ULONG,ULONG);
ULONG   NTAPI NtGdiQueryFontAssocInfo(HANDLE);
BOOLEAN NTAPI NtGdiRectangle(HANDLE,INT,INT,INT,INT);
BOOLEAN NTAPI NtGdiRestoreDC(HGDIOBJ,int);
int NTAPI     NtGdiSaveDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiSelectBitmap(HGDIOBJ,HGDIOBJ);
ULONG   NTAPI NtGdiSetDIBitsToDeviceInternal(HGDIOBJ,int,int,ULONG,ULONG,int,int,ULONG,ULONG,PVOID,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
ULONG   NTAPI NtGdiSetFontEnumeration(ULONG);
BOOLEAN NTAPI NtGdiSetIcmMode(HANDLE,ULONG,ULONG);
BOOLEAN NTAPI NtGdiSetPixel(HANDLE,INT,INT,COLORREF);
BOOL    NTAPI NtGdiSetRectRgn(HRGN,int,int,int,int);

#endif // __NTGDI_H__
