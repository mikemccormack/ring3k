/*
 * stub winlogon replacement
 *
 * Copyright 2008-2009 Mike McCormack
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
#include <stdio.h>

// from ntdll
int __cdecl vsprintf(char *, const char *, va_list);

//////////////////////////////////////////////////////////////////////////////

#ifdef LOGGING

// logging

HANDLE log_handle;
void log_open(void)
{
	log_handle = CreateFile("c:\\mmlog.txt", GENERIC_READ | GENERIC_WRITE,
		 FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
}

// compatible with win2k int 2d
struct log_buffer {
	USHORT len;
	USHORT pad;
	ULONG empty[3];
	char buffer[0x100];
};

void log_write( struct log_buffer *lb )
{
	ULONG written;
	WriteFile( log_handle, lb->buffer, lb->len, &written, 0 );
	FlushFileBuffers( log_handle );
}

void log_debug( struct log_buffer *lb )
{
	__asm__ __volatile ( "\tint $0x2d\n" : : "c"(lb), "a"(1) );
}

void dprintf(char *string, ...)
{
	struct log_buffer x;
	va_list va;

	x.empty[0] = 0;
	x.empty[1] = 0;
	x.empty[2] = 0;

	va_start( va, string );
	vsprintf( x.buffer, string, va );
	va_end( va );

	x.len = 0;
	while (x.buffer[x.len])
		x.len++;

	if (log_handle != 0 && log_handle != INVALID_HANDLE_VALUE)
		log_write( &x );
	else
		log_debug( &x );
}

void log_close( void )
{
	CloseHandle( log_handle );
	log_handle = 0;
}

#else

void log_open( void ) {}
void log_close( void ) {}
void dprintf( const char *format, ... ) {}

#endif

//////////////////////////////////////////////////////////////////////////////

void start_process(const char *cmd)
{
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	BOOL r;

	memset( &si, 0, sizeof si );
	si.cb = sizeof pi;
	si.lpDesktop = "WinSta0\\Winlogon";

	r = CreateProcessA( cmd, 0, 0, 0, TRUE, 0, 0, 0, &si, &pi );
	if (!r)
		dprintf( "CreateProcessA failed %ld\n", GetLastError());
	else
	{
		dprintf( "CreateProcessA succeeded, waiting\n");

		r = WaitForSingleObject( pi.hProcess, INFINITE );
		if ( r == WAIT_OBJECT_0 )
		{
			dprintf( "process exitted\n");
		}
		dprintf("wait exitted %d\n", r);
	}
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );
}

void init_desktop(void)
{
	HANDLE hwsta, hdesk;
	BOOL r;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;

	hwsta = CreateWindowStationW( L"WinSta0", 0, MAXIMUM_ALLOWED, &sa );
	if (!hwsta)
		dprintf("CreateWindowStationW failed %ld\n", GetLastError());

	r = SetProcessWindowStation( hwsta );
	if (!r)
		dprintf("SetProcessWindowStation failed %ld\n", GetLastError());

	hdesk = CreateDesktopW( L"Winlogon", 0, 0, 0, MAXIMUM_ALLOWED, &sa );
	if (!hdesk)
		dprintf("CreateDesktopW failed %ld\n", GetLastError());

	r = SetThreadDesktop( hdesk );
	if (!r)
		dprintf("SetThreadDesktop failed %ld\n", GetLastError());
}

int main( int argc, char **argv )
{
	log_open();
	dprintf("started\n");

	// create a desktop to put the process on
	init_desktop();

	dprintf("desktop window = %p\n", GetDesktopWindow());

	// start the process in the desktop
	start_process( "winmine.exe" );

	log_close();
	Sleep(1000);
	return 0;
}
