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

object_list_t object_list;

class object_dir_impl_t : public object_dir_t
{
	object_list_t object_list;
public:
	object_dir_impl_t();
	virtual ~object_dir_impl_t();
	virtual bool access_allowed( ACCESS_MASK access, ACCESS_MASK handle_access );
};

object_dir_t::object_dir_t()
{
}

object_dir_t::~object_dir_t()
{
}

object_dir_impl_t::object_dir_impl_t()
{
}

object_dir_impl_t::~object_dir_impl_t()
{
}

bool object_dir_impl_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
			 DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY,
			 DIRECTORY_ALL_ACCESS );
}

class object_dir_factory : public object_factory
{
public:
	object_dir_factory() {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS object_dir_factory::alloc_object(object_t** obj)
{
	*obj = new object_dir_impl_t;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

object_t *create_directory_object( PCWSTR name )
{
	object_dir_impl_t *obj = new object_dir_impl_t;

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

NTSTATUS find_object_by_name( object_t **out, const OBJECT_ATTRIBUTES *oa )
{
	if (!oa || !oa->ObjectName || !oa->ObjectName->Buffer || !oa->ObjectName->Buffer[0])
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	for( object_iter_t i(object_list); i; i.next() )
	{
		object_t *obj = i;

		if (!obj->name.compare( oa->ObjectName, oa->Attributes & OBJ_CASE_INSENSITIVE ))
			continue;

		addref( obj );
		*out = obj;
		return STATUS_SUCCESS;
	}

	return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS name_object( object_t *obj, const OBJECT_ATTRIBUTES *oa )
{
	object_t *existing = NULL;
	NTSTATUS r;

	if (!oa)
		 return STATUS_SUCCESS;

	obj->attr = oa->Attributes;
	if (!oa->ObjectName)
		 return STATUS_SUCCESS;
	if (!oa->ObjectName->Buffer)
		 return STATUS_SUCCESS;

	r = find_object_by_name( &existing, oa );
	if (r == STATUS_SUCCESS)
	{
		release( existing );
		return STATUS_OBJECT_NAME_COLLISION;
	}

	r = obj->name.copy( oa->ObjectName );
	if (r != STATUS_SUCCESS)
		return r;

	if (0)
		dprintf("name is -> %pus", &obj->name );

	object_list.append( obj );

	return STATUS_SUCCESS;
}

NTSTATUS get_named_object( object_t **out, const OBJECT_ATTRIBUTES *oa )
{
	object_t *obj;
	NTSTATUS r;

	if (!oa || !oa->ObjectName || !oa->ObjectName->Buffer || !oa->ObjectName->Buffer[0])
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	r = find_object_by_name( &obj, oa );
	if (r != STATUS_SUCCESS)
		return r;

	*out = obj;
	return STATUS_SUCCESS;
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

	object_dir_t* dir = 0;
	r = object_from_handle( dir, DirectoryHandle, DIRECTORY_QUERY );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("fixme\n");

	return STATUS_NO_MORE_ENTRIES;
}
