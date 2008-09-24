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


#include <stdarg.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "objdir.h"
#include "object.inl"
#include "ntcall.h"

object_dir_t::object_dir_t()
{
}

object_dir_t::~object_dir_t()
{
}

bool object_dir_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
			 DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY,
			 DIRECTORY_ALL_ACCESS );
}

object_dir_t* object_dir_from_object( object_t *obj )
{
	return dynamic_cast<object_dir_t*>(obj);
}

class object_dir_factory : public object_factory
{
public:
	object_dir_factory() {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS object_dir_factory::alloc_object(object_t** obj)
{
	*obj = new object_dir_t;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

object_t *create_directory_object( PCWSTR name )
{
	object_dir_t *obj = new object_dir_t;

	unicode_string_t us;
	us.copy(name);
	OBJECT_ATTRIBUTES oa;
	oa.Length = sizeof oa;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.ObjectName = &us;
	NTSTATUS r = name_object( obj, &oa );
	if (r != STATUS_SUCCESS)
	{
		release( obj );
		obj = 0;
	}
	return obj;
}

NTSTATUS NTAPI NtCreateDirectoryObject(
	PHANDLE DirectoryHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	dprintf("%p %08lx %p\n", DirectoryHandle, DesiredAccess, ObjectAttributes );

	object_dir_factory factory;
	return factory.create( DirectoryHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtOpenDirectoryObject(
	PHANDLE DirectoryObjectHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	return nt_open_object<object_dir_t>( DirectoryObjectHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtQueryDirectoryObject(
	HANDLE DirectoryHandle,
	PVOID Buffer,
	ULONG BufferLength,
	BOOLEAN ReturnSingleEntry,
	BOOLEAN RestartScan,
	PULONG Offset,  // called Context in Native API reference
	PULONG ReturnLength)
{
	dprintf("%p %p %lu %u %u %p %p\n", DirectoryHandle, Buffer, BufferLength,
			ReturnSingleEntry, RestartScan, Offset, ReturnLength);

	ULONG ofs = 0;
	NTSTATUS r = copy_from_user( &ofs, Offset, sizeof ofs );
	if (r != STATUS_SUCCESS)
		return r;

	object_t* obj;
	r = object_from_handle( obj, DirectoryHandle, DIRECTORY_QUERY );
	if (r != STATUS_SUCCESS)
		return r;

	object_dir_t* dir = object_dir_from_object( obj );
	if (!dir)
		return STATUS_OBJECT_TYPE_MISMATCH;

	dprintf("fixme\n");

	return STATUS_NO_MORE_ENTRIES;
}
