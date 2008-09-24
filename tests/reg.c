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

void test_open_key( void )
{
	NTSTATUS r;
	HANDLE key = 0;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR str[] = L"\\Registry";
	WCHAR str2[] = L"\\REGISTRY";

	r = NtOpenKey( NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtOpenKey( &key, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	oa.Length = 0;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	us.Buffer = NULL;
	us.Length = 0;
	us.MaximumLength = 0;

	oa.ObjectName = &us;

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	oa.Length = FIELD_OFFSET( OBJECT_ATTRIBUTES, Attributes );

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	oa.Length = FIELD_OFFSET( OBJECT_ATTRIBUTES, SecurityDescriptor );

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	oa.Length = FIELD_OFFSET( OBJECT_ATTRIBUTES, SecurityQualityOfService );

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	oa.Length = sizeof oa + 4;

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

#if 0
	oa.Length = sizeof oa;

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "wrong return %08lx\n", r);

	us.Buffer = L"";

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "wrong return %08lx\n", r);

	us.Length = 2;

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_OBJECT_PATH_SYNTAX_BAD, "wrong return %08lx\n", r);

	us.Buffer = L"\\";

	r = NtOpenKey( &key, 0, &oa );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "wrong return %08lx\n", r);

	r = NtOpenKey( &key, GENERIC_ALL, &oa );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "wrong return %08lx\n", r);
#endif

	us.Buffer = str;
	us.Length = sizeof str - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenKey( &key, GENERIC_ALL, &oa );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "wrong return %08lx\n", r);

	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenKey( &key, GENERIC_ALL, &oa );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtClose( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	us.Buffer = str2;
	us.Length = sizeof str2 - 2;

	r = NtOpenKey( &key, GENERIC_ALL, &oa );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtClose( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenKey( &key, GENERIC_ALL, &oa );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtClose( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
}

void test_create_registry_directory( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR str[] = L"\\REGISTRY";
	WCHAR str2[] = L"\\REGISTRY\\Machine";
	HANDLE handle;
	NTSTATUS r;

	us.Buffer = str;
	us.Length = sizeof str - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateDirectoryObject( &handle, STANDARD_RIGHTS_READ, &oa );
	ok( r == STATUS_OBJECT_NAME_COLLISION, "wrong return %08lx\n", r);

	us.Buffer = str2;
	us.Length = sizeof str2 - 2;

	r = NtCreateDirectoryObject( &handle, STANDARD_RIGHTS_READ, &oa );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "wrong return %08lx\n", r);
}

void test_queue_reg_val( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR keyname[] = L"\\REGISTRY\\Machine\\SOFTWARE\\ntregtest";
	WCHAR valname[] = L"xyz";
	ULONG dispos, val, sz;
	HANDLE key;
	NTSTATUS r;
	BYTE buffer[0x100];
	KEY_VALUE_FULL_INFORMATION *info;
	KEY_VALUE_PARTIAL_INFORMATION *partial;

	us.Buffer = keyname;
	us.Length = sizeof keyname - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	// create the key the first time
	dispos = 0;
	r = NtCreateKey( &key, KEY_ALL_ACCESS, &oa, 0, NULL, 0, &dispos );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( dispos == REG_CREATED_NEW_KEY, "dispos wrong %ld\n", dispos);

	r = NtClose( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	// create the key the first time
	dispos = 0;
	r = NtCreateKey( &key, KEY_ALL_ACCESS, &oa, 0, NULL, 0, &dispos );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( dispos == REG_OPENED_EXISTING_KEY, "dispos wrong %ld\n", dispos);

	us.Buffer = valname;
	us.Length = sizeof valname - 2;

	r = NtSetValueKey( key, &us, 0, REG_DWORD, &val, sizeof val );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtQueryValueKey( NULL, &us, KeyValueFullInformation, NULL, 0, NULL );
	ok( r == STATUS_INVALID_HANDLE, "wrong return %08lx\n", r);

	r = NtQueryValueKey( NULL, &us, -1, NULL, 0, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	r = NtQueryValueKey( NULL, NULL, KeyValueFullInformation, NULL, 0, NULL );
	ok( r == STATUS_INVALID_HANDLE, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, NULL, KeyValueFullInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValueFullInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 0x20, "wrong size %08lx\n", sz);

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, 0x10, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 0x20, "wrong size %08lx\n", sz);

	memset( buffer, 0x44, sizeof buffer );
	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x20, "wrong size %08lx\n", sz);

	info = (void*) buffer;
	ok( info->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( info->Type == REG_DWORD, "Type wrong\n" );
	ok( info->DataOffset == FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name ) + sizeof valname,
		"DataOffset wrong (%ld)\n", info->DataOffset );
	ok( info->DataLength == sizeof val, "DataLength wrong\n" );
	ok( info->NameLength == us.Length, "NameLength wrong\n" );
	ok( !memcmp( info->Name, valname, sizeof valname-2 ), "Name wrong\n" );
	ok( !memcmp( &buffer[ info->DataOffset ], &val, sizeof val ), "Data wrong\n" );

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == sizeof val, "DataLength wrong\n" );
	ok( !memcmp( partial->Data, &val, sizeof val ), "Data wrong\n" );

	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, buffer, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	sz = 0;
	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, NULL, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	memset( buffer, 0, sizeof buffer );
	sz = 0;
	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == sizeof val, "DataLength wrong\n" );
	ok( !memcmp( partial->Data, &val, sizeof val ), "Data wrong\n" );

	sz = 1;
	r = NtEnumerateValueKey( key, 1, KeyValuePartialInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);
	ok( sz == 1, "wrong size %08lx\n", sz);

	sz = 1;
	r = NtEnumerateValueKey( key, 1, KeyValuePartialInformation, NULL, 0, &sz );
	ok( r == STATUS_NO_MORE_ENTRIES, "wrong return %08lx\n", r);
	ok( sz == 1, "wrong size %08lx\n", sz);

	buffer[0] = 'x';
	buffer[1] = 0;
	r = NtSetValueKey( key, &us, 0, REG_DWORD, buffer, 1 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset( buffer, 0xff, sizeof buffer );
	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 1, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 13, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0xffffffff, "TitleIndex wrong\n" );
	ok( partial->Type == 0xffffffff, "Type wrong\n" );
	ok( partial->DataLength == 0xffffffff, "DataLength wrong %ld\n", partial->DataLength );
	ok( partial->Data[0] == 0xff, "Data wrong %d\n", partial->Data[0] );

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 0x10, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 13, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == 1, "DataLength wrong %ld\n", partial->DataLength );
	ok( partial->Data[0] == 'x', "Data wrong %d\n", partial->Data[0] );

	memset( buffer, 'x', sizeof buffer );
	r = NtSetValueKey( key, &us, 0, REG_DWORD, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset(buffer, 0, sizeof buffer);
	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 0x20, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( !memcmp( partial->Data, "xxxxxxxxxxxxxxxx", 0x10 ), "buffer wrong\n");

	memset( buffer, 'x', sizeof buffer );
	r = NtSetValueKey( key, &us, 0, REG_SZ, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset(buffer, 0, sizeof buffer);
	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, 0x20, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_SZ, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( !memcmp( partial->Data, "xxxxxxxxxxxxxxxx", 0x10 ), "buffer wrong\n");

	r = NtDeleteValueKey( key, &us );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset(buffer, 'x', sizeof buffer);
	r = NtSetValueKey( key, &us, 0, REG_BINARY, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_BINARY, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( !memcmp( partial->Data, "xxxxxxxxxxxxxxxx", 0x10 ), "buffer wrong\n");

	sz = 0;
	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x2c, "wrong size %08lx\n", sz);

	info = (void*) buffer;
	ok( info->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( info->Type == REG_BINARY, "Type wrong\n" );
	ok( info->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( info->DataOffset == FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name ) + sizeof valname,
		"DataOffset wrong (%ld)\n", info->DataOffset );
	ok( !memcmp( info->Name, "x\0y\0z\0xx", sizeof valname ), "Name wrong\n" );
	ok( !memcmp( &buffer[ info->DataOffset ], "xxxxxxxxxxxxxxxx", 16), "Data wrong\n" );

	memcpy( buffer, L"this val", 0x12 );
	r = NtSetValueKey( key, &us, 0, REG_BINARY, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_BINARY, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( !memcmp( partial->Data, L"this val", 0x10 ), "buffer wrong\n");

	memcpy( buffer, L"this value", 0x14 );
	r = NtSetValueKey( key, &us, 0, REG_BINARY, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_BINARY, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );
	ok( !memcmp( partial->Data, L"this val", 0x10 ), "buffer wrong\n");

	memset( buffer, 0xee, 0x10 );
	r = NtSetValueKey( key, &us, 0, 0xfeedbeef, buffer, 0x10 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset( buffer, 0, sizeof buffer );
	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x1c, "wrong size %08lx\n", sz);
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == 0xfeedbeef, "Type wrong\n" );
	ok( partial->DataLength == 0x10, "DataLength wrong %ld\n", partial->DataLength );

	r = NtDeleteValueKey( key, &us );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtDeleteKey( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtClose( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
}

int check_bytes_zero( BYTE *buf, size_t len )
{
	while (len)
		if (buf[--len])
			 return 0;
	return 1;
}

// does NtEnumerateValueKey return multiple values?
void test_reg_query_val( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us, cls;
	WCHAR keyname[] = L"\\REGISTRY\\Machine\\SOFTWARE\\ntregtest";
	WCHAR valname[] = L"v1";
	WCHAR clsname[] = L"c1";
	ULONG dispos, val, sz;
	HANDLE key;
	NTSTATUS r;
	BYTE buffer[0x100];
	KEY_VALUE_PARTIAL_INFORMATION *partial;
	KEY_FULL_INFORMATION *keyfull;

	us.Buffer = keyname;
	us.Length = sizeof keyname - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	cls.Buffer = clsname;
	cls.Length = sizeof clsname - 2;
	cls.MaximumLength = 0;

	r = NtCreateKey( &key, KEY_ALL_ACCESS, &oa, 0, &cls, 0, &dispos );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	us.Buffer = valname;
	us.Length = sizeof valname - 2;

	val = 1;
	r = NtSetValueKey( key, &us, 0, REG_DWORD, &val, sizeof val );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	valname[1]++;

	val = 2;
	r = NtSetValueKey( key, &us, 0, REG_DWORD, &val, sizeof val );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	memset(buffer, 0, sizeof buffer);
	sz = 0;
	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == sizeof val, "DataLength wrong %ld\n", partial->DataLength );
	ok( partial->Data[0] == 1, "value wrong\n");
	ok( check_bytes_zero( &partial->Data[1], sizeof buffer - sz ), "buffer not zero\n");

	memset(buffer, 0, sizeof buffer);
	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, buffer, 0, &sz );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);
	ok( partial->Type == 0, "Type wrong\n" );

	memset(buffer, 0, sizeof buffer);
	r = NtEnumerateValueKey( key, 0, KeyValuePartialInformation, buffer, FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data ), &sz );
	ok( r == STATUS_BUFFER_OVERFLOW, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == sizeof val, "DataLength wrong %ld\n", partial->DataLength );
	ok( partial->Data[0] == 0, "value wrong\n");
	ok( check_bytes_zero( &partial->Data[1], sizeof buffer - sz ), "buffer not zero\n");

	sz = 0;
	r = NtEnumerateValueKey( key, 1, KeyValuePartialInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( sz == 0x10, "wrong size %08lx\n", sz);

	partial = (void*) buffer;
	ok( partial->TitleIndex == 0, "TitleIndex wrong\n" );
	ok( partial->Type == REG_DWORD, "Type wrong\n" );
	ok( partial->DataLength == sizeof val, "DataLength wrong %ld\n", partial->DataLength );
	ok( partial->Data[0] == 2, "value wrong\n");
	ok( check_bytes_zero( &partial->Data[1], sizeof buffer - sz ), "buffer not zero\n");

	memset(buffer, 0, sizeof buffer);
	sz = 0;
	r = NtQueryKey( key, KeyFullInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	keyfull = (void*) buffer;
	ok( keyfull->ClassOffset == FIELD_OFFSET( KEY_FULL_INFORMATION, Class ), "ClassOffset wrong %ld\n", keyfull->ClassOffset );
	ok( keyfull->ClassLength == cls.Length, "ClassLength wrong %ld\n", keyfull->ClassLength );
	ok( keyfull->SubKeys == 0, "ClassOffset wrong %ld\n", keyfull->SubKeys );
	ok( keyfull->MaxNameLen == 0, "MaxNameLen wrong %ld\n", keyfull->MaxNameLen );
	ok( keyfull->MaxClassLen == 0, "MaxClassLen wrong %ld\n", keyfull->MaxClassLen );
	ok( keyfull->Values == 2, "Values wrong %ld\n", keyfull->Values );
	ok( keyfull->MaxValueNameLen == us.Length, "MaxValueNameLen wrong %ld\n", keyfull->MaxValueNameLen );
	ok( keyfull->MaxValueDataLen == sizeof val, "MaxValueDataLen wrong %ld\n", keyfull->MaxValueDataLen );

	r = NtDeleteValueKey( key, &us );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	valname[1]--;

	r = NtDeleteValueKey( key, &us );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtDeleteKey( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	NtClose( key );
}

void test_reg_missing_val( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR keyname[] = L"\\REGISTRY\\Machine\\SOFTWARE\\ntregtest";
	WCHAR valname[] = L"non-existant";
	ULONG dispos, sz;
	HANDLE key;
	NTSTATUS r;
	BYTE buffer[0x100];

	us.Buffer = keyname;
	us.Length = sizeof keyname - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateKey( &key, KEY_ALL_ACCESS, &oa, 0, NULL, 0, &dispos );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	us.Buffer = valname;
	us.Length = sizeof valname - 2;

	sz = 0xff;
	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "wrong return %08lx\n", r);
	ok( sz == 0xff, "wrong size %08lx\n", sz);

	r = NtQueryValueKey( key, &us, KeyValueFullInformation, NULL, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtQueryValueKey( key, &us, KeyValuePartialInformation, NULL, 0, &sz );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "wrong return %08lx\n", r);

	us.Buffer = NULL;
	us.Length = 0;

	r = NtQueryValueKey( key, &us, KeyValueFullInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "wrong return %08lx\n", r);

	r = NtDeleteKey( key );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	NtClose( key );
}

void NtProcessStartup( void )
{
	log_init();

	test_open_key();
	//test_create_registry_directory();
	test_queue_reg_val();
	test_reg_query_val();
	test_reg_missing_val();

	log_fini();
}
