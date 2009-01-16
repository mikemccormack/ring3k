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

#ifndef __TIMER_H__
#define __TIMER_H__

#include "list.h"

class timeout_t;

typedef list_element<timeout_t> timeout_entry_t;
typedef list_anchor<timeout_t,0> timeout_list_t;
typedef list_iter<timeout_t,0> timeout_iter_t;

class timeout_t
{
	friend class list_anchor<timeout_t,0> ;
	friend class list_iter<timeout_t,0> ;
	timeout_entry_t entry[1];
private:
	static timeout_list_t g_timeouts;
	LARGE_INTEGER expires;
protected:
	void add();
	void remove();
	//void set();
public:
	explicit timeout_t(PLARGE_INTEGER t = 0);
	void set(PLARGE_INTEGER t);
	virtual ~timeout_t();
	static LARGE_INTEGER current_time();
	static ULONG get_tick_count();
	void do_timeout();
	void set_timeout(PLARGE_INTEGER t);
	virtual void signal_timeout() = 0;
	//static bool timers_active();
	static bool check_timers(LARGE_INTEGER& ret);
	bool has_expired();
	void time_remaining( LARGE_INTEGER& remaining );
	static bool queue_is_valid();
};

void get_system_time_of_day( SYSTEM_TIME_OF_DAY_INFORMATION& time_of_day );

#endif // __TIMER_H__
