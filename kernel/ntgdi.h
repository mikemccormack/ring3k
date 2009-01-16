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
#define WHITE_BRUSH		0
#define LTGRAY_BRUSH		1
#define GRAY_BRUSH		2
#define DKGRAY_BRUSH		3
#define BLACK_BRUSH		4
#define NULL_BRUSH		5
#define HOLLOW_BRUSH		5
#define WHITE_PEN		6
#define BLACK_PEN		7
#define NULL_PEN		8
#define OEM_FIXED_FONT		10
#define ANSI_FIXED_FONT		11
#define ANSI_VAR_FONT		12
#define SYSTEM_FONT		13
#define DEVICE_DEFAULT_FONT	14
#define DEFAULT_PALETTE		15
#define SYSTEM_FIXED_FONT	16
#define DEFAULT_GUI_FONT	17
#define DC_BRUSH		18
#define DC_PEN			19
#define STOCK_LAST		19

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

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD			bmiColors[1];
} BITMAPINFO, *PBITMAPINFO, *LPBITMAPINFO;

#define LF_FACESIZE     32
#define LF_FULLFACESIZE 64

typedef struct tagLOGFONTW {
    LONG   lfHeight;
    LONG   lfWidth;
    LONG   lfEscapement;
    LONG   lfOrientation;
    LONG   lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    WCHAR  lfFaceName[LF_FACESIZE];
} LOGFONTW, *PLOGFONTW, *LPLOGFONTW;

typedef struct {
  LOGFONTW elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE];
  WCHAR      elfStyle[LF_FACESIZE];
  WCHAR      elfScript[LF_FACESIZE];
} ENUMLOGFONTEXW,*LPENUMLOGFONTEXW;

typedef struct {
    LONG      tmHeight;
    LONG      tmAscent;
    LONG      tmDescent;
    LONG      tmInternalLeading;
    LONG      tmExternalLeading;
    LONG      tmAveCharWidth;
    LONG      tmMaxCharWidth;
    LONG      tmWeight;
    LONG      tmOverhang;
    LONG      tmDigitizedAspectX;
    LONG      tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    DWORD     ntmFlags;
    UINT      ntmSizeEM;
    UINT      ntmCellHeight;
    UINT      ntmAvgWidth;
} NEWTEXTMETRICW, *PNEWTEXTMETRICW, *LPNEWTEXTMETRICW;

typedef struct {
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE, *LPFONTSIGNATURE;

typedef struct {
    NEWTEXTMETRICW	ntmTm;
    FONTSIGNATURE       ntmFontSig;
} NEWTEXTMETRICEXW;

#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

#define DEFAULT_PITCH       0x00
#define FIXED_PITCH         0x01
#define VARIABLE_PITCH      0x02
#define MONO_FONT           0x08

#define FF_DONTCARE         0x00
#define FF_ROMAN            0x10
#define FF_SWISS            0x20
#define FF_MODERN           0x30
#define FF_SCRIPT           0x40
#define FF_DECORATIVE       0x50

#define FW_DONTCARE	    0
#define FW_THIN		    100
#define FW_EXTRALIGHT	    200
#define FW_ULTRALIGHT	    200
#define FW_LIGHT	    300
#define FW_NORMAL	    400
#define FW_REGULAR	    400
#define FW_MEDIUM	    500
#define FW_SEMIBOLD	    600
#define FW_DEMIBOLD	    600
#define FW_BOLD		    700
#define FW_EXTRABOLD	    800
#define FW_ULTRABOLD	    800
#define FW_HEAVY	    900
#define FW_BLACK	    900

#define ANSI_CHARSET	      (BYTE)0
#define OEM_CHARSET	      (BYTE)255

typedef struct tagTEXTMETRICW
{
    LONG      tmHeight;
    LONG      tmAscent;
    LONG      tmDescent;
    LONG      tmInternalLeading;
    LONG      tmExternalLeading;
    LONG      tmAveCharWidth;
    LONG      tmMaxCharWidth;
    LONG      tmWeight;
    LONG      tmOverhang;
    LONG      tmDigitizedAspectX;
    LONG      tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRICW, *LPTEXTMETRICW, *PTEXTMETRICW;

static inline BYTE GetRValue(COLORREF rgb)
{
	return rgb&0xff;
}

static inline BYTE GetGValue(COLORREF rgb)
{
	return (rgb>>8)&0xff;
}

static inline BYTE GetBValue(COLORREF rgb)
{
	return (rgb>>16)&0xff;
}

static inline COLORREF RGB( BYTE red, BYTE green, BYTE blue )
{
	return red | (green << 8) | (blue << 16);
}

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

BOOLEAN NTAPI NtGdiAddFontResourceW(PVOID,ULONG,ULONG,ULONG,PVOID,ULONG);
BOOLEAN NTAPI NtGdiBitBlt(HGDIOBJ,INT,INT,INT,INT,HGDIOBJ,INT,INT,ULONG,ULONG,ULONG);
BOOLEAN NTAPI NtGdiComputeXformCoefficients(HANDLE);
HGDIOBJ NTAPI NtGdiCreateBitmap(int,int,UINT,UINT,VOID*);
HANDLE  NTAPI NtGdiCreateCompatibleBitmap(HANDLE,int,int);
HGDIOBJ NTAPI NtGdiCreateCompatibleDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiCreateSolidBrush(COLORREF,ULONG);
HGDIOBJ NTAPI NtGdiCreateDIBitmapInternal(HDC,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
HANDLE  NTAPI NtGdiCreateDIBSection(HDC,HANDLE,ULONG,PBITMAPINFO,ULONG,ULONG,ULONG,ULONG_PTR,PVOID);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HGDIOBJ);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,ULONG,PULONG,PVOID);
BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PULONG);
ULONG NTAPI   NtGdiExtGetObjectW(HGDIOBJ,ULONG,PVOID);
BOOLEAN NTAPI NtGdiExtTextOutW(HANDLE,INT,INT,UINT,LPRECT,WCHAR*,UINT,INT*,UINT);
BOOLEAN NTAPI NtGdiFlush(void);
HGDIOBJ NTAPI NtGdiGetDCforBitmap(HGDIOBJ);
HGDIOBJ NTAPI NtGdiGetDCObject(HGDIOBJ,ULONG);
BOOLEAN NTAPI NtGdiGetFontResourceInfoInternalW(LPWSTR,ULONG,ULONG,UINT,PULONG,PVOID,ULONG);
HGDIOBJ NTAPI NtGdiGetStockObject(ULONG);
BOOLEAN NTAPI NtGdiGetTextMetricsW(HANDLE,PVOID,ULONG);
BOOLEAN NTAPI NtGdiInit(void);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
ULONG   NTAPI NtGdiQueryFontAssocInfo(HANDLE);
BOOLEAN NTAPI NtGdiRectangle(HANDLE,INT,INT,INT,INT);
BOOLEAN NTAPI NtGdiRestoreDC(HGDIOBJ,int);
int NTAPI     NtGdiSaveDC(HGDIOBJ);
HGDIOBJ NTAPI NtGdiSelectBitmap(HGDIOBJ,HGDIOBJ);
ULONG   NTAPI NtGdiSetDIBitsToDeviceInternal(HGDIOBJ,int,int,ULONG,ULONG,int,int,ULONG,ULONG,PVOID,PVOID,ULONG,ULONG,ULONG,ULONG,ULONG);
BOOL    NTAPI NtGdiSetFontEnumeration(PVOID);
BOOLEAN NTAPI NtGdiSetIcmMode(HANDLE,ULONG,ULONG);
BOOLEAN NTAPI NtGdiSetPixel(HANDLE,INT,INT,COLORREF);

#endif // __NTGDI_H__
