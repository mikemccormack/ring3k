#ifndef __NTWIN32_H__
#define __NTWIN32_H__

#include "ntapi.h"

#ifndef WINXP

#define NTWIN32_THREAD_INIT_CALLBACK 74
#define NUM_USER32_CALLBACKS 90
#define NUMBER_OF_USER_SHARED_SECTIONS 29

#else

#define NTWIN32_THREAD_INIT_CALLBACK 75
#define NUM_USER32_CALLBACKS 98
#define NUMBER_OF_USER_SHARED_SECTIONS 33

#endif

#define GDI_OBJECT_DC      0x01
#define GDI_OBJECT_REGION  0x04
#define GDI_OBJECT_BITMAP  0x05
#define GDI_OBJECT_PALETTE 0x08
#define GDI_OBJECT_FONT    0x0a
#define GDI_OBJECT_BRUSH   0x10
#define GDI_OBJECT_EMF     0x21
#define GDI_OBJECT_PEN     0x30
#define GDI_OBJECT_EXTPEN  0x50

#define LF_FACESIZE     32
#define LF_FULLFACESIZE 64

typedef struct tagLOGFONTW
{
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

typedef struct
{
  LOGFONTW elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE];
  WCHAR      elfStyle[LF_FACESIZE];
  WCHAR      elfScript[LF_FACESIZE];
} ENUMLOGFONTEXW,*LPENUMLOGFONTEXW;

typedef struct
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
    DWORD     ntmFlags;
    UINT      ntmSizeEM;
    UINT      ntmCellHeight;
    UINT      ntmAvgWidth;
} NEWTEXTMETRICW, *PNEWTEXTMETRICW, *LPNEWTEXTMETRICW;

typedef struct
{
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE, *LPFONTSIGNATURE;

typedef struct
{
    NEWTEXTMETRICW	ntmTm;
    FONTSIGNATURE       ntmFontSig;
} NEWTEXTMETRICEXW;

#if 0
#define MM_MAX_AXES_NAMELEN 16
#define MM_MAX_NUMAXES 16

typedef struct tagAXISINFO
{
  LONG axMinValue;
  LONG axMaxValue;
  WCHAR axAxisName[MM_MAX_AXES_NAMELEN];
} AXISINFOW, *PAXISINFOW;

typedef struct tagAXESLIST {
  DWORD    axlReserved;
  DWORD    axlNumAxes;
  AXISINFOW axlAxisInfo[MM_MAX_NUMAXES];
} AXESLISTW, *PAXESLISTW;

typedef struct tagENUMTEXTMETRIC
{
  NEWTEXTMETRICEXW etmNewTextMetricEx;
  AXESLISTW etmAxesList;
} ENUMTEXTMETRICW, *PENUMTEXTMETRICW;
#endif

#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

NTSTATUS NTAPI NtGdiInit(void);
HANDLE NTAPI NtGdiCreateCompatibleDC(HANDLE);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HANDLE);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,ULONG,PULONG,PVOID);
BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PULONG);
HANDLE NTAPI NtGdiGetStockObject(ULONG Index);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
NTSTATUS NTAPI NtGdiQueryFontAssocInfo(HANDLE);

typedef struct USER_SHARED_MEMORY_INFO {
    ULONG Flags;
    PVOID Address;
} USER_SHARED_MEMORY_INFO, *PUSER_SHARED_MEMORY_INFO;

typedef struct _USER_PROCESS_CONNECT_INFO {
    ULONG Version;
    ULONG Unknown;
    ULONG MinorVersion;
    PVOID Ptr[4];
    USER_SHARED_MEMORY_INFO SharedSection[NUMBER_OF_USER_SHARED_SECTIONS];
} USER_PROCESS_CONNECT_INFO, *PUSER_PROCESS_CONNECT_INFO;

NTSTATUS NTAPI NtUserCallNoParam(ULONG);
NTSTATUS NTAPI NtUserCallOneParam(ULONG,ULONG);
NTSTATUS NTAPI NtUserCallTwoParam(ULONG,ULONG,ULONG);
HANDLE NTAPI   NtUserFindExistingCursorIcon(PUNICODE_STRING,PUNICODE_STRING,PVOID);
HANDLE NTAPI   NtUserGetDC(HANDLE);
LONG NTAPI     NtUserGetClassInfo(PVOID,PUNICODE_STRING,PVOID,PULONG,ULONG);
HANDLE NTAPI   NtUserGetThreadDesktop(ULONG,ULONG);
NTSTATUS NTAPI NtUserGetThreadState(ULONG);
NTSTATUS NTAPI NtUserInitializeClientPfnArrays(PVOID,PVOID,PVOID,PVOID);
ATOM NTAPI	   NtUserRegisterClassExWOW(PVOID,PVOID,PVOID,PVOID,ULONG,ULONG);
NTSTATUS NTAPI NtUserProcessConnect(HANDLE,PVOID,ULONG);

#endif // __NTWIN32_H__
