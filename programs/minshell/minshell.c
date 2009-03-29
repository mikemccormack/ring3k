/*
 * minimal shell
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

#include <windows.h>
#include <stdio.h>

#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 25

char terminal_buffer[TERMINAL_WIDTH*TERMINAL_HEIGHT];

void dprintf( const char *format, ... )
{
	char str[0x100];
	va_list va;
	va_start( va, format );
	vsprintf( str, format, va );
	va_end( va );
	OutputDebugString( str );
}

void do_paint( HWND hwnd )
{
	PAINTSTRUCT ps;
	int len = strlen( terminal_buffer );
	BeginPaint( hwnd, &ps );
	TextOut( ps.hdc, 0, 0, terminal_buffer, len );
	EndPaint( hwnd, &ps );
}

void do_addchar( HWND hwnd, UINT ch )
{
	int len = strlen( terminal_buffer );
	// backspace
	if (ch == 0x08)
	{
		if (len <= 0)
			return;
		len--;
	}
	else
	{
		if (len >= sizeof terminal_buffer - 1)
			return;
		terminal_buffer[len] = ch;
		len++;
	}
	terminal_buffer[len] = 0;
	InvalidateRect( hwnd, 0, 0 );
}

LRESULT CALLBACK minshell_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_PAINT:
		do_paint( hwnd );
		break;
	case WM_CHAR:
		do_addchar( hwnd, wparam );
		break;
	case WM_CLOSE:
		PostQuitMessage( 1 );
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// normal windows program
int window_winmain( HINSTANCE Instance )
{
	WNDCLASS wc;
	MSG msg;
	HWND hwnd;

	wc.style = 0;
	wc.lpfnWndProc = minshell_wndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = Instance;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = 0;
	wc.lpszClassName = "MINSHELL";
	if (!RegisterClass( &wc ))
	{
		dprintf("failed to register class\n");
		//MessageBox( NULL, "Failed to register class", "Error", MB_OK );
		return 0;
	}

	hwnd = CreateWindow("MINSHELL", "Minimal shell", WS_VISIBLE|WS_POPUP|WS_DLGFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
		NULL, NULL, Instance, NULL);
	if (!hwnd)
	{
		dprintf("failed to create window\n");
		//MessageBox( NULL, "Failed to create window", "Error", MB_OK );
		return 0;
	}

	while (GetMessage( &msg, 0, 0, 0 ))
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}

int APIENTRY WinMain( HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show )
{
	// running as normal windows program
	return window_winmain( Instance );
}

