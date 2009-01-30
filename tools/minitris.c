/*
 * A simple game for testing
 *
 * Copyright 2009 Thomas Horsten
 * Copyright 2009 Mike McCormack
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
#include <time.h>

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 20
#define DEFAULT_BLOCKSIZE 20
#define PREVIEW_BLOCKSIZE 7

#define BLACK 0
#define CYAN 1

ULONG board[DEFAULT_WIDTH * DEFAULT_HEIGHT];

int board_width = DEFAULT_WIDTH;
int board_height = DEFAULT_HEIGHT;
int block_size = DEFAULT_BLOCKSIZE;
int interval = 1000;
int piece_orientation = 0;
int next_piece_type = 0;
int piece_type = 0;
int piece_color = CYAN;
int piece_x = 0;
int piece_y = 0;

BOOL show_next_piece = TRUE;
int preview_pos_x = (DEFAULT_WIDTH * DEFAULT_BLOCKSIZE)-(5*PREVIEW_BLOCKSIZE)-1;
int preview_pos_y = 1;

BOOL game_over = FALSE;

HBRUSH brushes[8];
HPEN null_pen;

const unsigned int piece[] = {
// The pieces are stored in specially formatted 32-bit integers.

// The top 8 bits specify the x and y size of the piece (e.g. 0x41 for
// a 4*1 piece).  The lower (xsize*ysize) bits are a bitmap of the
// piece.  This is used instead of a fixed size e.g. 4x4, so that
// rotation can be done correctly relating to the piece size and
// prevents "wobbling" when rotating the pieces. When displaying,
// every piece is mapped (and centered) to a 5x5 area.

        // straight
	0x41000000 | 0x0f,
        // bent left
	0x23000000 | (0x3 << 4) | (0x1 << 2) | 0x1,
        // bent right
	0x23000000 | (0x3 << 4) | (0x2 << 2) | 0x2,
        // T shaped
	0x32000000 | (0x2 << 3) | 0x7,
        // dogleg left
	0x32000000 | (0x3 << 3) | 0x6,
        // dogleg right
	0x32000000 | (0x6 << 3) | 0x3,
        // square
	0x2200000f,
};

ULONG *get_block_ptr( int x, int y )
{
	if (x < 0 || x >= board_width)
		return NULL;
	if (y < 0 || y >= board_height)
		return NULL;
	return &board[y*board_width + x];
}

void draw_block( HDC hdc, int x, int y )
{
	ULONG *ptr = get_block_ptr( x, y );
	if (!ptr)
		return;
	SelectObject( hdc, brushes[ *ptr ] );
	Rectangle( hdc, x*block_size, y*block_size,
		 (x+1)*block_size - 1, (y+1)*block_size - 1 );
}

BOOL piece_has_block( int type, int orientation, int x, int y )
{
	int size_x = (piece[type] & 0xf0000000) >> 28;
	int size_y = (piece[type] & 0x0f000000) >> 24;
	int new_x, new_y;
	BOOL has_block = FALSE;

	// Center in 5x5 area
	x = (x - (2-(((orientation & 1)?size_y:size_x)>>1)));
	y = (y - (2-(((orientation & 1)?size_x:size_y)>>1)));
	if ((x<0)||(y<0))
		return FALSE;

	if ((orientation & 1) == 0) {
		new_x = x;
		new_y = y;
	} else {
		new_x = size_x-y-1;
		new_y = x;
	}
	if ((orientation & 2) != 0) {
		new_x = size_x-1-new_x;
		new_y = size_y-1-new_y;
	}

	if ((new_x < 0) || (new_x >= size_x) || (new_y < 0) || (new_y >= size_y))
		has_block = FALSE;
	else
		has_block = ((piece[type] & ((1<<(size_x*size_y-1)) >> ((new_x + size_x*new_y)))) != 0);
	return has_block;
}

void draw_next_piece_preview(HDC hdc)
{
	int x, y;

	SelectObject( hdc, brushes[0] );

	for (x=0; x<5; x++)
		for (y=0; y<5; y++)
		{
			if (piece_has_block(next_piece_type, 0, x, y))
			{
				SelectObject( hdc, brushes[next_piece_type+1] );
			} else {
				SelectObject( hdc, brushes[0] );
			}
			Rectangle( hdc, preview_pos_x+(x*PREVIEW_BLOCKSIZE), preview_pos_y+y*PREVIEW_BLOCKSIZE,
				   preview_pos_x+((x+1)*PREVIEW_BLOCKSIZE), preview_pos_y+(y+1)*PREVIEW_BLOCKSIZE );
		}
}

void paint_board( HDC hdc )
{
	HBRUSH old_brush;
	HPEN old_pen;
	int i, j;

	old_brush = SelectObject( hdc, brushes[0] );
	old_pen = SelectObject( hdc, null_pen );

	for (i=0; i<board_width; i++)
	{
		for (j=0; j<board_height; j++)
		{
			draw_block( hdc, i, j );
		}
	}

	// draw shadow under current block
	SelectObject( hdc, brushes[0] );
	Rectangle( hdc, 1, board_height*block_size,
		   board_width*block_size-1, board_height*block_size+5 );
	SelectObject( hdc, brushes[piece_color] );
	for (i=0; i<4; i++)
	{
		for (j=0; j<5; j++) {
			if (piece_has_block(piece_type, piece_orientation,
					    i, j))
				break;
		}
		if (j<5) {
			Rectangle( hdc, (i+piece_x)*block_size, board_height*block_size,
				   (i+piece_x+1)*block_size - 1, board_height*block_size+5 );
		}
	}

	if (show_next_piece)
		draw_next_piece_preview(hdc);

	SelectObject( hdc, old_brush );
	SelectObject( hdc, old_pen );
}

void set_block( int x, int y, int color )
{
	ULONG* ptr;
	ptr = get_block_ptr( x, y );
	if (ptr)
		*ptr = color;
}

void block_at_cursor( int type, int orientation, int color )
{
	int i, j;
	for (i=0; i<5; i++)
	{
		for (j=0; j<5; j++)
		{
			if (!piece_has_block( type, orientation, i, j ))
				continue;
			set_block( piece_x + i, piece_y + j, color );
		}
	}
}

BOOL block_fits_at( int type, int orientation, int x, int y )
{
	int i, j;
	for (i=0; i<5; i++)
	{
		for (j=0; j<5; j++)
		{
			ULONG *ptr;
			if (!piece_has_block( type, orientation, i, j ))
				continue;
			ptr = get_block_ptr( x + i, y + j );
			if (!ptr)
				return FALSE;
			if (*ptr != BLACK)
				return FALSE;
		}
	}
	return TRUE;
}

BOOL move_to( int x, int y )
{
	if (!block_fits_at(piece_type, piece_orientation, x, y ))
		return FALSE;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	piece_x = x;
	piece_y = y;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return TRUE;
}

BOOL move_down()
{
	int new_y = piece_y + 1;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	if (block_fits_at(piece_type, piece_orientation, piece_x, new_y ))
		piece_y = new_y;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return (piece_y == new_y);
}

BOOL drop_down()
{
	int rows_dropped = 0;
	while (move_down())
		rows_dropped++;
	return rows_dropped;
}

BOOL move_left()
{
	int new_x = piece_x - 1;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	if (block_fits_at(piece_type, piece_orientation, new_x, piece_y ))
		piece_x = new_x;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return piece_x == new_x;
}

BOOL move_right()
{
	int new_x = piece_x + 1;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	if (block_fits_at(piece_type, piece_orientation, new_x, piece_y ))
		piece_x = new_x;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return piece_x == new_x;
}

BOOL do_rotate()
{
	int new_orientation = (piece_orientation + 1) & 3;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	if (block_fits_at(piece_type, new_orientation, piece_x, piece_y ))
		piece_orientation = new_orientation;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return piece_orientation == new_orientation;
}

BYTE get_random_number()
{
	ULONG ticks = GetTickCount();
	static BYTE random_store;
	random_store ^= (ticks >> 16) & 0xff;
	random_store ^= (ticks >> 8) & 0xff;
	random_store ^= ticks&0xff;
	return random_store;
}

BOOL new_block( void )
{
	int new_x, new_y, new_orientation;

	/* check if we can place a new block */
	new_x = board_width/2 - 2;
	new_y = 0;
	new_orientation = 0;
	if ( !block_fits_at( next_piece_type, new_orientation, new_x, new_y ) )
	{
		game_over = TRUE;
		return FALSE;
	}

	/* place the new piece */
	piece_type = next_piece_type;
	piece_color = piece_type+1;
	piece_orientation = new_orientation;
	piece_x = new_x;
	piece_y = new_y;

	/* calculate the next piece */
	next_piece_type = get_random_number()%7;

	block_at_cursor( piece_type, piece_orientation, piece_color );

	return TRUE;
}

void clear_board()
{
	int i, j;
	for (i=0; i<board_width; i++)
	{
		for (j=0; j<board_height; j++)
		{
			ULONG *ptr = get_block_ptr( i, j );
			*ptr = BLACK;
		}
	}
}

BOOL row_full( int row )
{
	int i;
	for (i=0; i<board_width; i++)
	{
		ULONG *ptr = get_block_ptr( i, row );
		if (*ptr == BLACK)
			return FALSE;
	}
	return TRUE;
}

void move_row( int to, int from )
{
	ULONG *from_ptr, *to_ptr;
	int i;

	if (from == to)
		return;

	for (i=0; i<board_width; i++)
	{
		from_ptr = get_block_ptr( i, from );
		to_ptr = get_block_ptr( i, to );
		if (!to_ptr)
			continue;
		if (from_ptr)
			*to_ptr = *from_ptr;
		else
			*to_ptr = BLACK;
	}
}

ULONG erase_rows( void )
{
	int row = board_height - 1, collapsed_rows = 0;
	while (row >= collapsed_rows)
	{
		if (row_full(row))
			collapsed_rows ++;
		else
			row--;
		move_row( row, row - collapsed_rows );
	}
	return collapsed_rows;
}

BOOL do_space( void )
{
	drop_down();
	erase_rows();
	new_block();
	return TRUE;
}

BOOL do_keydown( ULONG vkey )
{
	if (game_over && vkey != VK_ESCAPE)
		return FALSE;

	switch (vkey)
	{
	case VK_ESCAPE:
		PostQuitMessage(0);
		break;
	case VK_DOWN:
		return move_down();
	case VK_UP:
		return do_rotate();
	case VK_LEFT:
		return move_left();
	case VK_RIGHT:
		return move_right();
	case VK_SPACE:
		return do_space();
	}
	return FALSE;
}

void do_size( HWND hwnd, int width, int height )
{
	RECT rcClient, rcWindow;
	POINT ptDiff;

	GetClientRect(hwnd, &rcClient);
	GetWindowRect(hwnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow( hwnd, rcWindow.left, rcWindow.top, width + ptDiff.x, height + ptDiff.y, TRUE );
}

void do_timer( void )
{
	if (game_over)
		return;

	if (move_down())
		return;
	erase_rows();
	new_block();
}

void do_paint( HWND hwnd )
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint( hwnd, &ps );
	paint_board( hdc );
	EndPaint( hwnd, &ps );
}

LRESULT CALLBACK minitris_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	char str[80];
	sprintf(str, "minitris_wndproc %p %08x %08x %08lx", hwnd, msg, wparam, lparam);
	OutputDebugString( str );
	switch (msg)
	{
	case WM_SIZE:
		do_size( hwnd, block_size * board_width, block_size * board_height + 5 );
		break;
	case WM_KEYDOWN:
		if (do_keydown(wparam))
			InvalidateRect( hwnd, 0, 0 );
		break;
	case WM_PAINT:
		do_paint( hwnd );
		break;
	case WM_NCHITTEST:
		return HTCAPTION;
	case WM_TIMER:
		do_timer();
		InvalidateRect( hwnd, 0, 0 );
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void init_brushes( void )
{
	null_pen = GetStockObject( NULL_PEN );
	brushes[0] = GetStockObject( BLACK_BRUSH );
	brushes[1] = CreateSolidBrush( RGB( 0x00, 0xf0, 0xf0 ) );
	brushes[2] = CreateSolidBrush( RGB( 0xf0, 0xa0, 0x00 ) );
	brushes[3] = CreateSolidBrush( RGB( 0x00, 0x00, 0xf0 ) );
	brushes[4] = CreateSolidBrush( RGB( 0xa0, 0x00, 0xf0 ) );
	brushes[5] = CreateSolidBrush( RGB( 0x00, 0xf0, 0x00 ) );
	brushes[6] = CreateSolidBrush( RGB( 0xf0, 0x00, 0x00 ) );
	brushes[7] = CreateSolidBrush( RGB( 0xf0, 0xf0, 0x00 ) );
}

// normal windows program
int window_winmain( HINSTANCE Instance )
{
	WNDCLASS wc;
	MSG msg;
	HWND hwnd;

	wc.style = 0;
	wc.lpfnWndProc = minitris_wndproc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = Instance;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = 0;
	wc.lpszClassName = "MINITRIS";
	if (!RegisterClass( &wc ))
	{
		MessageBox( NULL, "Failed to register class", "Error", MB_OK );
		return 0;
	}

	hwnd = CreateWindow("MINITRIS", "Minitris", WS_VISIBLE|WS_POPUP|WS_DLGFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, block_size * board_width, block_size * board_height + 5,
		NULL, NULL, Instance, NULL);
	if (!hwnd)
	{
		MessageBox( NULL, "Failed to create window", "Error", MB_OK );
		return 0;
	}

	init_brushes();
	new_block();
	SetTimer( hwnd, 0, interval, 0 );
	PostMessage( hwnd, WM_NULL, 0, 0 );
	OutputDebugString("Before GetMessage");
	while (GetMessage( &msg, 0, 0, 0 ))
	{
		OutputDebugString("dispatching message");
		DispatchMessage( &msg );
		OutputDebugString("dispatched message");
	}

	DeleteObject( brushes[1] );
	DeleteObject( brushes[2] );
	DeleteObject( brushes[3] );

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

ULONG key_states[0x100];

// debounce...
BOOL check_pressed( ULONG key )
{
	BOOL ret;
	ULONG val = GetAsyncKeyState( key );
	ret = ((val & 0x8000) && !(key_states[key] & 0x8000));
	key_states[key] = val;
	return ret;
}

// polled input, as synchronous input is not supported as yet :(
int polled_winmain( void )
{
	BOOL repaint, restart = TRUE;
	ULONG count = 0;

	init_brushes();

	while (1)
	{
		if (restart)
		{
			clear_board();
			new_block();
			restart = 0;
			game_over = FALSE;
		}

		repaint = 0;
		if (check_pressed( VK_ESCAPE ))
		{
			restart = 1;
			continue;
		}
		if (check_pressed( VK_UP ))
			repaint |= do_rotate();
		if (check_pressed( VK_DOWN ))
			repaint |= move_down();
		if (check_pressed( VK_LEFT ))
			repaint |= move_left();
		if (check_pressed( VK_RIGHT ))
			repaint |= move_right();
		if (check_pressed( VK_SPACE ))
			repaint |= do_space();

		count++;
		if (count>10)
		{
			do_timer();
			repaint++;
			count = 0;
		}
		if (repaint)
		{
			HDC hdc = GetDC( 0 );
			paint_board( hdc );
			ReleaseDC( 0, hdc );
		}
		Sleep( 50 );
	}
	return 0;
}

//#define FORCE_MESSAGE_LOOP

int APIENTRY WinMain( HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show )
{
	// running as winlogon.exe?
	HWINSTA hwsta = GetProcessWindowStation();
	if (!hwsta)
		init_window_station();

#ifndef FORCE_MESSAGE_LOOP
	// a bit of a hack...
	// the message loop doesn't yet work in ring3k... eew...
	if (!hwsta)
		return polled_winmain();
#endif

	// running as normal windows program
	return window_winmain( Instance );
}

