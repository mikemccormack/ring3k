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

// stock objects
#define WHITE_BRUSH		 0
#define LTGRAY_BRUSH		1
#define GRAY_BRUSH		  2
#define DKGRAY_BRUSH		3
#define BLACK_BRUSH		 4
#define NULL_BRUSH		  5
#define HOLLOW_BRUSH		5
#define WHITE_PEN		   6
#define BLACK_PEN		   7
#define NULL_PEN			8
#define OEM_FIXED_FONT	  10
#define ANSI_FIXED_FONT	 11
#define ANSI_VAR_FONT	   12
#define SYSTEM_FONT		 13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE	 15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT	17

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

HGDIOBJ alloc_gdi_object( BOOL stock, ULONG type, void *user_info );

typedef struct {
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct
{
	INT	bmType;
	INT	bmWidth;
	INT	bmHeight;
	INT	bmWidthBytes;
	WORD	bmPlanes;
	WORD	bmBitsPixel;
	LPVOID	bmBits;
} BITMAP, *PBITMAP, *LPBITMAP;

typedef struct {
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER, *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD			bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

BOOLEAN NTAPI NtGdiAddFontResourceW(PVOID,ULONG,ULONG,ULONG,PVOID,ULONG);
BOOLEAN NTAPI NtGdiBitBlt(HGDIOBJ,INT,INT,INT,INT,HGDIOBJ,INT,INT,ULONG,ULONG,ULONG);
HGDIOBJ NTAPI NtGdiCreateBitmap(int,int,UINT,UINT,VOID**);
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF,ULONG);
HGDIOBJ NTAPI NtGdiCreateDIBitmapInternal(HDC,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
HANDLE  NTAPI NtGdiCreateDIBSection(HDC,HANDLE,ULONG,PBITMAPINFO,ULONG,ULONG,ULONG,ULONG_PTR,PVOID);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HGDIOBJ);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,PVOID,PVOID,PVOID);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
ULONG NTAPI   NtGdiExtGetObjectW(HGDIOBJ,ULONG,PVOID);
BOOLEAN NTAPI NtGdiFlush(void);
HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ);
HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ,ULONG);
BOOLEAN NTAPI NtGdiGetFontResourceInfoInternalW(LPWSTR,ULONG,ULONG,UINT,PULONG,PVOID,ULONG);
HGDIOBJ NTAPI NtGdiGetStockObject(ULONG);
BOOLEAN NTAPI NtGdiInit(void);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
ULONG   NTAPI NtGdiQueryFontAssocInfo(HANDLE);
BOOLEAN NTAPI NtGdiRestoreDC(HGDIOBJ,int);
int NTAPI     NtGdiSaveDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiSelectBitmap(HGDIOBJ,HGDIOBJ);
ULONG   NTAPI NtGdiSetDIBitsToDeviceInternal(HGDIOBJ,int,int,ULONG,ULONG,int,int,ULONG,ULONG,PVOID,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
BOOL    NTAPI NtGdiSetFontEnumeration(PVOID);

#endif // __NTGDI_H__
