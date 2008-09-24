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

mempages::~mempages()
{
}

class corepages : public mempages {
protected:
	corepages();
public:
	corepages( int _fd, size_t sz );
	corepages( size_t sz );
	virtual ULONG get_size();
	virtual BYTE* map_local( int prot );
	virtual int map_remote( address_space *vm, BYTE *address, ULONG length, int prot, ULONG ofs );
	virtual mempages *split(ULONG sz);
	~corepages();
private:
	int fd;
	void *kernel_address;
	int core_ofs;
	size_t size;
};

corepages::corepages( int _fd, size_t sz ) :
	kernel_address(0),
	core_ofs(0),
	size( sz )
{
	fd = dup(_fd);
}

int create_mapping_fd( int sz )
{
	static int core_num = 0;

	char name[0x40];
	sprintf(name, "/tmp/win2k-%d", ++core_num);
	int fd = open( name, O_CREAT | O_TRUNC | O_RDWR, 0600 );
	if (fd < 0)
		return -1;

	unlink( name );

	int r = ftruncate( fd, sz );
	if (r < 0)
	{
		close(fd);
		fd = -1;
	}
	return fd;
}

corepages::corepages(size_t sz) :
	kernel_address(0),
	core_ofs(0),
	size( sz )
{
	fd = create_mapping_fd( size );
	if (fd < 0)
		throw STATUS_NO_MEMORY;
}

corepages::corepages() :
	fd(-1),
	kernel_address(0),
	core_ofs(0),
	size(0)
{
}

ULONG corepages::get_size()
{
	return size;
}

BYTE *corepages::map_local( int prot )
{
	return (BYTE*) mmap( NULL, size, prot, MAP_SHARED, fd, core_ofs );
}

int corepages::map_remote( address_space *vm, BYTE *address, ULONG length, int prot, ULONG ofs )
{
	return vm->mmap( address, length, prot,
					 MAP_SHARED | MAP_FIXED, fd, core_ofs + ofs );
}

mempages *corepages::split(ULONG sz)
{
	if (sz >= size)
		return NULL;

	corepages *rest = new corepages();
	rest->core_ofs = core_ofs + sz;
	rest->size = size - sz;
	rest->fd = dup(fd);
	size = sz;
	return rest;
}

corepages::~corepages()
{
	close(fd);
}

class guardpages : public mempages {
protected:
	guardpages();
public:
	guardpages(size_t sz);
	virtual ULONG get_size();
	virtual BYTE* map_local( int prot );
	virtual int map_remote( address_space *vm, BYTE *address, ULONG length, int prot, ULONG ofs );
	virtual mempages *split(ULONG sz);
	~guardpages();
private:
	size_t size;
};

guardpages::guardpages(size_t sz) :
	size(sz)
{
}

guardpages::~guardpages()
{
}

ULONG guardpages::get_size()
{
	return size;
}

BYTE* guardpages::map_local( int prot )
{
	return (BYTE*)-1;
}

int guardpages::map_remote( address_space *vm, BYTE *address, ULONG length, int prot, ULONG ofs )
{
	return 0;
}

mempages* guardpages::split(ULONG sz)
{
	if (sz <= size)
		return NULL;
	mempages *pgs = new guardpages(sz);
	size -= sz;
	return pgs;
}

mempages* alloc_guard_pages(ULONG size)
{
	return new guardpages(size);
}

mempages* alloc_core_pages(ULONG size)
{
	return new corepages(size);
}

mempages* alloc_fd_pages(ULONG length, int fd)
{
	return new corepages(fd, length);
}

mblock::mblock( BYTE *address, size_t size, mempages *pgs ) :
	BaseAddress( address ),
	RegionSize( size ),
	State( MEM_FREE ),
	pages( pgs ),
	kernel_address( NULL ),
	tracer(0),
	section(0)
{
}

mblock::~mblock()
{
	if (section)
		release( section );
	if (pages)
		delete pages;
	assert( is_free() );
}

void mblock::dump()
{
	dprintf("%p %08lx %08lx %08lx %08lx\n", BaseAddress, RegionSize, Protect, State, Type );
}

mblock *mblock::split( size_t length )
{
	mblock *ret;
	mempages *pgs;

	assert( length >= 0x1000);
	assert( !(length&0xfff) );

	if (RegionSize == length)
		return NULL;

	assert( length >= 0 );
	assert( length < RegionSize );

	pgs = pages->split(length);
	if (!pgs)
	{
		dprintf("split failed!\n");
		return NULL;
	}

	ret = new mblock( BaseAddress + length, RegionSize - length, pgs );
	if (!ret)
		return NULL;

	RegionSize = length;

	ret->State = State;
	ret->Type = Type;
	ret->Protect = Protect;
	if (kernel_address)
		ret->kernel_address = kernel_address + RegionSize;

	assert( !(RegionSize&0xfff) );
	assert( !(ret->RegionSize&0xfff) );
	assert( BaseAddress < ret->BaseAddress );
	assert( (BaseAddress+RegionSize) == ret->BaseAddress );

	return ret;
}

int mblock::map_local( int prot )
{
	kernel_address = pages->map_local( prot );
	if (kernel_address == (BYTE*) -1)
		return -1;
	return 0;
}

int mblock::map_remote( address_space *vm, BYTE *address, ULONG length, int prot )
{
	ULONG ofs = address - BaseAddress;

	assert( address >= BaseAddress );
	assert( address < (BaseAddress + RegionSize) );

	assert( length <= RegionSize );
	assert( (address + length) <= (BaseAddress + RegionSize) );

	return pages->map_remote( vm, address, length, prot, ofs );
}

int mblock::local_unmap()
{
	if (kernel_address)
		::munmap( kernel_address, RegionSize );
	kernel_address = 0;
	return 0;
}

int mblock::remote_unmap( address_space *vm )
{
	return vm->munmap( BaseAddress, RegionSize );
}

ULONG mblock::mmap_flag_from_page_prot( ULONG prot )
{
	// calculate the right protections first
	switch (prot)
	{
	case PAGE_EXECUTE:
		return PROT_EXEC;
	case PAGE_EXECUTE_READ:
		return PROT_EXEC | PROT_READ;
	case PAGE_EXECUTE_READWRITE:
		return PROT_EXEC | PROT_READ | PROT_WRITE;
	case PAGE_EXECUTE_WRITECOPY:
		dprintf("FIXME, PAGE_EXECUTE_WRITECOPY not supported\n");
		return PROT_EXEC | PROT_READ | PROT_WRITE;
	case PAGE_NOACCESS:
		return 0;
	case PAGE_READONLY:
		return PROT_READ;
	case PAGE_READWRITE:
		return PROT_READ | PROT_WRITE;
	case PAGE_WRITECOPY:
		dprintf("FIXME, PAGE_WRITECOPY not supported\n");
		return PROT_READ | PROT_WRITE;
	}
	dprintf("shouldn't get here\n");
	return STATUS_INVALID_PAGE_PROTECTION;
}

void mblock::commit( address_space *vm, BYTE *address, size_t length, int prot )
{
	if (State != MEM_COMMIT)
	{
		State = MEM_COMMIT;
		Protect = prot;
		Type = MEM_PRIVATE;

		//dprintf("committing %p/%p %08lx\n", kernel_address, BaseAddress, RegionSize);
		if (0 > map_local( PROT_READ | PROT_WRITE ) &&
			0 > map_local( PROT_READ ))
			die("couldn't map user memory into kernel %d\n", errno);
	}

	remote_remap( vm, address, length, prot, tracer != 0 );
}

void mblock::remote_remap( address_space *vm, BYTE *address, size_t length, int prot, bool except )
{
	int mmap_flags = 0;

	if (!except)
		mmap_flags = mmap_flag_from_page_prot( prot );

	if (0 > map_remote( vm, address, length, mmap_flags ))
		die("map_remote failed\n");
}

void mblock::set_tracer( address_space *vm, block_tracer *bt )
{
	tracer = bt;
	remote_remap( vm, BaseAddress, RegionSize, Protect, tracer != 0 );
}

void mblock::reserve( address_space *vm, size_t length, int prot )
{
	assert( State != MEM_COMMIT );
	if (State == MEM_RESERVE)
		return;
	State = MEM_RESERVE;
	Protect = prot;
	Type = MEM_PRIVATE;
	// FIXME: maybe allocate memory here
	assert( length == RegionSize );
}

void mblock::uncommit( address_space *vm )
{
	if (State != MEM_COMMIT)
		return;
	remote_unmap( vm );
	local_unmap();
	State = MEM_RESERVE;
	kernel_address = NULL;
}

void mblock::unreserve( address_space *vm )
{
	assert( State != MEM_COMMIT );
	if (State != MEM_RESERVE)
		return;

	/* mark it as unallocated */
	Protect = 0;
	State = MEM_FREE;
	Type = 0;

	// FIXME: free core here?
}

NTSTATUS mblock::query( BYTE *start, MEMORY_BASIC_INFORMATION *info )
{
	info->BaseAddress = (void*)((UINT)start & 0xfffff000);
	info->AllocationBase = BaseAddress;
	info->AllocationProtect = Protect;
	info->RegionSize = RegionSize;
	info->State = State;
	if (State == MEM_RESERVE)
		info->Protect = 0;
	else
		info->Protect = Protect;
	info->Type = Type;

	return STATUS_SUCCESS;
}

void mblock::on_access( BYTE *address, ULONG Eip )
{
	if (tracer)
		tracer->on_access( this, address, Eip );
}

void mblock::set_section( object_t *s )
{
	if (section)
		release( section );
	section = s;
	addref( section );
}

void block_tracer::on_access( mblock *mb, BYTE *address, ULONG Eip )
{
	fprintf(stderr, "%04lx: accessed %p from %08lx\n",
			current->trace_id(), address, Eip );
}

block_tracer::~block_tracer()
{
}
