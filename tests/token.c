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

void test_open_thread_token( void )
{
	HANDLE token = NULL;
	NTSTATUS r;

	r = NtOpenThreadToken( 0, 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtOpenThreadToken( 0, 0, 0, &token );
	ok( r == STATUS_INVALID_HANDLE, "wrong return %08lx\n", r);

	r = NtOpenThreadToken( NtCurrentThread(), 0, 0, &token );
	ok( r == STATUS_NO_TOKEN, "wrong return %08lx\n", r);

	r = NtOpenThreadToken( NtCurrentProcess(), 0, 0, &token );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "wrong return %08lx\n", r);
}

void test_open_process_token( void )
{
	HANDLE token = NULL;
	NTSTATUS r;
	TOKEN_PRIVILEGES in, out;
	ULONG sz;

	r = NtOpenProcessToken( 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r);

	r = NtOpenProcessToken( 0, 0, &token );
	ok( r == STATUS_INVALID_HANDLE, "wrong return %08lx\n", r);

	r = NtOpenProcessToken( NtCurrentThread(), 0, &token );
	ok( r == STATUS_OBJECT_TYPE_MISMATCH, "wrong return %08lx\n", r);

	r = NtOpenProcessToken( NtCurrentProcess(), 0, &token );
	//ok( r == STATUS_ACCESS_DENIED, "wrong return %08lx\n", r);

	r = NtAdjustPrivilegesToken( NULL, 0, NULL, 0, NULL, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	// try with ADJUST
	r = NtOpenProcessToken( NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( token != NULL, "token was NULL\n");

	r = NtAdjustPrivilegesToken( token, 0, NULL, 0, NULL, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "wrong return %08lx\n", r);

	sz = sizeof out;
	memset( &out, 0, sz );

	in.PrivilegeCount = 1;
	in.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	in.Privileges[0].Luid.LowPart = SECURITY_LOCAL_SERVICE_RID;
	in.Privileges[0].Luid.HighPart = 0;
	r = NtAdjustPrivilegesToken( token, 0, &in, sizeof in, &out, &sz );
	//ok( r == STATUS_ACCESS_DENIED, "wrong return %08lx\n", r);

	r = NtAdjustPrivilegesToken( token, 0, &in, 0, 0, 0 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtAdjustPrivilegesToken( token, 0, &in, 0, 0, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	in.PrivilegeCount = 0;
	r = NtAdjustPrivilegesToken( token, 0, &in, 0, NULL, NULL );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	r = NtClose( token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	// try with ADJUST + QUERY rights
	r = NtOpenProcessToken( NtCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	ok( token != NULL, "token was NULL\n");

	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	sz = 0;
	memset( &out, 0, sz );

	in.PrivilegeCount = 1;
	in.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	in.Privileges[0].Luid.LowPart = SECURITY_LOCAL_SERVICE_RID;
	in.Privileges[0].Luid.HighPart = 0;
	r = NtAdjustPrivilegesToken( token, 0, &in, sizeof in, &out, &sz );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	//ok( sz == sizeof out, "size wrong %ld\n", sz);
	//ok( out.PrivilegeCount == 1, "priv count wrong (%ld)\n", out.PrivilegeCount);
	//ok( out.Privileges[0].Attributes == 0, "attributes wrong\n");
	//ok( out.Privileges[0].Luid.LowPart == SECURITY_LOCAL_SERVICE_RID, "luid wrong\n");
	//ok( out.Privileges[0].Luid.HighPart == 0, "luid wrong\n");

	r = NtClose( token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
}

void test_query_token( void )
{
	HANDLE token = NULL;
	NTSTATUS r;
	BYTE buffer[0x100];
	ULONG len;
	PTOKEN_USER user;
	SID *sid;

	r = NtOpenProcessToken( NtCurrentProcess(), TOKEN_QUERY, &token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);

	len = 0;
	r = NtQueryInformationToken( token, TokenUser, NULL, 0, &len );
	ok( r == STATUS_BUFFER_TOO_SMALL, "wrong return %08lx\n", r);
	//ok( len == 0x14, "wrong length %08lx\n", len);

	memset( buffer, 0, sizeof buffer );
	len = 0;
	r = NtQueryInformationToken( token, TokenUser, buffer, sizeof buffer, &len );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
	//ok( len == 0x14, "wrong length %08lx\n", len);

	user = (void*) buffer;
	ok( user->User.Attributes == 0, "Attributes wrong\n");
	ok( user->User.Sid == &buffer[8], "Sid wrong\n");

	if (0)
	{
		dprintf("%p %p %08lx\n", user->User.Sid, buffer, user->User.Attributes);

		{
			char b[0x100];
			int i;
			for( i=0; i<0x30; i++)
				sprintf(&b[i*3], "%02x%c", buffer[8+i], (i+1)%16 ? ' ' : '\n');
			dprintf("\n%s\n", b);
		}
	}

	sid = user->User.Sid;

	ok( sid->Revision == SID_REVISION, "Revision wrong\n");
	//ok( sid->SubAuthorityCount == 1, "SubAuthorityCount wrong\n");
	ok( !memcmp( &sid->IdentifierAuthority, (BYTE[]) SECURITY_NT_AUTHORITY, 6), "IdentifierAuthority wrong\n");

	r = NtClose( token );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();

	test_open_thread_token();
	test_query_token();
	test_open_process_token();

	log_fini();
}
