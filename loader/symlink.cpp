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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "object.inl"
#include "ntcall.h"
#include "symlink.h"

symlink_t::symlink_t( unicode_string_t& us )
{
	target.copy( &us );
}

symlink_t::~symlink_t()
{
}

NTSTATUS symlink_t::open( object_t *&out, open_info_t& info )
{
	dprintf("opening symlinks oa.Attributes = %08lx\n", info.Attributes);
	if (info.Attributes & OBJ_OPENLINK)
	{
		dprintf("OBJ_OPENLINK specified\n");
		return STATUS_INVALID_PARAMETER;
	}

	return object_t::open( out, info );
}

NTSTATUS NTAPI NtCreateSymbolicLinkObject(
	PHANDLE SymbolicLinkHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PUNICODE_STRING TargetName )
{
	object_attributes_t oa;
	unicode_string_t target;
	NTSTATUS r;
	object_t *link;

	r = verify_for_write( SymbolicLinkHandle, sizeof *SymbolicLinkHandle );
	if (r != STATUS_SUCCESS)
		return r;

	r = target.copy_from_user( TargetName );
	if (r != STATUS_SUCCESS)
		return r;

	if (target.Length == 0)
		return STATUS_INVALID_PARAMETER;

	if (target.Length > target.MaximumLength)
		return STATUS_INVALID_PARAMETER;

	if (ObjectAttributes)
	{
		r = oa.copy_from_user( ObjectAttributes );
		if (r != STATUS_SUCCESS)
			return r;
	}

	dprintf("root %p attr %08lx %pus -> %pus\n",
		oa.RootDirectory, oa.Attributes, oa.ObjectName, &target);

	link = new symlink_t( target );
	if (!link)
		return STATUS_NO_MEMORY;

	r = name_object( link, &oa );
	if (r == STATUS_SUCCESS)
	{
		r = alloc_user_handle( link, DesiredAccess, SymbolicLinkHandle );
		release( link );
	}

	return r;
}

NTSTATUS NTAPI NtOpenSymbolicLinkObject(
	PHANDLE SymlinkHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	return nt_open_object<symlink_t>( SymlinkHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtQuerySymbolicLinkObject(
	HANDLE SymbolicLinkHandle,
	PUNICODE_STRING LinkName,
	PULONG DataWritten )
{
	UNICODE_STRING name;
	NTSTATUS r;

	r = copy_from_user( &name, LinkName, sizeof name );
	if (r != STATUS_SUCCESS)
		return r;

	// make sure we can write back the length
	r = verify_for_write( &LinkName->Length, sizeof LinkName->Length );
	if (r != STATUS_SUCCESS)
		return r;

	if (DataWritten)
	{
		r = verify_for_write( DataWritten, sizeof DataWritten );
		if (r != STATUS_SUCCESS)
			return r;
	}

	symlink_t *symlink = 0;
	r = object_from_handle( symlink, SymbolicLinkHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	const unicode_string_t& target = symlink->get_target();

	if (name.MaximumLength < target.Length)
		return STATUS_BUFFER_TOO_SMALL;

	r = copy_to_user( name.Buffer, target.Buffer, target.Length );
	if (r != STATUS_SUCCESS)
		return r;

	copy_to_user( &LinkName->Length, &target.Length, sizeof target.Length );

	if (DataWritten)
	{
		// convert from USHORT to ULONG
		ULONG len = target.Length;
		copy_to_user( DataWritten, &len, sizeof len );
	}

	return r;
}
