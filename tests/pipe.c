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

static WCHAR pipename[] = L"\\??\\PIPE\\test";

void test_create_pipe( void )
{
	IO_STATUS_BLOCK iosb;
	HANDLE pipe = 0;
	NTSTATUS r;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	LARGE_INTEGER timeout;

	r = NtCreateNamedPipeFile(0,0,0,0,0,0,0,0,0,0,0,0,0,0);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	us.Buffer = pipename;
	us.Length = sizeof pipename - 2;
	us.MaximumLength = us.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	iosb.Status = 0;
	iosb.Information = 0;

	// timeout should be negative
	timeout.QuadPart = 100000LL;
	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0x800, 0x800, &timeout );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	timeout.QuadPart = -100000LL;

	// test bad parameters
	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile( 0, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				0, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				0, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	// test good parameters
	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( pipe );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void pipe_client( PVOID param )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	IO_STATUS_BLOCK iosb;
	HANDLE client = 0;
	NTSTATUS r;
	//ULONG i;

	us.Buffer = pipename;
	us.Length = sizeof pipename - 2;
	us.MaximumLength = us.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenFile( &client, GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// wait named pipe here

#if 0
	for (i=0; i<10; i++)
	{
		r = NtWriteFile( client, 0, 0, 0, &iosb, &i, sizeof i, 0, 0 );
		if (r != STATUS_SUCCESS)
			break;
	}

	r = NtClose( client );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
#endif

	NtTerminateThread( NtCurrentThread(), 0 );
}

void test_pipe_server( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	IO_STATUS_BLOCK iosb;
	HANDLE pipe = 0, thread = 0;
	NTSTATUS r;
	CLIENT_ID id;
	LARGE_INTEGER timeout;
	//ULONG i;
	//BYTE buf[0x10];

	us.Buffer = pipename;
	us.Length = sizeof pipename - 2;
	us.MaximumLength = us.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	timeout.QuadPart = -10000LL;
	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, &pipe_client, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	// connect named pipe here

#if 0
	for (i=0; i<10; i++)
	{
		r = NtReadFile( pipe, 0, 0, 0, &iosb, buf, sizeof buf, 0, 0 );
		if (r != STATUS_SUCCESS)
		{
			dprintf("read returned %08lx\n", r);
			break;
		}
	}
	ok (i == 10, "wrong number of reads %ld\n", i);
#endif

	r = NtWaitForSingleObject( thread, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( pipe );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_create_pipe();
	test_pipe_server();
	log_fini();
}
