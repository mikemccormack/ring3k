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
#include "object.h"
#include "ntuser.h"
#include "mem.h"
#include "debug.h"
#include "object.inl"

// derived from Wine's struct thread_input
// see wine/server/queue.c (by Alexandre Julliard)
class thread_message_queue_tt : public sync_object_t
{
	bool	quit_message;    // is there a pending quit message?
	int	exit_code;       // exit code of pending quit message

public:
	thread_message_queue_tt();
	void post_quit_message( ULONG exit_code );
	bool get_quit_message( MSG &msg );
	virtual BOOLEAN is_signalled( void );
};

thread_message_queue_tt::thread_message_queue_tt() :
	quit_message( 0 ),
	exit_code( 0 )
{
}

bool thread_message_queue_tt::get_quit_message( MSG& msg )
{
	bool ret = quit_message;
	if (quit_message)
	{
		msg.message = WM_QUIT;
		msg.wParam = exit_code;
		quit_message = false;
	}
	return ret;
}

BOOLEAN thread_message_queue_tt::is_signalled( void )
{
	return FALSE;
}

void thread_message_queue_tt::post_quit_message( ULONG ret )
{
	quit_message = true;
	exit_code = ret;
}

BOOLEAN NTAPI NtUserGetMessage(PMSG Message, HANDLE Window, ULONG MinMessage, ULONG MaxMessage)
{
	// no input queue...
	thread_message_queue_tt* queue = current->queue;
	if (!queue)
		return FALSE;

	MSG msg;
	memset( &msg, 0, sizeof msg );
	if (queue->get_quit_message( msg ))
	{
		copy_to_user( Message, &msg, sizeof msg );
		return FALSE;
	}

	current->stop();
	return TRUE;
}

