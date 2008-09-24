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
#include "ntwin32.h"

const USHORT magic = 0xfedc;

static void test_atom(void)
{
	NTSTATUS r;
	WCHAR name[] = L"unique281470";
	WCHAR name2[] = L"Unique281470";
	USHORT atom, nameatom;

	r = NtAddAtom(NULL, 0, NULL);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	atom = magic;
	r = NtAddAtom(NULL, 0, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);
	ok(atom == magic, "atom value set\n");

	atom = magic;
	r = NtAddAtom(NULL, 8, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);
	ok(atom == magic, "atom value set\n");

	r = NtAddAtom(name, 0, NULL);
	ok(r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);

	atom = magic;
	r = NtAddAtom(name, 0, &atom);
	ok(r == STATUS_OBJECT_NAME_INVALID, "return wrong %08lx\n", r);
	ok(atom == magic, "atom value set\n");

	r = NtAddAtom(name, sizeof name, NULL);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// add the atom with a nul character on the end
	r = NtAddAtom(name, sizeof name, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);

	// try an atom differing by case
	atom = magic;
	r = NtAddAtom(name2, sizeof name2, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom != magic, "atom value set\n");
	nameatom = atom;

	// second atom with the same name should have the same value
	atom = magic;
	r = NtAddAtom(name, sizeof name - 2, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom == nameatom, "atom value wrong\n");
	nameatom = atom;

	r = NtAddAtom(name, sizeof name - 1, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom == nameatom, "atom value wrong\n");

	// try NtFindAtom
	r = NtFindAtom(NULL, 0, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	atom = magic;
	r = NtFindAtom(NULL, 0, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);
	ok(atom == magic, "atom value wrong\n");

	r = NtFindAtom(NULL, 0, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtFindAtom(NULL, sizeof name, &atom);
	ok(r == STATUS_INVALID_PARAMETER, "return wrong %08lx\n", r);

	r = NtFindAtom(name, sizeof name-2, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom == nameatom, "atom value wrong\n");

	r = NtFindAtom(name, sizeof name-1, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom == nameatom, "atom value wrong\n");

	r = NtFindAtom(name, sizeof name, &atom);
	ok(r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok(atom == nameatom, "atom value wrong\n");
}

// magic numbers for everybody
void NTAPI init_callback(void *arg)
{
	NtCallbackReturn( 0, 0, 0 );
}

void *callback_table[NUM_USER32_CALLBACKS];

void init_callbacks( void )
{
	callback_table[NTWIN32_THREAD_INIT_CALLBACK] = &init_callback;
	__asm__ (
		"movl %%fs:0x18, %%eax\n\t"
		"movl 0x30(%%eax), %%eax\n\t"
		"movl %%ebx, 0x2c(%%eax)\n\t"  // set PEB's KernelCallbackTable
		: : "b" (&callback_table) : "eax" );
}

void become_gui_thread( void )
{
	init_callbacks();
	NtUserGetThreadState(0x11);
}

void NtProcessStartup( void )
{
	NTSTATUS r;

	log_init();

	// until connected to the windows subsystem,
	// we get STATUS_ACCESS_DENIED when trying to access the atom table
	r = NtAddAtom(NULL, 0, NULL);
	ok(r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	r = NtFindAtom(NULL, 0, NULL);
	ok(r == STATUS_ACCESS_DENIED, "return wrong %08lx\n", r);

	// connect to windows kernel subsystem
	become_gui_thread();

	test_atom();
	log_fini();
}
