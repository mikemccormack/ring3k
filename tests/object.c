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

HANDLE get_root( void )
{
	NTSTATUS r;
	HANDLE root = 0;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR rootdir[] = L"\\";

	us.Length = sizeof rootdir - 2;
	us.Buffer = rootdir;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenDirectoryObject( &root, GENERIC_READ, &oa );
	ok( r == 0, "return wrong %08lx\n", r);

	return root;
}

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
	HANDLE handle = 0, handle2 = 0, handle3 = 0;
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

	r = NtOpenDirectoryObject( &handle, 0, &oa );
	ok( r == STATUS_OBJECT_PATH_NOT_FOUND, "return wrong %08lx\n", r);

	// try again with a sub path that doesn't exist name
	r = NtCreateDirectoryObject( &handle, 0, &oa );
	ok( r == STATUS_OBJECT_PATH_NOT_FOUND, "return wrong %08lx\n", r);

	// try again with a valid name
	oa.ObjectName = &us;

	r = NtCreateDirectoryObject( &handle, 0, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtOpenDirectoryObject( &handle2, GENERIC_READ, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtOpenDirectoryObject( &handle3, GENERIC_READ, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle3 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle2 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

NTSTATUS open_object_dir( BOOL create, BOOL open_existing, HANDLE root, PWSTR path )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	HANDLE handle = 0;
	NTSTATUS r;

	init_us( &us, path );

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	if (open_existing)
		oa.Attributes |= OBJ_OPENIF;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	if (create)
		r = NtCreateDirectoryObject( &handle, GENERIC_READ, &oa );
	else
		r = NtOpenDirectoryObject( &handle, GENERIC_READ, &oa );

	if (r >= STATUS_SUCCESS)
	{
		ok( STATUS_SUCCESS == NtClose( handle ), "failed to close handle\n");
	}

	return r;
}

void test_open_object_dir( void )
{
	NTSTATUS r;
	HANDLE root = get_root();

	r = open_object_dir( FALSE, FALSE, 0, L"" );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	r = open_object_dir( FALSE, FALSE, 0, L"\\" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = open_object_dir( FALSE, FALSE, 0, L"\\\\" );
	ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	r = open_object_dir( FALSE, FALSE, root, L"\\" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = open_object_dir( FALSE, FALSE, root, L"" );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	r = open_object_dir( FALSE, FALSE, root, L"\\\\" );
	ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, 0, L"" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, (HANDLE)0xfeedface, L"" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, 0, L"\\" );
	ok( r == STATUS_OBJECT_NAME_COLLISION, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, TRUE, 0, L"\\" );
	ok( r == STATUS_OBJECT_NAME_EXISTS, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, 0, L"\\\\" );
	ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, root, L"\\" );
	ok( r == STATUS_OBJECT_NAME_COLLISION, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, TRUE, root, L"\\" );
	ok( r == STATUS_OBJECT_NAME_EXISTS, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, root, L"" );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = open_object_dir( TRUE, FALSE, root, L"\\\\" );
	ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);
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
	HANDLE handle = 0, handle2 = 0;
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

	// open a second handle
	oa.Attributes |= OBJ_OPENIF;
	r = NtOpenSymbolicLinkObject( &handle2, GENERIC_READ, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle2 );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	oa.Attributes |= OBJ_OPENLINK;
	r = NtOpenSymbolicLinkObject( &handle2, GENERIC_READ, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// reopen the object... it does not persist
	handle = 0;
	r = NtOpenSymbolicLinkObject( &handle, GENERIC_READ, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	r = NtCreateSymbolicLinkObject( &handle, 0, 0, &target );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void test_symbolic_open_link( void )
{
	WCHAR testlink[] = L"testsymlink2";
	UNICODE_STRING us;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS r;
	HANDLE link = 0;

	us.Length = sizeof testlink - 2;
	us.Buffer = testlink;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenSymbolicLinkObject( &link, GENERIC_READ, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "return wrong %08lx\n", r);

	oa.RootDirectory = get_root();

	r = NtOpenSymbolicLinkObject( &link, GENERIC_READ, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);
}

void test_symbolic_open_target( void )
{
	WCHAR testlink[] = L"\\testsymlink3";
	WCHAR testdir[] = L"\\testdir3";
	WCHAR testpath[] = L"\\testsymlink3\\testdir3";
	WCHAR relpath[] = L"xyz";
	UNICODE_STRING us, target;
	OBJECT_ATTRIBUTES oa;
	WCHAR rootdir[] = L"\\";
	NTSTATUS r;
	HANDLE link = 0, dir = 0;
	HANDLE dir2 = 0;

	us.Length = sizeof testlink - 2;
	us.Buffer = testlink;
	us.MaximumLength = us.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	target.Length = sizeof rootdir - 2;
	target.Buffer = rootdir;
	target.MaximumLength = target.Length;

	oa.ObjectName = &target;

	r = NtCreateSymbolicLinkObject( &link, GENERIC_READ, &oa, &target );
	//ok( r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong %08lx\n", r);

	oa.ObjectName = &us;

	r = NtCreateSymbolicLinkObject( &link, GENERIC_READ, &oa, &target );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	//r = NtOpenDirectoryObject( &dir, GENERIC_READ, &oa );
	//ok( r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	r = NtClose( link );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// link to self?
	r = NtCreateSymbolicLinkObject( &link, GENERIC_READ, &oa, &us );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( link );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// empty link?
	target.Length = 0;
	target.MaximumLength = 0;
	target.Buffer = 0;
	r = NtCreateSymbolicLinkObject( &link, GENERIC_READ, &oa, &target );
	ok( r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	// relative path
	target.Length = sizeof relpath - 2;
	target.MaximumLength = target.Length;
	target.Buffer = relpath;
	r = NtCreateSymbolicLinkObject( &link, GENERIC_READ, &oa, &target );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( link );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// create testdir3
	us.Buffer = testdir;
	us.Length = sizeof testdir - 2;
	us.MaximumLength = us.Length;

	r = NtCreateDirectoryObject( &dir, GENERIC_READ, &oa );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	us.Buffer = testpath;
	us.Length = sizeof testpath - 2;
	us.MaximumLength = us.Length;

	r = NtOpenDirectoryObject( &dir2, GENERIC_READ, &oa );
	ok( r == STATUS_OBJECT_PATH_NOT_FOUND, "return wrong %08lx\n", r);

	r = NtClose( dir );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_create_directory_object();
	test_query_directory_object();
	test_query_object_security();
	test_named_directory();
	test_open_object_dir();
	test_symbolic_link();
	test_symbolic_open_link();
	test_symbolic_open_target();
	log_fini();
}
