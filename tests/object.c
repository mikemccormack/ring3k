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

void test_create_directory_object(void)
{
	NTSTATUS r;
	HANDLE handle = 0;

	r = NtCreateDirectoryObject( 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateDirectoryObject( &handle, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( handle != 0, "handle was zero\n");

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

NTSTATUS try_access_dir( ACCESS_MASK access )
{
	HANDLE handle = 0;
	NTSTATUS r;
	ULONG pos;

	r = NtCreateDirectoryObject( &handle, access, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	r = NtQueryDirectoryObject( handle, 0, 0, 0, 0, &pos, 0 );
	NtClose( handle );
	return r;
}

void test_query_directory_object(void)
{
	NTSTATUS r;
	HANDLE handle = 0;
	unsigned char buffer[0x100];
	ULONG retlen = 0, pos = 0;

	handle = 0;
	r = NtCreateDirectoryObject( &handle, 0, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( handle != 0, "handle was zero\n");

	r = NtQueryDirectoryObject( 0, 0, 0, 0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQueryDirectoryObject( handle, 0, 0, 0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQueryDirectoryObject( handle, buffer, sizeof buffer, 0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQueryDirectoryObject( handle, buffer, sizeof buffer, 0, 0, &pos, 0 );
	ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	r = NtQueryDirectoryObject( handle, buffer, sizeof buffer, 0, 0, 0, &retlen );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// start again, give more access this time
	handle = 0;
	r = NtCreateDirectoryObject( &handle, DIRECTORY_ALL_ACCESS, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( handle != 0, "handle was zero\n");

	r = NtQueryDirectoryObject( handle, buffer, sizeof buffer, 0, 0, &pos, 0 );
	ok( r == STATUS_NO_MORE_ENTRIES, "return wrong %08lx\n", r);

	r = NtQueryDirectoryObject( handle, 0, 0, 0, 0, &pos, 0 );
	ok( r == STATUS_NO_MORE_ENTRIES, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// check DIRECTORY_QUERY works
	r = try_access_dir( DIRECTORY_QUERY );
	ok( r == STATUS_NO_MORE_ENTRIES, "return wrong %08lx\n", r);

	// check GENERIC_WRITE does not work
	r = try_access_dir( GENERIC_WRITE );
	ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	// check GENERIC_READ works
	r = try_access_dir( GENERIC_READ );
	ok( r == STATUS_NO_MORE_ENTRIES, "return wrong %08lx\n", r);

	// check GENERIC_ALL works
	r = try_access_dir( GENERIC_ALL );
	ok( r == STATUS_NO_MORE_ENTRIES, "return wrong %08lx\n", r);

	// check DIRECTORY_TRAVERSE
	r = try_access_dir( DIRECTORY_TRAVERSE );
	ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	// check DIRECTORY_CREATE_OBJECT
	r = try_access_dir( DIRECTORY_CREATE_OBJECT );
	ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);
}

char hex(BYTE x)
{
	if (x<10)
		return x+'0';
	return x+'A'-10;
}

void dump_bin(BYTE *buf, ULONG sz)
{
	char str[0x33];
	int i;
	for (i=0; i<sz; i++)
	{
		str[(i%16)*3] = hex(buf[i]>>4);
		str[(i%16)*3+1] = hex(buf[i]&0x0f);
		str[(i%16)*3+2] = ' ';
		str[(i%16)*3+3] = 0;
		if ((i+1)%16 == 0 || (i+1) == sz)
			dprintf("%s\n", str);
	}
}

void test_query_object_security(void)
{
	NTSTATUS r;
	HANDLE handle = 0;
	ULONG sz = 0;
	BYTE buf[0x100];
	PISECURITY_DESCRIPTOR_RELATIVE sdr;

	r = NtQuerySecurityObject( 0, 0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQuerySecurityObject( 0, ~0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQuerySecurityObject( 0, 0, 0, 0, &sz );
	ok( r == STATUS_INVALID_HANDLE, "return wrong %08lx\n", r);

	r = NtQuerySecurityObject( 0, ~0, 0, 0, &sz );
	ok( r == STATUS_INVALID_HANDLE, "return wrong %08lx\n", r);

	r = NtCreateEvent(&handle, 0, 0, 0, 0);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	//r = NtQuerySecurityObject( handle, ~0, 0, 0, &sz );
	//ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	r = NtQuerySecurityObject( handle, 0, 0, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "return wrong %08lx\n", r);
	ok( sz == 20, "size wrong %ld\n", sz);

	r = NtQuerySecurityObject( handle, 0, (PSECURITY_DESCRIPTOR) buf, sz, &sz );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( sz == 20, "size wrong %ld\n", sz);

	sdr = (void*) buf;
	ok(sdr->Revision == SECURITY_DESCRIPTOR_REVISION, "Revision wrong\n");
	ok(sdr->Sbz1 == 0, "Sbz1 wrong\n");
	ok(sdr->Control == SE_SELF_RELATIVE, "Control wrong\n");
	ok(sdr->Owner == 0, "Owner wrong\n");
	ok(sdr->Group == 0, "Group wrong\n");
	ok(sdr->Sacl == 0, "Sacl wrong\n");
	ok(sdr->Dacl == 0, "Dacl wrong\n");

	//r = NtQuerySecurityObject( handle, OWNER_SECURITY_INFORMATION, 0, 0, &sz );
	//ok( r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtCreateEvent(&handle, EVENT_ALL_ACCESS, 0, 0, 0);
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtQuerySecurityObject( handle, OWNER_SECURITY_INFORMATION, 0, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "return wrong %08lx\n", r);
	ok( sz == 20, "size wrong %ld\n", sz);

	r = NtQuerySecurityObject( handle, OWNER_SECURITY_INFORMATION, (PSECURITY_DESCRIPTOR) buf, sz, &sz );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( sz == 20, "size wrong %ld\n", sz);

	sdr = (void*) buf;
	ok(sdr->Revision == SECURITY_DESCRIPTOR_REVISION, "Revision wrong\n");
	ok(sdr->Sbz1 == 0, "Sbz1 wrong\n");
	ok(sdr->Control == SE_SELF_RELATIVE, "Control wrong\n");
	ok(sdr->Owner == 0, "Owner wrong\n");
	ok(sdr->Group == 0, "Group wrong\n");
	ok(sdr->Sacl == 0, "Sacl wrong\n");
	ok(sdr->Dacl == 0, "Dacl wrong\n");

	//dump_bin(buf, sz);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void test_named_directory( void )
{
	NTSTATUS r;
	HANDLE handle = 0, handle2 = 0;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us, us2;
	WCHAR testdir[] = L"\\testdir";
	WCHAR testdir2[] = L"\\testdir\\foo";

	us.Length = sizeof testdir - 2;
	us.Buffer = testdir;
	us.MaximumLength = 0;

	us2.Length = sizeof testdir2 - 2;
	us2.Buffer = testdir2;
	us2.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us2;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	// try again with a sub path that doesn't exist name
	//r = NtCreateDirectoryObject( &handle, 0, &oa );
	//ok( r == STATUS_OBJECT_PATH_NOT_FOUND, "return wrong %08lx\n", r);

	// try again with a valid name
	oa.ObjectName = &us;

	r = NtCreateDirectoryObject( &handle, 0, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtOpenDirectoryObject( &handle2, GENERIC_READ, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle2 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

}

BOOLEAN unicode_string_equal( PUNICODE_STRING a, PUNICODE_STRING b )
{
	ULONG i;
	if (a->Length != b->Length)
		return FALSE;
	for (i=0; i<a->Length/2; i++)
		if (a->Buffer[i] != b->Buffer[i])
			return FALSE;
	return TRUE;
}

void test_symbolic_link( void )
{
	NTSTATUS r;
	HANDLE handle = 0;
	OBJECT_ATTRIBUTES oa;
	ULONG len = 0;
	UNICODE_STRING us, target, out;
	WCHAR buffer[40];
	WCHAR testlink[] = L"\\testsymlink";
	WCHAR targetname[] = L"\\foo\\bar\\1";

	us.Length = sizeof testlink - 2;
	us.Buffer = testlink;
	us.MaximumLength = 0;

	target.Length = sizeof targetname - 2;
	target.Buffer = targetname;
	target.MaximumLength = 0;

	oa.Length = 0;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateSymbolicLinkObject( 0, 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( 0, 0, 0, &target );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( 0, 0, 0, &target );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( 0, 0, &oa, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( 0, 0, &oa, &target );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( &handle, 0, &oa, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( &handle, 0, &oa, &target );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( &handle, 0, &oa, &target );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	oa.Length = 0;
	oa.ObjectName = 0;

	r = NtCreateSymbolicLinkObject( &handle, 0, &oa, &target );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateSymbolicLinkObject( &handle, GENERIC_READ|GENERIC_WRITE, &oa, &target );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	target.MaximumLength = target.Length;

	r = NtCreateSymbolicLinkObject( &handle, GENERIC_READ, &oa, &target );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtQuerySymbolicLinkObject( 0, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtQuerySymbolicLinkObject( handle, 0, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	out.Buffer = 0;
	out.Length = 0;
	out.MaximumLength = 0;

	r = NtQuerySymbolicLinkObject( 0, &out, 0 );
	ok( r == STATUS_INVALID_HANDLE, "return wrong %08lx\n", r);

	r = NtQuerySymbolicLinkObject( handle, &out, 0 );
	ok( r == STATUS_BUFFER_TOO_SMALL, "return wrong %08lx\n", r);

	out.Buffer = buffer;
	out.MaximumLength = sizeof buffer;

	r = NtQuerySymbolicLinkObject( handle, &out, 0 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	ok( out.Length == target.Length, "Length wrong %d\n", out.Length);
	ok( out.MaximumLength == sizeof buffer, "MaximumLength wrong %d\n", out.Length);
	ok( unicode_string_equal( &out, &target ), "string wrong\n");

	out.MaximumLength = target.Length;

	r = NtQuerySymbolicLinkObject( handle, &out, &len );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	ok( len == out.Length, "length wrong\n");

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_create_directory_object();
	test_query_directory_object();
	test_query_object_security();
	test_named_directory();
	test_symbolic_link();
	log_fini();
}
