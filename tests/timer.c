/*
 * native test suite
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
#include "ntapi.h"
#include "log.h"

void test_timer(void)
{
	NTSTATUS r;
	HANDLE timer = 0;
	TIMER_BASIC_INFORMATION info;
	ULONG sz;

	r = NtCreateTimer( NULL, 0, NULL, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );
	ok( timer == 0, "timer was not NULL\n");

	r = NtCreateTimer( &timer, 0, NULL, ~0 );
	ok( r == STATUS_INVALID_PARAMETER_4, "return %08lx\n", r );
	ok( timer == 0, "timer was not NULL\n");

	r = NtCreateTimer( &timer, 0, NULL, 0 );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer != 0, "timer was NULL\n");

	r = NtClose( timer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	timer = 0;
	r = NtCreateTimer( &timer, TIMER_ALL_ACCESS, NULL, SynchronizationTimer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer != 0, "timer was NULL\n");

	sz = 0;
	r = NtQueryTimer( timer, TimerBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( info.TimeRemaining.QuadPart < 0LL, "time not negative\n");
	ok( info.SignalState == 0, "signal state wrong %d\n", info.SignalState);
	ok( sz == sizeof info, "size wrong\n");

	r = NtSetTimer( 0, NULL, NULL, NULL, 0, 0, NULL);
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	r = NtSetTimer( timer, NULL, NULL, NULL, 0, 0, NULL);
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	LARGE_INTEGER timeout;
	BOOLEAN prev = ~0;
	timeout.QuadPart = 0LL;
	r = NtSetTimer( timer, &timeout, NULL, NULL, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 0, "timer signalled (%d)\n", prev);

	r = NtSetTimer( timer, &timeout, NULL, NULL, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 1, "timer signalled (%d)\n", prev);

	r = NtClose( timer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
}

ULONG timer_apc_called = 0;
void *timer_context_magic = (void*) 0x19730425;

void APIENTRY timer_apc(PVOID context, ULONG low, ULONG high)
{
	ok( context == timer_context_magic, "context wrong\n" );
	//dprintf("timer_apc called\n");
	timer_apc_called++;
}

void test_timer_apc(void)
{
	LARGE_INTEGER timeout;
	BOOLEAN prev = ~0;
	HANDLE timer = 0;
	NTSTATUS r;
	TIMER_BASIC_INFORMATION info;
	ULONG sz;

	r = NtCreateTimer( &timer, TIMER_ALL_ACCESS, NULL, SynchronizationTimer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	// set timer
	timeout.QuadPart = -10000LL; // 1ms
	r = NtSetTimer( timer, &timeout, NULL, NULL, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 0, "timer signalled (%d)\n", prev);

	// wait for timer
	r = NtWaitForSingleObject( timer, TRUE, NULL );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called somehow?\n");

	sz = 0;
	r = NtQueryTimer( timer, TimerBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( info.TimeRemaining.QuadPart < 0LL, "time not negative\n");
	ok( info.SignalState == 0, "signal state wrong %d\n", info.SignalState);
	ok( sz == sizeof info, "size wrong\n");

	// start timer again
	timer_apc_called = 0;
	r = NtSetTimer( timer, &timeout, &timer_apc, timer_context_magic, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 0, "timer signalled (%d)\n", prev);
	ok( timer_apc_called == 0, "APC called too soon\n");

	// APC should not be called during NtDelayExecution
	timer_apc_called = 0;
	timeout.QuadPart = -20000LL;
	r = NtDelayExecution( TRUE, &timeout );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called too soon\n");

	// restart timer without waiting... prev should be non-zero
	timer_apc_called = 0;
	r = NtSetTimer( timer, &timeout, &timer_apc, timer_context_magic, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 1, "timer signalled (%d)\n", prev);
	ok( timer_apc_called == 0, "APC called too soon\n");

	// wait on the timer - should succeed, but not APC is called
	timer_apc_called = 0;
	r = NtWaitForSingleObject( timer, TRUE, NULL );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called %ld\n", timer_apc_called);

	// wait on the timer again - APC should be called
	timer_apc_called = 0;
	r = NtWaitForSingleObject( timer, TRUE, NULL );
	ok( r == STATUS_USER_APC, "return %08lx\n", r );
	ok( timer_apc_called == 1, "APC called %ld\n", timer_apc_called);

	// and again (should timeout)
	timeout.QuadPart = 0LL;
	timer_apc_called = 0;
	r = NtWaitForSingleObject( timer, FALSE, &timeout );
	ok( r == STATUS_TIMEOUT, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called %ld\n", timer_apc_called);

	// try set timer again, let APC be called during NtTestAlert
	timer_apc_called = 0;
	timeout.QuadPart = -10000LL;
	r = NtSetTimer( timer, &timeout, &timer_apc, timer_context_magic, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 0, "timer signalled (%d)\n", prev);

	// don't let ourselves be alerted here
	timer_apc_called = 0;
	r = NtWaitForSingleObject( timer, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called %ld\n", timer_apc_called);

	// APC should be called when testing for alerted... ?
	timer_apc_called = 0;
	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 1, "APC not called %ld\n", timer_apc_called);

	// try again - there should only be one APC
	timer_apc_called = 0;
	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called %ld\n", timer_apc_called);

	r = NtClose( timer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
}

void test_delay(void)
{
	LARGE_INTEGER t;
	NTSTATUS r;

	r = NtDelayExecution(FALSE, 0);
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	t.QuadPart = 0;
	r = NtDelayExecution(FALSE, &t);
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	t.QuadPart = 10000000;
	r = NtDelayExecution(FALSE, &t);
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
}

// TODO: test NtDelayExecution with alertability

void test_timer_query(void)
{
	LARGE_INTEGER timeout;
	BOOLEAN prev = ~0;
	HANDLE timer = 0;
	NTSTATUS r;
	TIMER_BASIC_INFORMATION info;
	ULONG sz;

	r = NtQueryTimer( 0, TimerBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_INVALID_HANDLE, "return %08lx\n", r );

	r = NtCreateTimer( &timer, TIMER_ALL_ACCESS, NULL, SynchronizationTimer );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	r = NtQueryTimer( timer, -1, &info, sizeof info, &sz );
	ok( r == STATUS_INVALID_INFO_CLASS, "return %08lx\n", r );

	r = NtQueryTimer( timer, TimerBasicInformation, 0, sizeof info, &sz );
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	r = NtQueryTimer( timer, TimerBasicInformation, &info, 0, &sz );
	ok( r == STATUS_INFO_LENGTH_MISMATCH, "return %08lx\n", r );

	r = NtQueryTimer( timer, TimerBasicInformation, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	r = NtQueryTimer( timer, TimerBasicInformation, 0, -1, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return %08lx\n", r );

	r = NtQueryTimer( timer, TimerBasicInformation, &info, sizeof info, 0 );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	// set timer
	timeout.QuadPart = -10000LL; // 1ms
	r = NtSetTimer( timer, &timeout, NULL, NULL, 0, 0, &prev );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( prev == 0, "timer signalled (%d)\n", prev);

	// wait for timer
	r = NtWaitForSingleObject( timer, TRUE, NULL );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( timer_apc_called == 0, "APC called somehow?\n");

	sz = 0;
	r = NtQueryTimer( timer, TimerBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
	ok( info.TimeRemaining.QuadPart < 0LL, "time not negative\n");
	ok( info.SignalState == 0, "signal state wrong %d\n", info.SignalState);
	ok( sz == sizeof info, "size wrong\n");
}

void NtProcessStartup( void )
{
	log_init();
	test_timer();
	test_timer_apc();
	test_delay();
	test_timer_query();
	log_fini();
}
