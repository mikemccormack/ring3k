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
#include "log.h"

void test_open_process( void )
{
	NTSTATUS r;
	CLIENT_ID id;
	HANDLE handle, dummy = (void*) 0xf0de;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;

	r = NtOpenProcess( NULL, 0, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	r = NtOpenProcess( NULL, 0, NULL, &id );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	id.UniqueProcess = 0;
	id.UniqueThread = 0;
	r = NtOpenProcess( &handle, 0, NULL, &id );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	oa.Length = 0;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	oa.Length = sizeof oa;
	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	us.Buffer = NULL;
	us.Length = 0;
	us.MaximumLength = 0;
	oa.ObjectName = &us;

	r = NtOpenProcess( &handle, 0, &oa, NULL );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "wrong return %08lx\n", r );

	oa.Length = 0;
	r = NtOpenProcess( &handle, 0, &oa, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	id.UniqueProcess = dummy;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	oa.ObjectName = NULL;
	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	r = NtOpenProcess( &handle, 0, NULL, &id );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	oa.ObjectName = &us;
	us.Buffer = L"\\Process\\non-existant";
	us.Length = 21*2;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	id.UniqueProcess = 0;

#if 0
	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	r = NtOpenProcess( &handle, 0, &oa, NULL );
	ok( r == STATUS_OBJECT_PATH_NOT_FOUND, "wrong return %08lx\n", r );
#endif

	id.UniqueProcess = (void*) 0x10000;
	id.UniqueThread = (void*) 0x10000;

	r = NtOpenProcess( &handle, 0, NULL, &id );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	memset( &oa, 0, sizeof oa );
	id.UniqueProcess = dummy;
	id.UniqueThread = dummy;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_CID, "wrong return %08lx\n", r );

	oa.Length = 1;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_CID, "wrong return %08lx\n", r );

	oa.Length = sizeof oa;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_CID, "wrong return %08lx\n", r );

	id.UniqueProcess = 0;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_CID, "wrong return %08lx\n", r );

	oa.ObjectName = &us;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	oa.Length = 0;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	id.UniqueProcess = 0;
	id.UniqueThread = 0;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER_MIX, "wrong return %08lx\n", r );

	oa.ObjectName = NULL;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	oa.Length = sizeof oa;

	r = NtOpenProcess( &handle, 0, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );

	r = NtOpenProcess( &handle, PROCESS_DUP_HANDLE, &oa, &id );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r );
}

void test_open_process_param_size( void )
{
	NTSTATUS r;
	PVOID p = 0;
	PCLIENT_ID id;
	ULONG sz = 0x1000;
	OBJECT_ATTRIBUTES oa;
	HANDLE handle = 0, dummy = (void*) 1;

	r = NtAllocateVirtualMemory(NtCurrentProcess(), &p, 0, &sz, MEM_COMMIT, PAGE_READWRITE);
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	memset( &oa, 0, sizeof oa );

	id = (p + sz - 4);
	id->UniqueProcess = dummy;
	r = NtOpenProcess( &handle, 0, &oa, id );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	id = (p + sz - 8);
	id->UniqueProcess = dummy;
	id->UniqueThread = dummy;
	r = NtOpenProcess( &handle, 0, &oa, id );
	ok( r == STATUS_INVALID_CID, "wrong return %08lx\n", r );

	r = NtFreeVirtualMemory(NtCurrentProcess(), &p, &sz, MEM_RELEASE );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

void test_read_exception_port( void )
{
	NTSTATUS r;

	r = NtQueryInformationProcess( 0, ProcessExceptionPort, 0, 0, 0 );
	ok( r == STATUS_INVALID_INFO_CLASS, "wrong return %08lx\n", r );

	r = NtQueryInformationProcess( NtCurrentProcess(), ProcessExceptionPort, 0, 0, 0 );
	ok( r == STATUS_INVALID_INFO_CLASS, "wrong return %08lx\n", r );

	r = NtQueryInformationProcess( NtCurrentProcess(), -1, 0, 0, 0 );
	ok( r == STATUS_INVALID_INFO_CLASS, "wrong return %08lx\n", r );
}

void test_terminate_process( void )
{
	PROCESS_BASIC_INFORMATION info;
	NTSTATUS r;

	r = NtTerminateProcess( 0, STATUS_UNSUCCESSFUL );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtQueryInformationProcess( NtCurrentProcess(), ProcessBasicInformation, &info, sizeof info, 0);
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	ok( info.ExitStatus == STATUS_PENDING, "Exit code wrong %08lx\n", info.ExitStatus );
}

void NtProcessStartup( void )
{
	log_init();
	test_open_process();
	test_open_process_param_size();
	test_read_exception_port();
	test_terminate_process();
	log_fini();
}
