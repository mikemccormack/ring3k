#include <windows.h>

#define DEFAULT_WIDTH 11
#define DEFAULT_HEIGHT 20

#define BLACK 0
#define RED   1
#define GREEN 2
#define BLUE  3

ULONG board[DEFAULT_WIDTH * DEFAULT_HEIGHT];

int board_width = DEFAULT_WIDTH;
int board_height = DEFAULT_HEIGHT;
int block_size = 20;
int interval = 1000;
int piece_orientation = 0;
int piece_type = 0;
int piece_color = RED;
int piece_x = 0;
int piece_y = 0;

HBRUSH brushes[4];
HPEN null_pen;

const char piece[] = {
// straight
	"     "
	"  X  "
	"  X  "
	"  X  "
	"  X  "

	"     "
	"     "
	" XXXX"
	"     "
	"     "

	"  X  "
	"  X  "
	"  X  "
	"  X  "
	"     "

	"     "
	"     "
	"XXXX "
	"     "
	"     "
// bent left
	"     "
	" XX  "
	"  X  "
	"  X  "
	"     "

	"     "
	"     "
	" XXX "
	" X   "
	"     "

	"     "
	"  X  "
	"  X  "
	"  XX "
	"     "

	"     "
	"   X "
	" XXX "
	"     "
	"     "
// bent right
	"     "
	"  XX "
	"  X  "
	"  X  "
	"     "

	"     "
	" X   "
	" XXX "
	"     "
	"     "

	"     "
	"  X  "
	"  X  "
	" XX  "
	"     "

	"     "
	"     "
	" XXX "
	"   X "
	"     "
// T shaped
	"     "
	"     "
	" XXX "
	"  X  "
	"     "

	"     "
	"  X  "
	"  XX "
	"  X  "
	"     "

	"     "
	"  X  "
	" XXX "
	"     "
	"     "

	"     "
	"  X  "
	" XX  "
	"  X  "
	"     "
// dogleg left
	"     "
	"     "
	"  XX "
	" XX  "
	"     "

	"     "
	"  X  "
	"  XX "
	"   X "
	"     "

	"     "
	"  XX "
	" XX  "
	"     "
	"     "

	"     "
	" X   "
	" XX  "
	"  X  "
	"     "

// dogleg right
	"     "
	"     "
	" XX  "
	"  XX "
	"     "

	"     "
	"   X "
	"  XX "
	"  X  "
	"     "

	"     "
	" XX  "
	"  XX "
	"     "
	"     "

	"     "
	"  X  "
	" XX  "
	" X   "
	"     "
};

#undef max
#undef min

int max( int x, int y )
{
	return x<y?y:x;
}

int min( int x, int y )
{
	return x>y?y:x;
}

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

void do_paint( HWND hwnd )
{
	PAINTSTRUCT ps;
	HDC hdc;
	HBRUSH old_brush;
	HPEN old_pen;
	int i, j;

	hdc = BeginPaint( hwnd, &ps );

	old_brush = SelectObject( hdc, brushes[0] );
	old_pen = SelectObject( hdc, null_pen );
	for (i=0; i<board_width; i++)
	{
		for (j=0; j<board_height; j++)
		{
			draw_block( hdc, i, j );
		}
	}
	SelectObject( hdc, old_brush );
	SelectObject( hdc, old_pen );

	EndPaint( hwnd, &ps );
}

void set_block( int x, int y, int color )
{
	ULONG* ptr;
	ptr = get_block_ptr( x, y );
	if (ptr)
		*ptr = color;
}

BOOL piece_has_block( int type, int orientation, int x, int y )
{
	return piece[ piece_type*100 + orientation*25 + y*5 + x] == 'X';
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
	int new_orientation = piece_orientation + 1;
	if (new_orientation >= 4)
		new_orientation = 0;
	block_at_cursor( piece_type, piece_orientation, BLACK );
	if (block_fits_at(piece_type, new_orientation, piece_x, piece_y ))
		piece_orientation = new_orientation;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return piece_orientation == new_orientation;
}

BOOL new_block()
{
	piece_type += 1;
	if (piece_type > 5)
		piece_type = 0;
	piece_x = board_width/2 - 2;
	piece_y = 1;
	block_at_cursor( piece_type, piece_orientation, piece_color );
	return TRUE;
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

BOOL do_keydown( ULONG vkey )
{
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
		drop_down();
		erase_rows();
		new_block();
		return TRUE;
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

void do_timer()
{
	if (move_down())
		return;
	erase_rows();
	new_block();
}

LRESULT CALLBACK minitris_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_SIZE:
		do_size( hwnd, block_size * board_width, block_size * board_height );
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

int APIENTRY WinMain( HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show )
{
	WNDCLASS wc;
	HWND hwnd;
	MSG msg;

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
		CW_USEDEFAULT, CW_USEDEFAULT, block_size * board_width, block_size * board_height,
		NULL, NULL, Instance, NULL);
	if (!hwnd)
	{
		MessageBox( NULL, "Failed to create window", "Error", MB_OK );
		return 0;
	}

	null_pen = GetStockObject( NULL_PEN );
	brushes[0] = GetStockObject( BLACK_BRUSH );
	brushes[1] = CreateSolidBrush( RGB( 0x80, 0, 0 ) );
	brushes[2] = CreateSolidBrush( RGB( 0, 0x80, 0 ) );
	brushes[3] = CreateSolidBrush( RGB( 0, 0, 0x80 ) );

	new_block();
	SetTimer( hwnd, 0, interval, 0 );
	while (GetMessage( &msg, 0, 0, 0 ))
	{
		DispatchMessage( &msg );
	}

	DeleteObject( brushes[1] );
	DeleteObject( brushes[2] );
	DeleteObject( brushes[3] );

	return 0;
}
