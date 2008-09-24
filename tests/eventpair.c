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

static void test_eventpair(void)
{
	NTSTATUS r;
	HANDLE eventpair;

	r = NtCreateEventPair( NULL, STANDARD_RIGHTS_ALL, NULL );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtCreateEventPair( &eventpair, STANDARD_RIGHTS_ALL, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtSetLowEventPair( eventpair );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitLowEventPair( eventpair );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtSetHighEventPair( eventpair );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtWaitHighEventPair( eventpair );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( eventpair );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
}


void NtProcessStartup( void )
{
	log_init();
	test_eventpair();
	log_fini();
}
