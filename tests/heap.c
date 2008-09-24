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
#include "rtlapi.h"
#include "log.h"

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

const int ofs_peb_in_teb = 0x30;

void* get_peb( void )
{
	void **p = get_teb();
	return p[ofs_peb_in_teb/sizeof (*p)];
}

HANDLE get_process_heap( void )
{
	void **p = get_peb();
	return p[0x18/sizeof (*p)];
}

// heap functions are implemented by ntdll
void test_heap( void )
{
	HANDLE heap = get_process_heap();
	NTSTATUS r;
	void *p;

	p = RtlAllocateHeap(heap, 0, 0x100);
	ok(p != 0, "heap alloc failed\n");

	r = RtlFreeHeap(heap, 0, p);
	ok(r == TRUE, "heap free failed\n");

	p = RtlAllocateHeap(heap, 0, 0x100);
	ok(p != 0, "heap alloc failed\n");

	r = RtlFreeHeap(heap, 0, p);
	ok(r == TRUE, "heap free failed\n");

	p = RtlAllocateHeap(heap, 0, 0x100000);
	ok(p != 0, "heap alloc failed\n");

	r = RtlFreeHeap(heap, 0, p);
	ok(r == TRUE, "heap free failed\n");
}

// make sure that the memory allocation code doesn't interfere with ntdll's heap code
void test_heap_and_section( void )
{
	void *address = NULL;
	ULONG size = 0x1000;
	NTSTATUS r;
	HANDLE handle = NtCurrentProcess();
	HANDLE heap = get_process_heap();
	void *p;
	MEMORY_BASIC_INFORMATION info;

	p = RtlAllocateHeap(heap, 0, 0x100);
	ok(p != 0, "heap alloc failed\n");

	// try throw a spanner in the works
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);
	ok(address != NULL, "NULL allocated\n");

	p = RtlReAllocateHeap(heap, 0, p, 0x100000 );
	ok(p != 0, "heap realloc failed\n");

	r = NtClose( handle );
	ok(r == STATUS_INVALID_HANDLE, "wrong return code %08lx\n", r);

	RtlFreeHeap( heap, 0, p );

	r = NtQueryVirtualMemory( handle, address, MemoryBasicInformation, &info, sizeof info, 0);
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);

	ok( info.BaseAddress == address, "base address wrong %p\n", info.BaseAddress);
	ok( info.AllocationBase == address, "allocation base wrong %p\n", info.BaseAddress);
	ok( info.RegionSize == size, "region size wrong %08lx\n", info.RegionSize);
	ok( info.State == MEM_RESERVE, "state wrong %08lx\n", info.State);
	ok( info.Protect == 0, "protect wrong %08lx\n", info.Protect);
	ok( info.Type == MEM_PRIVATE, "type wrong %08lx\n", info.Protect);

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);
}

void test_big_alloc( void )
{
	void *address = NULL;
	ULONG size = 0x100000;
	NTSTATUS r;
	HANDLE handle = NtCurrentProcess();
	MEMORY_BASIC_INFORMATION info;

	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_READWRITE );
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);

	r = NtQueryVirtualMemory( handle, address, MemoryBasicInformation, &info, sizeof info, 0);
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);

	r = NtQueryVirtualMemory( handle, address+0x10000, MemoryBasicInformation, &info, sizeof info, 0);
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code %08lx\n", r);
}

void NtProcessStartup( void )
{
	log_init();
	test_heap();
	test_heap_and_section();
	test_big_alloc();
	log_fini();
}
