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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "unicode.h"
#include "ntcall.h"

#include "file.h"
#include "debug.h"

class named_pipe_t;

typedef list_anchor<named_pipe_t,0> pipe_server_list_t;
typedef list_element<named_pipe_t> pipe_list_element_t;

class pipe_container_t : public object_t
{
	pipe_server_list_t pipe_servers;
	ULONG num_instances;
	ULONG max_instances;
public:
	pipe_container_t( ULONG max );
	NTSTATUS create( named_pipe_t*& pipe, ULONG max_inst );
	void unlink( named_pipe_t *pipe );
};

pipe_container_t::pipe_container_t( ULONG max ) :
	num_instances(0),
	max_instances(max)
{
}

void pipe_container_t::unlink( named_pipe_t *pipe )
{
	pipe_servers.unlink( pipe );
}

class named_pipe_t : public io_object_t
{
	pipe_container_t *container;
public:
	pipe_list_element_t entry[1];
public:
	named_pipe_t( pipe_container_t *container );
	~named_pipe_t();
	virtual NTSTATUS read( PVOID buffer, ULONG length, ULONG *read );
	virtual NTSTATUS write( PVOID buffer, ULONG length, ULONG *written );
};

NTSTATUS pipe_container_t::create( named_pipe_t *& pipe, ULONG max_inst )
{
	if (max_inst != max_instances)
		return STATUS_INVALID_PARAMETER;

	pipe = new named_pipe_t( this );

	pipe_servers.append( pipe );
	addref( this );

	return STATUS_SUCCESS;
}

named_pipe_t::named_pipe_t( pipe_container_t *_container ) :
	container( _container )
{
}

named_pipe_t::~named_pipe_t()
{
	container->unlink( this );
	release( container );
}

NTSTATUS named_pipe_t::read( PVOID buffer, ULONG length, ULONG *read )
{
	dprintf("implement\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS named_pipe_t::write( PVOID buffer, ULONG length, ULONG *written )
{
	dprintf("implement\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtCreateNamedPipeFile(
	PHANDLE PipeHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	BOOLEAN TypeMessage,
	BOOLEAN ReadModeMessage,
	BOOLEAN NonBlocking,
	ULONG MaxInstances,
	ULONG InBufferSize,
	ULONG OutBufferSize,
	PLARGE_INTEGER DefaultTimeout)
{
	object_attributes_t oa;
	NTSTATUS r;

	r = oa.copy_from_user( ObjectAttributes );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("name = %pus\n", oa.ObjectName );

	r = verify_for_write( IoStatusBlock, sizeof IoStatusBlock );
	if (r != STATUS_SUCCESS)
		return r;

	// normal object factory doesn't apply here
	// multiple named pipe servers can share the same name
	pipe_container_t *container = 0;
	object_t *obj = 0;
	r = find_object_by_name( &obj, &oa );
	if (r == STATUS_SUCCESS)
	{
		container = dynamic_cast<pipe_container_t*>( obj );
		if (!container)
			return STATUS_OBJECT_TYPE_MISMATCH;
	}
	else
	{
		container = new pipe_container_t( MaxInstances );
		if (!container)
			return STATUS_NO_MEMORY;

		r = name_object( container, &oa );
		if (r != STATUS_SUCCESS)
			return r;
	}

	named_pipe_t *pipe = 0;
	r = container->create( pipe, MaxInstances );
	if (r == STATUS_SUCCESS)
	{
		HANDLE handle = 0;
		r = process_alloc_user_handle( current->process, pipe, AccessMask, PipeHandle, &handle );
		dprintf("new handle is %p\n", handle );
		release( pipe );
	}

	release( container );

	return r;
}
