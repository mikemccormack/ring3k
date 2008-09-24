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

void test_sema( void )
{
	NTSTATUS r;
	HANDLE sema;
	ULONG count;

	r = NtReleaseSemaphore( 0, 0, NULL );
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtCreateSemaphore( NULL, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateSemaphore( &sema, SEMAPHORE_ALL_ACCESS, NULL, 0, 2 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 3, &count );
	ok(r == STATUS_SEMAPHORE_LIMIT_EXCEEDED, "return wrong %08lx\n", r);
	ok(count == 0xf00, "count was %08lx\n", count);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 1, &count );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(count == 0, "count was %08lx\n", count);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 0, &count );
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);
	ok(count == 0xf00, "count was %08lx\n", count);

	r = NtWaitForSingleObject( sema, 0, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 1, &count );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(count == 0, "count was %08lx\n", count);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 1, &count );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(count == 1, "count was %08lx\n", count);

	r = NtWaitForSingleObject( sema, 0, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( sema, 0, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	count = 0xf00;
	r = NtReleaseSemaphore( sema, 2, &count );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(count == 0, "count was %08lx\n", count);

	r = NtReleaseSemaphore( sema, 1, &count );
	ok(r == STATUS_SEMAPHORE_LIMIT_EXCEEDED, "return wrong %08lx\n", r);

	r = NtClose( sema );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_sema();
	log_fini();
}
