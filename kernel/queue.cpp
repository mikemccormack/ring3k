/*
 * message queues
 *
 * Based on wine/server/queue.c
 * Copyright (C) 2000 Alexandre Julliard
 *
 * Modifications for ring3k
 * Copyright 2006-2009 Mike McCormack
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
#include "ntwin32.h"
#include "mem.h"
#include "debug.h"
#include "object.inl"
#include "list.h"
#include "timer.h"
#include "win.h"
#include "queue.h"
#include "spy.h"

msg_tt::msg_tt( HWND _hwnd, UINT _message, WPARAM _wparam, LPARAM _lparam ) :
	hwnd( _hwnd ),
	message( _message ),
	wparam( _wparam ),
	lparam( _lparam )
{
	time = timeout_t::get_tick_count();
}

thread_message_queue_tt::thread_message_queue_tt() :
	quit_message( 0 ),
	exit_code( 0 )
{
}

thread_message_queue_tt::~thread_message_queue_tt()
{
	msg_tt *msg;

	while ((msg = msg_list.head()))
	{
		msg_list.unlink( msg );
		delete msg;
	}
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

bool thread_message_queue_tt::get_paint_message( HWND Window, MSG& msg )
{
	window_tt *win = window_tt::find_window_to_repaint( Window, current );
	if (!win)
		return FALSE;

	msg.message = WM_PAINT;
	msg.time = timeout_t::get_tick_count();
	msg.hwnd = win->handle;

	return TRUE;
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

BOOL thread_message_queue_tt::post_message(
	HWND Window, UINT Message, WPARAM Wparam, LPARAM Lparam )
{
	msg_waiter_tt *waiter = waiter_list.head();
	if (waiter)
	{
		MSG& msg = waiter->msg;
		msg.hwnd = Window;
		msg.message = Message;
		msg.wParam = Wparam;
		msg.lParam = Lparam;
		msg.time = timeout_t::get_tick_count();
		msg.pt.x = 0;
		msg.pt.y = 0;

		// remove from the list first
		waiter_list.unlink( waiter );
		set_timeout( 0 );

		// start the thread (might reschedule here )
		waiter->t->start();

		return TRUE;
	}

	// no waiter, so store the message
	msg_tt* msg = new msg_tt( Window, Message, Wparam, Lparam );
	if (!msg)
		return FALSE;
	msg_list.append( msg );

	// FIXME: wake up a thread that is waiting
	return TRUE;
}

// return true if we copied a message
bool thread_message_queue_tt::get_posted_message( HWND Window, MSG& Message )
{
	msg_tt *m = msg_list.head();
	if (!m)
		return false;

	msg_list.unlink( m );
	Message.hwnd = m->hwnd;
	Message.message = m->message;
	Message.wParam = m->wparam;
	Message.lParam = m->lparam;
	Message.time = m->time;
	Message.pt.x = 0;
	Message.pt.y = 0;
	delete m;

	return true;
}

msg_waiter_tt::msg_waiter_tt( MSG& m):
	msg( m )
{
	t = current;
}

win_timer_tt::win_timer_tt( HWND Window, UINT Identifier ) :
	hwnd( Window ),
	id( Identifier )
{
}

win_timer_tt* thread_message_queue_tt::find_timer( HWND Window, UINT Identifier )
{
	for (win_timer_iter_t i(timer_list); i; i.next())
	{
		win_timer_tt *t = i;
		if (t->id != Identifier)
			continue;
		if (t->hwnd != Window )
			continue;
		return t;
	}
	return NULL;
}

void win_timer_tt::reset()
{
	expiry = timeout_t::current_time();
	expiry.QuadPart += period*10000LL;
}

bool win_timer_tt::expired() const
{
	LARGE_INTEGER now = timeout_t::current_time();
	return (now.QuadPart >= expiry.QuadPart);
}

void thread_message_queue_tt::timer_add( win_timer_tt* timer )
{
	win_timer_tt *t = NULL;

	// maintain list in order of expiry time
	for (win_timer_iter_t i(timer_list); i; i.next())
	{
		t = i;
		if (t->expiry.QuadPart >= timer->expiry.QuadPart)
			break;
	}
	if (t)
		timer_list.insert_before( t, timer );
	else
		timer_list.append( timer );
}

bool thread_message_queue_tt::get_timer_message( HWND Window, MSG& msg )
{
	LARGE_INTEGER now = timeout_t::current_time();
	win_timer_tt *t = NULL;
	for (win_timer_iter_t i(timer_list); i; i.next())
	{
		t = i;
		// stop searching after we reach a timer that has not expired
		if (t->expiry.QuadPart > now.QuadPart)
			return false;
		if (t->hwnd == Window)
			break;
	}

	if (!t)
		return false;

	// remove from the front of the queue
	timer_list.unlink( t );

	msg.hwnd = t->hwnd;
	msg.message = WM_TIMER;
	msg.wParam = t->id;
	msg.lParam = (UINT) t->lparam;
	msg.time = timeout_t::get_tick_count();
	msg.pt.x = 0;
	msg.pt.y = 0;

	// reset and add back to the queue
	t->reset();
	timer_add( t );

	return true;
}

BOOLEAN thread_message_queue_tt::set_timer( HWND Window, UINT Identifier, UINT Elapse, PVOID TimerProc )
{
	win_timer_tt* timer = find_timer( Window, Identifier );
	if (timer)
		timer_list.unlink( timer );
	else
		timer = new win_timer_tt( Window, Identifier );
	dprintf("adding timer %p hwnd %p id %d\n", timer, Window, Identifier );
	timer->period = Elapse;
	timer->lparam = TimerProc;
	timer_add( timer );
	return TRUE;
}

BOOLEAN thread_message_queue_tt::kill_timer( HWND Window, UINT Identifier )
{
	win_timer_tt* timer = find_timer( Window, Identifier );
	if (!timer)
		return FALSE;
	dprintf("deleting timer %p hwnd %p id %d\n", timer, Window, Identifier );
	timer_list.unlink( timer );
	delete timer;
	return TRUE;
}

bool thread_message_queue_tt::get_message_timeout( HWND Window, LARGE_INTEGER& timeout )
{
	for (win_timer_iter_t i(timer_list); i; i.next())
	{
		win_timer_tt *t = i;
		if (t->hwnd != Window )
			continue;
		timeout = t->expiry;
		return true;
	}
	return false;
}

// return true if we succeeded in copying a message
BOOLEAN thread_message_queue_tt::get_message_no_wait(
	MSG& Message, HWND Window, ULONG MinMessage, ULONG MaxMessage)
{
	dprintf("checking posted messages\n");
	if (get_posted_message( Window, Message ))
		return true;

	dprintf("checking quit messages\n");
	if (get_quit_message( Message ))
		return true;

	dprintf("checking paint messages\n");
	if (get_paint_message( Window, Message ))
		return true;

	dprintf("checking timer messages\n");
	if (get_timer_message( Window, Message ))
		return true;

	return false;
}

void thread_message_queue_tt::signal_timeout()
{
	msg_waiter_tt *waiter = waiter_list.head();
	if (waiter)
	{
		waiter_list.unlink( waiter );
		set_timeout( 0 );

		// start the thread (might reschedule here )
		waiter->t->start();
	}
}

BOOLEAN thread_message_queue_tt::get_message(
	MSG& Message, HWND Window, ULONG MinMessage, ULONG MaxMessage)
{
	if (get_message_no_wait( Message, Window, MinMessage, MaxMessage))
		return true;

	LARGE_INTEGER t;
	if (get_message_timeout( Window, t ))
		set_timeout( &t );

	// wait for a message
	// a thread sending a message will restart us
	msg_waiter_tt wait( Message );
	waiter_list.append( &wait );
	current->stop();

	return !current->is_terminated();
}

BOOLEAN NTAPI NtUserGetMessage(PMSG Message, HWND Window, ULONG MinMessage, ULONG MaxMessage)
{
	// no input queue...
	thread_message_queue_tt* queue = current->queue;
	if (!queue)
		return FALSE;

	NTSTATUS r = verify_for_write( Message, sizeof *Message );
	if (r != STATUS_SUCCESS)
		return FALSE;

	MSG msg;
	memset( &msg, 0, sizeof msg );
	if (queue->get_message( msg, Window, MinMessage, MaxMessage ))
		copy_to_user( Message, &msg, sizeof msg );

	if (option_trace)
	{
		fprintf(stderr, "%04lx: %s\n", current->trace_id(), __FUNCTION__);
		fprintf(stderr, " msg.hwnd    = %p\n", msg.hwnd);
		fprintf(stderr, " msg.message = %08x (%s)\n", msg.message, get_message_name(msg.message));
		fprintf(stderr, " msg.wParam  = %08x\n", msg.wParam);
		fprintf(stderr, " msg.lParam  = %08lx\n", msg.lParam);
		fprintf(stderr, " msg.time    = %08lx\n", msg.time);
		fprintf(stderr, " msg.pt.x    = %08lx\n", msg.pt.x);
		fprintf(stderr, " msg.pt.y    = %08lx\n", msg.pt.y);
	}

	return msg.message != WM_QUIT;
}

BOOLEAN NTAPI NtUserPostMessage( HWND Window, UINT Message, WPARAM Wparam, LPARAM Lparam )
{
	window_tt *win = window_from_handle( Window );
	if (!win)
		return FALSE;

	thread_t*& thread = win->get_win_thread();
	assert(thread != NULL);

	return thread->queue->post_message( Window, Message, Wparam, Lparam );
}

BOOLEAN NTAPI NtUserPeekMessage( PMSG Message, HWND Window, UINT MaxMessage, UINT MinMessage, UINT Remove)
{
	thread_message_queue_tt* queue = current->queue;
	if (!queue)
		return FALSE;

	NTSTATUS r = verify_for_write( Message, sizeof *Message );
	if (r != STATUS_SUCCESS)
		return FALSE;

	MSG msg;
	memset( &msg, 0, sizeof msg );
	BOOL ret = queue->get_message_no_wait( msg, Window, MinMessage, MaxMessage );
	if (ret)
		copy_to_user( Message, &msg, sizeof msg );

	return ret;
}

UINT NTAPI NtUserSetTimer( HWND Window, UINT Identifier, UINT Elapse, PVOID TimerProc )
{
	window_tt *win = window_from_handle( Window );
	if (!win)
		return FALSE;

	thread_t*& thread = win->get_win_thread();
	assert(thread != NULL);

	return thread->queue->set_timer( Window, Identifier, Elapse, TimerProc );
}

BOOLEAN NTAPI NtUserKillTimer( HWND Window, UINT Identifier )
{
	window_tt *win = window_from_handle( Window );
	if (!win)
		return FALSE;

	thread_t*& thread = win->get_win_thread();
	assert(thread != NULL);

	return thread->queue->kill_timer( Window, Identifier );
}
