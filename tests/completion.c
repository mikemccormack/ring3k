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
#include "rtlapi.h"
#include "log.h"

// NtWaitForSingleObject appears to never block on an IO completion port handle

void completion_proc( void *param )
{
	HANDLE completion = param;
	NTSTATUS r;

	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtSetIoCompletion( completion, 0, 0, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtSetIoCompletion( completion, 1, 2, 3, 4 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), r );
}

static void test_completion(void)
{
	HANDLE thread = 0, completion = 0;
	ULONG key, val;
	CLIENT_ID id;
	NTSTATUS r;
	IO_STATUS_BLOCK iosb;

	// show invalid use of NtCreateIoCompletion
	r = NtCreateIoCompletion(0, GENERIC_READ|GENERIC_WRITE, 0, 0);
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	// show SYNCHRONIZE rights are required to wait on the object
	r = NtCreateIoCompletion(&completion, GENERIC_READ|GENERIC_WRITE, 0, 0);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	r = NtClose( completion );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// single threaded use
	r = NtCreateIoCompletion(&completion, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE, 0, 1);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// looks like we can wait on the object multiple times
	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// posting a io completion packet makes no difference to its state
	r = NtSetIoCompletion( completion, 0, 0, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtSetIoCompletion( completion, 0, 0, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( completion, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtRemoveIoCompletion( completion, &key, &val, &iosb, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtRemoveIoCompletion( completion, &key, &val, &iosb, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// multi threaded use
	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, completion_proc, completion, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	key = ~0;
	val = ~0;
	iosb.Status = ~0;
	iosb.Information = ~0;

	// will block here
	r = NtRemoveIoCompletion( completion, &key, &val, &iosb, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	ok( key == 0, "key wrong\n");
	ok( val == 0, "val wrong\n");
	ok( iosb.Status == 0, "status wrong\n");
	ok( iosb.Information == 0, "info wrong\n");

	r = NtRemoveIoCompletion( completion, &key, &val, &iosb, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	ok( key == 1, "key wrong\n");
	ok( val == 2, "val wrong\n");
	ok( iosb.Status == 3, "status wrong\n");
	ok( iosb.Information == 4, "info wrong\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose(completion);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

static const int num_completions = 10;
static const int num_threads = 10;
HANDLE thread[10];
HANDLE completion;

void threaded_completion_proc( void *param )
{
	//int thread_no = (int) param;
	NTSTATUS r;
	int i;
	ULONG key, val, prev;
	IO_STATUS_BLOCK iosb;

	prev = ~0;
	for (i=0; i<num_completions; i++)
	{
		// will block here if there are no I/O completion packets available
		// or if there are already N other threads running
		// (where N is the last parameter from NtCreateIoCompletionPort)
		r = NtRemoveIoCompletion( completion, &key, &val, &iosb, 0 );
		if ( r != STATUS_SUCCESS || iosb.Status != 0 || iosb.Information != 0 )
			break;

		// As this loop has no sleeps, this thread should run to completion
		// removing num_completions entries from the port.  Other threads
		// waiting on the completion port should not run.
		ok( prev == ~0 || (prev+1) == val, "not in sequence %ld %ld\n", prev, val);
		prev = val;
	}

	NtTerminateThread( NtCurrentThread(), r );
}

static void test_completion_threaded(void)
{
	CLIENT_ID id;
	NTSTATUS r;
	int i;

	r = NtCreateIoCompletion(&completion, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE, 0, 1);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// add completion entries to port (before threads are created)
	for (i=0; i<num_threads*num_completions; i++)
	{
		r = NtSetIoCompletion( completion, 0, i, 0, 0 );
		if( r != STATUS_SUCCESS)
			break;
	}

	// create 10 threads
	for (i=0; i<num_threads; i++)
	{
		r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
								NULL, 0, 0, threaded_completion_proc, (PVOID)i, &thread[i], &id );
		ok( r == STATUS_SUCCESS, "failed to create thread\n" );
	}

	// wait for all threads to complete
	r = NtWaitForMultipleObjects( num_threads, thread, WaitAll, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r );

	r = NtClose(completion);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r );
}

void NtProcessStartup( void )
{
	log_init();
	test_completion();
	test_completion_threaded();
	log_fini();
}
