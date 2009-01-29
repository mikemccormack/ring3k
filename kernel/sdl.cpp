/*
 * nt loader
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "debug.h"
#include "win32mgr.h"
#include "ntwin32.h"
#include "sdl.h"

#if defined (HAVE_SDL) && defined (HAVE_SDL_SDL_H)
#include <SDL/SDL.h>

class win32k_sdl_t : public win32k_manager_t, public sleeper_t
{
protected:
	SDL_Surface *screen;
public:
	virtual BOOL init();
	virtual void fini();
	win32k_sdl_t();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual BOOL bitblt( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop );

protected:
	Uint16 map_colorref( COLORREF );
	virtual SDL_Surface* set_mode() = 0;
	virtual void set_pixel_l( INT x, INT y, COLORREF color ) = 0;
	virtual void rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush ) = 0;
	virtual void bitblt_l( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop ) = 0;
	virtual bool check_events( bool wait );
	static Uint32 timeout_callback( Uint32 interval, void *arg );
	bool handle_sdl_event( SDL_Event& event );
	WORD sdl_keysum_to_vkey( SDLKey sym );
};

win32k_sdl_t::win32k_sdl_t()
{
}

BOOL win32k_sdl_t::set_pixel( INT x, INT y, COLORREF color )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	set_pixel_l( x, y, color );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect(screen, x, y, 1, 1);

	return TRUE;
}

template<typename T> void swap( T& A, T& B )
{
	T x = A;
	A = B;
	B = x;
}

BOOL win32k_sdl_t::rectangle(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	if (left > right)
		swap( left, right );
	if (top > bottom)
		swap( top, bottom );

	top = max( 0, top );
	left = max( 0, left );
	right = min( screen->w - 1, right );
	bottom = min( screen->h - 1, bottom );

	rectangle_l( left, top, right, bottom, brush );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, left, top, right - left, bottom - top );

	return TRUE;
}

BOOL win32k_sdl_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	dprintf("text: %pus\n", &text );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	//SDL_UpdateRect( screen, left, top, right - left, bottom - top );

	return TRUE;
}

BOOL win32k_sdl_t::bitblt(
	INT xDest, INT yDest,
	INT cx, INT cy,
	device_context_t *src,
	INT xSrc, INT ySrc, ULONG rop )
{
	// keep everything on the screen
	xDest = max( xDest, 0 );
	yDest = max( yDest, 0 );
	if ((xDest + cx) > screen->w)
		cx = screen->w - xDest;
	if ((yDest + cy) > screen->h)
		cy = screen->h - yDest;

	// keep everything on the source bitmap
	bitmap_t *bitmap = src->get_selected_bitmap();
	xSrc = max( xSrc, 0 );
	ySrc = max( ySrc, 0 );
	if ((xSrc + cx) > bitmap->get_width())
		cx = bitmap->get_width() - xSrc;
	if ((ySrc + cy) > bitmap->get_height())
		cy = bitmap->get_height() - ySrc;

	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	bitblt_l( xDest, yDest, cx, cy, src, xSrc, ySrc, rop );

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, xDest, yDest, cx, cy );

	return TRUE;
}

Uint32 win32k_sdl_t::timeout_callback( Uint32 interval, void *arg )
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = 0;
	event.user.data1 = 0;
	event.user.data2 = 0;
	SDL_PushEvent( &event );
	return 0;
}

WORD win32k_sdl_t::sdl_keysum_to_vkey( SDLKey sym )
{
	assert ( SDLK_a == 'a' );
	assert ( SDLK_1 == '1' );
	if ((sym >= 'A' && sym <= 'Z') || (sym >= '0' && sym <= '9'))
		return (WORD) sym;

	switch (sym)
	{
#define mk(k) case SDLK_##k: return VK_##k;
	mk(SPACE)
	mk(UP)
	mk(DOWN)
	mk(LEFT)
	mk(RIGHT)
	mk(ESCAPE)
#undef mk
	default:
		dprintf("%d unhandled\n", sym);
	}
	return (WORD) sym;
}

bool win32k_sdl_t::handle_sdl_event( SDL_Event& event )
{
	INPUT input;
	DWORD flags = 0;
	switch (event.type)
	{
	case SDL_QUIT:
		return true;
	case SDL_KEYUP:
		flags |= KEYEVENTF_KEYUP;
		// fall through
	case SDL_KEYDOWN:
		input.ki.time = timeout_t::get_tick_count();
		input.ki.wVk = sdl_keysum_to_vkey( event.key.keysym.sym );
		input.ki.wScan = event.key.keysym.scancode;
		input.ki.dwFlags = flags;
		input.ki.dwExtraInfo = 0;
		send_input( &input );
		break;
	}

	return false;
}

// wait for timers or input
// return true if we're quitting
bool win32k_sdl_t::check_events( bool wait )
{
	LARGE_INTEGER timeout;
	SDL_Event event;
	bool quit = false;

	bool timers_left = timeout_t::check_timers(timeout);

	// quit if we got an SDL_QUIT
	if (SDL_PollEvent( &event ) && handle_sdl_event( event ))
		return true;

	// Check for a deadlock and quit.
	//  This happens if we're the only active thread,
	//  there's no more timers, nobody listening for input and we're asked to wait.
	if (!timers_left && !active_window && wait && fiber_t::last_fiber())
		return true;

	// only wait if asked to
	if (!wait)
		return false;

	Uint32 interval = get_int_timeout( timeout );
	SDL_TimerID id = SDL_AddTimer( interval, win32k_sdl_t::timeout_callback, 0 );

	while (!quit && SDL_WaitEvent( &event ))
	{
		if (event.type == SDL_USEREVENT && event.user.code == 0)
		{
			// timer has expired, no need to cancel it
			id = NULL;
			break;
		}

		quit = handle_sdl_event( event );
	}

	if (id != NULL)
		SDL_RemoveTimer( id );
	return quit;
}

BOOL win32k_sdl_t::init()
{
	if ( SDL_WasInit(SDL_INIT_VIDEO) )
		return TRUE;

	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 )
		return FALSE;

	screen = set_mode();

	brush_t light_blue(0, RGB(0x3b, 0x72, 0xa9), 0);
	rectangle( 0, 0, screen->w, screen->h, &light_blue );

	sleeper = this;

	return TRUE;
}

void win32k_sdl_t::fini()
{
	if ( !SDL_WasInit(SDL_INIT_VIDEO) )
		return;
	SDL_Quit();
}

class win32k_sdl_16bpp_t : public win32k_sdl_t
{
public:
	virtual SDL_Surface* set_mode();
	virtual void set_pixel_l( INT x, INT y, COLORREF color );
	virtual void rectangle_l( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual void bitblt_l( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop );
	Uint16 map_colorref( COLORREF color );
};

SDL_Surface* win32k_sdl_16bpp_t::set_mode()
{
	return SDL_SetVideoMode( 640, 480, 16, SDL_SWSURFACE );
}

Uint16 win32k_sdl_16bpp_t::map_colorref( COLORREF color )
{
	return SDL_MapRGB(screen->format, GetRValue(color), GetGValue(color), GetBValue(color));
}

void win32k_sdl_16bpp_t::set_pixel_l( INT x, INT y, COLORREF color )
{
	Uint16 *bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
	*bufp = map_colorref( color );
}

void win32k_sdl_16bpp_t::rectangle_l(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	COLORREF brush_val, pen_val;

	// FIXME: use correct pen color
	pen_val = map_colorref( RGB( 0, 0, 0 ) );
	brush_val = map_colorref( brush->get_color() );
	dprintf("brush color = %08lx\n", brush->get_color());

	Uint16 *ptr = (Uint16 *)screen->pixels + top*screen->pitch/2;

	// top line
	for (INT count = left; count <= right; count++)
		ptr[count] = pen_val;
	ptr += screen->pitch/2;
	top++;

	while (top < (bottom -1))
	{
		// left border drawn by pen
		ptr[ left ] = pen_val;

		// filled by brush
		INT count;
		for (count = left+1; count < (right - 1); count++)
			ptr[count] = brush_val;

		// right border drawn by pen
		ptr[ count ] = pen_val;

		//next line
		top++;
		ptr += screen->pitch/2;
	}

	// bottom line
	for (INT count = left; count <= right; count++)
		ptr[count] = pen_val;
}

void win32k_sdl_16bpp_t::bitblt_l(
	INT xDest, INT yDest,
	INT cx, INT cy,
	device_context_t *src,
	INT xSrc, INT ySrc, ULONG rop )
{
	dprintf("%d,%d %dx%d <- %d,%d\n", xDest, yDest, cx, cy, xSrc, ySrc );
	if (rop != SRCCOPY)
		dprintf("ROP %ld not supported\n", rop);

	// copy the pixels
	COLORREF pixel;
	for (int i=0; i<cy; i++)
	{
		for (int j=0; j<cx; j++)
		{
			pixel = src->get_pixel( xSrc+j, ySrc+i );
			set_pixel_l( xDest+j, yDest+i, pixel );
		}
	}
}

win32k_sdl_16bpp_t win32k_manager_sdl_16bpp;

win32k_manager_t* init_sdl_win32k_manager()
{
	return &win32k_manager_sdl_16bpp;
}

#else

win32k_manager_t* init_sdl_win32k_manager()
{
	return NULL;
}

#endif

