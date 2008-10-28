/*
 * advapi32 stub
 *
 * Useful for avoiding loading of rpcrt4 and use of named pipes during startup
 * Intended only as a debugging and development too.
 * Avoid extending, need to make the native advapi32 work.
 *
 * Copyright 2008 Mike McCormack
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

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

__declspec(dllexport)
HANDLE WINAPI RegisterEventSourceW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
	return 0;
}

__declspec(dllexport)
BOOL WINAPI ReportEventW( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID,
    PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCWSTR *lpStrings, LPVOID lpRawData )
{
	return TRUE;
}

__declspec(dllexport)
BOOL WINAPI DeregisterEventSource( HANDLE hEventLog )
{
	return TRUE;
}

__declspec(dllexport)
BOOL WINAPI
GetTokenInformation( HANDLE token, TOKEN_INFORMATION_CLASS tokeninfoclass,
                     LPVOID tokeninfo, DWORD tokeninfolength, LPDWORD retlen )
{
	*(BYTE*)0 = 1;
	return FALSE;
}

__declspec(dllexport)
BOOL WINAPI
OpenProcessToken( HANDLE ProcessHandle, DWORD DesiredAccess,
                  HANDLE *TokenHandle )
{
	*(BYTE*)0 = 2;
	TokenHandle = (HANDLE)0xdeadbeef;
	return TRUE;
}

__declspec(dllexport)
BOOL WINAPI
OpenThreadToken( HANDLE ThreadHandle, DWORD DesiredAccess,
                 BOOL OpenAsSelf, HANDLE *TokenHandle)
{
	*(BYTE*)0 = 3;
	TokenHandle = (HANDLE)0xdeadbeef;
	return TRUE;
}
