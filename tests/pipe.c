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
static WCHAR pipedev[] = L"\\DosDevices\\pipe\\";
static WCHAR waitpipe[] = L"test";

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

	r = NtCreateNamedPipeFile(&pipe,0,0,0,0,0,0,0,0,0,0,0,0,0);
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

	r = NtCreateNamedPipeFile(&pipe, 0, 0, &iosb, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	// timeout should be negative
	timeout.QuadPart = -100000LL;
	r = NtCreateNamedPipeFile(&pipe, 0, 0, &iosb, 0, 0, 0, 0, 0, 0, 0, 0, 0, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, 0, 0, 0, 0, 0, 0, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, 0, 0, 0, 0, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, 0, 0, 0, -1, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, 0, TRUE, 0, -1, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, TRUE, TRUE, 0, -1, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa, &iosb, 0, 0, 0, TRUE, TRUE, 0, -1, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				&iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, 0, TRUE,
				TRUE, 0, -1, 0x800, 0x800, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				&iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, 0, -1, 0, 0, &timeout);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	NtClose( pipe );

	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				&iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, 0, 0, 0, 0, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				&iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, 0,
				0, 0, 0, 0, 0, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	// no sharing
	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				&iosb, 0, FILE_OPEN_IF, 0, TRUE,
				TRUE, 0, -1, 0, 0, &timeout);
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	// no io status block
	r = NtCreateNamedPipeFile(&pipe, 0, &oa,
				0, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, 0, -1, 0, 0, &timeout);
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

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

NTSTATUS try_create_pipe( PWSTR name )
{
	IO_STATUS_BLOCK iosb;
	HANDLE pipe = 0;
	NTSTATUS r;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	LARGE_INTEGER timeout;

	init_oa( &oa, &us, name );
	us.MaximumLength = us.Length;

	iosb.Status = 0;
	iosb.Information = 0;

	timeout.QuadPart = -100000LL;

	r = NtCreateNamedPipeFile( &pipe, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE,
				&oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_OPEN_IF, 0, TRUE,
				TRUE, FALSE, -1, 0, 0, &timeout );
	if (r == STATUS_SUCCESS)
		r = NtClose( pipe );

	return r;
}

void test_create_pipe_names( void )
{
	NTSTATUS r;

	r = try_create_pipe( L"\\" );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong %08lx\n", r);

	//r = try_create_pipe( pipedev );
	//ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	r = try_create_pipe( waitpipe );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	//r = try_create_pipe( L"\\foobar" );
	//ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	r = try_create_pipe( L"\\device\\namedpipe\\foobar" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	//r = try_create_pipe( L"\\device\\namedpipe\\foo\\bar" );
	//ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = try_create_pipe( L"\\device\\namedpipe\\&" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = try_create_pipe( L"\\??\\PIPE\\xyz" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	//r = try_create_pipe( L"\\??\\xyz" );
	//ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	//r = try_create_pipe( L"\\device\\namedpipe\\foobar\\" );
	//ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void pipe_client( PVOID param )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	IO_STATUS_BLOCK iosb;
	HANDLE client = 0, event = 0, device = 0;
	NTSTATUS r;
	LARGE_INTEGER timeout;
	ULONG i;
	BYTE buf[0x100];
	PFILE_PIPE_WAIT_FOR_BUFFER pwfb;
	ULONG len;

	us.Buffer = pipedev;
	us.Length = sizeof pipedev - 2;
	us.MaximumLength = us.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	// wait named pipe here
	timeout.QuadPart = -10000LL;
	r = NtOpenFile( &device, SYNCHRONIZE | FILE_READ_ATTRIBUTES, &oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DEVICE_IS_MOUNTED );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	pwfb = (void*) buf;
	pwfb->Timeout.QuadPart = -10000LL;
	pwfb->NameLength = sizeof waitpipe - 2;
	pwfb->TimeoutSpecified = FALSE;
	memcpy( pwfb->Name, waitpipe, pwfb->NameLength );

	len = FIELD_OFFSET( FILE_PIPE_WAIT_FOR_BUFFER, Name );
	len += pwfb->NameLength;

	r = NtFsControlFile( device, 0, 0, 0, &iosb, FSCTL_PIPE_WAIT, 0, 0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER, "failed to wait %08lx\n", r );

	r = NtFsControlFile( device, 0, 0, 0, &iosb, FSCTL_PIPE_WAIT, pwfb, len, 0, 0 );
	ok( r == STATUS_SUCCESS, "failed to wait %08lx\n", r );

	r = NtClose( device );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	us.Buffer = pipename;
	us.Length = sizeof pipename - 2;
	us.MaximumLength = us.Length;

	r = NtOpenFile( &client, GENERIC_READ | GENERIC_WRITE, &oa, &iosb, FILE_SHARE_READ|FILE_SHARE_WRITE, 0 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	for (i=0; i<10; i++)
	{
		ULONG val = 0;

		// write a number
		r = NtWriteFile( client, event, 0, 0, &iosb, &i, sizeof i, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "write %ld returned %08lx\n", i, r);

		// read a reply
		r = NtReadFile( client, event, 0, 0, &iosb, &val, sizeof val, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "read %ld returned %08lx\n", i, r);

		ok( val == (i|0x100), "value wrong\n");
	}

	r = NtClose( client );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( event );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), 0 );
}

void test_pipe_server( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	IO_STATUS_BLOCK iosb;
	HANDLE pipe = 0, thread = 0, event = 0;
	NTSTATUS r;
	CLIENT_ID id;
	LARGE_INTEGER timeout;
	ULONG i;

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

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, NotificationEvent, 0 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, &pipe_client, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	// connect named pipe here
	r = NtFsControlFile( pipe, event, 0, 0, &iosb, FSCTL_PIPE_LISTEN, 0, 0, 0, 0 );
	if (r == STATUS_PENDING)
		r = NtWaitForSingleObject( event, TRUE, 0 );
	ok( r == STATUS_SUCCESS, "failed to listen %08lx\n", r );

	for (i=0; i<10; i++)
	{
		ULONG val = 0;

		r = NtReadFile( pipe, event, 0, 0, &iosb, &val, sizeof val, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "read %ld returned %08lx\n", i, r);

		ok( val == i, "sequence wrong\n");

		val |= 0x100;

		r = NtWriteFile( pipe, event, 0, 0, &iosb, &val, sizeof val, 0, 0 );
		if (r == STATUS_PENDING)
			r = NtWaitForSingleObject( event, TRUE, 0 );
		ok (r == STATUS_SUCCESS, "write %ld returned %08lx\n", i, r);
	}
	ok (i == 10, "wrong number of reads %ld\n", i);

	r = NtFsControlFile( pipe, event, 0, 0, &iosb, FSCTL_PIPE_DISCONNECT, 0, 0, 0, 0 );
	if (r == STATUS_PENDING)
		r = NtWaitForSingleObject( event, TRUE, 0 );
	ok( r == STATUS_SUCCESS, "failed to disconnect %08lx\n", r );

	r = NtWaitForSingleObject( thread, TRUE, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( pipe );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void create_link( PWSTR linkname, PWSTR targetname )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us, target;
	HANDLE symlink;

	init_oa( &oa, &us, linkname );
	init_us( &target, targetname );
	target.MaximumLength = target.Length;

	NtCreateSymbolicLinkObject( &symlink, DIRECTORY_ALL_ACCESS, &oa, &target );
}

void NtProcessStartup( void )
{
	log_init();
	create_link( L"\\??\\PIPE", L"\\Device\\NamedPipe" );
	test_create_pipe();
	test_create_pipe_names();
	test_pipe_server();
	log_fini();
}
