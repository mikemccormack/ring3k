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
#include <sys/time.h>
#include <time.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "timer.h"
#include "ntcall.h"
#include "object.h"
#include "debug.h"
#include "object.inl"

timeout_list_t timeout_t::g_timeouts;

bool timeout_t::has_expired()
{
	return !entry[0].is_linked();
}

bool timeout_t::queue_is_valid()
{
	timeout_iter_t i(g_timeouts);
	timeout_t *prev = 0;

	i.reset();
	while (i)
	{
		timeout_t* x = i;
		if (prev && prev->expires.QuadPart > x->expires.QuadPart)
			return false;
		i.next();
		prev = x;
	}

	return true;
}

// returns false if there were no timers
bool timeout_t::check_timers(LARGE_INTEGER& ret)
{
	LARGE_INTEGER now = current_time();
	timeout_t *t;

	if (!g_timeouts.head())
		return false;

	ret.QuadPart = 0LL;
	while (1)
	{
		t = g_timeouts.head();
		if (!t)
			return true;

		if (t->expires.QuadPart > now.QuadPart)
			break;

		t->do_timeout();
	}

	// calculate the timeout
	ret.QuadPart = t->expires.QuadPart - now.QuadPart;

	return true;
}

// FIXME: consider unifying this with logic in check_timers
//        to avoid conflicting return values
void timeout_t::time_remaining( LARGE_INTEGER& remaining )
{
	LARGE_INTEGER now = current_time();
	remaining.QuadPart = expires.QuadPart - now.QuadPart;
}

timeout_t::timeout_t(PLARGE_INTEGER t)
{
	set_timeout(t);
}

void timeout_t::set_timeout(PLARGE_INTEGER t)
{
	if (!t)
	{
		remove();
		expires.QuadPart = 0LL;
		return;
	}

	LARGE_INTEGER now = current_time();
	if (t->QuadPart <= 0LL)
		expires.QuadPart = now.QuadPart - t->QuadPart;
	else
		expires.QuadPart = t->QuadPart;

	// check there wasn't an overflow
	if (expires.QuadPart < 0LL)
		expires.QuadPart = 0x7fffffffffffffffLL;

	add();
	assert(!g_timeouts.empty());
	assert( queue_is_valid() );
}

void timeout_t::add()
{
	timeout_iter_t i(g_timeouts);
	timeout_t* x;

	while ( (x = i) )
	{
		if (x->expires.QuadPart > expires.QuadPart)
			break;
		i.next();
	}

	// if there's a timer before this one, no need to set the interval timer
	if (x)
		g_timeouts.insert_before(x, this);
	else
		g_timeouts.append(this);
}

void timeout_t::remove()
{
	if (entry[0].is_linked())
		g_timeouts.unlink(this);
}

LARGE_INTEGER timeout_t::current_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	LARGE_INTEGER ret;
	ret.QuadPart = (tv.tv_sec * 1000000LL + tv.tv_usec) * 10LL;
	return ret;
}

void get_system_time_of_day( SYSTEM_TIME_OF_DAY_INFORMATION& time_of_day )
{
	static LARGE_INTEGER boot_time;
	if (!boot_time.QuadPart)
		boot_time = timeout_t::current_time();

	time_of_day.CurrentTime = timeout_t::current_time();
	time_of_day.BootTime = boot_time;
	time_of_day.TimeZoneBias.QuadPart = 0LL;
	time_of_day.CurrentTimeZoneId = 0;
}

void timeout_t::do_timeout()
{
	// remove first so we can be added again
	remove();
	signal_timeout();
}

timeout_t::~timeout_t()
{
	remove();
}

class nttimer_t : public sync_object_t, public timeout_t
{
protected:
	BOOLEAN expired;
	ULONG interval;
	thread_t *thread;
	PKNORMAL_ROUTINE apc_routine;
	PVOID apc_context;
public:
	nttimer_t();
	~nttimer_t();
	NTSTATUS set(LARGE_INTEGER& DueTime, PKNORMAL_ROUTINE apc, PVOID context, BOOLEAN Resume, ULONG Period, BOOLEAN& prev);
	virtual BOOLEAN is_signalled( void );
	virtual BOOLEAN satisfy( void );
	virtual void signal_timeout();
	void cancel( BOOLEAN& prev );
};

nttimer_t::nttimer_t() :
	expired(FALSE),
	interval(0),
	thread(0),
	apc_routine(0),
	apc_context(0)
{
}

nttimer_t::~nttimer_t()
{
	if (thread)
		release( thread );
}

nttimer_t *nttimer_from_obj( object_t *obj )
{
	return dynamic_cast<nttimer_t*>( obj );
}

BOOLEAN nttimer_t::is_signalled( void )
{
	return expired;
}

BOOLEAN nttimer_t::satisfy( void )
{
	// FIXME: user correct time values
	if (apc_routine)
		thread->queue_apc_thread( apc_routine, apc_context, 0, 0 );

	// restart the timer
	if (interval)
	{
		LARGE_INTEGER when;
		when.QuadPart = interval * -10000LL;
		set_timeout( &when );
	}
	return TRUE;
}

void nttimer_t::signal_timeout()
{
	expired = TRUE;
	notify_watchers();
}

NTSTATUS nttimer_t::set(
	LARGE_INTEGER& DueTime,
	PKNORMAL_ROUTINE apc,
	PVOID context,
	BOOLEAN Resume,
	ULONG Period,
	BOOLEAN& prev)
{
	//dprintf("%ld %p %p %d %ld\n", (ULONG)DueTime.QuadPart, apc, context, Resume, Period );

	prev = expired;
	interval = Period;
	thread = current;
	addref( thread );
	apc_routine = apc;
	apc_context = context;

	if (DueTime.QuadPart == 0LL)
	{
		expired = TRUE;
		return STATUS_SUCCESS;
	}

	expired = FALSE;
	set_timeout( &DueTime );

	return STATUS_SUCCESS;
}

void nttimer_t::cancel( BOOLEAN& prev )
{
	prev = expired;
	set_timeout( 0 );
	expired = FALSE;
}

// sync_timer_t is a specialized timer
class sync_timer_t : public nttimer_t
{
public:
	virtual BOOLEAN satisfy( void );
};

BOOLEAN sync_timer_t::satisfy()
{
	expired = FALSE;
	return nttimer_t::satisfy();
}

class timer_factory : public object_factory
{
private:
	TIMER_TYPE Type;
public:
	timer_factory(TIMER_TYPE t) : Type(t) {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS timer_factory::alloc_object(object_t** obj)
{
	switch (Type)
	{
	case SynchronizationTimer:
		*obj = new sync_timer_t();
		break;
	case NotificationTimer:
		//*obj = new notify_timer_t();
		*obj = new nttimer_t();
		break;
	default:
		return STATUS_INVALID_PARAMETER_4;
	}
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateTimer(
	PHANDLE TimerHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes,
	TIMER_TYPE Type)
{
	dprintf("%p %08lx %p %u\n", TimerHandle, AccessMask, ObjectAttributes, Type );

	timer_factory factory( Type );
	return factory.create( TimerHandle, AccessMask, ObjectAttributes );
}

NTSTATUS NtOpenTimer(
	PHANDLE TimerHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	dprintf("\n");
	return nt_open_object<nttimer_t>( TimerHandle, AccessMask, ObjectAttributes );
}

NTSTATUS NtCancelTimer(
	HANDLE TimerHandle,
	PBOOLEAN PreviousState)
{
	NTSTATUS r;

	nttimer_t* timer = 0;
	r = object_from_handle( timer, TimerHandle, TIMER_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	if (PreviousState)
	{
		r = verify_for_write( PreviousState, sizeof *PreviousState );
		if (r < STATUS_SUCCESS)
			return r;
	}

	BOOLEAN prev = 0;
	timer->cancel(prev);

	if (PreviousState)
		copy_to_user( PreviousState, &prev, sizeof prev );

	return STATUS_SUCCESS;
}

NTSTATUS NtSetTimer(
	HANDLE TimerHandle,
	PLARGE_INTEGER DueTime,
	PTIMER_APC_ROUTINE TimerApcRoutine,
	PVOID TimerContext,
	BOOLEAN Resume,
	LONG Period,
	PBOOLEAN PreviousState)
{
	LARGE_INTEGER due;

	NTSTATUS r = copy_from_user( &due, DueTime, sizeof due );
	if (r < STATUS_SUCCESS)
		return r;

	dprintf("due = %llx\n", due.QuadPart);

	nttimer_t* timer = 0;
	r = object_from_handle( timer, TimerHandle, TIMER_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	if (PreviousState)
	{
		r = verify_for_write( PreviousState, sizeof *PreviousState );
		if (r < STATUS_SUCCESS)
			return r;
	}

	BOOLEAN prev = FALSE;
	r = timer->set( due, (PKNORMAL_ROUTINE)TimerApcRoutine, TimerContext, Resume, Period, prev );
	if (r == STATUS_SUCCESS && PreviousState )
		 copy_to_user( PreviousState, &prev, sizeof prev );

	return r;
}

NTSTATUS NTAPI NtQueryTimer(
	HANDLE TimerHandle,
	TIMER_INFORMATION_CLASS TimerInformationClass,
	PVOID TimerInformation,
	ULONG TimerInformationLength,
	PULONG ResultLength)
{
	NTSTATUS r;

	nttimer_t* timer = 0;
	r = object_from_handle( timer, TimerHandle, TIMER_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	union {
		TIMER_BASIC_INFORMATION basic;
	} info;
	ULONG sz = 0;

	switch (TimerInformationClass)
	{
	case TimerBasicInformation:
		sz = sizeof info.basic;
		break;
	default:
		return STATUS_INVALID_INFO_CLASS;
	}

	// this seems like the wrong order, but agrees with what tests show
	r = verify_for_write( TimerInformation, sz );
	if (r < STATUS_SUCCESS)
		return r;

	if (sz != TimerInformationLength)
		return STATUS_INFO_LENGTH_MISMATCH;

	switch (TimerInformationClass)
	{
	case TimerBasicInformation:
		info.basic.SignalState = timer->is_signalled();
		timer->time_remaining( info.basic.TimeRemaining );
		break;
	default:
		assert(0);
	}

	r = copy_to_user( TimerInformation, &info, sz );
	if (r < STATUS_SUCCESS)
		return r;

	if (ResultLength)
		copy_to_user( ResultLength, &sz, sizeof sz );

	return r;
}

NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER CurrentTime)
{
	LARGE_INTEGER now = timeout_t::current_time();

	return copy_to_user( CurrentTime, &now, sizeof now );
}
