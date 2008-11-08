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
#include "symlink.h"

class object_dir_impl_t : public object_dir_t
{
	object_list_t object_list;
public:
	object_dir_impl_t();
	virtual ~object_dir_impl_t();
	virtual bool access_allowed( ACCESS_MASK access, ACCESS_MASK handle_access );
	virtual void unlink( object_t *child );
	virtual void append( object_t *child );
public:
	object_t *lookup( UNICODE_STRING& name, bool ignore_case );
	NTSTATUS add( object_t *obj, UNICODE_STRING& name, bool ignore_case );
};

static object_dir_impl_t *root = 0;

object_dir_t::object_dir_t()
{
}

object_dir_t::~object_dir_t()
{
}

void object_dir_t::set_obj_parent( object_t *child, object_dir_t *dir )
{
	child->parent = dir;
}

object_dir_impl_t::object_dir_impl_t()
{
}

object_dir_impl_t::~object_dir_impl_t()
{
	object_iter_t i(object_list);
	while( i )
	{
		object_t *obj = i;
		i.next();
		unlink( obj );
	}
}

void object_dir_impl_t::unlink( object_t *obj )
{
	assert( obj );
	object_list.unlink( obj );
	set_obj_parent( obj, 0 );
}

void object_dir_impl_t::append( object_t *obj )
{
	assert( obj );
	object_list.append( obj );
	set_obj_parent( obj, this );
}

bool object_dir_impl_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
			 DIRECTORY_CREATE_OBJECT | DIRECTORY_CREATE_SUBDIRECTORY,
			 DIRECTORY_ALL_ACCESS );
}

object_t *object_dir_impl_t::lookup( UNICODE_STRING& name, bool ignore_case )
{
	//dprintf("searching for %pus\n", &name );
	for( object_iter_t i(object_list); i; i.next() )
	{
		object_t *obj = i;
		unicode_string_t& entry_name  = obj->get_name();
		//dprintf("checking %pus\n", &entry_name );
		if (!entry_name.compare( &name, ignore_case ))
			continue;
		return obj;
	}
	return 0;
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
	memset( &oa, 0, sizeof oa );
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

NTSTATUS parse_path( const OBJECT_ATTRIBUTES* oa, object_dir_t*& dir, ULONG& ofs )
{
	// no name
	if (!oa || !oa->ObjectName || !oa->ObjectName->Buffer)
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	// too short
	if (oa->ObjectName->Length < 2)
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	// odd length
	if (oa->ObjectName->Length & 1)
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	// look each directory in the path and make sure it exists
	bool case_insensitive = oa->Attributes & OBJ_CASE_INSENSITIVE;
	UNICODE_STRING& name = *oa->ObjectName;

	NTSTATUS r;
	ofs = 0;

	// parse the root directory
	if (oa->RootDirectory)
	{
		// relative path
		if (oa->ObjectName->Buffer[0] == '\\')
			return STATUS_OBJECT_PATH_SYNTAX_BAD;

		r = object_from_handle( dir, oa->RootDirectory, DIRECTORY_QUERY );
		if (r != STATUS_SUCCESS)
			return r;
	}
	else
	{
		// absolute path
		if (oa->ObjectName->Buffer[0] != '\\')
			return STATUS_OBJECT_PATH_SYNTAX_BAD;
		dir = root;
		ofs++;
	}

	while (ofs < name.Length/2)
	{
		ULONG n = ofs;

		while (n < name.Length/2 && name.Buffer[n] != '\\')
			n++;

		// don't check the last part of the path
		if (n == name.Length/2)
			break;

		UNICODE_STRING segment;
		segment.Buffer = &name.Buffer[ofs];
		segment.Length = (n - ofs) * 2;
		segment.MaximumLength = 0;

		object_t *obj = dir->lookup( segment, case_insensitive );
		if (!obj)
		{
			dprintf("path %pus not found\n", &segment);
			return STATUS_OBJECT_PATH_NOT_FOUND;
		}

		// check symlinks
		symlink_t *link = dynamic_cast<symlink_t*>( obj );
		if (link)
		{
			unicode_string_t& target = link->get_target();

			// resolve the link
			OBJECT_ATTRIBUTES target_oa;
			memset( &target_oa, 0, sizeof target_oa );
			target_oa.Length = sizeof target_oa;
			target_oa.Attributes = OBJ_CASE_INSENSITIVE;
			target_oa.ObjectName = &target;

			obj = NULL;
			r = find_object_by_name( &obj, &target_oa );
			if (r != STATUS_SUCCESS)
			{
				dprintf("target %pus not found\n", &target);
				return STATUS_OBJECT_PATH_NOT_FOUND;
			}
		}

		dir = dynamic_cast<object_dir_t*>( obj );
		if (!dir)
		{
			dprintf("path %pus invalid\n", &segment);
			return STATUS_OBJECT_PATH_SYNTAX_BAD;
		}

		ofs = n + 1;
	}

	return STATUS_SUCCESS;
}

NTSTATUS validate_path( const OBJECT_ATTRIBUTES *oa )
{
	object_dir_t* dir = 0;
	ULONG ofs = 0;

	return parse_path( oa, dir, ofs );
}

NTSTATUS find_object_by_name( object_t **out, const OBJECT_ATTRIBUTES *oa )
{
	NTSTATUS r;

	object_dir_t* dir = 0;
	ULONG ofs = 0;
	r = parse_path( oa, dir, ofs );
	if (r != STATUS_SUCCESS)
		return r;

	UNICODE_STRING us;
	us.Buffer = oa->ObjectName->Buffer;
	us.Length = oa->ObjectName->Length;
	us.MaximumLength = oa->ObjectName->MaximumLength;

	us.Buffer += ofs;
	us.Length -= ofs*2;
	us.MaximumLength -= ofs*2;

	object_t *obj = 0;
	bool case_insensitive = oa->Attributes & OBJ_CASE_INSENSITIVE;
	obj = dir->lookup( us, case_insensitive );
	if (!obj)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	// need to pass extra info to open - length of path parsed

	open_info_t oi;
	oi.Attributes = oa->Attributes;
	oi.factory = 0;
	oi.dir = dir;
	oi.path.Length = 0;
	oi.path.MaximumLength = 0;
	oi.path.Buffer = 0;

	return obj->open( *out, oi );
}

NTSTATUS name_object( object_t *obj, const OBJECT_ATTRIBUTES *oa )
{
	if (!oa)
		 return STATUS_SUCCESS;

	obj->attr = oa->Attributes;
	if (!oa->ObjectName)
		return STATUS_SUCCESS;
	if (!oa->ObjectName->Buffer)
		return STATUS_SUCCESS;

	NTSTATUS r;

	object_dir_t* dir = 0;
	ULONG ofs = 0;
	r = parse_path( oa, dir, ofs );
	if (r != STATUS_SUCCESS)
		return r;

	assert( dir != NULL );
	assert( ofs < oa->ObjectName->Length );

	UNICODE_STRING us;
	us.Buffer = oa->ObjectName->Buffer;
	us.Length = oa->ObjectName->Length;
	us.MaximumLength = oa->ObjectName->MaximumLength;

	us.Buffer += ofs;
	us.Length -= ofs*2;
	us.MaximumLength -= ofs*2;

	object_t *existing = 0;
	bool case_insensitive = oa->Attributes & OBJ_CASE_INSENSITIVE;
	existing = dir->lookup( us, case_insensitive );
	if (existing)
		return STATUS_OBJECT_NAME_COLLISION;

	r = obj->name.copy( &us );
	if (r != STATUS_SUCCESS)
		return r;

	if (0)
		dprintf("name is -> %pus", &obj->name );

	dir->append( obj );

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

void init_root()
{
	root = new object_dir_impl_t;
	assert( root );
}

void free_root()
{
	//delete root;
}

NTSTATUS NTAPI NtCreateDirectoryObject(
	PHANDLE DirectoryHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	dprintf("%p %08lx %p\n", DirectoryHandle, DesiredAccess, ObjectAttributes );

	object_dir_factory factory;
	NTSTATUS r;
	r = factory.create( DirectoryHandle, DesiredAccess, ObjectAttributes );
	if (r != STATUS_SUCCESS)
	{
		r = NtOpenDirectoryObject( DirectoryHandle, DesiredAccess, ObjectAttributes );
	}
	return r;
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
