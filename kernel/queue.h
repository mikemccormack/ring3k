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

#include "ntuser.h"

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

#endif // __RING3K_QUEUE__
