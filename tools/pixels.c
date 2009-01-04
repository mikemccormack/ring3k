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

void draw_checkerboard( int x_pos, int y_pos )
{
	HDC hdc = GetDC( 0 );
	int i, j, x, y;
	HBRUSH black, white, old;
	const int side_count = 8;
	const int square_size = 10;

	black = GetStockObject( BLACK_BRUSH );
	white = GetStockObject( WHITE_BRUSH );

	old = SelectObject( hdc, black );

	y = y_pos;
	for (i=0; i<side_count; i++)
	{
		x = x_pos;
		for (j=0; j<side_count; j++)
		{
			SelectObject( hdc, (i ^ j) & 1 ? black : white );
			Rectangle( hdc, x, y, x + square_size, y + square_size );
			x += square_size;
		}
		y += square_size;
	}
	SelectObject( hdc, old );
}

void draw_text( void )
{
	HDC hdc = GetDC( 0 );

	TextOutW(hdc, 100, 100, L"Hello world", 11);
	ReleaseDC( 0, hdc );
}

const char number_data[] =
	"        "
	"  XXXX  "
	" X    X "
	" X    X "
	" X    X "
	" X    X "
	"  XXXX  "
	"        "

	"        "
	"   XX   "
	"  X X   "
	"    X   "
	"    X   "
	"    X   "
	" XXXXXX "
	"        "

	"        "
	"  XXXX  "
	" X    X "
	"     X  "
	"    X   "
	"   X    "
	" XXXXXX "
	"        "

	"        "
	"  XXXX  "
	" X    X "
	"    XX  "
	"      X "
	" X    X "
	"  XXXX  "
	"        "

	"        "
	"    X   "
	"   XX   "
	"  X X   "
	" XXXXXX "
	"    X   "
	"    X   "
	"        "

	"        "
	" XXXXXX "
	" X      "
	" XXXXX  "
	"      X "
	" X    X "
	"  XXXX  "
	"        "

	"        "
	"  XXXX  "
	" X    X "
	" X      "
	" XXXXX  "
	" X    X "
	"  XXXX  "
	"        "

	"        "
	" XXXXXX "
	"      X "
	"      X "
	"     X  "
	"    X   "
	"   X    "
	"        "

	"        "
	"  XXXX  "
	" X    X "
	"  XXXX  "
	" X    X "
	" X    X "
	"  XXXX  "
	"        "

	"        "
	"  XXXX  "
	" X    X "
	"  XXXXX "
	"      X "
	" X    X "
	"  XXXX  "
	"        "
;

HBITMAP bitmaps[9];
const int bm_width = 16;

// create a bitmap for a number from the string above
void string_to_bits( unsigned char *bits, const char *number_data )
{
	int i, j;
	for (i=0; i<8; i++)
	{
		int val = 0;
		for (j=0; j<8; j++)
		{
			val <<= 2;
			if (number_data[i*8 + j] == 'X')
				val |= 3;
		}
		bits[i*4+1] = val&0xff;
		bits[i*4] = (val>>8)&0xff;
		bits[i*4+2] = bits[i*4];
		bits[i*4+3] = bits[i*4 + 1];
	}
}

int use_bitmaps = 1;
int show_seconds = 0;

unsigned char bitmap_bits[8*4];

void setup_bitmaps( void )
{
	int i;

	if (!use_bitmaps)
		return;

	for (i=0; i<10; i++)
	{
		string_to_bits( bitmap_bits, &number_data[i*64] );
		bitmaps[i] = CreateBitmap(bm_width, bm_width, 1, 1, bitmap_bits);
	}
}

void draw_number( HDC hdc, int x_pos, int y_pos, int num )
{
	if (use_bitmaps)
	{
		HDC compat = CreateCompatibleDC( hdc );
		HBITMAP old = SelectObject( compat, bitmaps[num%10] );
		BitBlt( hdc, x_pos, y_pos, bm_width, bm_width, compat, 0, 0, SRCCOPY );
		SelectObject( compat, old );
	}
	else
	{
		WCHAR ch = (num%10) + '0';
		TextOutW(hdc, x_pos, y_pos, &ch, 1);
	}
}

void draw_time( int x_pos, int y_pos, SYSTEMTIME *st )
{
	HDC hdc = GetDC( 0 );

	// draw the year
	draw_number( hdc, x_pos, y_pos, st->wYear/1000 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wYear/100 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wYear/10 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wYear );
	x_pos += bm_width*2;

	// draw the month
	draw_number( hdc, x_pos, y_pos, st->wMonth/10 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wMonth );
	x_pos += bm_width*2;

	// draw the day of the month
	draw_number( hdc, x_pos, y_pos, st->wDay/10 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wDay );
	x_pos += bm_width*2;

	// hour
	draw_number( hdc, x_pos, y_pos, st->wHour/10 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wHour );
	x_pos += bm_width*2;

	// minute
	draw_number( hdc, x_pos, y_pos, st->wMinute/10 );
	x_pos += bm_width;
	draw_number( hdc, x_pos, y_pos, st->wMinute );
	x_pos += bm_width*2;

	if (show_seconds)
	{
		// second
		draw_number( hdc, x_pos, y_pos, st->wSecond/10 );
		x_pos += bm_width;
		draw_number( hdc, x_pos, y_pos, st->wSecond );
		x_pos += bm_width*2;
	}

	// done
	ReleaseDC( 0, hdc );
}

BOOL time_changed( SYSTEMTIME *st, WORD* last )
{
	WORD val = show_seconds ? st->wSecond : st->wMinute;
	BOOL ret = (*last != val);
	*last = val;
	return ret;
}

void number_test( void )
{
	int i;
	HDC hdc = GetDC( 0 );
	for (i=0; i<10; i++)
		draw_number( hdc, 20*i, 150, i);
	ReleaseDC( 0, hdc );
}

// draw a digital clock, and update it
// never returns
void do_clock( void )
{
	WORD last = - 1;
	SYSTEMTIME st;

	setup_bitmaps();
	number_test();
	GetSystemTime( &st );
	while (1)
	{
		if (time_changed( &st, &last ))
			draw_time( 0, 200, &st );

		Sleep(100);
		GetSystemTime( &st );
	}
}

int main( int argc, char **argv )
{
	init_window_station();

	draw_pixels();
	draw_rectangle();
	draw_checkerboard( 0, 50 );
	//draw_text();

	do_clock();

	Sleep(100*1000);
	return 0;
}
