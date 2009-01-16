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


#include "config.h"

#include <stdarg.h>
#include <sys/mman.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "mem.h"
#include "object.h"
#include "ntcall.h"
#include "object.inl"
#include "section.h"
#include "unicode.h"
#include "file.h"

struct pe_section_t : public section_t {
public:
	pe_section_t( int f, BYTE *a, size_t l, ULONG attr, ULONG prot );
	virtual ~pe_section_t();
	virtual NTSTATUS mapit( address_space *vm, BYTE *&addr, ULONG ZeroBits, ULONG State, ULONG Protect );
	virtual NTSTATUS query( SECTION_IMAGE_INFORMATION *image );
	IMAGE_EXPORT_DIRECTORY* get_exports_table();
	IMAGE_NT_HEADERS* get_nt_header();
	DWORD get_proc_address( const char *name );
	DWORD get_proc_address( ULONG ordinal );
	void add_relay( address_space *vm );
	bool add_relay_stub( address_space *vm, BYTE *stub_addr, ULONG func, ULONG *user_addr, ULONG thunk_ofs );
	const char *get_symbol( ULONG address );
	const char *name_of_ordinal( ULONG ordinal );
private:
	void *virtual_addr_to_offset( DWORD virtual_ofs );
};

section_t::~section_t()
{
	munmap( addr, len );
	close(fd);
}

int section_t::get_fd()
{
	return fd;
}

void section_t::addref()
{
	::addref( this );
}

void section_t::release()
{
	::release( this );
}

NTSTATUS section_t::mapit( address_space *vm, BYTE *&addr, ULONG ZeroBits, ULONG State, ULONG prot )
{
	dprintf("anonymous map\n");
	if ((prot&0xff) > (Protect&0xff))
		return STATUS_INVALID_PARAMETER;
	NTSTATUS r = vm->map_fd( &addr, ZeroBits, len, State, prot, this );
	return r;
}

section_t::section_t( int _fd, BYTE *a, size_t l, ULONG attr, ULONG prot ) :
	fd( _fd )
{
	len = l;
	addr = a;
	Attributes = attr;
	Protect = prot;
}

NTSTATUS section_t::query( SECTION_BASIC_INFORMATION *basic )
{
	basic->BaseAddress = 0; // FIXME
	basic->Attributes = Attributes;
	basic->Size.QuadPart = len;
	return STATUS_SUCCESS;
}

NTSTATUS section_t::query( SECTION_IMAGE_INFORMATION *image )
{
	return STATUS_INVALID_PARAMETER;
}

void* section_t::get_kernel_address()
{
	return addr;
}

const char *section_t::get_symbol( ULONG address )
{
	return 0;
}

pe_section_t::pe_section_t( int fd, BYTE *a, size_t l, ULONG attr, ULONG prot ) :
	section_t( fd, a, l, attr, prot )
{
}

pe_section_t::~pe_section_t()
{
}

NTSTATUS pe_section_t::query( SECTION_IMAGE_INFORMATION *image )
{
	IMAGE_NT_HEADERS *nt = get_nt_header();

	// FIXME: assumes fixed base address...?
	image->EntryPoint = (BYTE*) nt->OptionalHeader.ImageBase +
					   nt->OptionalHeader.AddressOfEntryPoint;
	image->StackZeroBits = 0;
	image->StackReserved = nt->OptionalHeader.SizeOfStackReserve;
	image->StackCommit = nt->OptionalHeader.SizeOfStackCommit;
	image->ImageSubsystem = nt->OptionalHeader.Subsystem;
	image->SubsystemVersionLow = nt->OptionalHeader.MinorSubsystemVersion;
	image->SubsystemVersionHigh = nt->OptionalHeader.MajorSubsystemVersion;
	image->Unknown1 = 0;
	image->ImageCharacteristics = 0x80000000 | nt->FileHeader.Characteristics;
	image->ImageMachineType = 0x10000 | nt->FileHeader.Machine;
	//image->ImageMachineType = 0;
	//info.image.Unknown2[3];
	return STATUS_SUCCESS;
}

NTSTATUS create_section( object_t **obj, object_t *file, PLARGE_INTEGER psz, ULONG attribs, ULONG protect )
{
	section_t *sec;
	NTSTATUS r = create_section( &sec, file, psz, attribs, protect );
	if (r == STATUS_SUCCESS)
		*obj = sec;
	return r;
}

NTSTATUS create_section( section_t **section, object_t *obj, PLARGE_INTEGER psz, ULONG attribs, ULONG protect )
{
	section_t *s;
	BYTE *addr;
	int fd, ofs = 0, r;
	ULONG len;

	if (obj)
	{
		// FIXME: probably better to have a file_t passed in
		file_t *file = dynamic_cast<file_t*>( obj );
		if (!file)
			return STATUS_OBJECT_TYPE_MISMATCH;

		fd = file->get_fd();
		if (fd<0)
			return STATUS_OBJECT_TYPE_MISMATCH;
		fd = dup(fd);

		if (psz)
			len = psz->QuadPart;
		else
		{
			r = lseek( fd, 0, SEEK_END );
			if (r < 0)
				return STATUS_UNSUCCESSFUL;
			len = r;
		}
	}
	else
	{
		if (!psz)
			return STATUS_INVALID_PARAMETER;
		len = psz->QuadPart;
		fd = create_mapping_fd( len );
		ofs = 0;
	}

	len += 0xfff;
	len &= ~0xfff;

	int write_mask = PAGE_READWRITE | PAGE_WRITECOPY /* |
					 PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY */;
	int mmap_prot = PROT_READ;
	if (protect & write_mask)
		mmap_prot |= PROT_WRITE;
	addr = (BYTE*) mmap( NULL, len, mmap_prot, MAP_SHARED, fd, ofs );
	if (addr == (BYTE*) -1)
	{
		dprintf("map failed!\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (attribs & SEC_IMAGE)
		s = new pe_section_t( fd, addr, len, attribs, protect );
	else
		s = new section_t( fd, addr, len, attribs, protect );

	if (!s)
		return STATUS_NO_MEMORY;

	*section = s;

	return STATUS_SUCCESS;
}

IMAGE_NT_HEADERS *pe_section_t::get_nt_header()
{
	IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*) addr;
	IMAGE_NT_HEADERS *nt;

	if (dos->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	nt = (IMAGE_NT_HEADERS*) ((BYTE*) addr + dos->e_lfanew);
	if (len < (dos->e_lfanew + sizeof (*nt)))
		return NULL;

	if (nt->Signature != IMAGE_NT_SIGNATURE)
		return NULL;

	if (nt->FileHeader.SizeOfOptionalHeader != sizeof nt->OptionalHeader)
		return NULL;

	return nt;
}

NTSTATUS mapit( address_space *vm, object_t *obj, BYTE *&addr )
{
	section_t *sec = dynamic_cast<section_t*>( obj );
	if (!sec)
		return STATUS_OBJECT_TYPE_MISMATCH;
	return sec->mapit( vm, addr, 0, MEM_COMMIT, PAGE_READONLY );
}

// stub code for inserting into user address space
extern BYTE *relay_code;
extern ULONG relay_code_size;
__asm__ (
	"\n"
".data\n"
".globl " ASM_NAME_PREFIX "relay_code\n"
ASM_NAME_PREFIX "relay_code:\n"
".align 4\n"
	"\tpushl %eax\n"			// save registers
	"\tpushl %ecx\n"
	"\tmovl 8(%esp), %ecx\n"	// get the return address
	"\tmovl (%ecx), %ecx\n"
	"\tmovl %ecx, 8(%esp)\n"
	"\tmovl $0x101, %eax\n"
	"\tint $0x2d\n"				// debug call
	"\tpopl %ecx\n"
	"\tpopl %eax\n"
	"\tret\n"
ASM_NAME_PREFIX "relay_code_end:\n"
".align 4\n"
ASM_NAME_PREFIX "relay_code_size:\n"
	"\t.long " ASM_NAME_PREFIX "relay_code_end - " ASM_NAME_PREFIX "relay_code\n"
);

struct __attribute__ ((packed)) relay_stub {
	BYTE x1; //  0xe8 call target
	ULONG common;
	ULONG target;
};

bool pe_section_t::add_relay_stub( address_space *vm, BYTE *stub_addr, ULONG func, ULONG *user_addr, ULONG thunk_ofs )
{
	IMAGE_NT_HEADERS *nt = get_nt_header();

	if (!func)
		return true;

	// replace the value in the stub with
	// the address of the function to forward to
	relay_stub stub;
	assert( sizeof stub == 9 );
	stub.x1 = 0xe8; // jump relative
	stub.common = thunk_ofs - 5;
	stub.target = nt->OptionalHeader.ImageBase + func;

	// copy the stub
	//PBYTE stub_addr = p + relay_code_size + sizeof stub*i;
	NTSTATUS r = vm->copy_to_user( stub_addr, &stub, sizeof stub );
	if (r < STATUS_SUCCESS)
	{
		dprintf("stub copy failed %08lx\n", r);
		return false;
	}

	// write the offset of the stub back to the import table
	ULONG ofs = stub_addr - (PBYTE) nt->OptionalHeader.ImageBase;
	r = vm->copy_to_user( user_addr, &ofs, sizeof ofs );
	if (r < STATUS_SUCCESS)
	{
		dprintf("failed to set address %08lx\n", r);
		return false;
	}

	//dprintf("[%02ld] old = %08lx new = %p\n", i, stub.target, stub_addr);
	return true;
}

// parse the exports table and generate relay code
void pe_section_t::add_relay(address_space *vm)
{
	IMAGE_DATA_DIRECTORY *export_data_dir;
	IMAGE_SECTION_HEADER *section;
	IMAGE_EXPORT_DIRECTORY *exp;
	IMAGE_NT_HEADERS *nt;
	ULONG *funcs;
	ULONG i;

	nt = get_nt_header();

	export_data_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	section = (IMAGE_SECTION_HEADER*) &nt[1];
	exp = (IMAGE_EXPORT_DIRECTORY*) virtual_addr_to_offset( export_data_dir->VirtualAddress );
	if (!exp)
	{
		dprintf("no exports\n");
		return;
	}

	BYTE *p = 0;
	ULONG sz = (exp->NumberOfNames * sizeof (relay_stub) + 0xfff) & ~0xfff;
	dprintf("relay stubs at %p, %08lx\n", p, sz);
	NTSTATUS r = vm->allocate_virtual_memory( &p, 0, sz, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (r < STATUS_SUCCESS)
	{
		die("anonymous map failed %08lx\n", r);
		return;
	}

	funcs = (DWORD*) virtual_addr_to_offset( exp->AddressOfFunctions );
	if (!funcs)
	{
		die("virtual_addr_to_offset failed\n");
		return;
	}

	r = vm->copy_to_user( p, relay_code, relay_code_size );
	if (r < STATUS_SUCCESS)
	{
		dprintf("relay copy failed %08lx\n", r);
		return;
	}

	//dprintf("%ld exported functions\n", exp->NumberOfFunctions);
	for (i = 0; i<exp->NumberOfFunctions; i++)
	{
		ULONG *user_addr = (ULONG*) (nt->OptionalHeader.ImageBase + exp->AddressOfFunctions);
		ULONG thunk_ofs = 0 - (relay_code_size + sizeof (relay_stub)*i );
		PBYTE stub_addr = p + relay_code_size + sizeof (relay_stub)*i;

		if (!add_relay_stub( vm, stub_addr, funcs[i], user_addr + i, thunk_ofs ))
			break;
	}
}

// maybe create a temp file to remap?
NTSTATUS pe_section_t::mapit( address_space *vm, BYTE *&base, ULONG ZeroBits, ULONG State, ULONG Protect )
{
	IMAGE_DOS_HEADER *dos;
	IMAGE_NT_HEADERS *nt;
	IMAGE_SECTION_HEADER *sections;
	int r, sz, i;
	BYTE *p;
	mblock *mb;

	dos = (IMAGE_DOS_HEADER*) addr;

	nt = get_nt_header();
	if (!nt)
		return STATUS_UNSUCCESSFUL;

	p = (BYTE*) nt->OptionalHeader.ImageBase;
	dprintf("image at %p\n", p);
	r = vm->allocate_virtual_memory( &p, ZeroBits, 0x1000, MEM_COMMIT, PAGE_READONLY );
	if (r < STATUS_SUCCESS)
	{
		dprintf("map failed\n");
		goto fail;
	}

	// use of mblock here is a bit of a hack
	// should convert this function to create a flat file to map
	mb = vm->find_block( p );
	mb->set_section( this );

	r = vm->copy_to_user( p, addr, 0x1000 );
	if (r < STATUS_SUCCESS)
		dprintf("copy_to_user failed\n");

	sections = (IMAGE_SECTION_HEADER*) (addr + dos->e_lfanew + sizeof (*nt));

	if (option_trace)
		 dprintf("read %d sections, load at %08lx\n",
		   nt->FileHeader.NumberOfSections,
		   nt->OptionalHeader.ImageBase);

	for ( i=0; i<nt->FileHeader.NumberOfSections; i++ )
	{
		if (option_trace)
			dprintf("%-8s %08lx %08lx %08lx %08lx\n",
			   sections[i].Name,
			   sections[i].VirtualAddress,
			   sections[i].PointerToRawData,
			   sections[i].SizeOfRawData,
			   sections[i].Misc.VirtualSize);
		if (sections[i].VirtualAddress == 0)
			die("virtual address was zero!\n");

		sz = (sections[i].Misc.VirtualSize + 0xfff )& ~0xfff;
		if (!sz)
			continue;

		p = (BYTE*) (nt->OptionalHeader.ImageBase + sections[i].VirtualAddress);
		// FIXME - map sections with correct permissions
		r = vm->allocate_virtual_memory( &p, 0, sz, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (r < STATUS_SUCCESS)
			die("anonymous map failed %08x\n", r);
		mb = vm->find_block( p );
		mb->set_section( this );

		if (sections[i].SizeOfRawData)
		{
			r = vm->copy_to_user( p, addr + sections[i].PointerToRawData, sections[i].SizeOfRawData);
			if (r < STATUS_SUCCESS)
				dprintf("copy_to_user failed\n");
		}
	}

	//if (option_trace)
		//add_relay(vm);

	base = (BYTE*) nt->OptionalHeader.ImageBase;
fail:

	return r;
}

void *pe_section_t::virtual_addr_to_offset( DWORD virtual_ofs )
{
	IMAGE_NT_HEADERS *nt = get_nt_header();
	IMAGE_SECTION_HEADER *section = (IMAGE_SECTION_HEADER*) &nt[1];
	int i;

	for (i=0; i<nt->FileHeader.NumberOfSections; i++)
	{
		if (section[i].VirtualAddress <= virtual_ofs &&
			(section[i].VirtualAddress + section[i].SizeOfRawData) > virtual_ofs )
		{
			return addr + (virtual_ofs - section[i].VirtualAddress + section[i].PointerToRawData);
		}
	}
	return NULL;
}

// just to find LdrInitializeThunk
DWORD get_proc_address( object_t *obj, const char *name )
{
	pe_section_t *sec = dynamic_cast<pe_section_t*>( obj );
	if (!sec)
		return 0;
	return sec->get_proc_address( name );
}

IMAGE_EXPORT_DIRECTORY* pe_section_t::get_exports_table()
{
	IMAGE_NT_HEADERS* nt = get_nt_header();
	IMAGE_DATA_DIRECTORY *export_data_dir;

	export_data_dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	return (IMAGE_EXPORT_DIRECTORY*) virtual_addr_to_offset( export_data_dir->VirtualAddress );
}

DWORD pe_section_t::get_proc_address( const char *name )
{
	dprintf("%s\n", name);

	IMAGE_EXPORT_DIRECTORY *exp = get_exports_table();
	if (!exp)
		return 0;

	DWORD *p = (DWORD*) virtual_addr_to_offset( exp->AddressOfNames );
	if (!p)
		return 0;

	ULONG left = 0, n = 0, right = exp->NumberOfNames - 1;
	int r = -1;
	while ( left <= right )
	{
		n = (left+right)/2;
		char *x = (char*) virtual_addr_to_offset( p[n] );
		//dprintf("compare %s,%s\n", name, x);
		r = strcmp(name, x);
		if (r == 0)
			break;
		if (r < 0)
			right = n - 1;
		else
			left = n + 1;
	}

	// return the address if we get a match
	if (r != 0)
		return 0;

	assert( n < exp->NumberOfNames );

	return get_proc_address( n );
}

DWORD pe_section_t::get_proc_address( ULONG ordinal )
{
	IMAGE_EXPORT_DIRECTORY *exp = get_exports_table();

	if (ordinal >= exp->NumberOfFunctions)
		return 0;
	WORD *ords = (WORD*) virtual_addr_to_offset( exp->AddressOfNameOrdinals );
	if (!ords)
		return 0;
	DWORD *funcs = (DWORD*) virtual_addr_to_offset( exp->AddressOfFunctions );
	if (!funcs)
		return 0;
	//dprintf("returning %ld -> %04x -> %08lx\n", ordinal, ords[ordinal], funcs[ords[ordinal]]);
	return funcs[ords[ordinal]];
}

const char *pe_section_t::name_of_ordinal( ULONG ordinal )
{
	IMAGE_EXPORT_DIRECTORY* exp = get_exports_table();

	DWORD *names = (DWORD*) virtual_addr_to_offset( exp->AddressOfNames );
	if (!names)
		return 0;
	WORD *ords = (WORD*) virtual_addr_to_offset( exp->AddressOfNameOrdinals );
	if (!ords)
		return 0;

	// there's no NumberOfNameOrdinals.  ordinal better be valid...
	for (int i=0; i<0xffff; i++)
		if (ords[i] == ordinal)
			return (char*) virtual_addr_to_offset( names[i] );
	return 0;
}

const char *pe_section_t::get_symbol( ULONG address )
{
	IMAGE_EXPORT_DIRECTORY* exp = get_exports_table();

	// this translation probably should be done in address_space_impl_t::get_symbol
	IMAGE_NT_HEADERS *nt = get_nt_header();
	address -= nt->OptionalHeader.ImageBase;

	ULONG *funcs = (ULONG*) virtual_addr_to_offset( exp->AddressOfFunctions );
	if (!funcs)
		return 0;

	for (ULONG i=0; i<exp->NumberOfFunctions; i++)
		if (funcs[i] == address)
			return name_of_ordinal( i );

	return 0;
}

const char *get_section_symbol( object_t *object, ULONG address )
{
	dprintf("%p %08lx\n", object, address);
	if (!object)
		return 0;
	section_t *section = dynamic_cast<section_t*>( object );
	dprintf("%p %08lx\n", section, address);
	return section->get_symbol( address );
}

void *get_entry_point( process_t *p )
{
	IMAGE_DOS_HEADER *dos = NULL;
	IMAGE_NT_HEADERS *nt;
	ULONG ofs = 0;
	ULONG entry = 0;
	NTSTATUS r;

	PPEB ppeb = (PPEB) p->peb_section->get_kernel_address();
	dos = (IMAGE_DOS_HEADER*) ppeb->ImageBaseAddress;

	r = p->vm->copy_from_user( &ofs, &dos->e_lfanew, sizeof dos->e_lfanew );
	if (r < STATUS_SUCCESS)
		return NULL;

	nt = (IMAGE_NT_HEADERS*) ((BYTE*) dos + ofs);

	r = p->vm->copy_from_user( &entry, &nt->OptionalHeader.AddressOfEntryPoint,
						   sizeof nt->OptionalHeader.AddressOfEntryPoint );
	if (r < STATUS_SUCCESS)
		return NULL;

	return ((BYTE*)dos) + entry;
}

class section_factory : public object_factory
{
private:
	object_t *file;
	PLARGE_INTEGER SectionSize;
	ULONG Attributes;
	ULONG Protect;
public:
	section_factory( object_t *_file, PLARGE_INTEGER _SectionSize, ULONG _Attributes, ULONG _Protect );
	virtual NTSTATUS alloc_object(object_t** obj);
};

section_factory::section_factory(
	object_t *_file,
	PLARGE_INTEGER _SectionSize,
	ULONG _Attributes,
	ULONG _Protect ) :
	file(_file),
	SectionSize( _SectionSize),
	Attributes( _Attributes),
	Protect( _Protect )
{
}

NTSTATUS section_factory::alloc_object(object_t** obj)
{
	NTSTATUS r = create_section( obj, file, SectionSize, Attributes, Protect );
	if (r < STATUS_SUCCESS)
		return r;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

#define VALID_SECTION_FLAGS (\
	SEC_BASED | SEC_NO_CHANGE | SEC_FILE | SEC_IMAGE |\
	SEC_VLM | SEC_RESERVE | SEC_COMMIT | SEC_NOCACHE)

NTSTATUS NTAPI NtCreateSection(
	PHANDLE SectionHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PLARGE_INTEGER SectionSize,
	ULONG Protect,
	ULONG Attributes,
	HANDLE FileHandle )
{
	NTSTATUS r;
	object_t *file = NULL;
	LARGE_INTEGER sz;

	dprintf("%p %08lx %p %p %08lx %08lx %p\n", SectionHandle, DesiredAccess,
			ObjectAttributes, SectionSize, Protect, Attributes, FileHandle );

	// check there's no bad flags
	if (Attributes & ~VALID_SECTION_FLAGS)
		return STATUS_INVALID_PARAMETER_6;

	switch (Attributes & (SEC_IMAGE|SEC_COMMIT|SEC_RESERVE))
	{
	case SEC_IMAGE:
	case SEC_RESERVE:
	case SEC_COMMIT:
		break;
	default:
		return STATUS_INVALID_PARAMETER_6;
	}

	r = verify_for_write( SectionHandle, sizeof *SectionHandle );
	if (r < STATUS_SUCCESS)
		return r;

	// PE sections cannot be written to
	if (Attributes & SEC_IMAGE)
	{
		switch (Protect)
		{
		case PAGE_READONLY:
		case PAGE_EXECUTE:
		case PAGE_EXECUTE_READ:
			break;
		default:
			return STATUS_INVALID_PAGE_PROTECTION;
		}

		if (!FileHandle)
			return STATUS_INVALID_FILE_FOR_SECTION;

		r = object_from_handle( file, FileHandle, 0 );
		if (r < STATUS_SUCCESS)
			return r;

		SectionSize = 0;
	}
	else
	{
		switch (Protect)
		{
		case PAGE_READONLY:
		case PAGE_READWRITE:
		case PAGE_EXECUTE:
		case PAGE_EXECUTE_READ:
		case PAGE_WRITECOPY:
		case PAGE_EXECUTE_READWRITE:
		case PAGE_EXECUTE_WRITECOPY:
			break;
		default:
			return STATUS_INVALID_PAGE_PROTECTION;
		}

		if (FileHandle)
		{
			r = object_from_handle( file, FileHandle, 0 );
			if (r < STATUS_SUCCESS)
				return r;
		}
	}

	if (SectionSize)
	{
		r = copy_from_user( &sz, SectionSize, sizeof sz );
		if (r < STATUS_SUCCESS)
			return r;
		if (sz.QuadPart == 0)
			return STATUS_INVALID_PARAMETER_4;
		SectionSize = &sz;
	}

	section_factory factory( file, SectionSize, Attributes, Protect );
	return factory.create( SectionHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtOpenSection(
	PHANDLE SectionHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	return nt_open_object<section_t>( SectionHandle, DesiredAccess, ObjectAttributes );
}

// pg 108
NTSTATUS NTAPI NtMapViewOfSection(
	HANDLE SectionHandle,
	HANDLE ProcessHandle,
	PVOID *BaseAddress,
	ULONG ZeroBits,
	ULONG CommitSize,
	PLARGE_INTEGER SectionOffset,
	PULONG ViewSize,
	SECTION_INHERIT InheritDisposition,
	ULONG AllocationType,
	ULONG Protect )
{
	process_t *p = NULL;
	BYTE *addr = NULL;
	NTSTATUS r;

	dprintf("%p %p %p %lu %08lx %p %p %u %08lx %08lx\n",
			SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize,
			SectionOffset, ViewSize, InheritDisposition, AllocationType, Protect );

	r = process_from_handle( ProcessHandle, &p );
	if (r < STATUS_SUCCESS)
		return r;

	section_t *section = 0;
	r = object_from_handle( section, SectionHandle, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_from_user( &addr, BaseAddress, sizeof addr );
	if (r < STATUS_SUCCESS)
		return r;

	if (addr)
		dprintf("requested specific address %p\n", addr);

	r = verify_for_write( ViewSize, sizeof *ViewSize );
	if (r < STATUS_SUCCESS)
		return r;

	r = section->mapit( p->vm, addr, ZeroBits,
						MEM_COMMIT | (AllocationType&MEM_TOP_DOWN), Protect );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_to_user( BaseAddress, &addr, sizeof addr );

	dprintf("mapped at %p\n", addr );

	return r;
}

NTSTATUS NTAPI NtUnmapViewOfSection(
	HANDLE ProcessHandle,
	PVOID BaseAddress )
{
	process_t *p = NULL;
	NTSTATUS r;

	dprintf("%p %p\n", ProcessHandle, BaseAddress );

	r = process_from_handle( ProcessHandle, &p );
	if (r < STATUS_SUCCESS)
		return r;

	r = p->vm->unmap_view( BaseAddress );

	return r;
}

NTSTATUS NTAPI NtQuerySection(
	HANDLE SectionHandle,
	SECTION_INFORMATION_CLASS SectionInformationClass,
	PVOID SectionInformation,
	ULONG SectionInformationLength,
	PULONG ResultLength )
{
	union {
		SECTION_BASIC_INFORMATION basic;
		SECTION_IMAGE_INFORMATION image;
	} info;
	NTSTATUS r;
	ULONG len;

	dprintf("%p %u %p %lu %p\n", SectionHandle, SectionInformationClass,
			SectionInformation, SectionInformationLength, ResultLength );

	section_t *section = 0;
	r = object_from_handle( section, SectionHandle, SECTION_QUERY );
	if (r < STATUS_SUCCESS)
		return r;

	memset( &info, 0, sizeof info );

	switch (SectionInformationClass)
	{
	case SectionBasicInformation:
		len = sizeof info.basic;
		r = section->query( &info.basic );
		break;

	case SectionImageInformation:
		len = sizeof info.image;
		r = section->query( &info.image );
		break;

	default:
		r = STATUS_INVALID_PARAMETER;
	}

	if (r < STATUS_SUCCESS)
		return r;

	if (len > SectionInformationLength)
		return STATUS_BUFFER_TOO_SMALL;

	r = copy_to_user( SectionInformation, &info, len );
	if (r == STATUS_SUCCESS && ResultLength)
		r = copy_to_user( ResultLength, &len, sizeof len );

	return r;
}
