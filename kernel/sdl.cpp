/*
 * nt loader
 *
 * Copyright 2006-2009 Mike McCormack
 *
 * Portions based upon Wine DIB engine implementation by:
 *
 *  Copyright 2007 Jesse Allen
 *  Copyright 2008 Massimo Del Fedele
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

// the freetype project certainly has their own way of doing things :/
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#if defined (HAVE_SDL) && defined (HAVE_SDL_SDL_H)
#include <SDL/SDL.h>

class sdl_16bpp_bitmap_t : public bitmap_impl_t<16>
{
	SDL_Surface *surface;
public:
	sdl_16bpp_bitmap_t( SDL_Surface *s );
	void lock();
	void unlock();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual COLORREF get_pixel( INT x, INT y );
	virtual BOOL bitblt( INT xDest, INT yDest, INT cx, INT cy,
		 bitmap_t *src, INT xSrc, INT ySrc, ULONG rop );
	virtual BOOL rectangle( INT x, INT y, INT width, INT height, brush_t* brush );
	virtual BOOL line( INT x1, INT y1, INT x2, INT y2, pen_t *pen );
protected:
	virtual ULONG map_colorref( COLORREF color );
};

class sdl_device_context_t : public device_context_t
{
public:
	bitmap_t *sdl_bitmap;
	window_tt *win;
public:
	sdl_device_context_t( bitmap_t *b );
	virtual bitmap_t* get_bitmap();
	virtual HANDLE select_bitmap( bitmap_t *bitmap );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual int getcaps( int index );
protected:
	void freetype_bitblt( int x, int y, FT_Bitmap* bitmap );
};

class sdl_sleeper_t : public sleeper_t
{
	win32k_manager_t *manager;
public:
	sdl_sleeper_t( win32k_manager_t* mgr );
	virtual bool check_events( bool wait );
	static Uint32 timeout_callback( Uint32 interval, void *arg );
	bool handle_sdl_event( SDL_Event& event );
	WORD sdl_keysum_to_vkey( SDLKey sym );
	ULONG get_mouse_button( Uint8 button, bool up );
};

class win32k_sdl_t : public win32k_manager_t
{
protected:
	SDL_Surface *screen;
	FT_Library ftlib;
	FT_Face face;
	sdl_sleeper_t sdl_sleeper;
	bitmap_t* sdl_bitmap;
public:
	virtual BOOL init();
	virtual void fini();
	win32k_sdl_t();
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual device_context_t* alloc_screen_dc_ptr();

protected:
	Uint16 map_colorref( COLORREF );
	virtual SDL_Surface* set_mode() = 0;
	virtual int getcaps( int index );
	void freetype_bitblt( int x, int y, FT_Bitmap* bitmap );
	COLORREF freetype_get_pixel( int x, int y, FT_Bitmap* bitmap );
};

win32k_sdl_t::win32k_sdl_t() :
	sdl_sleeper( this )
{
}

BOOL sdl_16bpp_bitmap_t::set_pixel( INT x, INT y, COLORREF color )
{
	BOOL r;
	lock();
	r = bitmap_t::set_pixel( x, y, color );
	SDL_UpdateRect(surface, x, y, 1, 1);
	unlock();
	return r;
}

COLORREF sdl_16bpp_bitmap_t::get_pixel( INT x, INT y )
{
	BOOL r;
	lock();
	r = bitmap_t::get_pixel(x, y);
	unlock();
	return r;
}

BOOL sdl_16bpp_bitmap_t::rectangle(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	dprintf("sdl_16bpp_bitmap_t::rectangle\n");
	BOOL r;
	lock();
	r = bitmap_t::rectangle( left, top, right, bottom, brush );
	unlock();
	SDL_UpdateRect( surface, left, top, right - left, bottom - top );
	return TRUE;
}

COLORREF win32k_sdl_t::freetype_get_pixel( int x, int y, FT_Bitmap* bitmap )
{
	int bytes_per_row;
	int val;
	switch (bitmap->pixel_mode)
	{
	case FT_PIXEL_MODE_MONO:
		bytes_per_row = bitmap->pitch;
		val = (bitmap->buffer[bytes_per_row*y + (x>>3)] << (x&7)) & 0x80;
		return val ? RGB( 255, 255, 255 ) : RGB( 0, 0, 0 );
	default:
		dprintf("unknown freetype pixel mode %d\n", bitmap->pixel_mode);
		return 0;
	}
}

void win32k_sdl_t::freetype_bitblt( int x, int y, FT_Bitmap* bitmap )
{
	INT bmpX, bmpY;
	BYTE *buf;
	INT j, i;

	dprintf("glyph is %dx%d\n", bitmap->rows, bitmap->width);
	dprintf("pixel mode is %d\n", bitmap->pixel_mode );
	dprintf("destination is %d,%d\n", x, y );
	dprintf("pitch is %d\n", bitmap->pitch );

	/* loop for every pixel in bitmap */
	buf = bitmap->buffer;
	for (bmpY = 0, i = y; bmpY < bitmap->rows; bmpY++, i++)
	{
		for (bmpX = 0, j = x; bmpX < bitmap->width; bmpX++, j++)
		{
			// FIXME: assumes text color is black
			COLORREF color = freetype_get_pixel( bmpX, bmpY, bitmap );
			sdl_bitmap->set_pixel( j, i, color );
		}
	}

}

static char vgasys[] = "drive/winnt/system32/vgasys.fon";

BOOL win32k_sdl_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	dprintf("text: %pus\n", &text );

	FT_Open_Args args;
	memset( &args, 0, sizeof args );
	args.flags = FT_OPEN_PATHNAME;
	args.pathname = vgasys;

	FT_Error r = FT_Open_Face( ftlib, &args, 0, &face );
	if (r)
		return FALSE;

	if ( SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 )
		return FALSE;

	int dx = 0, dy = 0;
	for (int i=0; i<text.Length/2; i++)
	{
		WCHAR ch = text.Buffer[i];
		FT_UInt glyph_index = FT_Get_Char_Index( face, ch );

		r = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if (r)
			continue;

		FT_Glyph glyph;
		r = FT_Get_Glyph( face->glyph, &glyph );
		if (r)
			continue;

		if (glyph->format != FT_GLYPH_FORMAT_BITMAP )
			continue;

		FT_BitmapGlyph bitmap = (FT_BitmapGlyph) glyph;
		freetype_bitblt( x+dx+bitmap->left, y+dy+bitmap->top, &bitmap->bitmap );

		dx += bitmap->bitmap.width;
		dy += 0;

		FT_Done_Glyph( glyph );
	}

	if ( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, x, y, x+dx, y+dy);

	return TRUE;
}

BOOL sdl_16bpp_bitmap_t::bitblt( INT xDest, INT yDest, INT cx, INT cy, bitmap_t *src, INT xSrc, INT ySrc, ULONG rop )
{
	BOOL r;
	lock();
	assert(cx>=0);
	assert(cy>=0);
	r = bitmap_t::bitblt(xDest, yDest, cx, cy, src, xSrc, ySrc, rop);
	SDL_UpdateRect(surface, xDest, yDest, xDest + cx, yDest + cy);
	unlock();
	return r;
}

BOOL sdl_16bpp_bitmap_t::line( INT x1, INT y1, INT x2, INT y2, pen_t *pen )
{
	BOOL r;
	lock();
	r = bitmap_t::line(x1, y1, x2, y2, pen);
	// FIXME: possible optimization when updating?
	if (x1 > x2)
		swap(x1, x2);
	if (y1 > y2)
		swap(y1, y2);
	SDL_UpdateRect(surface, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	unlock();
	return r;
}

sdl_sleeper_t::sdl_sleeper_t( win32k_manager_t* mgr ) :
	manager( mgr )
{
}

Uint32 sdl_sleeper_t::timeout_callback( Uint32 interval, void *arg )
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = 0;
	event.user.data1 = 0;
	event.user.data2 = 0;
	SDL_PushEvent( &event );
	return 0;
}

WORD sdl_sleeper_t::sdl_keysum_to_vkey( SDLKey sym )
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
	//mk(ESCAPE)
	case SDLK_ESCAPE:
		dprintf("escape!\n");
		return VK_ESCAPE;
#undef mk
	default:
		dprintf("%d unhandled\n", sym);
		return 0;
	}
}

ULONG sdl_sleeper_t::get_mouse_button( Uint8 button, bool up )
{
	switch (button)
	{
	case SDL_BUTTON_LEFT:
		return up ? MOUSEEVENTF_LEFTUP : MOUSEEVENTF_LEFTDOWN;
	case SDL_BUTTON_RIGHT:
		return up ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_RIGHTDOWN;
	case SDL_BUTTON_MIDDLE:
		return up ? MOUSEEVENTF_MIDDLEUP : MOUSEEVENTF_MIDDLEDOWN;
	default:
		dprintf("unknown mouse button %d\n", button );
		return 0;
	}
}

bool sdl_sleeper_t::handle_sdl_event( SDL_Event& event )
{
	INPUT input;

	switch (event.type)
	{
	case SDL_QUIT:
		return true;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		dprintf("got SDL keyboard event\n");
		input.type = INPUT_KEYBOARD;
		input.ki.time = timeout_t::get_tick_count();
		input.ki.wVk = sdl_keysum_to_vkey( event.key.keysym.sym );
		input.ki.wScan = event.key.keysym.scancode;
		input.ki.dwFlags = (event.type == SDL_KEYUP) ? KEYEVENTF_KEYUP : 0;
		input.ki.dwExtraInfo = 0;
		manager->send_input( &input );
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		dprintf("got SDL mouse button event\n");
		input.type = INPUT_MOUSE;
		input.mi.dx = event.button.x;
		input.mi.dy = event.button.y;
		input.mi.mouseData = 0;
		input.mi.dwFlags = get_mouse_button( event.button.button, event.type == SDL_MOUSEBUTTONUP );
		input.mi.time = timeout_t::get_tick_count();
		input.mi.dwExtraInfo = 0;
		manager->send_input( &input );
		break;

	case SDL_MOUSEMOTION:
		dprintf("got SDL mouse motion event\n");
		input.type = INPUT_MOUSE;
		input.mi.dx = event.motion.x;
		input.mi.dy = event.motion.y;
		input.mi.mouseData = 0;
		input.mi.dwFlags = MOUSEEVENTF_MOVE;
		input.mi.time = timeout_t::get_tick_count();
		input.mi.dwExtraInfo = 0;
		manager->send_input( &input );
		break;
	}

	return false;
}

// wait for timers or input
// return true if we're quitting
bool sdl_sleeper_t::check_events( bool wait )
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

	// wait for a timer, if there is one
	SDL_TimerID id = 0;
	Uint32 interval = 0;
	if (timers_left)
	{
		interval = get_int_timeout( timeout );
		id = SDL_AddTimer( interval, sdl_sleeper_t::timeout_callback, 0 );
	}

	if (SDL_WaitEvent( &event ))
	{
		if (event.type == SDL_USEREVENT && event.user.code == 0)
		{
			// timer has expired, no need to cancel it
			id = NULL;
		}
		else
		{
			quit = handle_sdl_event( event );
		}
	}
	else
	{
		dprintf("SDL_WaitEvent returned error\n");
		quit = true;
	}

	if (id != NULL)
		SDL_RemoveTimer( id );
	return quit;
}

int win32k_sdl_t::getcaps( int index )
{
	switch (index)
	{
	case NUMCOLORS:
		return 1 << screen->format->BitsPerPixel;
	case BITSPIXEL:
		return screen->format->BitsPerPixel;
	default:
		dprintf("%d\n", index );
		return 0;
	}
}

BOOL win32k_sdl_t::init()
{
	if ( SDL_WasInit(SDL_INIT_VIDEO) )
		return TRUE;

	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0 )
		return FALSE;

	screen = set_mode();

	FT_Error r = FT_Init_FreeType( &ftlib );
	if (r != 0)
		return FALSE;

	sdl_bitmap = new sdl_16bpp_bitmap_t( screen );

	// FIXME: move this to caller
	brush_t light_blue(0, RGB(0x3b, 0x72, 0xa9), 0);
	sdl_bitmap->rectangle( 0, 0, screen->w, screen->h, &light_blue );

	::sleeper = &sdl_sleeper;

	return TRUE;
}

void win32k_sdl_t::fini()
{
	if ( !SDL_WasInit(SDL_INIT_VIDEO) )
		return;
	FT_Done_FreeType( ftlib );
	SDL_Quit();
}

sdl_16bpp_bitmap_t::sdl_16bpp_bitmap_t( SDL_Surface *s ) :
	bitmap_impl_t<16>( s->w, s->h ),
	surface( s )
{
	bits = reinterpret_cast<unsigned char*>( s->pixels );
}

void sdl_16bpp_bitmap_t::lock()
{
	if ( SDL_MUSTLOCK(surface) )
		SDL_LockSurface(surface);
}

void sdl_16bpp_bitmap_t::unlock()
{
	if ( SDL_MUSTLOCK(surface) )
		SDL_UnlockSurface(surface);
}

ULONG sdl_16bpp_bitmap_t::map_colorref( COLORREF color )
{
	return SDL_MapRGB(surface->format, GetRValue(color), GetGValue(color), GetBValue(color));
}

class win32k_sdl_16bpp_t : public win32k_sdl_t
{
public:
	virtual SDL_Surface* set_mode();
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

win32k_sdl_16bpp_t win32k_manager_sdl_16bpp;

win32k_manager_t* init_sdl_win32k_manager()
{
	return &win32k_manager_sdl_16bpp;
}

sdl_device_context_t::sdl_device_context_t( bitmap_t *b ) :
	sdl_bitmap( b ),
	win( 0 )
{
}

bitmap_t* sdl_device_context_t::get_bitmap()
{
	return sdl_bitmap;
}

HANDLE sdl_device_context_t::select_bitmap( bitmap_t *bitmap )
{
	dprintf("trying to change device's bitmap...\n");
	return 0;
}

BOOL sdl_device_context_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return win32k_manager->exttextout( x, y, options, rect, text );
}

int sdl_device_context_t::getcaps( int index )
{
	return win32k_manager->getcaps( index );
}

device_context_t* win32k_sdl_t::alloc_screen_dc_ptr()
{
	dprintf("allocating SDL DC sdl_bitmap = %p\n", sdl_bitmap);
	return new sdl_device_context_t( sdl_bitmap );
}

#else

win32k_manager_t* init_sdl_win32k_manager()
{
	return NULL;
}

#endif

