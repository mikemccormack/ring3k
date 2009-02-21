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

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <new>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "ntwin32.h"
#include "message.h"
#include "debug.h"
#include "queue.h"
#include "spy.h"

template<class Pack>
generic_message_tt<Pack>::generic_message_tt()
{
	memset( &info, 0, sizeof info );
}

template<class Pack>
ULONG generic_message_tt<Pack>::get_size() const
{
	return sizeof info;
}

template<class Pack>
NTSTATUS generic_message_tt<Pack>::copy_to_user( void *ptr ) const
{
	return ::copy_to_user( ptr, &info, sizeof info );
}

template<class Pack>
void generic_message_tt<Pack>::set_window_info( window_tt *win )
{
	info.wininfo = win->get_wininfo();
	info.wndproc = win->get_wndproc();
	info.func = (typeof(info.func)) g_funcsW[17];
}

template<class Pack>
const char *generic_message_tt<Pack>::description()
{
	return get_message_name( info.msg );
}

nccreate_message_tt::nccreate_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name ) :
	create_message_tt( cs, cls, name )
{
	info.msg = WM_NCCREATE;
}

create_message_tt::create_message_tt( NTCREATESTRUCT& cs,
			const UNICODE_STRING& _cls, const UNICODE_STRING& _name ) :
	cls( _cls ),
	name( _name )
{
	memset( &info, 0, sizeof info );

	info.pi.x = 0;
	info.pi.count = 0;
	info.pi.kernel_address = 0;
	info.pi.adjust_info_ofs = 0;
	info.pi.no_adjust = 0;

	info.pi.sz = sizeof info;
	info.wininfo = NULL;
	info.msg = WM_CREATE;
	info.wparam = 0;
	info.cs_nonnull = TRUE;
	info.cs = cs;
}

ULONG create_message_tt::get_callback_num() const
{
	return NTWIN32_CREATE_CALLBACK;
}

getminmaxinfo_tt::getminmaxinfo_tt()
{
	info.msg = WM_GETMINMAXINFO;
}

ULONG getminmaxinfo_tt::get_callback_num() const
{
	return NTWIN32_MINMAX_CALLBACK;
}

nccalcsize_message_tt::nccalcsize_message_tt()
{
	info.msg = WM_NCCALCSIZE;
}

ULONG nccalcsize_message_tt::get_callback_num() const
{
	return NTWIN32_NCCALC_CALLBACK;
}

basicmsg_tt::basicmsg_tt()
{
}

basicmsg_tt::basicmsg_tt( INT message )
{
	info.msg = message;
}

ULONG basicmsg_tt::get_callback_num() const
{
	return NTWIN32_BASICMSG_CALLBACK;
}

showwindowmsg_tt::showwindowmsg_tt( bool show )
{
	info.msg = WM_SHOWWINDOW;
	info.wparam = show;
	info.lparam = 0;
}

winposchange_tt::winposchange_tt( ULONG message, WINDOWPOS& _pos ) :
	pos( _pos )
{
	info.msg = message;
}

// comes BEFORE a window's position changes
winposchanging_tt::winposchanging_tt( WINDOWPOS& pos ) :
	winposchange_tt( WM_WINDOWPOSCHANGING, pos )
{
}

ULONG winposchanging_tt::get_callback_num() const
{
	return NTWIN32_POSCHANGING_CALLBACK;
}

// comes AFTER a window's position changes
winposchanged_tt::winposchanged_tt( WINDOWPOS& pos ) :
	winposchange_tt( WM_WINDOWPOSCHANGED, pos )
{
}

ULONG winposchanged_tt::get_callback_num() const
{
	return NTWIN32_POSCHANGED_CALLBACK;
}

appactmsg_tt::appactmsg_tt( UINT type )
{
	info.msg = WM_ACTIVATEAPP;
	info.wparam = type;
}

ncactivate_tt::ncactivate_tt()
{
	info.msg = WM_NCACTIVATE;
}

activate_tt::activate_tt()
{
	info.msg = WM_ACTIVATE;
}

setfocusmsg_tt::setfocusmsg_tt()
{
	info.msg = WM_SETFOCUS;
}

ncpaintmsg_tt::ncpaintmsg_tt() :
	basicmsg_tt( WM_NCPAINT )
{
}

paintmsg_tt::paintmsg_tt() :
	basicmsg_tt( WM_PAINT )
{
}

erasebkgmsg_tt::erasebkgmsg_tt( HANDLE dc ) :
	basicmsg_tt( WM_ERASEBKGND )
{
	info.wparam = (WPARAM) dc;
}

keyup_msg_tt::keyup_msg_tt( UINT key )
{
	info.msg = WM_KEYUP;
	info.wparam = key;
}

keydown_msg_tt::keydown_msg_tt( UINT key )
{
	info.msg = WM_KEYDOWN;
	info.wparam = key;
}

sizemsg_tt::sizemsg_tt( INT cx, INT cy ) :
	basicmsg_tt( WM_SIZE )
{
	info.lparam = MAKELONG( cx, cy );
}

movemsg_tt::movemsg_tt( INT x, INT y ) :
	basicmsg_tt( WM_MOVE )
{
	info.lparam = MAKELONG( x, y );
}

ncdestroymsg_tt::ncdestroymsg_tt() :
	basicmsg_tt( WM_NCDESTROY )
{
}

destroymsg_tt::destroymsg_tt() :
	basicmsg_tt( WM_DESTROY )
{
}

