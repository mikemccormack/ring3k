/*
 * nt loader
 *
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

#ifndef __RING3K_QUEUE__
#define __RING3K_QUEUE__

#include "ntwin32.h"

class msg_tt;
class msg_waiter_tt;
class thread_message_queue_tt;

typedef list_anchor<msg_tt,0> msg_list_t;
typedef list_iter<msg_tt,0> msg_iter_t;
typedef list_element<msg_tt> msg_element_t;

typedef list_anchor<msg_waiter_tt,0> msg_waiter_list_t;
typedef list_iter<msg_waiter_tt,0> msg_waiter_iter_t;
typedef list_element<msg_waiter_tt> msg_waiter_element_t;

class msg_waiter_tt
{
	friend class thread_message_queue_tt;
	friend class list_anchor<msg_waiter_tt,0>;
	msg_waiter_element_t entry[1];
	thread_t *t;
	MSG& msg;
public:
	msg_waiter_tt( MSG& m);
};

class msg_tt {
public:
	msg_element_t entry[1];
	HWND hwnd;
	UINT message;
	WPARAM wparam;
	LPARAM lparam;
	DWORD time;
public:
	msg_tt( HWND _hwnd, UINT Message, WPARAM Wparam, LPARAM Lparam );
};

// derived from Wine's struct thread_input
// see wine/server/queue.c (by Alexandre Julliard)
class thread_message_queue_tt : public sync_object_t
{
	bool	quit_message;    // is there a pending quit message?
	int	exit_code;       // exit code of pending quit message
	msg_list_t msg_list;
	msg_waiter_list_t waiter_list;
public:
	thread_message_queue_tt();
	~thread_message_queue_tt();
	BOOL post_message( HWND Window, UINT Message, WPARAM Wparam, LPARAM Lparam );
	void post_quit_message( ULONG exit_code );
	bool get_quit_message( MSG &msg );
	virtual BOOLEAN is_signalled( void );
	BOOLEAN get_message( MSG& Message, HWND Window, ULONG MinMessage, ULONG MaxMessage);
	bool copy_msg( MSG& Message );
};

#endif // __RING3K_QUEUE__
