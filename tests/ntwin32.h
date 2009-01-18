// many definitions borrowed from Wine's winuser.h and wingdi.h

#ifndef __NTWIN32_H__
#define __NTWIN32_H__

#include "ntapi.h"

#ifndef WINXP

// Windows 2000
#define NTWIN32_BASICMSG_CALLBACK 2
#define NTWIN32_CREATE_CALLBACK 9
#define NTWIN32_MINMAX_CALLBACK 17
#define NTWIN32_NCCALC_CALLBACK 20
#define NTWIN32_THREAD_INIT_CALLBACK 74
#define NUM_USER32_CALLBACKS 90
#define NUMBER_OF_USER_SHARED_SECTIONS 29

#else

// Windows XP
#define NTWIN32_CREATE_CALLBACK 10
#define NTWIN32_MINMAX_CALLBACK 18
#define NTWIN32_NCCALC_CALLBACK 21
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

typedef struct tagTEXTMETRICW {
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

#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

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
#define DEFAULT_CHARSET       (BYTE)1
#define OEM_CHARSET	      (BYTE)255

#define WHITE_BRUSH	    0
#define LTGRAY_BRUSH	    1
#define GRAY_BRUSH	    2
#define DKGRAY_BRUSH	    3
#define BLACK_BRUSH	    4
#define NULL_BRUSH	    5
#define HOLLOW_BRUSH	    5
#define WHITE_PEN	    6
#define BLACK_PEN	    7
#define NULL_PEN	    8
#define OEM_FIXED_FONT	    10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT	    12
#define SYSTEM_FONT	    13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17
#define DC_BRUSH            18
#define DC_PEN              19

#define STOCK_LAST          19

#define RGB(r,g,b)          ((COLORREF)((BYTE)(r) | ((BYTE)(g) << 8) | ((BYTE)(b) << 16)))

NTSTATUS NTAPI NtGdiInit(void);
BOOLEAN NTAPI NtGdiComputeXformCoefficients(HANDLE);
HANDLE  NTAPI NtGdiCreateCompatibleDC(HANDLE);
HANDLE  NTAPI NtGdiCreateSolidBrush(COLORREF,ULONG);
BOOLEAN NTAPI NtGdiDeleteObjectApp(HANDLE);
BOOLEAN NTAPI NtGdiEnumFontChunk(HANDLE,HANDLE,ULONG,PULONG,PVOID);
BOOLEAN NTAPI NtGdiEnumFontClose(HANDLE);
HANDLE  NTAPI NtGdiEnumFontOpen(HANDLE,ULONG,ULONG,ULONG,ULONG,ULONG,PULONG);
BOOLEAN NTAPI NtGdiGetTextMetricsW(HANDLE,PVOID,ULONG);
HANDLE  NTAPI NtGdiGetStockObject(ULONG Index);
HANDLE  NTAPI NtGdiOpenDCW(ULONG,ULONG,ULONG,ULONG,ULONG,ULONG,PVOID);
NTSTATUS NTAPI NtGdiQueryFontAssocInfo(HANDLE);
ULONG   NTAPI NtGdiSetFontEnumeration(ULONG);

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

typedef struct _NTWINCALLBACKRETINFO {
	LRESULT	val;
	ULONG	size;
	PVOID	buf;
} NTWINCALLBACKRETINFO;

typedef struct tagNTWNDCLASSEX {
	UINT	Size;
	UINT	Style;
	PVOID	WndProc;
	INT	ClsExtra;
	INT	WndExtra;
	PVOID	Instance;
	HANDLE	Icon;
	HANDLE	Cursor;
	HANDLE	Background;
	PWSTR	MenuName;
	PWSTR	ClassName;
	HANDLE	IconSm;
} NTWNDCLASSEX, *PNTWNDCLASSEX;

typedef struct _NTSIMPLEMESSAGEPACKEDINFO {
	PVOID*	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	LPARAM	lparam;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,LPARAM,PVOID);
} NTSIMPLEMESSAGEPACKEDINFO;

typedef struct tagMINMAXINFO {
	POINT	ptReserved;
	POINT	ptMaxSize;
	POINT	ptMaxPosition;
	POINT	ptMinTrackSize;
	POINT	ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

typedef struct _NTMINMAXPACKEDINFO {
	PVOID*	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	MINMAXINFO minmax;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,MINMAXINFO*,PVOID);
} NTMINMAXPACKEDINFO;

typedef struct _NTPACKEDPOINTERINFO {
	ULONG	sz;
	ULONG	x;
	ULONG	count;
	PVOID	kernel_address;
	ULONG	adjust_info_ofs;
	BOOL	no_adjust;
} NTPACKEDPOINTERINFO;

typedef struct _NTCREATESTRUCT {
	LPVOID	lpCreateParams;
	PVOID	hInstance;
	HANDLE	hMenu;
	HANDLE	hwndParent;
	INT	cy;
	INT	cx;
	INT	y;
	INT	x;
	LONG	style;
	PVOID	lpszName;
	PVOID	lpszClass;
	DWORD	dwExStyle;
} NTCREATESTRUCT;

typedef struct _NTCREATEPACKEDINFO {
	NTPACKEDPOINTERINFO pi;
	PVOID*	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	BOOL	cs_nonnull;
	NTCREATESTRUCT cs;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,NTCREATESTRUCT,PVOID);
} NTCREATEPACKEDINFO;

typedef struct _WINDOWPOS {
	HANDLE	hwnd;
	HANDLE	hwndInsertAfter;
	INT	x;
	INT	y;
	INT	cx;
	INT	cy;
	UINT	flags;
} WINDOWPOS;

typedef struct _NCCALCSIZE_PARAMS {
	RECT	rgrc[3];
	WINDOWPOS *lppos;
} NCCALCSIZE_PARAMS;

typedef struct _NTNCCALCSIZEPACKEDINFO {
	PVOID*	wininfo;
	ULONG	msg;
	BOOL	wparam;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,NCCALCSIZE_PARAMS*,PVOID);
	NCCALCSIZE_PARAMS params;
	WINDOWPOS winpos;
} NTNCCALCSIZEPACKEDINFO;

typedef struct _NTMOVINGPACKEDINFO {
	PVOID*	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	RECT	rect;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,VOID*,PVOID);
} NTMOVINGPACKEDINFO;

#define WM_CREATE           0x0001
#define WM_MOVE             0x0003
#define WM_ACTIVATE         0x0006
#define WM_SHOWWINDOW       0x0018
#define WM_NCCREATE         0x0081
#define WM_NCCALCSIZE       0x0083
#define WM_GETMINMAXINFO    0x0024
#define WM_MOVING           0x0216

#define CS_VREDRAW          0x00000001
#define CS_HREDRAW          0x00000002
#define CS_DBLCLKS          0x00000008
#define CS_PARENTDC         0x00000080
#define CS_GLOBALCLASS      0x00004000

#define WS_CLIPSIBLINGS  0x04000000L
#define WS_CAPTION       0x00C00000L
#define WS_BORDER        0x00800000L
#define WS_DLGFRAME      0x00400000L
#define WS_SYSMENU       0x00080000L
#define WS_GROUP         0x00020000L

#define WS_EX_WINDOWEDGE       0x00000100L

#define CW_USEDEFAULT ((INT)0x80000000)

#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT      10
#define SW_FORCEMINIMIZE    11
#define SW_MAX              11
#define SW_NORMALNA         0xCC        /* undoc. flag in MinMaximize */

typedef struct tagUSER32_UNICODE_STRING {
	ULONG Length;
	ULONG MaximumLength;
	PWSTR Buffer;
} USER32_UNICODE_STRING, *PUSER32_UNICODE_STRING;

NTSTATUS NTAPI NtUserCallNoParam(ULONG);
NTSTATUS NTAPI NtUserCallOneParam(ULONG,ULONG);
HANDLE   NTAPI NtUserCreateWindowEx(ULONG,PUSER32_UNICODE_STRING,PUSER32_UNICODE_STRING,ULONG,LONG,LONG,LONG,LONG,HANDLE,HANDLE,PVOID,PVOID,BOOL);
NTSTATUS NTAPI NtUserCallTwoParam(ULONG,ULONG,ULONG);
HANDLE NTAPI   NtUserFindExistingCursorIcon(PUNICODE_STRING,PUNICODE_STRING,PVOID);
HANDLE NTAPI   NtUserGetDC(HANDLE);
LONG NTAPI     NtUserGetClassInfo(PVOID,PUNICODE_STRING,PVOID,PULONG,ULONG);
HANDLE NTAPI   NtUserGetThreadDesktop(ULONG,ULONG);
NTSTATUS NTAPI NtUserGetThreadState(ULONG);
NTSTATUS NTAPI NtUserInitializeClientPfnArrays(PVOID,PVOID,PVOID,PVOID);
NTSTATUS NTAPI NtUserProcessConnect(HANDLE,PVOID,ULONG);
ATOM NTAPI     NtUserRegisterClassExWOW(PNTWNDCLASSEX,PUNICODE_STRING,PVOID,USHORT,ULONG,ULONG);
BOOL NTAPI     NtUserResolveDesktop(HANDLE,PVOID,ULONG,PVOID);
BOOL NTAPI     NtUserShowWindow(HANDLE,int);

#endif // __NTWIN32_H__
