/*
 * Winlogon replacement - runs cmd.exe on a serial port
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

#include <windows.h>
#include <stdio.h>

// from ntdll
int __cdecl vsprintf(char *, const char *, va_list);

// handle of the cmd.exe process
HANDLE cmd_process;

//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

// comm port handling

HANDLE comm_handle;

void comm_open(void)
{
	comm_handle = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
		 0, 0, OPEN_EXISTING, 0, 0);
	if (comm_handle == INVALID_HANDLE_VALUE)
		return;

	COMMCONFIG cc;
	ULONG sz = sizeof cc;
	if (!GetCommConfig( comm_handle, &cc, &sz ))
	{
		dprintf("GetCommConfig failed %d\n", GetLastError());
		return;
	}

	cc.dcb.fOutX = 0;
	cc.dcb.fInX = 0;
	cc.dcb.fOutxCtsFlow = 0;
	cc.dcb.fOutxDsrFlow = 0;
	cc.dcb.fRtsControl = RTS_CONTROL_ENABLE;
	cc.dcb.fDtrControl = DTR_CONTROL_ENABLE;
	cc.dcb.fDsrSensitivity = 0;
	cc.dcb.fTXContinueOnXoff = 0;
	cc.dcb.BaudRate = BAUD_115200;
	SetCommConfig( comm_handle, &cc, sizeof cc );

	SetCommMask( comm_handle, EV_RXCHAR );

	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout = MAXDWORD;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(comm_handle, &cto );
}

BOOL write_comm( const char *buffer, ULONG len )
{
	DWORD written = 0;

	return WriteFile( comm_handle, buffer, len, &written, 0 );
}

BOOL read_comm( char* buffer, ULONG len )
{
	BOOL r;
	DWORD read = 0;

	r = ReadFile( comm_handle, buffer, len, &read, 0 );
	return (r && read);
}

//////////////////////////////////////////////////////////////////////////////

// terminal handling

#define BACKSPACE 0x08
#define NEWLINE   0x0d
#define ESCAPE    0x1b

char ch2hex( int x )
{
	if ( x < 10 ) return x + '0';
	return x + 'A' - 10;
}

BOOL add_input_char( HANDLE pipe, CHAR *buffer, ULONG *n, ULONG buffer_size, CHAR ch )
{
	CHAR erase[] = { BACKSPACE, ' ', BACKSPACE };
	BOOL r;

	// backspace
	if (ch == BACKSPACE || ch == 0x7f)
	{
		if ((*n) > 0)
		{
			write_comm( erase, sizeof erase );
			(*n)--;
		}
		return TRUE;
	}

	if (ch == NEWLINE)
	{
		// erase the line, cmd.exe echos it again
		ULONG i;
		ULONG count = 0;
		for (i=0; i<(*n); i++)
			write_comm( erase, sizeof erase );

		// send the command
		i = 0;
		while (i < *n)
		{
			// guard against partial writes
			r = WriteFile( pipe, buffer + i, *n, &count, 0 );
			if (!r)
				return FALSE;
			i += count;
		}

		r = WriteFile( pipe, "\n", 1, &count, 0 );
		if (!r)
			return FALSE;

		(*n) = 0;
		return TRUE;
	}

	if (ch < 0x20 || ch > 0x7e)
		return TRUE;

	// ignore overrun, leave space for CRLF
	if ((*n) >= (buffer_size-2))
		return TRUE;

	buffer[(*n)++] = ch;
	write_comm( &ch, 1 );
	return TRUE;
}

BOOL read_input( HANDLE pipe, CHAR *buffer, ULONG *n, ULONG buffer_size )
{
	while (1)
	{
		CHAR ch;
		if (!read_comm( &ch, 1 ))
			return TRUE;

		if (!add_input_char( pipe, buffer, n, buffer_size, ch ))
			return FALSE;
	}
}

DWORD WINAPI read_output( HANDLE pipe )
{
	CHAR ch;
	ULONG count;
	BOOL r;

	r = ReadFile( pipe, &ch, 1, &count, 0 );
	if (!r || count == 0)
		return 0;
	write_comm( &ch, 1 );
	return TRUE;
}

DWORD WINAPI do_loop( HANDLE pipeout, HANDLE pipein )
{
	CHAR buffer[80], output[80];
	ULONG n = 0;
	ULONG r;
	ULONG count;
	BOOL finished = FALSE;

	while (!finished)
	{
		r = PeekNamedPipe( pipein, 0, 0, 0, &count, 0 );
		if (r && count)
		{
			if (count > sizeof output)
				count = sizeof output;
			if (!ReadFile( pipein, output, count, &count, 0 ))
				break;
			write_comm( output, count );
			continue;
		}

		// check for serial input
		if (!read_input( pipeout, buffer, &n, sizeof buffer ))
		{
			finished = TRUE;
			break;
		}

		// check whether cmd.exe exitted
		if (WAIT_OBJECT_0 == WaitForSingleObject( cmd_process, 0 ))
		{
			finished = TRUE;
			break;
		}

		Sleep( 30 );
	}

	return 0;
}

BOOL start_process( HANDLE in, HANDLE out )
{
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	BOOL r;

	memset( &si, 0, sizeof si );
	si.cb = sizeof pi;
	si.hStdInput = in;
	si.hStdOutput = out;
	si.hStdError = 0;
	si.dwFlags |= STARTF_USESTDHANDLES;

	r = CreateProcessW( L"c:\\winnt\\system32\\cmd.exe", 0, 0, 0, TRUE, 0, 0, 0, &si, &pi );
	if (!r)
	{
		dprintf("CreateProcessW failed %d\n", GetLastError());
		return r;
	}

	cmd_process = pi.hProcess;

	return TRUE;
}

void init_window_station( void )
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hwsta, hdesk;
	BOOL r;

	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;

	hwsta = CreateWindowStationW( L"winsta0", 0, MAXIMUM_ALLOWED, &sa );
	if (!hwsta)
		dprintf("CreateWindowStationW failed %d\n", GetLastError());

	r = SetProcessWindowStation( hwsta );
	if (!r)
		dprintf("SetProcessWindowStation failed %d\n", GetLastError());

	hdesk = CreateDesktopW( L"Winlogon", 0, 0, 0, MAXIMUM_ALLOWED, &sa );
	if (!hdesk)
		dprintf("CreateDesktopW failed %d\n", GetLastError());

	r = SetThreadDesktop( hdesk );
	if (!r)
		dprintf("SetThreadDesktop failed %d\n", GetLastError());
}

void run_cmd_exe(void)
{
	// create a pipe
	HANDLE p1in = 0, p1out = 0, p2in = 0, p2out = 0;
	SECURITY_ATTRIBUTES sa;

	dprintf("creating pipe\n");

	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe( &p1in, &p1out, &sa, 0 ))
		dprintf("CreatePipe failed %d\n", GetLastError());

	SetHandleInformation( p1out, HANDLE_FLAG_INHERIT, 0);

	if (!CreatePipe( &p2in, &p2out, &sa, 0 ))
		dprintf("CreatePipe failed %d\n", GetLastError());

	SetHandleInformation( p2in, HANDLE_FLAG_INHERIT, 0);

	dprintf("starting cmd.exe\n");
	start_process( p1in, p2out );

	ULONG mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
	if (!SetNamedPipeHandleState( p2in, &mode, 0, 0 ))
		dprintf("set handle state failed\n");

	do_loop( p1out, p2in );
}

void draw_pixel( void )
{
	HDC hdc = GetDC( 0 );
	int i, j, ok = TRUE;

	for (i=0; ok && i<0x10; i++)
		for (j=0; ok && j<0x10; j++)
			ok = SetPixel( hdc, i, j, RGB(128, 128, 128));
	if (!ok)
		dprintf("SetPixel failed (%ld)\n", GetLastError());

	ReleaseDC( 0, hdc );
}

int main( int argc, char **argv )
{
	log_open();
	comm_open();

	dprintf("attaching to window station\n");
	init_window_station();

	draw_pixel();

	dprintf("close & sleeping\n");
	log_close();
	Sleep(100*1000);
	return 0;
}
