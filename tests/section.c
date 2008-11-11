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

// test various combinations of invalid parameters to NtCreateSection
void test_createsection_invalid( void )
{
	NTSTATUS r;
	LARGE_INTEGER sz;
	HANDLE handle = 0;

	sz.QuadPart = 0;

	r = NtCreateSection( NULL, 0, NULL, NULL, 0, 0, NULL );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, 0, NULL, (void*) 1, 0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, 0, NULL, &sz, 0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_IMAGE, 0 );
	ok( r == STATUS_ACCESS_VIOLATION, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_FILE, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, NULL, 0, SEC_FILE, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_BASED, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_NOCHANGE, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, 0, ~0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, PAGE_READONLY, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( NULL, SECTION_ALL_ACCESS, NULL, &sz, ~0, 0, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_IMAGE, 0 );
	ok( r == STATUS_INVALID_PAGE_PROTECTION, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, &sz, 0, SEC_BASED, 0 );
	ok( r == STATUS_INVALID_PARAMETER_6, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, 0, SEC_IMAGE, 0 );
	ok( r == STATUS_INVALID_PAGE_PROTECTION, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_READONLY, SEC_IMAGE, 0 );
	ok( r == STATUS_INVALID_FILE_FOR_SECTION, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_READONLY, SEC_IMAGE, (HANDLE)0x8000 );
	ok( r == STATUS_INVALID_HANDLE, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_READONLY, SEC_RESERVE, 0 );
	ok( r == STATUS_INVALID_PARAMETER_4, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, NULL, PAGE_EXECUTE, SEC_IMAGE, 0 );
	ok( r == STATUS_INVALID_FILE_FOR_SECTION, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_NOACCESS, SEC_RESERVE, 0 );
	ok( r == STATUS_INVALID_PAGE_PROTECTION, "return code wrong %08lx\n", r);

	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_EXECUTE, SEC_RESERVE, 0 );
	ok( r == STATUS_INVALID_PARAMETER_4, "return code wrong %08lx\n", r);
}

void test_createsection( void )
{
	LARGE_INTEGER sz;
	HANDLE handle = (HANDLE)0xf00baa;
	SECTION_BASIC_INFORMATION basic;
	NTSTATUS r;
	ULONG len, view_sz;
	PVOID base = NULL;

	// this one should succeed
	sz.QuadPart = 0x1000;
	r = NtCreateSection( &handle, 0, NULL, &sz, PAGE_EXECUTE, SEC_RESERVE, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

#if 0  /// FIXME: when permissions work correctly
	// but we can't query the section because we lack permissions
	memset( &basic, 0xff, sizeof basic );
	len = 0;
	r = NtQuerySection( handle, SectionBasicInformation, &basic, sizeof basic, &len );
	ok( r == STATUS_ACCESS_DENIED, "return code wrong %08lx\n", r);
	ok( len == 0, "length wrong\n");
#endif

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	// this one should succeed
	sz.QuadPart = 0x1000;
	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, &sz, PAGE_EXECUTE, SEC_RESERVE, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtQuerySection( handle, SectionBasicInformation, &basic, sizeof basic, &len );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
	ok( len == sizeof basic, "size didn't match\n");

	ok( basic.BaseAddress == NULL, "base address not NULL(%p)\n", basic.BaseAddress );
	ok( basic.Attributes == SEC_RESERVE, "attributes wrong(%08lx)\n", basic.Attributes );
	ok( basic.Size.QuadPart == sz.QuadPart, "size didn't match\n" );

	r = NtMapViewOfSection( handle, NtCurrentProcess(), &base, 0, 0x1000, 0, 0, 1, 0, PAGE_EXECUTE );
	ok( r == STATUS_ACCESS_VIOLATION, "return code wrong %08lx\n", r);

	r = NtUnmapViewOfSection( handle, base );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "return code wrong %08lx\n", r);

	view_sz = 0;
	r = NtMapViewOfSection( handle, NtCurrentProcess(), &base, 0, 0x1000, 0, &view_sz, 1, 0, PAGE_EXECUTE );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtUnmapViewOfSection( NtCurrentProcess(), base );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
}

void test_createsection_commit( void )
{
	LARGE_INTEGER sz;
	HANDLE handle = 0;
	NTSTATUS r;

	sz.QuadPart = 0x1000;
	r = NtCreateSection( &handle, SECTION_ALL_ACCESS, NULL, &sz, PAGE_EXECUTE_READWRITE, SEC_COMMIT, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
	ok( handle != 0, "section zero\n");

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
}

void test_createsection_objectname(void)
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING name;
	NTSTATUS r;
	HANDLE file, section;
	IO_STATUS_BLOCK iosb;

	// FIXME: write a file
	init_us( &name, L"\\??\\c:\\winnt\\system32\\c_1252.nls" );

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenFile(&file, FILE_READ_DATA | SYNCHRONIZE, &oa, &iosb,
				   FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = NULL;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	section = NULL;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, &oa, 0, PAGE_READONLY, SEC_COMMIT, file);
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtClose( section );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	section = NULL;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, 0, 0, PAGE_READONLY, SEC_COMMIT, file);
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtClose( section );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	NtClose( file );
}

void test_anonymous_section( void )
{
	NTSTATUS r;
	LARGE_INTEGER sz;
	HANDLE section;

	// create a section the same way as csrss.exe
	sz.QuadPart = 0x10000;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, 0, &sz,
						 PAGE_EXECUTE_READWRITE, SEC_BASED|SEC_RESERVE, 0);
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);

	r = NtClose( section );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
}

void test_nls_section( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING name;
	HANDLE handle = 0;
	NTSTATUS r;

	name.Buffer = L"\\NLS\\NlsSectionSortkey00000409";
	name.Length = 30*2;
	name.MaximumLength = 0;

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenSection(0, 0, 0);
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtOpenSection( &handle, SECTION_MAP_READ, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "return wrong %08lx\n", r);

	//r = NtClose( handle );
	//ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

// create the NLS directory, as it may not exist
void create_nls_dir( void )
{
	HANDLE root = 0;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR nlsdir[] = L"\\NLS";

	us.Length = sizeof nlsdir - 2;
	us.Buffer = nlsdir;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	NtCreateDirectoryObject( &root, GENERIC_READ, &oa );
}

void NtProcessStartup( void )
{
	log_init();
	create_nls_dir();
	test_createsection_invalid();
	test_createsection();
	test_createsection_commit();
	test_createsection_objectname();
	test_anonymous_section();
	test_nls_section();
	log_fini();
}
