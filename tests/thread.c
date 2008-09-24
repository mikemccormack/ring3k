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


#include "ntapi.h"
#include "rtlapi.h"
#include "log.h"

ULONG check;

ULONG get_process_id( void )
{
	ULONG id;
	__asm__ ( "movl %%fs:0x20, %%eax\n\t" : "=a" (id) );
	return id;
}

ULONG get_thread_id( void )
{
	ULONG id;
	__asm__ ( "movl %%fs:0x24, %%eax\n\t" : "=a" (id) );
	return id;
}

void basic_thread_proc( void *param )
{
	NTSTATUS r = 1;
	//dprintf("%p %p %p %p\n", ((void**)&param)[-1], &param, param, &check);
	if (!get_process_id())
		r |= 4;
	if (!get_thread_id())
		r |= 8;
	if (param == &check) r |= 2;
	NtTerminateThread( NtCurrentThread(), r );
}

void basic_thread_test( void )
{
	THREAD_BASIC_INFORMATION info;
	CLIENT_ID id;
	HANDLE thread = 0;
	NTSTATUS r;
	ULONG sz;

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, basic_thread_proc, &check, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");

	ok( info.ClientId.UniqueThread == id.UniqueThread, "thread id mismatch\n");
	ok( info.ClientId.UniqueProcess == id.UniqueProcess, "thread id mismatch\n");
	ok( info.ExitStatus == 3, "exit code wrong\n");
}

void event_thread_proc( void *param )
{
	HANDLE **event = param;
	NTSTATUS r;

	r = NtWaitForSingleObject( event[0], FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtWaitForSingleObject( event[1], FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	NtTerminateThread( NtCurrentThread(), r );
}

void event_thread_test( void )
{
	HANDLE thread = 0;
	HANDLE event[2] = { 0, 0 };
	CLIENT_ID id;
	NTSTATUS r;
	ULONG prev;

	r = NtCreateEvent( &event[0], EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 1 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	r = NtCreateEvent( &event[1], EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	prev = 2;
	r = NtSetEvent( event[0], &prev );
	ok( r == STATUS_SUCCESS, "failed to set event\n" );
	ok( prev == 1, "previous value wrong\n" );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, event_thread_proc, event, &thread, &id );

	prev = 2;
	r = NtSetEvent( event[1], &prev );
	ok( r == STATUS_SUCCESS, "failed to set event\n" );
	ok( prev == 0, "previous value wrong\n" );

	prev = 2;
	r = NtSetEvent( event[0], &prev );
	ok( r == STATUS_SUCCESS, "failed to set event\n" );
	// racy check...
	//ok( prev == 1, "previous value wrong\n" );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	prev = 2;
	r = NtSetEvent( event[1], &prev );
	ok( r == STATUS_SUCCESS, "failed to set event\n" );
	ok( prev == 1, "previous value wrong\n" );

	prev = 2;
	r = NtSetEvent( event[0], &prev );
	ok( r == STATUS_SUCCESS, "failed to set event\n" );
	// racy check...
	//ok( prev == 0, "previous value wrong\n" );

	prev = 2;
	r = NtResetEvent( event[1], &prev );
	ok( r == STATUS_SUCCESS, "failed to reset event\n" );
	ok( prev == 1, "previous value wrong\n" );

	prev = 2;
	r = NtResetEvent( event[0], &prev );
	ok( r == STATUS_SUCCESS, "failed to reset event\n" );
	ok( prev == 1, "previous value wrong\n" );

	NtClose( event[0] );
	NtClose( event[1] );

	r = NtTerminateThread( thread, 0 );
	ok( r == STATUS_INVALID_PARAMETER, "should failed %08lx\n", r);

	NtClose( thread );
}

void close_event_proc( void *param )
{
	HANDLE event = param;
	NTSTATUS r;

	r = NtClose( event );
	ok( r == STATUS_SUCCESS, "failed to close handle\n" );

	NtTerminateThread( NtCurrentThread(), r );
}

// what happens when a handle we're waiting on is closed?
void event_wait_close_test( void )
{
	HANDLE thread = 0, event = 0;
	CLIENT_ID id;
	NTSTATUS r;
	LARGE_INTEGER timeout;

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, close_event_proc, event, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	timeout.QuadPart = -10000000L;
	r = NtWaitForSingleObject( event, FALSE, &timeout );
	ok( r == STATUS_TIMEOUT, "wait failed\n" );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	NtClose( thread );
}

// how does thread alerting work?

void thread_alert_proc( void *param )
{
	HANDLE event = param;
	NTSTATUS r;

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "failed %08lx\n", r );

	r = NtWaitForSingleObject( event, TRUE, NULL );
	ok( r == STATUS_ALERTED, "failed %08lx\n", r );

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "failed %08lx\n", r );

	NtTerminateThread( NtCurrentThread(), r );
}

void test_alert(void)
{
	HANDLE thread = 0, event = 0;
	CLIENT_ID id;
	NTSTATUS r;
	LARGE_INTEGER timeout;

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 0 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_alert_proc, event, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread %08lx\n", r );

	// give the thread time to start up and wait
	timeout.QuadPart = -10000000L;
	r = NtWaitForSingleObject( event, FALSE, &timeout );
	ok( r == STATUS_TIMEOUT, "wait failed\n" );

	r = NtAlertThread( thread );
	ok( r == STATUS_SUCCESS, "failed to close handle %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	NtClose( thread );
	NtClose( event );
}

static volatile int stop = 0;

void busy_loop_proc( void )
{
	while (!stop)
		;
	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_busy_loop_timeout( void )
{
	HANDLE thread = 0;
	CLIENT_ID id;
	NTSTATUS r;
	LARGE_INTEGER timeout;

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, busy_loop_proc, 0, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread %08lx\n", r );

	timeout.QuadPart = -10000000L;
	r = NtDelayExecution( FALSE, &timeout );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	stop++;

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	NtClose( thread );
}

void test_wait_multiple( void )
{
	NTSTATUS r;
	HANDLE events[3] = {0, 0, 0};
	LARGE_INTEGER timeout;
	ULONG prev = 0;

	timeout.QuadPart = 0;

	r = NtWaitForMultipleObjects( 0, 0, WaitAll, 0, NULL );
	ok( r == STATUS_INVALID_PARAMETER_1, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 0, 0, WaitAll, 0, &timeout );
	ok( r == STATUS_INVALID_PARAMETER_1, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 1, 0, WaitAll, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 100, 0, WaitAll, 0, NULL );
	ok( r == STATUS_INVALID_PARAMETER_1, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 0, events, WaitAll, 0, NULL );
	ok( r == STATUS_INVALID_PARAMETER_1, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 2, events, WaitAll, 0, NULL );
	ok( r == STATUS_INVALID_HANDLE, "incorrect return %08lx\n", r );

	r = NtCreateEvent( &events[0], EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 1 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	r = NtWaitForMultipleObjects( 2, events, WaitAll, 0, NULL );
	ok( r == STATUS_INVALID_HANDLE, "incorrect return %08lx\n", r );

	r = NtCreateEvent( &events[1], EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 1 );
	ok( r == STATUS_SUCCESS, "NtCreateEvent failed\n" );

	// both events set, wait any
	r = NtWaitForMultipleObjects( 2, events, WaitAny, 0, NULL );
	ok( r == 0, "incorrect return %08lx\n", r );

	// one event, event set
	r = NtSetEvent( events[0], &prev );
	ok( r == STATUS_SUCCESS, "incorrect return %08lx\n", r );
	ok( prev == 0, "event state incorrect %ld\n", prev);

	r = NtWaitForMultipleObjects( 1, events, WaitAny, 0, NULL );
	ok( r == 0, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 1, events, WaitAny, 0, &timeout );
	ok( r == STATUS_TIMEOUT, "incorrect return %08lx\n", r );

	// both events set, wait all
	r = NtSetEvent( events[0], &prev );
	ok( r == STATUS_SUCCESS, "incorrect return %08lx\n", r );
	ok( prev == 0, "event state incorrect %ld\n", prev);

	r = NtSetEvent( events[1], &prev );
	ok( r == STATUS_SUCCESS, "incorrect return %08lx\n", r );
	ok( prev == 1, "event state incorrect %ld\n", prev);

	r = NtWaitForMultipleObjects( 2, events, WaitAll, 0, &timeout );
	ok( r == 0, "incorrect return %08lx\n", r );

	// one event set, wait all
	r = NtSetEvent( events[0], &prev );
	ok( r == 0, "incorrect return %08lx\n", r );

	r = NtWaitForMultipleObjects( 2, events, WaitAll, 0, &timeout );
	ok( r == STATUS_TIMEOUT, "incorrect return %08lx\n", r );

	r = NtSetEvent( events[1], &prev );
	ok( r == 0, "incorrect return %08lx\n", r );
	ok( prev == 0, "event state incorrect %ld\n", prev);

	// one event set, wait any
	r = NtWaitForMultipleObjects( 2, events, WaitAny, 0, &timeout );
	ok( r == 0, "incorrect return %08lx\n", r );

	// neither event set, wait any
	r = NtWaitForMultipleObjects( 2, events, WaitAny, 0, &timeout );
	ok( r == 1, "incorrect return %08lx\n", r );

	// wait on the same object twice
	events[2] = events[1];
	r = NtWaitForMultipleObjects( 3, events, WaitAny, 0, &timeout );
	ok( r == STATUS_TIMEOUT, "incorrect return %08lx\n", r );

	NtClose( events[0] );
	NtClose( events[1] );
}

void test_query_thread_information(void)
{
	BYTE buffer[0x100];
	KERNEL_USER_TIMES *kt = (void*) buffer;
	ULONG last = ~0, slot = 0;
	NTSTATUS r;

	r = NtQueryInformationThread(0, ThreadTimes, buffer, 0, 0);
	ok( r == STATUS_INFO_LENGTH_MISMATCH, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), -1, buffer, 0, 0);
	ok( r == STATUS_INVALID_INFO_CLASS, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), ThreadTimes, buffer, 0, 0);
	ok( r == STATUS_INFO_LENGTH_MISMATCH, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), ThreadTimes, 0, sizeof *kt, 0);
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), ThreadTimes, buffer, sizeof buffer, 0);
	ok( r == STATUS_INFO_LENGTH_MISMATCH, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), ThreadTimes, kt, sizeof *kt, 0);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentProcess(), ThreadTimes, kt, sizeof *kt, 0);
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong %08lx\n", r);

	r = NtQueryInformationThread(NtCurrentThread(), ThreadAmILastThread, &last, sizeof last, 0);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( last == 1, "return wrong %08lx\n", last);

	slot = 0;
	r = NtQueryInformationThread(NtCurrentThread(), ThreadZeroTlsCell, &slot, sizeof slot, 0);
	ok( r == STATUS_INVALID_INFO_CLASS, "return wrong %08lx\n", r);

	r = NtSetInformationThread(NtCurrentThread(), ThreadZeroTlsCell, &slot, sizeof slot);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

/*	// test TLS expansion slots
	slot = 100;
	r = NtSetInformationThread(NtCurrentThread(), ThreadZeroTlsCell, &slot, sizeof slot);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
*/
}

void test_context( void )
{
	HANDLE thread = 0;
	INITIAL_TEB it;
	CONTEXT ctx;
	NTSTATUS r;
	CLIENT_ID id;

	r = NtGetContextThread( 0, 0 );
	ok( r == STATUS_INVALID_HANDLE, "return was (%08lx)\n", r);

	r = NtGetContextThread( NtCurrentThread(), 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return was (%08lx)\n", r);

	memset( &ctx, 0, sizeof ctx );
	memset( &it, 0, sizeof it );
	r = NtCreateThread( &thread, THREAD_ALL_ACCESS, 0, NtCurrentProcess(), &id, &ctx, &it, TRUE);
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtGetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( ctx.ContextFlags == 0, "ContextFlags %08lx\n", ctx.ContextFlags);
	// register contain junk

	memset( &ctx, 0, sizeof ctx );
	ctx.ContextFlags = ~0;
	r = NtGetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( ctx.ContextFlags == ~0, "ContextFlags %08lx\n", ctx.ContextFlags);
	ok( ctx.Eax == 0, "Eax %08lx\n", ctx.Eax);

	ctx.ContextFlags = 0;
	ctx.Eax = 1;
	r = NtSetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	ctx.ContextFlags = CONTEXT_INTEGER;
	r = NtGetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( ctx.ContextFlags == CONTEXT_INTEGER, "ContextFlags %08lx\n", ctx.ContextFlags);
	ok( ctx.Eax == 0, "Eax %08lx\n", ctx.Eax);

	ctx.Eax = 1;
	r = NtSetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( ctx.ContextFlags == CONTEXT_INTEGER, "ContextFlags %08lx\n", ctx.ContextFlags);

	ctx.ContextFlags = CONTEXT_INTEGER;
	ctx.Eax = 1;
	r = NtGetContextThread( thread, &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( ctx.ContextFlags == CONTEXT_INTEGER, "ContextFlags %08lx\n", ctx.ContextFlags);
	ok( ctx.Eax == 1, "Eax %08lx\n", ctx.Eax);

	r = NtTerminateThread( thread, 0 );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtClose( thread );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
}

void NtProcessStartup( void )
{
	log_init();

	basic_thread_test();
	event_thread_test();
	event_wait_close_test();
	test_alert();
	test_busy_loop_timeout();
	test_wait_multiple();
	test_query_thread_information();
	test_context();

	log_fini();
}
