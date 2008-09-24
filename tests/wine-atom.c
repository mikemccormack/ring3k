/* Unit test suite for Ntdll atom API functions
 *
 * Copyright 2003 Gyorgy 'Nog' Jeney
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
 *
 */

#include <stdarg.h>
#include "ntapi.h"
#include "ntwin32.h"
#include "log.h"

static WCHAR testAtom1[] = {'H','e','l','l','o',' ','W','o','r','l','d',0};

int lstrcmpW(const WCHAR *a, const WCHAR *b)
{
	while (*a || *b)
	{
		if (*a < *b)
			return -1;
		if (*a > *b)
			return 1;
		a++;
		b++;
	}
	return 0;
}

int lstrlenW(const WCHAR *a)
{
	int n = 0;
	while (a[n])
		n++;
	return n;
}

void lstrcpy(WCHAR *dest, const WCHAR *src)
{
	while ((*dest++ = *src++))
		;
}

static void test_Global(void)
{
    NTSTATUS    res;
    USHORT      atom;
    char        ptr[sizeof(ATOM_BASIC_INFORMATION) + 255 * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION*     abi = (ATOM_BASIC_INFORMATION*)ptr;
    ULONG       ptr_size = sizeof(ATOM_BASIC_INFORMATION) + 255 * sizeof(WCHAR);

    res = NtAddAtom(testAtom1, lstrlenW(testAtom1) * sizeof(WCHAR), &atom);
    ok(!res, "Added atom (%lx)\n", res);

    memset(abi->Name, 0x55, 255 * sizeof(WCHAR));
    res = NtQueryInformationAtom( atom, AtomBasicInformation, (void*)ptr, ptr_size, NULL );
    ok(!res, "atom lookup\n");
    ok(!lstrcmpW(abi->Name, testAtom1), "ok strings\n");
    ok(abi->NameLength == lstrlenW(testAtom1) * sizeof(WCHAR), "wrong string length\n");
    ok(abi->Name[lstrlenW(testAtom1)] == 0, "wrong string termination %x\n", abi->Name[lstrlenW(testAtom1)]);
    ok(abi->Name[lstrlenW(testAtom1) + 1] == 0x5555, "buffer overwrite %x\n", abi->Name[lstrlenW(testAtom1) + 1]);

    ptr_size = sizeof(ATOM_BASIC_INFORMATION);
    res = NtQueryInformationAtom( atom, AtomBasicInformation, (void*)ptr, ptr_size, NULL );
    ok(res == STATUS_BUFFER_TOO_SMALL, "wrong return status (%08lx)\n", res);
    ok(abi->NameLength == lstrlenW(testAtom1) * sizeof(WCHAR), "ok string length\n");

    memset(abi->Name, 0x55, lstrlenW(testAtom1) * sizeof(WCHAR));
    ptr_size = sizeof(ATOM_BASIC_INFORMATION) + lstrlenW(testAtom1) * sizeof(WCHAR);
    res = NtQueryInformationAtom( atom, AtomBasicInformation, (void*)ptr, ptr_size, NULL );
    ok(!res, "atom lookup %08lx\n", res);
    ok(!lstrcmpW(abi->Name, testAtom1), "strings don't match\n");
    ok(abi->NameLength == lstrlenW(testAtom1) * sizeof(WCHAR), "wrong string length\n");
    ok(abi->Name[lstrlenW(testAtom1)] == 0, "buffer overwrite %x\n", abi->Name[lstrlenW(testAtom1)]);
    ok(abi->Name[lstrlenW(testAtom1) + 1] == 0x5555, "buffer overwrite %x\n", abi->Name[lstrlenW(testAtom1) + 1]);

    ptr_size = sizeof(ATOM_BASIC_INFORMATION) + 4 * sizeof(WCHAR);
    abi->Name[0] = abi->Name[1] = abi->Name[2] = abi->Name[3] = '\0';
    res = NtQueryInformationAtom( atom, AtomBasicInformation, (void*)ptr, ptr_size, NULL );
    ok(!res, "couldn't find atom\n");
    ok(abi->NameLength == 8, "wrong string length %u\n", abi->NameLength);
    ok(!memcmp(abi->Name, testAtom1, 8), "strings don't match\n");
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
	log_init();

	become_gui_thread();
	test_Global();

	log_fini();
}
