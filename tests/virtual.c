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

void test_NtAllocateVirtualMemory(void)
{
	VOID *address;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	r = NtAllocateVirtualMemory( 0, NULL, 0, NULL, 0, 0 );
	ok(r == STATUS_INVALID_PARAMETER_5, "wrong return code\n");

	r = NtAllocateVirtualMemory( 0, NULL, 0, NULL, MEM_RESERVE, 0 );
	ok(r == STATUS_INVALID_PAGE_PROTECTION, "wrong return code\n");

	r = NtAllocateVirtualMemory( 0, NULL, 0, NULL, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_ACCESS_VIOLATION, "wrong return code\n");

	address = NULL;
	r = NtAllocateVirtualMemory( 0, &address, 0, NULL, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_ACCESS_VIOLATION, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( 0, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_HANDLE, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");

	address = NULL;
	size = 0x100;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "NULL allocated\n");

	address = NULL;
	size = 0;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, 0x10000000, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_5, "wrong return code\n");

	address = NULL;
	size = 0x100;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");

	address = NULL;
	size = 0xffffffff;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_4, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0x1000, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_3, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 1, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_3, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 2, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 4, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 8, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0x100, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_3, "wrong return code\n");

#if 0
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0x10, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_NO_MEMORY, "wrong return code\n");

	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0x10, &size, MEM_COMMIT, PAGE_READONLY );
	ok(r == STATUS_NO_MEMORY, "wrong return code\n");

	/* something screwy seems to be going on with the third parameter ... */
#endif
}

void test_NtFreeVirtualMemory(void)
{
#if 0
	VOID *address;
	ULONG size;
#endif
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	r = NtFreeVirtualMemory( handle, NULL, NULL, 0 );
	ok(r == STATUS_INVALID_PARAMETER_4, "wrong return code\n");

	r = NtFreeVirtualMemory( 0, NULL, NULL, 0 );
	ok(r == STATUS_INVALID_PARAMETER_4, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, NULL, NULL, MEM_RELEASE );
	ok(r == STATUS_ACCESS_VIOLATION, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, NULL, NULL, MEM_COMMIT );
	ok(r == STATUS_INVALID_PARAMETER_4, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, NULL, NULL, 1 );
	ok(r == STATUS_INVALID_PARAMETER_4, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, NULL, NULL, MEM_DECOMMIT );
	ok(r == STATUS_ACCESS_VIOLATION, "wrong return code\n");

#if 0
	size = 0;
	address = 0;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_DECOMMIT );
	ok(r == STATUS_MEMORY_NOT_ALLOCATED, "wrong return code\n");

	size = 0;
	address = 0;
	r = NtFreeVirtualMemory( 0, &address, &size, MEM_DECOMMIT );
	ok(r == STATUS_INVALID_HANDLE, "wrong return code\n");

	/* addresses outside the address range are just invalid */
	address = (void*) 0xffff0000;
	size = 0x1000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_INVALID_PARAMETER_2, "wrong return code\n");

	address = (void*) 0x80000000;
	size = 0x1000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_INVALID_PARAMETER_2, "wrong return code\n");
#endif
}

void test_alloc_free(void)
{
	VOID *address, *a;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	/* allocate a page and free it */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	/* attempt a double free */
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_MEMORY_NOT_ALLOCATED, "wrong return code\n");

	/* free two halves */
	address = NULL;
	size = 0x2000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x2000, "size wrong\n");

	size = 0x1000;
	a = address;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x1000, "size wrong\n");
	ok(address == a, "address changed\n");

	size = 0x1000;
	address += 0x1000;
	a = address;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x1000, "size wrong\n");
	ok(address == a, "address changed\n");

	/* alloc and free with a one byte size */
	address = NULL;
	size = 1;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	size = 1;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x1000, "size wrong\n");

	/* allocate at an offset address */
	a = address++; /* reuse the previously free'd address... no threads! */
	size = 1;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address == a, "address changed\n");
	ok(size == 0x1000, "size wrong\n");

	a = address++;
	size = 1;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address == a, "address changed\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_MEMORY_NOT_ALLOCATED, "wrong return code\n");

	/* try a double allocate */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	a = address;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_CONFLICTING_ADDRESSES, "wrong return code\n");
	ok(address == a, "address changed\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	/* test MEM_RESERVE, MEM_COMMIT, MEM_DECOMMIT, MEM_RELEASE */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	a = address;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address == a, "address changed\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_DECOMMIT );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	/* commit then reserve? */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	a = address;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_CONFLICTING_ADDRESSES, "wrong return code\n");
	ok(address == a, "address changed\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_DECOMMIT );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	/* reserve then decommit? */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "NULL allocated\n");
	ok(size == 0x1000, "size wrong\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_DECOMMIT );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	/* reserved with MEM_FREE state? */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_FREE, PAGE_NOACCESS );
	ok(r == STATUS_INVALID_PARAMETER_5, "wrong return code\n");
	ok(address == NULL, "address chagned\n");
	ok(size == 0x1000, "size changed\n");

	/* reserved with MEM_FREE state? */
	address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, 0xffffffff );
	ok(r == STATUS_INVALID_PAGE_PROTECTION, "wrong return code\n");
	ok(address == NULL, "address chagned\n");
	ok(size == 0x1000, "size changed\n");
}

void test_granularity( void )
{
	VOID *address[0x100];
	ULONG size, i;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;
	UINT too_small, too_big;

	for (i=0; i<0x100; i++)
	{
		address[i] = NULL;
		size = 0x1000;
		r = NtAllocateVirtualMemory( handle, &address[i], 0, &size, MEM_RESERVE, PAGE_NOACCESS );
		ok(r == STATUS_SUCCESS, "wrong return code\n");
		ok(address[i] != NULL, "NULL allocated\n");
		ok(size == 0x1000, "size wrong\n");

		//dprintf("%p\n", address[i]);
	}

	too_small = 0;
	too_big = 0;
	for (i=1; i<0x100; i++)
	{
		if ((address[i] - address[i-1]) < 0x10000)
			too_small++;
		if ((address[i] - address[i-1]) > 0x10000)
			too_big++;
	}
	ok( too_small == 0, "virtual memory granularity too small\n");
	ok( too_big < 0x10, "virtual memory granularity too big\n");

	for (i=0; i<0x100; i++)
	{
		size = 0x1000;
		r = NtFreeVirtualMemory( handle, &address[i], &size, MEM_RELEASE );
		ok(r == STATUS_SUCCESS, "wrong return code\n");
	}
}

// The granulatiry of memory allocations on NT seems to be 0x10000 or 16 x 4k pages.
// The size is rounded to multiples of pages size.
// So what happens when we try to allocate memory at an address that is not
// a multiple of 0x10000?  What happens if a chunk of memory in that set of 16 pages
// is already allocated?  What happens if it's not already allocated?
void test_chunksize( void )
{
	VOID *address[2];
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	address[0] = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address[0], 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address[0] != NULL, "address wrong\n");
	ok(size == 0x1000, "size wrong\n");

	// try allocate memory one page after the previous allocation.
	address[1] = address[0] + 0x1000;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address[1], 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_CONFLICTING_ADDRESSES, "wrong return code (%08lx)\n", r);
	ok(address[1] == address[0] + 0x1000, "address wrong\n");
	ok(size == 0x1000, "size wrong\n");

	// free the memory... assume nobody else grabs it (ie. there's no other threads)
	r = NtFreeVirtualMemory( handle, &address[0], &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x1000, "size wrong\n");

	// try allocate memory one page after the previous (free'd) allocation
	r = NtAllocateVirtualMemory( handle, &address[1], 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code (%08lx)\n", r);
	ok(address[1] == address[0], "address wrong\n");
	ok(size == 0x2000, "size wrong\n");

	// free what we allocated
	address[1] = address[0] + 0x1000;
	size = 0x1000;
	r = NtFreeVirtualMemory( handle, &address[1], &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address[1] == address[0] + 0x1000, "address wrong\n");
	ok(size == 0x1000, "size wrong\n");

	// free what we didn't allocate...
	address[1] = address[0];
	size = 0x1000;
	r = NtFreeVirtualMemory( handle, &address[1], &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address[1] == address[0], "address wrong\n");
	ok(size == 0x1000, "size wrong\n");
}

// Figure out what combination of protections and states pages in a single allocation
// can have.  Can protections be mixed?  How about states (COMMIT/RESERVE/FREE)?
void test_prot_mem( void )
{
	VOID *address, *a;
	ULONG size, old;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	// allocate 16 pages
	address = NULL;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x10000, "size wrong\n");
	a = address;

	// try change access immediately
	size = 0x1000;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_NOACCESS, &old );
	ok(r == STATUS_NOT_COMMITTED, "wrong return code (%08lx)\n", r);

	// commit the page
	address = a;
	size = 0x1000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x1000, "size wrong\n");

	// try change access again
	address = a;
	size = 0x1000;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_READONLY, &old );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(old == PAGE_NOACCESS, "wrong access\n");
	ok(size == 0x1000, "size wrong\n");

	// and again
	address = a;
	size = 0x1000;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_READWRITE, &old );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(old == PAGE_READONLY, "wrong access\n");
	ok(size == 0x1000, "size wrong\n");

	// change the second page
	address = a + 0x1000;
	size = 0x1000;
	old = 0;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_READWRITE, &old );
	ok(r == STATUS_NOT_COMMITTED, "wrong return code (%08lx)\n", r);
	ok(size == 0x1000, "size wrong\n");
	ok(old == PAGE_READONLY, "wrong access\n");

	// commit the second page, overlapping with the first page
	address = a;
	size = 0x2000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_EXECUTE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x2000, "size wrong\n");
	ok(old == PAGE_READONLY, "wrong access\n");

	// overlaps the next uncommitted page
	address = a + 0x1000;
	size = 0x2000;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_READWRITE, &old );
	ok(r == STATUS_NOT_COMMITTED, "wrong return code (%08lx)\n", r);
	ok(size == 0x2000, "size wrong\n");
#ifdef WIN2K
	ok(old == PAGE_READONLY, "wrong access\n");
#endif

	// the page we just committed
	address = a + 0x1000;
	size = 0x1000;
	r = NtProtectVirtualMemory( handle, &address, &size, PAGE_READWRITE, &old );
	ok(r == STATUS_SUCCESS, "wrong return code (%08lx)\n", r);
	ok(size == 0x1000, "size wrong\n");
	ok(old == PAGE_EXECUTE, "wrong access\n");

	// free all the memory
	address = a;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
}

void test_free_mem( void )
{
	VOID *address, *a;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	// allocate 16 pages
	address = NULL;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x10000, "size wrong\n");
	a = address;

	// free half of the memory we committed
	address = a;
	size = 0x8000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(size == 0x8000, "size wrong\n");

	// try free memory across a block boundary
	address = a + 0xc000;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_UNABLE_TO_FREE_VM, "wrong return code (%08lx)\n", r);
	ok(size == 0x10000, "size wrong\n");

	// try free all the memory again
	address = a;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_MEMORY_NOT_ALLOCATED, "wrong return code\n");

	// free the last 4 pages
	address = a + 0xc000;
	size = 0x4000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	// free the last 4 pages again... should fail
	address = a + 0xc000;
	size = 0x4000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_MEMORY_NOT_ALLOCATED, "wrong return code\n");
	ok(size == 0x4000, "size wrong\n");

	// free the rest
	address = a + 0x8000;
	size = 0x4000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
}

// What happens if we try to free two blocks
// that were allocated separately at the same time?
void test_separate_alloc_single_free(void)
{
	VOID *address, *a;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	// allocate 32 pages
	address = NULL;
	size = 0x20000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x20000, "size wrong\n");
	a = address;

	// free all the memory again
	address = a;
	size = 0x20000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	// allocate again at the same address, but in two chunks
	address = a;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x10000, "size wrong\n");

	// allocate again at the same address, but in two chunks
	address = a + 0x10000;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x10000, "size wrong\n");

	// try free two separately allocated chunks in one go
	address = a;
	size = 0x20000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_UNABLE_TO_FREE_VM, "wrong return code\n");

	// that doesn't work... free it properly in two goes
	address = a;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	address = a + 0x10000;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
}

// What happens if we split an area by freeing a piece of virtual memory in the middle of it?
// Does it rejoin itself when we reallocate the area in the middle?
void test_split_and_join(void)
{
	VOID *address, *a;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	// allocate 32 pages
	address = NULL;
	size = 0x30000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x30000, "size wrong\n");
	a = address;

	// split it into three bits
	address = a + 0x10000;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	// reallocate the bits we just removed
	address = a + 0x10000;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x10000, "size wrong\n");

	// now try free everything
	address = a;
	size = 0x30000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_UNABLE_TO_FREE_VM, "wrong return code\n");

	// now really free it
	address = a;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	address = a + 0x10000;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	address = a + 0x20000;
	size = 0x10000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
}

// What happens if we commit an area in the middle of a reserved block?
// Does that affect how we can free it?
void test_commit_in_the_middle(void)
{
	VOID *address, *a;
	ULONG size;
	NTSTATUS r;
	HANDLE handle = (HANDLE) ~0;

	// allocate 32 pages
	address = NULL;
	size = 0x30000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_RESERVE, PAGE_NOACCESS );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
	ok(address != NULL, "address wrong\n");
	ok(size == 0x30000, "size wrong\n");
	a = address;

	// split it into three bits
	address = a + 0x10000;
	size = 0x10000;
	r = NtAllocateVirtualMemory( handle, &address, 0, &size, MEM_COMMIT, PAGE_READONLY );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	// now try free everything
	address = a;
	size = 0x30000;
	r = NtFreeVirtualMemory( handle, &address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");
}

void NtProcessStartup( void )
{
	log_init();
	test_NtAllocateVirtualMemory();
	test_NtFreeVirtualMemory();
#if 0
	test_alloc_free();
	test_granularity();
	test_chunksize();
	test_prot_mem();
	test_free_mem();
	test_separate_alloc_single_free();
	test_split_and_join();
	test_commit_in_the_middle();
#endif
	log_fini();
}
