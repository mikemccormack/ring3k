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

#ifndef __NTUSER_H__
#define __NTUSER_H__

#define NUMBER_OF_USER_SHARED_SECTIONS 29

typedef struct USER_SHARED_MEMORY_INFO {
	ULONG Flags;
	PVOID Address;
} USER_SHARED_MEMORY_INFO, *PUSER_SHARED_MEMORY_INFO;

typedef struct _USER_PROCESS_CONNECT_INFO {
	ULONG Version;  // set by caller
	ULONG Empty[2];
	PVOID Ptr[4];
	USER_SHARED_MEMORY_INFO SharedSection[NUMBER_OF_USER_SHARED_SECTIONS];
} USER_PROCESS_CONNECT_INFO, *PUSER_PROCESS_CONNECT_INFO;

#define NUMBER_OF_USER_SHARED_SECTIONS_XP 33
typedef struct _USER_PROCESS_CONNECT_INFO_XP {
	ULONG Version;  // set by caller
	ULONG Empty[2];
	PVOID Ptr[4];
	USER_SHARED_MEMORY_INFO SharedSection[NUMBER_OF_USER_SHARED_SECTIONS_XP];
} USER_PROCESS_CONNECT_INFO_XP, *PUSER_PROCESS_CONNECT_INFO_XP;

typedef struct _ICONINFO {
	BOOL	fIcon;
	DWORD	xHotspot;
	DWORD	yHotspot;
	HBITMAP	hbmMask;
	HBITMAP	hbmColor;
} ICONINFO, *PICONINFO;

typedef struct tagMSG
{
	HWND	hwnd;
	UINT	message;
	WPARAM	wParam;
	LPARAM	lParam;
	DWORD	time;
	POINT	pt;
} MSG, *PMSG;

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

ULONG NTAPI    NtUserCallNoParam(ULONG);
ULONG NTAPI    NtUserCallOneParam(ULONG,ULONG);
ULONG NTAPI    NtUserCallTwoParam(ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserConsoleControl(ULONG,PVOID,ULONG);
HANDLE NTAPI   NtUserCreateAcceleratorTable(PVOID,UINT);
HANDLE NTAPI   NtUserCreateDesktop(POBJECT_ATTRIBUTES,ULONG,ULONG,ULONG,ACCESS_MASK);
HANDLE NTAPI   NtUserCreateWindowEx(ULONG,PUSER32_UNICODE_STRING,PUSER32_UNICODE_STRING,ULONG,LONG,LONG,LONG,LONG,HANDLE,HANDLE,PVOID,PVOID,BOOL);
HANDLE NTAPI   NtUserCreateWindowStation(POBJECT_ATTRIBUTES,ACCESS_MASK,HANDLE,ULONG,PVOID,ULONG);
HANDLE NTAPI   NtUserFindExistingCursorIcon(PUNICODE_STRING,PUNICODE_STRING,PVOID);
ULONG NTAPI    NtUserGetAsyncKeyState(ULONG);
ULONG NTAPI    NtUserGetCaretBlinkTime(void);
LONG NTAPI     NtUserGetClassInfo(PVOID,PUNICODE_STRING,PVOID,PULONG,ULONG);
HANDLE NTAPI   NtUserGetDC(HANDLE);
NTSTATUS NTAPI NtUserGetKeyboardLayoutList(ULONG,ULONG);
BOOLEAN NTAPI  NtUserGetMessage(PMSG,HANDLE,ULONG,ULONG);
BOOLEAN NTAPI  NtUserGetObjectInformation(HANDLE,ULONG,PVOID,ULONG,PULONG);
HANDLE NTAPI   NtUserGetProcessWindowStation(void);
HANDLE NTAPI   NtUserGetThreadDesktop(ULONG,ULONG);
ULONG NTAPI    NtUserGetThreadState(ULONG);
BOOLEAN NTAPI  NtUserInitialize(ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserInitializeClientPfnArrays(PVOID,PVOID,PVOID,PVOID);
BOOLEAN NTAPI  NtUserLoadKeyboardLayoutEx(HANDLE,ULONG,ULONG,PVOID,ULONG,ULONG);
BOOLEAN NTAPI  NtUserMoveWindow(HANDLE,int,int,int,int,BOOLEAN);
BOOLEAN NTAPI  NtUserNotifyProcessCreate(ULONG,ULONG,ULONG,ULONG);
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
BOOLEAN NTAPI  NtUserSetProcessWindowStation(HANDLE);
BOOLEAN NTAPI  NtUserSetThreadDesktop(HANDLE);
UINT NTAPI     NtUserSetTimer(HANDLE,UINT,UINT,PVOID);
BOOLEAN NTAPI  NtUserSetWindowStationUser(HANDLE,PVOID,ULONG,ULONG);
BOOLEAN NTAPI  NtUserShowWindow(HANDLE,INT);
BOOLEAN NTAPI  NtUserSystemParametersInfo(ULONG,ULONG,ULONG,ULONG);
BOOLEAN NTAPI  NtUserUpdatePerUserSystemParameters(ULONG,ULONG);

#endif // __NTUSER_H__
