/*
 * Winlogon replacement - Mouse demo
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

int mouse_color = 0;

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
	BeginPaint( hwnd, &ps );
	EndPaint( hwnd, &ps );
}

void do_leftbuttondown( HWND hwnd )
{
	dprintf("track: left button down");
	mouse_color >>= 8;
	if (!mouse_color)
		mouse_color = 0xff0000;
}

void do_mousemove( HWND hwnd, INT x, INT y )
{
	dprintf("track: mouse move");
	HDC hdc = GetDC( hwnd );
	HBRUSH brush, old_brush;
	brush = CreateSolidBrush( mouse_color );
	old_brush = SelectObject( hdc, brush );
	Rectangle( hdc, x, y, x+10, y+10 );
	SelectObject( hdc, old_brush );
	ReleaseDC( hwnd, hdc );
}

LRESULT CALLBACK track_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	dprintf("track: wndproc %p %08x %08x %08lx", hwnd, msg, wparam, lparam);
	switch (msg)
	{
	case WM_PAINT:
		do_paint( hwnd );
		break;
	case WM_LBUTTONDOWN:
		do_leftbuttondown( hwnd );
		break;
	case WM_MOUSEMOVE:
		do_mousemove( hwnd, LOWORD(lparam), HIWORD(lparam));
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
	wc.lpfnWndProc = track_wndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = Instance;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = 0;
	wc.lpszClassName = "MOUSETRACK";
	if (!RegisterClass( &wc ))
	{
		MessageBox( NULL, "Failed to register class", "Error", MB_OK );
		return 0;
	}

	hwnd = CreateWindow("MOUSETRACK", "MouseTrack", WS_VISIBLE|WS_POPUP|WS_DLGFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 300,
		NULL, NULL, Instance, NULL);
	if (!hwnd)
	{
		MessageBox( NULL, "Failed to create window", "Error", MB_OK );
		return 0;
	}

	dprintf("track: Before GetMessage");
	while (GetMessage( &msg, 0, 0, 0 ))
	{
		dprintf("track: msg %08lx %p %08x %08lx", msg.message, msg.hwnd, msg.wParam, msg.lParam);
		DispatchMessage( &msg );
		dprintf("track: dispatched message");
	}

	return 0;
}

// this is required when we're replacing winlogon
void init_window_station( void )
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hwsta, hdesk;

	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;

	hwsta = CreateWindowStationW( L"winsta0", 0, MAXIMUM_ALLOWED, &sa );
	SetProcessWindowStation( hwsta );
	hdesk = CreateDesktopW( L"Winlogon", 0, 0, 0, MAXIMUM_ALLOWED, &sa );
	SetThreadDesktop( hdesk );
}

int APIENTRY WinMain( HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show )
{
	// running as winlogon.exe?
	HWINSTA hwsta = GetProcessWindowStation();
	if (!hwsta)
		init_window_station();

	// running as normal windows program
	return window_winmain( Instance );
}

