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

/*
 * Careful!  Playing with winlogon.exe will make your system unbootable!
 *
 * How to use this program:
 *
 * Mount your windows partition:
 * # mount -o loop,offset=32256,uid=1000 ~mike/win2k.img /mnt/
 *
 * Copy winlogon.exe to _winlogon.exe
 * # cp -i /mnt/winnt/system32/winlogon.exe /mnt/winnt/system32/_winlogon.exe
 *
 * Unmount your window partition before booting it
 * # umount /mnt
 *
 * Run qemu (Windows will boot normally until you change the start key):
 * $ qemu -hda ~mike/win2k.img
 *
 * Set the value "start" in the key HKEY_LOCAL_MACHINE\Software\ring3k
 *  to the program you want to run instead of winlogon.
 *
 * Touch c:\start-ignore or delete that the start value to revert to using winlogon.
 *
 * To restore winlogon completely:
 * # cp -i /mnt/winnt/system32/_winlogon.exe /mnt/winnt/system32/winlogon.exe
 */

#include <windows.h>
#include <stdio.h>

// renamed original winlogon.exe
const char backup_winlogon[] = "_winlogon.exe";

// name of the key to look in for the start_value
const char ring3k_key[] = "Software\\ring3k";

// name of the value containing a program to start instead of backup_winlogon
const char start_value[] = "start";

// if this file is present, backup_winlogon will be started
const char ignore_file[] = "c:\\start-ignore";

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

BOOL get_start_program( char *buffer, int len )
{
	HKEY hkey;
	LONG r;
	DWORD type = 0, sz;

	r = RegOpenKey( HKEY_LOCAL_MACHINE, ring3k_key, &hkey );
	if (r != ERROR_SUCCESS)
		return FALSE;

	sz = len;
	r = RegQueryValueEx( hkey, start_value, NULL, &type, (LPBYTE) buffer, &sz );
	if (r != ERROR_SUCCESS)
		return FALSE;

	RegCloseKey( hkey );

	if (type != REG_SZ)
		return FALSE;

	return TRUE;
}

BOOL ignore_start_key( void )
{
	HANDLE file;

	file = CreateFile(ignore_file, GENERIC_READ,
		 FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (file == INVALID_HANDLE_VALUE)
		return FALSE;
	CloseHandle( file );
	return TRUE;
}

int main( int argc, char **argv )
{
	char prog[0x40];

	log_open();
	dprintf("started\n");

	// create a desktop to put the process on
	init_desktop();

	dprintf("desktop window = %p\n", GetDesktopWindow());

	if (ignore_start_key() || !get_start_program( prog, sizeof prog ))
		strcpy( prog, backup_winlogon );

	// start the process in the desktop
	start_process( prog );

	log_close();
	Sleep(1000);
	return 0;
}
