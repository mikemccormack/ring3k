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

static void test_event(void)
{
	NTSTATUS r;
	HANDLE event;
	ULONG prev;
	LARGE_INTEGER timeout;
	NTSTATUS (NTAPI *func)(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES *,EVENT_TYPE,ULONG);

	/* check NtSetEvent and NtResetEvent for some invalid values */
	r = NtSetEvent( 0, NULL );
	ok(r == STATUS_INVALID_HANDLE, "return wrong\n");

	r = NtResetEvent( 0, NULL );
	ok(r == STATUS_INVALID_HANDLE, "return wrong\n");

	/* check NtCreateEvent for some invalid values */
	r = NtCreateEvent( NULL, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong\n");

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, 2, 0 );
	ok(r == STATUS_INVALID_PARAMETER, "return wrong (%08lx)\n", r);

	/* check NtResetEvent and NtSetEvent return the correct previous value */
	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 2 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	prev = 3;
	r = NtResetEvent( event, &prev );
	ok(r == STATUS_SUCCESS, "return wrong\n");
	ok(prev == 2, "previous state (%ld) wrong\n", prev);

	prev = 3;
	r = NtSetEvent( event, &prev );
	ok(r == STATUS_SUCCESS, "return wrong\n");
	ok(prev == 0, "previous state (%ld) wrong\n", prev);

	prev = 3;
	r = NtResetEvent( event, &prev );
	ok(r == STATUS_SUCCESS, "return wrong\n");
	ok(prev == 1, "previous state (%ld) wrong\n", prev);

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	/* what's the size of the state? */
	func = (void*) NtCreateEvent;
	r = func( &event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 0x12345678 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	prev = 3;
	r = NtSetEvent( event, &prev );
	ok(r == STATUS_SUCCESS, "return wrong\n");
	ok(prev == 0x12345678%256, "previous state (%ld) wrong\n", prev);

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	/* check a simple wait on the event works */
	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	r = NtSetEvent( event, NULL );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	r = NtWaitForSingleObject( event, FALSE, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtResetEvent( event, NULL );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	r = NtSetEvent( event, (void*)1 );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong\n");

	r = NtSetEvent( event, NULL );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	r = NtClearEvent( event );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	timeout.QuadPart = 0;
	r = NtWaitForSingleObject( event, FALSE, &timeout );
	ok(r == STATUS_TIMEOUT, "return wrong %08lx\n", r);

	prev = ~0;
	r = NtSetEvent( event, &prev );
	ok(r == STATUS_SUCCESS, "return wrong\n");
	ok(prev == 0, "previous state wrong\n");

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong\n");
}

void test_named_event( void )
{
	NTSTATUS r;
	HANDLE original, openned;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR name[] = L"\\test_event";

	us.Buffer = name;
	us.Length = sizeof name - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenEvent( &openned, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	r = NtCreateEvent( &original, EVENT_ALL_ACCESS, &oa, NotificationEvent, 0 );
	ok( r == STATUS_SUCCESS, "failed to create event\n");

	r = NtOpenEvent( &openned, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_SUCCESS, "failed to open event\n");

	r = NtClose( openned );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	r = NtClose( original );
	ok(r == STATUS_SUCCESS, "return wrong\n");

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = NULL;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenEvent( &original, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	us.Buffer = NULL;
	us.Length = 0;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenEvent( &original, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "failed to open event (%08lx)\n", r);

	us.Buffer = L"";
	us.Length = 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenEvent( &original, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "failed to open event (%08lx)\n", r);

	us.Buffer = L"\\";
	us.Length = 4;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenEvent( &original, EVENT_ALL_ACCESS, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong (%08lx)\n", r);
}

void test_access_event( void )
{
	HANDLE event = 0;
	NTSTATUS r;

	r = NtCreateEvent( &event, 0, NULL, NotificationEvent, 2 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	r = NtResetEvent( event, NULL );
	ok(r == STATUS_ACCESS_DENIED, "return wrong\n");

	r = NtSetEvent( event, NULL );
	ok(r == STATUS_ACCESS_DENIED, "return wrong\n");

	r = NtClearEvent( event );
	ok(r == STATUS_ACCESS_DENIED, "return wrong\n");

	r = NtPulseEvent( event, NULL );
	ok(r == STATUS_ACCESS_DENIED, "return wrong\n");

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong\n");
}

void NtProcessStartup( void )
{
	log_init();
	test_event();
	test_named_event();
	test_access_event();
	log_fini();
}
