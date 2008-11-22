/*
 * nt loader
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


#include <unistd.h>

#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "mem.h"
#include "ntcall.h"
#include "list.h"
#include "platform.h"

#define MAX_CORE_MEMORY 0x10000000

static inline BOOLEAN mem_allocation_type_is_valid(ULONG state)
{
	state &= ~MEM_TOP_DOWN;
	return (state == MEM_RESERVE || state == MEM_COMMIT);
}

static inline BOOLEAN mem_protection_is_valid(ULONG protect)
{
	switch (protect)
	{
	case PAGE_EXECUTE:
		break;
	case PAGE_EXECUTE_READ:
		break;
	case PAGE_EXECUTE_READWRITE:
		break;
	case PAGE_EXECUTE_WRITECOPY:
		break;
	case PAGE_NOACCESS:
		break;
	case PAGE_READONLY:
		break;
	case PAGE_READWRITE:
		break;
	case PAGE_WRITECOPY:
		break;
	default:
		return 0;
	}
	return 1;
}

void address_space_impl::verify()
{
	ULONG total = 0, count = 0;
	BOOLEAN free_blocks = 0, bad_xlate = 0;

	for ( mblock_iter_t i(blocks); i; i.next() )
	{
		mblock *mb = i;

		if ( mb->is_free() )
			free_blocks++;
		// check xlate entries
		for ( ULONG i=0; i<mb->get_region_size(); i+=0x1000 )
			if ( xlate_entry(mb->get_base_address() + i ) != mb )
				bad_xlate++;
		total += mb->get_region_size();
		count++;
	}

	ULONG sz = (size_t) highest_address;
	if (free_blocks || bad_xlate)
	{
		dprintf("invalid VM... %d free blocks %d bad xlate entries\n",
				free_blocks, bad_xlate);
		for ( mblock_iter_t i(blocks); i; i.next() )
		{
			mblock *mb = i;
			mb->dump();
		}
		dprintf("total %08lx in %ld allocations, %08lx\n", total, count, sz);
	}

	assert( free_blocks == 0 );
	assert( bad_xlate == 0 );
	assert( total < sz );
}

address_space_impl::address_space_impl() :
	lowest_address(0),
	highest_address(0)
{
}

address_space::~address_space()
{
}

address_space_impl::~address_space_impl()
{
}

void address_space_impl::destroy()
{
	verify();

	// free all the non-free allocations
	while (blocks.head())
		free_shared( blocks.head() );

	::munmap( xlate, num_pages * sizeof (mblock*) );
}

mblock* address_space_impl::alloc_guard_block(BYTE *address, ULONG size)
{
	mempages *pgs = alloc_guard_pages(size);
	mblock *mb = new mblock( address, size, pgs );
	if (!mb)
		return NULL;
	mb->reserve( this, PROT_NONE );
	update_page_translation( mb );
	insert_block( mb );
	return mb;
}

struct address_space_impl *(*pcreate_address_space)();

address_space *create_address_space( BYTE *high )
{
	address_space_impl *vm;

	vm = pcreate_address_space();
	if (!vm)
		return NULL;

	if (!vm->init(high))
	{
		delete vm;
		return 0;
	}
	return vm;
}

bool address_space_impl::init(BYTE *high)
{
	const size_t guard_size = 0x10000;

	highest_address = high;
	assert( high > (lowest_address + guard_size) );

	num_pages = ((unsigned long)high)>>12;
	xlate = (mblock**) ::mmap_anon( 0, num_pages * sizeof (mblock*), PROT_READ | PROT_WRITE );
	if (xlate == (mblock**) -1)
		die("failed to allocate page translation table\n");

	// make sure there's 0x10000 bytes of reserved memory at 0x00000000
	if (!alloc_guard_block( NULL, guard_size ))
		return false;
	if (!alloc_guard_block( highest_address - guard_size, guard_size ))
		return false;

	verify();

	return true;
}

void address_space_impl::dump()
{
	for ( mblock_iter_t i(blocks); i; i.next() )
	{
		mblock *mb = i;
		mb->dump();
	}
}

NTSTATUS address_space_impl::find_free_area( int zero_bits, size_t length, int top_down, BYTE *&base )
{
	ULONG free_size;

	//dprintf("%08x\n", length);
	length = (length + 0xfff) & ~0xfff;

	free_size = 0;
	if (!top_down)
	{
		base = lowest_address;
		while (free_size < length)
		{
			if ((base+free_size) >= highest_address)
				return STATUS_NO_MEMORY;

			if (xlate_entry( base+free_size ))
			{
				mblock *mb = xlate_entry( base+free_size );
				base = mb->get_base_address() + mb->get_region_size();
				free_size = 0;
			}
			else if (((ULONG)base)&0xffff)
				base += 0x1000;
			else
				free_size += 0x1000;
		}
	}
	else
	{
		base = (BYTE*)(((ULONG)highest_address - length)&~0xffff);
		while (free_size < length)
		{
			if ((base+free_size) <= lowest_address)
				return STATUS_NO_MEMORY;
			if (xlate_entry( base+free_size ))
			{
				mblock *mb = xlate_entry( base+free_size );
				base = mb->get_base_address() - length;
				free_size = 0;
			}
			else if (((ULONG)base)&0xffff)
				base -= 0x1000;
			else
				free_size += 0x1000;
		}
	}
	return STATUS_SUCCESS;
}

mblock *address_space_impl::get_mblock( BYTE *address )
{
	// check requested block is within limits
	if (address >= highest_address)
		return NULL;

	// try using the address translation table
	return xlate_entry( address );
}

// bitmask returned by check_area
#define AREA_VALID 1
#define AREA_FREE 2
#define AREA_CONTIGUOUS 4

ULONG address_space_impl::check_area( BYTE *address, size_t length )
{
	ULONG flags = 0;

	// check requested block is within limits
	if (address >= highest_address)
		return flags;
	if (address+length > highest_address)
		return flags;
	if (!length)
		return flags;

	flags |= AREA_VALID;

	mblock *mb = xlate_entry(address);

	if (!mb)
		flags |= AREA_FREE;

	flags |= AREA_CONTIGUOUS;
	for (ULONG i=0; i<length && (flags & AREA_CONTIGUOUS); i+=0x1000)
		if (mb != xlate_entry(address + i))
			flags &= ~(AREA_CONTIGUOUS | AREA_FREE);

	return flags;
}

void address_space_impl::insert_block( mblock *mb )
{
	blocks.append( mb );
}

void address_space_impl::remove_block( mblock *mb )
{
	assert( mb->is_free() );
	blocks.unlink( mb );
}

// splits one block into three parts (before, middle, after)
// returns the middle part
mblock *address_space_impl::split_area( mblock *mb, BYTE *address, size_t length )
{
	mblock *ret;

	assert( length >= 0x1000);
	assert( !(length&0xfff) );
	assert( !(((int)address)&0xfff) );

	assert( mb->get_base_address() <= address );
	assert( (mb->get_base_address() + mb->get_region_size()) >= address );

	if (mb->get_base_address() != address)
	{
		ret = mb->split( address - mb->get_base_address() );
		update_page_translation( ret );
		insert_block( ret );
	}
	else
		ret = mb;

	if (ret->get_region_size() != length)
	{
		mblock *extra = ret->split( length );
		update_page_translation( extra );
		insert_block( extra );
	}

	return ret;
}

void address_space_impl::update_page_translation( mblock *mb )
{
	ULONG i;

	for ( i = 0; i<mb->get_region_size(); i += 0x1000 )
	{
		if (!mb->is_free())
			xlate_entry( mb->get_base_address() + i ) = mb;
		else
			xlate_entry( mb->get_base_address() + i ) = NULL;
	}
}

NTSTATUS address_space_impl::get_mem_region( BYTE *start, size_t length, int state )
{
	verify();

	ULONG flags = check_area( start, length );

	if (!(flags & AREA_VALID))
	{
		dprintf("area not found\n");
		return STATUS_NO_MEMORY;
	}

	if ((state & MEM_RESERVE) && !(flags & AREA_FREE))
	{
		dprintf("memory not free\n");
		return STATUS_CONFLICTING_ADDRESSES;
	}

	/* check the size again */
	if (!(flags & AREA_CONTIGUOUS))
		return STATUS_CONFLICTING_ADDRESSES;

	return STATUS_SUCCESS;
}

NTSTATUS address_space_impl::allocate_virtual_memory( BYTE **start, int zero_bits, size_t length, int state, int prot )
{
	NTSTATUS r;

	r = check_params( *start, zero_bits, length, state, prot );
	if (r < STATUS_SUCCESS)
		return r;

	if (!*start)
	{
		r = find_free_area( zero_bits, length, state&MEM_TOP_DOWN, *start );
		if (r < STATUS_SUCCESS)
			 return r;
	}

	r = get_mem_region( *start, length, state );
	if (r < STATUS_SUCCESS)
		return r;

	mblock *mb = xlate_entry( *start );
	if (!mb)
	{
		mempages *pgs = alloc_core_pages( length );
		mb = new mblock( *start, length, pgs );
		insert_block( mb );
		//xlate_entry( start ) = mb;
	}
	else
	{
		mb = split_area( mb, *start, length );
	}

	assert( mb->is_linked() );

	assert( *start == mb->get_base_address());
	assert( length == mb->get_region_size());

	return set_block_state( mb, state, prot );
}

NTSTATUS address_space_impl::map_fd( BYTE **start, int zero_bits, size_t length, int state, int prot, int fd )
{
	NTSTATUS r;

	r = check_params( *start, zero_bits, length, state, prot );
	if (r < STATUS_SUCCESS)
		return r;

	if (!*start)
	{
		r = find_free_area( zero_bits, length, state&MEM_TOP_DOWN, *start );
		if (r < STATUS_SUCCESS)
			 return r;
	}

	r = get_mem_region( *start, length, state );
	if (r < STATUS_SUCCESS)
		return r;

	mblock *mb = xlate_entry( *start );
	if (mb)
		return STATUS_CONFLICTING_ADDRESSES;

	mempages *pgs = alloc_fd_pages( length, fd );
	mb = new mblock( *start, length, pgs );
	insert_block( mb );
	assert( mb->is_linked() );

	assert( *start == mb->get_base_address());
	assert( length == mb->get_region_size());

	return set_block_state( mb, state, prot );
}


NTSTATUS address_space_impl::check_params( BYTE *start, int zero_bits, size_t length, int state, int prot )
{
	//dprintf("%p %08x %08x\n", *start, length, prot);

	if (length == 0)
		return STATUS_MEMORY_NOT_ALLOCATED;

	assert( !(length & 0xfff) );

	// sanity checking
	if (length > (size_t)highest_address)
		return STATUS_INVALID_PARAMETER_2;

	if (start > highest_address)
		return STATUS_INVALID_PARAMETER_2;

	if (!mem_allocation_type_is_valid(state))
		return STATUS_INVALID_PARAMETER_5;

	if (!mem_protection_is_valid(prot))
		return STATUS_INVALID_PAGE_PROTECTION;

	return STATUS_SUCCESS;
}

NTSTATUS address_space_impl::set_block_state( mblock *mb, int state, int prot )
{
	if (mb->is_free())
	{
		mb->reserve( this, prot );
		update_page_translation( mb );
	}

	if (state & MEM_COMMIT)
		mb->commit( this, prot );

	assert( !mb->is_free() );
	verify();
	//mb->dump();

	return STATUS_SUCCESS;
}

void address_space_impl::free_shared( mblock *mb )
{
	//mb->dump();
	if (mb->is_committed())
		mb->uncommit( this );

	mb->unreserve( this );
	update_page_translation( mb );
	remove_block( mb );
	delete mb;
}

NTSTATUS address_space_impl::free_virtual_memory( void *start, size_t length, ULONG state )
{
	BYTE *addr;
	mblock *mb;

	//dprintf("%p %08x %08lx\n", start, length, state);

	assert( !(length & 0xfff) );
	assert( !(((int)start)&0xfff) );

	if (!start)
		return STATUS_INVALID_PARAMETER;

	if (length > (size_t)highest_address)
		return STATUS_INVALID_PARAMETER_2;

	if (!length)
		return STATUS_INVALID_PARAMETER_2;

	addr = (BYTE*)start;
	if (addr > highest_address)
		return STATUS_INVALID_PARAMETER_2;

	verify();

	mb = get_mblock( addr );
	if (!mb)
	{
		dprintf("no areas found!\n");
		return STATUS_NO_MEMORY;
	}

	if (mb->get_region_size()<length)
		return STATUS_UNABLE_TO_FREE_VM;

	mb = split_area( mb, addr, length );
	if (!mb)
	{
		dprintf("failed to split area!\n");
		return STATUS_NO_MEMORY;
	}

	free_shared( mb );

	verify();

	return STATUS_SUCCESS;
}

NTSTATUS address_space_impl::unmap_view( void *start )
{
	BYTE *addr = (BYTE*)start;

	mblock *mb = get_mblock( addr );
	if (!mb)
	{
		dprintf("no areas found!\n");
		return STATUS_NO_MEMORY;
	}

	// FIXME: should area be split?
	free_shared( mb );

	return STATUS_SUCCESS;
}

NTSTATUS address_space_impl::query( BYTE *start, MEMORY_BASIC_INFORMATION *info )
{
	mblock *mb;

	mb = get_mblock( start );
	if (!mb)
	{
		dprintf("no areas found!\n");
		return STATUS_INVALID_PARAMETER;
	}

	return mb->query( start, info );
}

NTSTATUS address_space_impl::get_kernel_address( BYTE **address, size_t *len )
{
	mblock *mb;
	ULONG ofs;

	//dprintf("%p %p %u\n", vm, *address, *len);

	if (*address >= highest_address)
		return STATUS_ACCESS_VIOLATION;

	if (*len > (size_t) highest_address)
		return STATUS_ACCESS_VIOLATION;

	if ((*address + *len) > highest_address)
		return STATUS_ACCESS_VIOLATION;

	mb = xlate_entry( *address );
	if (!mb)
		return STATUS_ACCESS_VIOLATION;

	//dprintf("%p\n", mb);

	assert (mb->get_base_address() <= (*address));
	assert ((mb->get_base_address() + mb->get_region_size()) > (*address));

	if (!mb->is_committed())
		return STATUS_ACCESS_VIOLATION;

	assert(mb->get_kernel_address() != NULL);

	ofs = (*address - mb->get_base_address());
	*address = mb->get_kernel_address() + ofs;

	if ((ofs + *len) > mb->get_region_size())
		*len = mb->get_region_size() - ofs;

	//dprintf("copying %04x bytes to %p (size %04lx)\n", *len, *address, mb->get_region_size());
	assert( *len <= mb->get_region_size() );

	return STATUS_SUCCESS;
}

const char *address_space_impl::get_symbol( BYTE *address )
{
	dprintf("%p\n", address );
	mblock *mb = get_mblock( address );
	if (!mb)
		return 0;

	// sections aren't continuous.
	//  when pe_section_t::mapit is fixed, fix here too
	//return get_section_symbol( mb->get_section(), address - mb->get_base_address() );

	return get_section_symbol( mb->get_section(), (ULONG) address );
}

NTSTATUS address_space_impl::copy_from_user( void *dest, const void *src, size_t len )
{
	NTSTATUS r = STATUS_SUCCESS;
	size_t n;
	BYTE *x;

	while (len)
	{
		n = len;
		x = (BYTE*) src;
		r = get_kernel_address( &x, &n );
		if (r < STATUS_SUCCESS)
			break;
		memcpy( dest, x, n );
		assert( len >= n );
		len -= n;
		src = (BYTE*)src + n;
		dest = (BYTE*)dest + n;
	}

	return r;
}

NTSTATUS address_space_impl::copy_to_user( void *dest, const void *src, size_t len )
{
	NTSTATUS r = STATUS_SUCCESS;
	size_t n;
	BYTE *x;

	//dprintf("%p %p %04x\n", dest, src, len);

	while (len)
	{
		n = len;
		x = (BYTE*)dest;
		r = get_kernel_address( &x, &n );
		if (r < STATUS_SUCCESS)
			break;
		//dprintf("%p %p %u\n", x, src, n);
		memcpy( x, src, n );
		assert( len >= n );
		len -= n;
		src = (BYTE*)src + n;
		dest = (BYTE*)dest + n;
	}

	if (len)
		dprintf("status %08lx copying to %p\n", r, dest );

	return r;
}

NTSTATUS address_space_impl::verify_for_write( void *dest, size_t len )
{
	NTSTATUS r = STATUS_SUCCESS;
	size_t n;
	BYTE *x;

	while (len)
	{
		n = len;
		x = (BYTE*) dest;
		r = get_kernel_address( &x, &n );
		if (r < STATUS_SUCCESS)
			break;
		len -= n;
		dest = (BYTE*)dest + n;
	}

	return r;
}

mblock* address_space_impl::find_block( BYTE *addr )
{
	return get_mblock( addr );
}

bool address_space_impl::traced_access( void* addr, ULONG Eip )
{
	BYTE* address = (BYTE*) addr;
	mblock* mb = get_mblock( address );
	if (!mb)
		return false;
	return mb->traced_access( address, Eip );
}

bool address_space_impl::set_traced( void* addr, bool traced )
{
	BYTE* address = (BYTE*) addr;
	mblock* mb = get_mblock( address );
	if (!mb)
		return false;

	mb->set_traced( this, traced );
	return true;
}

static inline ULONG mem_round_size(ULONG size)
{
	return (size + 0xfff)&~0xfff;
}

static inline BYTE *mem_round_addr(BYTE *addr)
{
	return (BYTE*) (((int)addr)&~0xfff);
}

NTSTATUS NTAPI NtAllocateVirtualMemory(
	HANDLE ProcessHandle,
	OUT PVOID *BaseAddress,
	ULONG ZeroBits,
	OUT PULONG AllocationSize,
	ULONG AllocationType,
	ULONG Protect)
{
	BYTE *addr = NULL;
	ULONG size = 0;
	NTSTATUS r;
	process_t *process;

	//dprintf("%p %p %lu %p %08lx %08lx\n", ProcessHandle, BaseAddress,
	//		ZeroBits, AllocationSize, AllocationType, Protect);

	/* check for a valid allocation type */
	if (!mem_allocation_type_is_valid(AllocationType))
		return STATUS_INVALID_PARAMETER_5;

	if (!mem_protection_is_valid(Protect))
		return STATUS_INVALID_PAGE_PROTECTION;

	r = copy_from_user( &size, AllocationSize, sizeof (ULONG) );
	if (r)
		return r;

	r = copy_from_user( &addr, BaseAddress, sizeof (PVOID) );
	if (r)
		return r;

	if (ZeroBits == 1 || ZeroBits > 20)
		return STATUS_INVALID_PARAMETER_3;

	r = process_from_handle( ProcessHandle, &process );
	if (r < STATUS_SUCCESS)
		return r;

	/* round address and size */
	if (size > mem_round_size( size ))
		return STATUS_INVALID_PARAMETER_4;
	size = mem_round_size( size );

	if (addr < mem_round_addr( addr ))
		return STATUS_INVALID_PARAMETER_2;
	addr = mem_round_addr( addr );

	r = process->vm->allocate_virtual_memory( &addr, ZeroBits, size, AllocationType, Protect );

	dprintf("returns  %p %08lx  %08lx\n", addr, size, r);

	if (r < STATUS_SUCCESS)
		return r;

	r = copy_to_user( AllocationSize, &size, sizeof (ULONG) );
	if (r)
		return r;

	r = copy_to_user( BaseAddress, &addr, sizeof (BYTE*) );

	return r;
}

NTSTATUS NTAPI NtQueryVirtualMemory(
	HANDLE ProcessHandle,
	LPCVOID BaseAddress,
	MEMORY_INFORMATION_CLASS MemoryInformationClass,
	PVOID MemoryInformation,
	SIZE_T MemoryInformationLength,
	SIZE_T* ReturnLength )
{
	MEMORY_BASIC_INFORMATION info;
	SIZE_T len;
	NTSTATUS r;

	dprintf("%p %p %d %p %lu %p\n", ProcessHandle,
			BaseAddress, MemoryInformationClass, MemoryInformation,
			MemoryInformationLength, ReturnLength);

	if (MemoryInformationClass != MemoryBasicInformation)
		return STATUS_INVALID_PARAMETER;

	if (ReturnLength)
	{
		r = copy_from_user( &len, ReturnLength, sizeof len );
		if (r)
			return r;
	}
	else
		len = sizeof info;

	process_t *p = 0;
	r = process_from_handle( ProcessHandle, &p );
	if (r < STATUS_SUCCESS)
		return r;

	r = p->vm->query( (BYTE*) BaseAddress, &info );
	if (r)
		return r;

	if (len >= sizeof info )
		len = sizeof info;
	else
		r = STATUS_INFO_LENGTH_MISMATCH;

	r = copy_to_user( MemoryInformation, &info, len );
	if (r == STATUS_SUCCESS && ReturnLength)
		r = copy_to_user( ReturnLength, &len, sizeof len );

	return r;
}

NTSTATUS NTAPI NtProtectVirtualMemory(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG NumberOfBytesToProtect,
	ULONG NewAccessProtection,
	PULONG OldAccessProtection )
{
	process_t *process;
	PVOID addr = NULL;
	ULONG size = 0;
	NTSTATUS r;

	dprintf("%p %p %p %lu %p\n", ProcessHandle, BaseAddress,
			NumberOfBytesToProtect, NewAccessProtection, OldAccessProtection);

	r = process_from_handle( ProcessHandle, &process );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_from_user( &addr, BaseAddress, sizeof addr );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_from_user( &size, NumberOfBytesToProtect, sizeof size );
	if (r < STATUS_SUCCESS)
		return r;

	dprintf("%p %08lx\n", addr, size );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtWriteVirtualMemory(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	ULONG NumberOfBytesToWrite,
	PULONG NumberOfBytesWritten )
{
	size_t len, written = 0;
	BYTE *src, *dest;
	NTSTATUS r = STATUS_SUCCESS;
	process_t *p;

	dprintf("%p %p %p %08lx %p\n", ProcessHandle, BaseAddress, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten );

	r = process_from_handle( ProcessHandle, &p );
	if (r < STATUS_SUCCESS)
		return r;

	while (NumberOfBytesToWrite)
	{
		len = NumberOfBytesToWrite;

		src = (BYTE*)Buffer;
		dest = (BYTE*)BaseAddress;

		r = current->process->vm->get_kernel_address( &src, &len );
		if (r < STATUS_SUCCESS)
			break;

		r = p->vm->get_kernel_address( &dest, &len );
		if (r < STATUS_SUCCESS)
			break;

		dprintf("%p <- %p %u\n", dest, src, (unsigned int) len);

		memcpy( dest, src, len );

		Buffer = (BYTE*)Buffer + len;
		BaseAddress = (BYTE*) BaseAddress + len;
		NumberOfBytesToWrite -= len;
		written += len;
	}

	dprintf("wrote %d bytes\n", (unsigned int) written);

	if (NumberOfBytesWritten)
		r = copy_to_user( NumberOfBytesWritten, &written, sizeof written );

	return r;
}

NTSTATUS NTAPI NtFreeVirtualMemory(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG RegionSize,
	ULONG FreeType )
{
	process_t *process;
	BYTE *addr = NULL;
	ULONG size = 0;
	NTSTATUS r;

	dprintf("%p %p %p %lu\n", ProcessHandle, BaseAddress, RegionSize, FreeType);

	switch (FreeType)
	{
	case MEM_DECOMMIT:
	case MEM_FREE:
	case MEM_RELEASE:
		break;
	default:
		return STATUS_INVALID_PARAMETER_4;
	}

	r = process_from_handle( ProcessHandle, &process );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_from_user( &size, RegionSize, sizeof (ULONG) );
	if (r)
		return r;

	r = copy_from_user( &addr, BaseAddress, sizeof (PVOID) );
	if (r)
		return r;

	/* round address and size */
	if (size > mem_round_size( size ))
		return STATUS_INVALID_PARAMETER_3;
	size = mem_round_size( size );

	if (addr > mem_round_addr( addr ))
		return STATUS_INVALID_PARAMETER_2;
	addr = mem_round_addr( addr );

	r = process->vm->free_virtual_memory( addr, size, FreeType );

	r = copy_from_user( &size, RegionSize, sizeof (ULONG) );
	if (r)
		return r;

	r = copy_from_user( &addr, BaseAddress, sizeof (PVOID) );

	dprintf("returning %08lx\n", r);

	return r;
}

NTSTATUS NTAPI NtAreMappedFilesTheSame(PVOID Address1, PVOID Address2)
{
	dprintf("%p %p\n", Address1, Address2);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtAllocateUserPhysicalPages(
	HANDLE ProcessHandle,
	PULONG NumberOfPages,
	PULONG PageFrameNumbers)
{
	dprintf("%p %p %p\n", ProcessHandle, NumberOfPages, PageFrameNumbers);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtFlushVirtualMemory(
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	PULONG FlushSize,
	PIO_STATUS_BLOCK IoStatusBlock)
{
	dprintf("%p %p %p %p\n", ProcessHandle,
			BaseAddress, FlushSize, IoStatusBlock);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtLockVirtualMemory(
	HANDLE ProcessHandle,
	PVOID *BaseAddres,
	PULONG Length,
	ULONG LockType)
{
	dprintf("does nothing\n");
	return STATUS_SUCCESS;
}
