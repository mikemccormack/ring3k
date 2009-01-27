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
	memset( &info, 0, sizeof info );
	info.msg = WM_GETMINMAXINFO;
}

ULONG getminmaxinfo_tt::get_callback_num() const
{
	return NTWIN32_MINMAX_CALLBACK;
}

nccalcsize_message_tt::nccalcsize_message_tt()
{
	memset( &info, 0, sizeof info );
	info.msg = WM_NCCALCSIZE;
}

ULONG nccalcsize_message_tt::get_callback_num() const
{
	return NTWIN32_NCCALC_CALLBACK;
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

