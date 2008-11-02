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

void test_mailslot(void)
{
	NTSTATUS r;
	HANDLE mailslot;
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;

	r = NtCreateMailslotFile( 0, 0, 0, 0, 0, 0, 0, 0);
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateMailslotFile( &mailslot, 0, 0, 0, 0, 0, 0x100, 0);
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateMailslotFile( &mailslot, 0, 0, &iosb, 0, 0, 0x100, 0);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateMailslotFile( &mailslot, FILE_ALL_ACCESS, 0, &iosb, FILE_SYNCHRONOUS_IO_NONALERT, 0x100, 0x100, 0);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	oa.Length = 0;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	oa.Length = sizeof oa;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	us.Buffer = L"\\";
	us.Length = 2;
	us.MaximumLength = 0;

	oa.ObjectName = &us;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong %08lx\n", r);

	us.Buffer = L"\\mailslot\\foo";
	us.Length = 13;
	us.MaximumLength = 0;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	us.Buffer = L"\\??\\mailslot\\foo";
	us.Length = 16;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	r = NtCreateMailslotFile( &mailslot, 0, &oa, &iosb, FILE_CREATE, 0, 0, 0);
	ok(r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	r = NtCreateMailslotFile( &mailslot, FILE_ALL_ACCESS, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	us.Buffer = L"\\??\\mailslot\\foo";
	us.Length = 16*2;

	r = NtCreateMailslotFile( &mailslot, FILE_ALL_ACCESS, &oa, &iosb, 0, 0, 0, 0);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( mailslot );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

WCHAR mailslot_name[] = L"\\??\\mailslot\\foo";

void mailslot_client( PVOID param )
{
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	HANDLE client, event;
	UNICODE_STRING us;
	NTSTATUS r;
	ULONG i;

	us.Buffer = mailslot_name;
	us.Length = sizeof mailslot_name - 2;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenFile( &client, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	for (i=0; i<10; i++)
	{
		// write a number
		r = NtWriteFile( client, event, 0, 0, &iosb, &i, sizeof i, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "write %ld returned %08lx\n", i, r);
	}

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( client );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), 0 );
}

void test_mailslot_server( void )
{
	HANDLE mailslot, thread, event;
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	NTSTATUS r;
	ULONG i;

	us.Buffer = mailslot_name;
	us.Length = sizeof mailslot_name - 2;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateMailslotFile( &mailslot, GENERIC_READ, &oa, &iosb, FILE_SHARE_WRITE, 0, 0, 0);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, &mailslot_client, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	for (i=0; i<10; i++)
	{
		ULONG val = 0;

		r = NtReadFile( mailslot, event, 0, 0, &iosb, &val, sizeof val, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "read %ld returned %08lx\n", i, r);

		ok( val == i, "value wrong\n");
	}

	r = NtClose( event );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( thread, TRUE, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( mailslot );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

// creation of \??\MAILSLOT is usually done at startup by smss.exe
// create it for tests
void create_mailslot_link( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us, target;
	WCHAR link[] = L"\\??\\MAILSLOT";
	WCHAR targetname[] = L"\\Device\\MailSlot";
	HANDLE symlink;

	us.Buffer = link;
	us.Length = sizeof link - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	target.Buffer = targetname;
	target.Length = sizeof targetname - 2;
	target.MaximumLength = target.Length;

	NtCreateSymbolicLinkObject( &symlink, DIRECTORY_ALL_ACCESS, &oa, &target );
}

void NtProcessStartup( void )
{
	log_init();
	create_mailslot_link();
	test_mailslot();
	//test_mailslot_server();
	log_fini();
}
