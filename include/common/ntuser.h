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

#ifndef __NTUSER_H__
#define __NTUSER_H__

#ifndef WINXP

// Windows 2000
#define NTWIN32_BASICMSG_CALLBACK 2
#define NTWIN32_CREATE_CALLBACK 9
#define NTWIN32_POSCHANGED_CALLBACK 16
#define NTWIN32_MINMAX_CALLBACK 17
#define NTWIN32_NCCALC_CALLBACK 20
#define NTWIN32_POSCHANGING_CALLBACK 21
#define NTWIN32_THREAD_INIT_CALLBACK 74
#define NUM_USER32_CALLBACKS 90
#define NUMBER_OF_MESSAGE_MAPS 29

#else

// Windows XP
#define NTWIN32_CREATE_CALLBACK 10
#define NTWIN32_POSCHANGED_CALLBACK 17
#define NTWIN32_MINMAX_CALLBACK 18
#define NTWIN32_NCCALC_CALLBACK 21
#define NTWIN32_POSCHANGING_CALLBACK 22
#define NTWIN32_THREAD_INIT_CALLBACK 75
#define NUM_USER32_CALLBACKS 98
#define NUMBER_OF_MESSAGE_MAPS 33

#endif

#define NUMBER_OF_MESSAGE_MAPS 29

typedef struct USER_SHARED_MEMORY_INFO {
	ULONG MaxMessage;
	BYTE* Bitmap;
} MESSAGE_MAP_SHARED_MEMORY, *PMESSAGE_MAP_SHARED_MEMORY;

typedef struct _USER_PROCESS_CONNECT_INFO {
	ULONG Version;
	ULONG Unknown;
	ULONG MinorVersion;
	PVOID Ptr[4];
	MESSAGE_MAP_SHARED_MEMORY MessageMap[NUMBER_OF_MESSAGE_MAPS];
} USER_PROCESS_CONNECT_INFO, *PUSER_PROCESS_CONNECT_INFO;

#define NUMBER_OF_MESSAGE_MAPS_XP 33
typedef struct _USER_PROCESS_CONNECT_INFO_XP {
	ULONG Version;
	ULONG Empty[2];
	PVOID Ptr[4];
	MESSAGE_MAP_SHARED_MEMORY MessageMap[NUMBER_OF_MESSAGE_MAPS_XP];
} USER_PROCESS_CONNECT_INFO_XP, *PUSER_PROCESS_CONNECT_INFO_XP;

typedef struct _NTWINCALLBACKRETINFO {
	LRESULT val;
	ULONG   size;
	PVOID   buf;
} NTWINCALLBACKRETINFO;

typedef struct tagUSER32_UNICODE_STRING {
	ULONG Length;
	ULONG MaximumLength;
	PWSTR Buffer;
} USER32_UNICODE_STRING, *PUSER32_UNICODE_STRING;

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

typedef struct tagNTCREATESTRUCT {
	PVOID	lpCreateParams;
	PVOID	hInstance;
	PVOID	hMenu;
	HWND	hwndParent;
	INT	cy;
	INT	cx;
	INT	y;
	INT	x;
	LONG	style;
	PVOID	lpszName;
	PVOID	lpszClass;
	DWORD	dwExStyle;
} NTCREATESTRUCT, *PNTCREATESTRUCT;

typedef struct tagNTCLASSMENUNAMES {
	PSTR name_a;
	PWSTR name_w;
	PUNICODE_STRING name_us;
} NTCLASSMENUNAMES, *PNTCLASSMENUNAMES;

// shared memory structure!!
// see http://winterdom.com/dev/ui/wnd.html
struct _CLASSINFO;
typedef struct _CLASSINFO CLASSINFO, *PCLASSINFO;

struct _CLASSINFO
{
	DWORD		unk1;
	ATOM		atomWindowType;
	WORD		unk2;
	DWORD		unk3;
	DWORD		unk4;
	DWORD		unk5;
	DWORD		unk6;
	DWORD		unk7;
	DWORD		unk8;
	CLASSINFO*	pSelf;
	DWORD		unk10;
	DWORD		unk11;
	DWORD		dwStyle;
	DWORD		unk13;
	DWORD		unk14;
	DWORD		dwClassBytes;
	HINSTANCE	hInstance;
};

struct _WNDMENU;
typedef struct _WNDMENU WNDMENU, *PWNDMENU;

struct _WND;
typedef struct _WND WND, *PWND;
struct _WND {
	HWND handle;
	ULONG unk1;
	ULONG unk2;
	ULONG unk3;
	PWND self;
	ULONG dwFlags;
	ULONG unk6;
	ULONG exstyle;
	ULONG style;
	PVOID hInstance;
	ULONG unk10;
	PWND next;
	PWND parent;
	PWND first_child;
	PWND owner;
	RECT rcWnd;
	RECT rcClient;
	void* wndproc;
	CLASSINFO* wndcls;
	ULONG unk25;
	ULONG unk26;
	ULONG unk27;
	ULONG unk28;
	union {
		ULONG dwWndID;
		WNDMENU* Menu;
	} id;
	ULONG unk30;
	ULONG unk31;
	ULONG unk32;
	PWSTR pText;
	ULONG dwWndBytes;
	ULONG unk35;
	ULONG unk36;
	ULONG wlUserData;
	ULONG wlWndExtra[1];
};

typedef struct _NTPACKEDPOINTERINFO {
	ULONG	sz;
	ULONG	x;
	ULONG	count;
	PVOID	kernel_address;
	ULONG	adjust_info_ofs;
	BOOL	no_adjust;
} NTPACKEDPOINTERINFO;

typedef struct _NTCREATEPACKEDINFO {
	NTPACKEDPOINTERINFO pi;
	PWND    wininfo;
	ULONG   msg;
	WPARAM  wparam;
	BOOL    cs_nonnull;
	NTCREATESTRUCT cs;
	PVOID   wndproc;
	ULONG   (CALLBACK *func)(PVOID,ULONG,WPARAM,NTCREATESTRUCT,PVOID);
} NTCREATEPACKEDINFO;

typedef struct _NTNCCALCSIZEPACKEDINFO {
	PWND	wininfo;
	ULONG	msg;
	BOOL	wparam;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,NCCALCSIZE_PARAMS*,PVOID);
	NCCALCSIZE_PARAMS params;
	WINDOWPOS winpos;
} NTNCCALCSIZEPACKEDINFO;

typedef struct _NTMINMAXPACKEDINFO {
	PWND	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	MINMAXINFO minmax;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,MINMAXINFO*,PVOID);
} NTMINMAXPACKEDINFO;

typedef struct _NTSIMPLEMESSAGEPACKEDINFO {
	PWND	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	LPARAM	lparam;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,LPARAM,PVOID);
} NTSIMPLEMESSAGEPACKEDINFO;

typedef struct _NTPOSCHANGINGPACKEDINFO {
	PWND wininfo;
	ULONG msg;
	WPARAM wparam;
	WINDOWPOS winpos;
	PVOID wndproc;
	ULONG (CALLBACK *func)(PVOID,ULONG,WPARAM,WINDOWPOS*,PVOID);
} NTPOSCHANGINGPACKEDINFO, *PNTPOSCHANGINGPACKEDINFO;

#define NTUCOP_GETWNDPTR 0x23
#define NTUCOP_POSTQUITMESSAGE 0x26

typedef struct _NTUSERINFO {
	ULONG x1;
	ULONG x2;
	PWND DesktopWindow;
} NTUSERINFO, *PNTUSERINFO;

HDC NTAPI      NtUserBeginPaint(HWND,PAINTSTRUCT*);
ULONG NTAPI    NtUserCallNoParam(ULONG);
ULONG NTAPI    NtUserCallOneParam(ULONG,ULONG);
BOOLEAN NTAPI  NtUserCallHwnd(HWND,ULONG);
ULONG NTAPI    NtUserCallTwoParam(ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserConsoleControl(ULONG,PVOID,ULONG);
HANDLE NTAPI   NtUserCreateAcceleratorTable(PVOID,UINT);
HANDLE NTAPI   NtUserCreateDesktop(POBJECT_ATTRIBUTES,ULONG,ULONG,ULONG,ACCESS_MASK);
HANDLE NTAPI   NtUserCreateWindowEx(ULONG,PUSER32_UNICODE_STRING,PUSER32_UNICODE_STRING,ULONG,LONG,LONG,LONG,LONG,HANDLE,HANDLE,PVOID,PVOID,BOOL);
HANDLE NTAPI   NtUserCreateWindowStation(POBJECT_ATTRIBUTES,ACCESS_MASK,HANDLE,ULONG,PVOID,ULONG);
BOOLEAN NTAPI  NtUserDestroyWindow(HWND);
LRESULT NTAPI  NtUserDispatchMessage(PMSG);
BOOLEAN NTAPI  NtUserEndPaint(HWND,PAINTSTRUCT*);
HANDLE NTAPI   NtUserFindExistingCursorIcon(PUNICODE_STRING,PUNICODE_STRING,PVOID);
ULONG NTAPI    NtUserGetAsyncKeyState(ULONG);
ULONG NTAPI    NtUserGetCaretBlinkTime(void);
LONG NTAPI     NtUserGetClassInfo(PVOID,PUNICODE_STRING,PVOID,PULONG,ULONG);
HANDLE NTAPI   NtUserGetDC(HANDLE);
NTSTATUS NTAPI NtUserGetKeyboardLayoutList(ULONG,ULONG);
BOOLEAN NTAPI  NtUserGetMessage(PMSG,HWND,ULONG,ULONG);
BOOLEAN NTAPI  NtUserGetObjectInformation(HANDLE,ULONG,PVOID,ULONG,PULONG);
HANDLE NTAPI   NtUserGetProcessWindowStation(void);
HANDLE NTAPI   NtUserGetThreadDesktop(ULONG,ULONG);
ULONG NTAPI    NtUserGetThreadState(ULONG);
BOOLEAN NTAPI  NtUserGetUpdateRgn(HWND,HRGN,BOOLEAN);
BOOLEAN NTAPI  NtUserInitialize(ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserInitializeClientPfnArrays(PVOID,PVOID,PVOID,PVOID);
BOOLEAN NTAPI  NtUserInvalidateRect(HWND,const RECT*,BOOLEAN);
BOOLEAN NTAPI  NtUserKillTimer(HWND,UINT);
BOOLEAN NTAPI  NtUserLoadKeyboardLayoutEx(HANDLE,ULONG,ULONG,PVOID,ULONG,ULONG);
BOOLEAN NTAPI  NtUserMessageCall(HWND,ULONG,ULONG,PVOID,ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserMoveWindow(HANDLE,int,int,int,int,BOOLEAN);
BOOLEAN NTAPI  NtUserNotifyProcessCreate(ULONG,ULONG,ULONG,ULONG);
HANDLE NTAPI   NtUserOpenDesktop(POBJECT_ATTRIBUTES,ULONG,ACCESS_MASK);
BOOLEAN NTAPI  NtUserPeekMessage(PMSG,HWND,UINT,UINT,UINT);
BOOLEAN NTAPI  NtUserPostMessage(HWND,UINT,WPARAM,LPARAM);
NTSTATUS NTAPI NtUserProcessConnect(HANDLE,PVOID,ULONG);
BOOLEAN NTAPI  NtUserRedrawWindow(HANDLE,RECT*,HANDLE,UINT);
ATOM NTAPI     NtUserRegisterClassExWOW(PNTWNDCLASSEX,PUNICODE_STRING,PNTCLASSMENUNAMES,USHORT,ULONG,ULONG);
ULONG NTAPI    NtUserRegisterWindowMessage(PUNICODE_STRING);
BOOLEAN NTAPI  NtUserResolveDesktop(HANDLE,PVOID,PVOID,PHANDLE);
HGDIOBJ NTAPI  NtUserSelectPalette(HGDIOBJ,HPALETTE,BOOLEAN);
BOOLEAN NTAPI  NtUserSetCursorIconData(HANDLE,PVOID,PUNICODE_STRING,PICONINFO);
BOOLEAN NTAPI  NtUserSetImeHotKey(ULONG,ULONG,ULONG,ULONG,ULONG);
NTSTATUS NTAPI NtUserSetInformationThread(HANDLE,ULONG,PVOID,ULONG);
BOOLEAN NTAPI  NtUserSetLogonNotifyWindow(HANDLE);
BOOLEAN NTAPI  NtUserSetMenu(HWND,ULONG,ULONG);
BOOLEAN NTAPI  NtUserSetProcessWindowStation(HANDLE);
BOOLEAN NTAPI  NtUserSetThreadDesktop(HANDLE);
UINT NTAPI     NtUserSetTimer(HWND,UINT,UINT,PVOID);
BOOLEAN NTAPI  NtUserSetWindowStationUser(HANDLE,PVOID,ULONG,ULONG);
BOOLEAN NTAPI  NtUserShowWindow(HANDLE,INT);
BOOLEAN NTAPI  NtUserSystemParametersInfo(ULONG,ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserUpdatePerUserSystemParameters(ULONG,ULONG);
BOOLEAN NTAPI  NtUserValidateRect(HWND,PRECT);

#endif // __NTUSER_H__
