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
	virtual NTSTATUS open( object_t*& obj, open_info_t& info );
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
	virtual NTSTATUS on_open( object_dir_t *dir, object_t*& obj, open_info_t& info );
};

NTSTATUS object_dir_factory::alloc_object(object_t** obj)
{
	*obj = new object_dir_impl_t;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

NTSTATUS object_dir_factory::on_open( object_dir_t *dir, object_t*& obj, open_info_t& info )
{
	dprintf("object_dir_factory %pus\n", &info.path);

	if (obj)
		return STATUS_SUCCESS;

	NTSTATUS r;
	r = alloc_object( &obj );
	if (r != STATUS_SUCCESS)
		return r;

	r = obj->name.copy( &info.path );
	if (r != STATUS_SUCCESS)
		return r;

	dir->append( obj );

	return STATUS_SUCCESS;
}

object_t *create_directory_object( PCWSTR name )
{
	object_dir_impl_t *obj = new object_dir_impl_t;

	if (name && name[0] == '\\' && name[1] == 0)
	{
		if (!root)
			root = obj;
		else
			delete obj;
		return root;
	}

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

NTSTATUS open_root( object_t*& obj, open_info_t& info )
{
	// look each directory in the path and make sure it exists
	object_dir_t *dir = 0;

	NTSTATUS r;

	// parse the root directory
	if (info.root)
	{
		// relative path
		if (info.path.Buffer[0] == '\\')
			return STATUS_OBJECT_PATH_SYNTAX_BAD;

		r = object_from_handle( dir, info.root, DIRECTORY_QUERY );
		if (r != STATUS_SUCCESS)
			return r;
	}
	else
	{
		// absolute path
		if (info.path.Buffer[0] != '\\')
			return STATUS_OBJECT_PATH_SYNTAX_BAD;
		dir = root;
		info.path.Buffer++;
		info.path.Length -= 2;
	}

	if (info.path.Length == 0)
	{
		obj = dir;
		return info.on_open( 0, obj, info );
	}

	return dir->open( obj, info );
}

NTSTATUS object_dir_impl_t::open( object_t*& obj, open_info_t& info )
{
	ULONG n = 0;
	UNICODE_STRING& path = info.path;

	dprintf("path = %pus\n", &path );

	while (n < path.Length/2 && path.Buffer[n] != '\\')
		n++;

	UNICODE_STRING segment;
	segment.Buffer = path.Buffer;
	segment.Length = n * 2;
	segment.MaximumLength = 0;

	obj = lookup( segment, info.case_insensitive() );

	if (n == path.Length/2)
		return info.on_open( this, obj, info );

	if (!obj)
		return STATUS_OBJECT_PATH_NOT_FOUND;

	path.Buffer += (n + 1);
	path.Length -= (n + 1) * 2;
	path.MaximumLength -= (n + 1) * 2;

	return obj->open( obj, info );
}

class find_object_t : public open_info_t
{
public:
	virtual NTSTATUS on_open( object_dir_t *dir, object_t*& obj, open_info_t& info );
};

NTSTATUS find_object_t::on_open( object_dir_t *dir, object_t*& obj, open_info_t& info )
{
	dprintf("find_object_t::on_open %pus %s\n", &info.path,
		 obj ? "exists" : "doesn't exist");
	if (!obj)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	// hack until NtOpenSymbolicLinkObject is fixed
	if (dynamic_cast<symlink_t*>( obj ) != NULL &&
		 (info.Attributes & OBJ_OPENLINK))
	{
		return STATUS_INVALID_PARAMETER;
	}

	addref( obj );

	return STATUS_SUCCESS;
}

NTSTATUS find_object_by_name( object_t **out, const OBJECT_ATTRIBUTES *oa )
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

	find_object_t oi;
	oi.Attributes = oa->Attributes;
	oi.root = oa->RootDirectory;
	oi.path.set( *oa->ObjectName );

	return open_root( *out, oi );
}

class name_object_t : public open_info_t
{
	object_t *obj_to_name;
public:
	name_object_t( object_t *in );
	virtual NTSTATUS on_open( object_dir_t *dir, object_t*& obj, open_info_t& info );
};

name_object_t::name_object_t( object_t *in ) :
	obj_to_name( in )
{
}

NTSTATUS name_object_t::on_open( object_dir_t *dir, object_t*& obj, open_info_t& info )
{
	dprintf("name_object_t::on_open %pus\n", &info.path);

	if (obj)
	{
		dprintf("object already exists\n");
		return STATUS_OBJECT_NAME_COLLISION;
	}

	obj = obj_to_name;

	NTSTATUS r;
	r = obj->name.copy( &info.path );
	if (r != STATUS_SUCCESS)
		return r;

	dir->append( obj );

	return STATUS_SUCCESS;
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
	if (!oa->ObjectName->Length)
		return STATUS_SUCCESS;

	dprintf("name_object_t %pus\n", oa->ObjectName);

	name_object_t oi( obj );
	oi.Attributes = oa->Attributes;
	oi.root = oa->RootDirectory;
	oi.path.set( *oa->ObjectName );

	return open_root( obj, oi );
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
