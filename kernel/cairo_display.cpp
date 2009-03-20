/*
 * Cairo backend
 *
 * Copyright 2009 Hilary Cheng
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

#include "cairo_display.h"

#ifdef HAVE_CAIRO

#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>

class cairo_device_context_t : public device_context_t
{
public:
	window_tt *win;
public:
	cairo_device_context_t();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT x, INT y, INT width, INT height );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual COLORREF get_pixel( INT x, INT y );
	virtual BOOL polypatblt( ULONG Rop, PRECT rect );
	virtual int getcaps( int index );
};

class win32k_cairo_t : public win32k_manager_t, public sleeper_t
{
public:
	virtual BOOL init();
	virtual void fini();
	win32k_cairo_t();
	virtual BOOL set_pixel( INT x, INT y, COLORREF color );
	virtual BOOL rectangle( INT left, INT top, INT right, INT bottom, brush_t* brush );
	virtual BOOL exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text );
	virtual BOOL bitblt( INT xDest, INT yDest, INT cx, INT cy, device_context_t *src, INT xSrc, INT ySrc, ULONG rop );
	virtual BOOL polypatblt( ULONG Rop, PRECT rect );
	virtual device_context_t* alloc_screen_dc_ptr();

protected:
	virtual bool check_events( bool wait );
	virtual int getcaps( int index );
	void handle_events();

protected:
	Window win;
	Display* disp;
	cairo_surface_t *cs, *buffer;
	cairo_t *cr;
	int screenNumber;
	int width, height;
	unsigned long white, black;
};

template<typename T> void swap( T& A, T& B )
{
	T x = A;
	A = B;
	B = x;
}

win32k_cairo_t::win32k_cairo_t() :
	disp(NULL),
	width(640),
	height(480)
{
}

BOOL win32k_cairo_t::init()
{
    if (disp) return TRUE;

    disp = XOpenDisplay(NULL);
    if (disp == NULL) return FALSE;

    screenNumber = DefaultScreen(disp);
    white = WhitePixel(disp,screenNumber);
    win = XCreateSimpleWindow(disp,
		    RootWindow(disp, screenNumber),
		    0, 0,   // origin
		    width, height, // size
		    0, black, // border
		    white );  // backgd
    XMapWindow(disp, win);
    XStoreName(disp, win, "ring3k");
    XSelectInput(disp, win, ExposureMask | ButtonReleaseMask | ButtonPressMask);
    cs = cairo_xlib_surface_create(disp, win, DefaultVisual(disp, 0), width, height);
    cr = cairo_create(cs);

    buffer = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);

    cairo_t *c = cairo_create(buffer);
    cairo_set_source_rgb(c, 0.231372549, 0.447058824, 0.662745098);
    cairo_rectangle(c, 0, 0, width, height);
    cairo_fill(c);
    cairo_destroy(c);

    sleeper = this;

    return TRUE;
}

void win32k_cairo_t::fini()
{
    cairo_destroy(cr);
    cairo_surface_destroy(cs);
    XDestroyWindow(disp, win);
    XCloseDisplay(disp);
}

void win32k_cairo_t::handle_events() {
    XEvent evt;

    XNextEvent(disp, &evt);

    if (evt.type == ButtonPress) {
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = evt.xbutton.x;
	input.mi.dy = evt.xbutton.y;
	input.mi.mouseData = 0;

        switch (evt.xbutton.button)
        {
        case 1:
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            break;
        case 2:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
            break;
        case 3:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
            break;
        }

	input.mi.time = timeout_t::get_tick_count();
	input.mi.dwExtraInfo = 0;
	send_input( &input );
    }

    if (evt.type == ButtonRelease) {
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = evt.xbutton.x;
	input.mi.dy = evt.xbutton.y;
	input.mi.mouseData = 0;

        switch (evt.xbutton.button)
        {
        case 1:
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            break;
        case 2:
            input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
            break;
        case 3:
            input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
            break;
        }

	input.mi.time = timeout_t::get_tick_count();
	input.mi.dwExtraInfo = 0;
	send_input( &input );
    }

    if (evt.type == Expose && evt.xexpose.count < 1) {
        cairo_save(cr);
        cairo_set_source_surface(cr, buffer, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    }
}

bool win32k_cairo_t::check_events( bool wait )
{
    LARGE_INTEGER timeout;

    bool timers_left = timeout_t::check_timers(timeout);

    while (XPending(disp) != 0) {
	    handle_events();
    }

    if (!timers_left && !active_window && wait && fiber_t::last_fiber())
	    return true;

    if (!wait)
	    return false;

    handle_events();

    return FALSE;
}

BOOL win32k_cairo_t::set_pixel( INT x, INT y, COLORREF color )
{
	printf("######## pixel ######## \n");
/*
	cairo_t *c = cairo_create(buffer);
	cairo_set_source_rgb(c, ((float) GetRValue(color)) / 255.0,
			((float) GetGValue(color)) / 255.0,
			((float) GetBValue(color)) / 255.0);

	cairo_move_to(c, x, y);
	cairo_line_to(c, x, y);
	cairo_destroy(c);
*/
	
	unsigned int *buf = (unsigned int *) cairo_image_surface_get_data(buffer);
	buf[y * width + x] = color;

	cairo_save(cr);
	cairo_set_source_surface(cr, buffer, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	return TRUE;
}

BOOL win32k_cairo_t::rectangle(INT left, INT top, INT right, INT bottom, brush_t* brush )
{
	printf("######## rectangle ########\n");

	cairo_t *c = cairo_create(buffer);
	cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
	if (left > right) swap(left, right);
	if (top > bottom) swap(top, bottom);
	cairo_rectangle(c, (float) left, (float) top, (float) (right - left), (float) (bottom - top));
	cairo_destroy(c);

	cairo_save(cr);
	cairo_set_source_surface(cr, buffer, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	return TRUE;
}

BOOL win32k_cairo_t::exttextout( INT x, INT y, UINT options, LPRECT rect, UNICODE_STRING& text )
{
#if 0
	char utf8[4];
	printf("######## exttextout ########\n");
	int dx = 0, dy = 0;
	cairo_t *c = cairo_create(buffer);

	for (int i=0; i<text.Length/2; i++)
	{
		WCHAR ch = text.Buffer[i];
		utf8[0] = ch; utf8[1] = 0;
		cairo_move_to(c, x + dx, x + dy);
		dx += 10;
		cairo_show_text(c, utf8);
		printf("C : %c\n", ch);
	}
	cairo_paint(c);
	cairo_destroy(c);

	cairo_save(cr);
	cairo_set_source_surface(cr, buffer, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);
#endif
	return TRUE;
}

BOOL win32k_cairo_t::bitblt(INT xDest, INT yDest, INT cx, INT cy,
		device_context_t *src, INT xSrc, INT ySrc, ULONG rop )
{
	unsigned int *buf = (unsigned int *) cairo_image_surface_get_data(buffer);

	bitmap_t *bitmap = src->get_selected_bitmap();
	if (!bitmap) return FALSE;

	COLORREF pixel;
	for (int i = 0; i < cy; i++)
	{
		for (int j = 0; j < cx; j++)
		{
			unsigned char c1, c2;
			pixel = src->get_pixel( xSrc + j, ySrc + i );
			c1 = pixel & 0x00FF;
			c2 = (pixel >> 16) & 0x00FF;
			pixel = (pixel & 0x0000FF00) | c2 | ( c1 << 16 );
			buf[width * (yDest + i) + (xDest + j)] = pixel;
		}
	}

	cairo_save(cr);
	cairo_set_source_surface(cr, buffer, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	return TRUE;
}

BOOL win32k_cairo_t::polypatblt( ULONG Rop, PRECT rect )
{
	printf("######## polypatblt ########\n");
	rect->left = max( rect->left, 0 );
	rect->top = max( rect->top, 0 );
	rect->right = min( width, rect->right );
	rect->bottom = min( width, rect->bottom );

	cairo_t *c = cairo_create(buffer);
	cairo_set_source_rgb(c, 0.0, 0.0, 0.0);
	cairo_rectangle(c, (float) rect->left, (float) rect->top,
			(float) (rect->right - rect->left), (float) (rect->bottom - rect->top));
	cairo_fill(c);
	cairo_destroy(c);

	cairo_save(cr);
	cairo_set_source_surface(cr, buffer, 0, 0);
	cairo_paint(cr);
	cairo_restore(cr);

	return TRUE;
}

int win32k_cairo_t::getcaps( int index )
{
	switch (index)
	{
	case NUMCOLORS:
		// return 1 << screen->format->BitsPerPixel;
		return 1 << 16;
	case BITSPIXEL:
		// return screen->format->BitsPerPixel;
		return 16;
	default:
		dprintf("%d\n", index );
		return 0;
	}
}

win32k_cairo_t win32k_manager_cairo;

win32k_manager_t* init_cairo_win32k_manager()
{
	return &win32k_manager_cairo;
}

BOOL cairo_device_context_t::rectangle(INT left, INT top, INT right, INT bottom )
{
	brush_t *brush = get_selected_brush();
	if (!brush)
		return FALSE;
	dprintf("drawing with brush %p with color %08lx\n", brush->get_handle(), brush->get_color() );
	return win32k_manager->rectangle( left, top, right, bottom, brush );
}

BOOL cairo_device_context_t::polypatblt( ULONG Rop, PRECT rect )
{
	return win32k_manager->polypatblt( Rop, rect );
}

cairo_device_context_t::cairo_device_context_t() :
	win( 0 )
{
}

BOOL cairo_device_context_t::set_pixel( INT x, INT y, COLORREF color )
{
	return win32k_manager->set_pixel( x, y, color );
}

BOOL cairo_device_context_t::exttextout( INT x, INT y, UINT options,
		 LPRECT rect, UNICODE_STRING& text )
{
	return win32k_manager->exttextout( x, y, options, rect, text );
}

COLORREF cairo_device_context_t::get_pixel( INT x, INT y )
{
	return 0;
}

int cairo_device_context_t::getcaps( int index )
{
	return win32k_manager->getcaps( index );
}

device_context_t* win32k_cairo_t::alloc_screen_dc_ptr()
{
	return new cairo_device_context_t;
}

#else

win32k_manager_t* init_cairo_win32k_manager()
{
	return NULL;
}

#endif
