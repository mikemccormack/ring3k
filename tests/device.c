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

WCHAR randev[] = L"\\Device\\KsecDD";

void test_open_random_number_device(void)
{
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	HANDLE file = 0;
	NTSTATUS r;

	init_oa( &oa, &us, randev );

	r = NtOpenFile( &file, SYNCHRONIZE | FILE_READ_DATA, &oa, &iosb,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_ALERT );
	ok( r == STATUS_SUCCESS, "failed to open file %08lx\n", r);

	r = NtClose( file );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_open_random_number_device();
	log_fini();
}
