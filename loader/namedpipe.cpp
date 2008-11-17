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
#include "objdir.h"
#include "debug.h"

class named_pipe_t;

typedef list_anchor<named_pipe_t,0> pipe_server_list_t;
typedef list_element<named_pipe_t> pipe_list_element_t;

class pipe_device_t : public object_dir_impl_t
{
public:
	virtual NTSTATUS open( object_t *&out, open_info_t& info );
};

NTSTATUS pipe_device_t::open( object_t *&out, open_info_t& info )
{
	dprintf("pipe = %pus\n", &info.path );
	return object_dir_impl_t::open( out, info );
}

class pipe_device_factory : public object_factory
{
public:
	NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS pipe_device_factory::alloc_object(object_t** obj)
{
	*obj = new pipe_device_t;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

void init_pipe_device()
{
	pipe_device_factory factory;
	unicode_string_t name;
	name.set( L"\\Device\\NamedPipe");

	NTSTATUS r;
	object_t *obj = 0;
	r = factory.create_kernel( obj, name );
	if (r < STATUS_SUCCESS)
		die("failed to create named pipe\n");
}

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
	NTSTATUS open( object_t *&out, open_info_t& info );
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

NTSTATUS named_pipe_t::open( object_t *&out, open_info_t& info )
{
	// should return a pointer to a pipe client
	dprintf("implement\n");
	return STATUS_NOT_IMPLEMENTED;
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

class pipe_factory : public object_factory
{
	ULONG MaxInstances;
public:
	pipe_factory( ULONG _MaxInstances );
	NTSTATUS alloc_object(object_t** obj);
	NTSTATUS on_open( object_dir_t* dir, object_t*& obj, open_info_t& info );
};

pipe_factory::pipe_factory( ULONG _MaxInstances ) :
	MaxInstances( _MaxInstances )
{
}

NTSTATUS pipe_factory::alloc_object(object_t** obj)
{
	assert(0);
	return STATUS_SUCCESS;
}

NTSTATUS pipe_factory::on_open( object_dir_t* dir, object_t*& obj, open_info_t& info )
{
	pipe_container_t *container = 0;
	if (!obj)
	{
		container = new pipe_container_t( MaxInstances );
		if (!container)
			return STATUS_NO_MEMORY;
	}
	else
	{
		container = dynamic_cast<pipe_container_t*>( obj );
		if (!container)
			return STATUS_OBJECT_TYPE_MISMATCH;
	}

	assert( container );

	named_pipe_t *pipe = 0;
	NTSTATUS r;
	r = container->create( pipe, MaxInstances );
	if (r == STATUS_SUCCESS)
		obj = pipe;
	return r;
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
	LARGE_INTEGER timeout;
	object_attributes_t oa;
	NTSTATUS r;

	if (CreateDisposition != FILE_OPEN_IF)
		return STATUS_INVALID_PARAMETER;

	if (MaxInstances == 0)
		return STATUS_INVALID_PARAMETER;

	if (ObjectAttributes == NULL)
		return STATUS_INVALID_PARAMETER;

	if (!(ShareAccess & FILE_SHARE_READ))
		return STATUS_INVALID_PARAMETER;
	if (!(ShareAccess & FILE_SHARE_WRITE))
		return STATUS_INVALID_PARAMETER;

	r = copy_from_user( &timeout, DefaultTimeout, sizeof timeout );
	if (r < STATUS_SUCCESS)
		return r;

	if (timeout.QuadPart > 0)
		return STATUS_INVALID_PARAMETER;

	r = verify_for_write( PipeHandle, sizeof *PipeHandle );
	if (r < STATUS_SUCCESS)
		return r;

	r = verify_for_write( IoStatusBlock, sizeof IoStatusBlock );
	if (r != STATUS_SUCCESS)
		return r;

	pipe_factory factory( MaxInstances );

	return factory.create( PipeHandle, AccessMask, ObjectAttributes );
}
