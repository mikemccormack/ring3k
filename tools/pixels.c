/*
 * Winlogon replacement - draws a grey square of 10x10 pixels to the screen
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

void draw_pixels( void )
{
	HDC hdc = GetDC( 0 );
	int i, j, ok = TRUE;

	for (i=0; ok && i<0x10; i++)
		for (j=0; ok && j<0x10; j++)
			ok = SetPixel( hdc, i, j, RGB(128, 128, 128));

	ReleaseDC( 0, hdc );
}

void draw_rectangle( void )
{
	HDC hdc = GetDC( 0 );

	Rectangle( hdc, 0x10, 0x10, 0x20, 0x20 );
	ReleaseDC( 0, hdc );
}

void draw_text( void )
{
	HDC hdc = GetDC( 0 );

	TextOutW(hdc, 100, 100, L"Hello world", 11);
	ReleaseDC( 0, hdc );
}

int main( int argc, char **argv )
{
	init_window_station();

	draw_pixels();
	draw_rectangle();
	draw_text();

	Sleep(100*1000);
	return 0;
}
