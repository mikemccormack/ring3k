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

#define MAGIC ((void*) 0xf00)

void test_mutant()
{
	NTSTATUS r;
	HANDLE mutant;

	r = NtCreateMutant( NULL, 0, NULL, 0 );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	mutant = MAGIC;
	r = NtCreateMutant( &mutant, 0, NULL, 0 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(mutant != MAGIC, "return wrong %08lx\n", r);

	r = NtClose( mutant );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	mutant = MAGIC;
	r = NtCreateMutant( &mutant, MUTANT_ALL_ACCESS, NULL, 0 );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(mutant != MAGIC, "return wrong %08lx\n", r);

	r = NtReleaseMutant( mutant, NULL );
	ok(r == STATUS_MUTANT_NOT_OWNED, "return wrong %08lx\n", r);

	r = NtReleaseMutant( mutant, MAGIC );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtWaitForSingleObject( mutant, 0, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtReleaseMutant( mutant, MAGIC );
	ok(r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	r = NtReleaseMutant( mutant, NULL );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	r = NtClose( mutant );
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

}

void NtProcessStartup( void )
{
	log_init();
	test_mutant();
	log_fini();
}
