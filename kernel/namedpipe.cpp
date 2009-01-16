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
#include "winioctl.h"
#include "unicode.h"
#include "ntcall.h"

#include "file.h"
#include "objdir.h"
#include "debug.h"

class pipe_server_t;
class pipe_client_t;
class pipe_container_t;

typedef list_anchor<pipe_server_t,0> pipe_server_list_t;
typedef list_element<pipe_server_t> pipe_server_element_t;
typedef list_iter<pipe_server_t,0> pipe_server_iter_t;
typedef list_anchor<pipe_client_t,0> pipe_client_list_t;
typedef list_element<pipe_client_t> pipe_client_element_t;
typedef list_iter<pipe_client_t,0> pipe_client_iter_t;

// the pipe device \Device\NamedPipe, contains pipes of different names
class pipe_device_t : public object_dir_impl_t, public io_object_t
{
public:
	virtual NTSTATUS read( PVOID buffer, ULONG length, ULONG *read );
	virtual NTSTATUS write( PVOID buffer, ULONG length, ULONG *written );
	virtual NTSTATUS open( object_t *&out, open_info_t& info );
	virtual NTSTATUS fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength );
	NTSTATUS wait_server_available( PFILE_PIPE_WAIT_FOR_BUFFER pwfb, ULONG Length );
};

// factory to create the pipe device at startup
class pipe_device_factory : public object_factory
{
public:
	NTSTATUS alloc_object(object_t** obj);
};

// contains all clients and servers associated with a specific pipe name
class pipe_container_t : virtual public object_t
{
	pipe_server_list_t servers;
	pipe_client_list_t clients;
	ULONG num_instances;
	ULONG max_instances;
public:
	pipe_container_t( ULONG max );
	NTSTATUS create_server( pipe_server_t*& pipe, ULONG max_inst );
	NTSTATUS create_client( pipe_client_t*& pipe );
	void unlink( pipe_server_t *pipe );
	pipe_server_list_t& get_servers() {return servers;}
	pipe_client_list_t& get_clients() {return clients;}
	virtual NTSTATUS open( object_t *&out, open_info_t& info );
	pipe_server_t* find_idle_server();
};

class pipe_message_t;

typedef list_anchor<pipe_message_t,0> pipe_message_list_t;
typedef list_element<pipe_message_t> pipe_message_element_t;
typedef list_iter<pipe_message_t,0> pipe_message_iter_t;

class pipe_message_t
{
protected:
	void *operator new(unsigned int count, void*&ptr) { assert( count == sizeof (pipe_message_t)); return ptr; }
	pipe_message_t(ULONG _Length);
public:
	pipe_message_element_t entry[1];
	ULONG Length;
	static pipe_message_t* alloc_pipe_message( ULONG _Length );
	unsigned char *data_ptr();
};

// a single server instance
class pipe_server_t : public io_object_t
{
	friend class pipe_container_t;
public:
	enum pipe_state {
		pipe_idle,
		pipe_wait_connect,
		pipe_connected,
		pipe_wait_disconnect,
		pipe_disconnected,
	};
	pipe_container_t *container;
	pipe_state state;
	pipe_client_t *client;
	thread_t *thread;
	pipe_server_element_t entry[1];
	pipe_message_list_t received_messages;
	pipe_message_list_t sent_messages;
public:
	pipe_server_t( pipe_container_t *container );
	~pipe_server_t();
	virtual NTSTATUS read( PVOID buffer, ULONG length, ULONG *read );
	virtual NTSTATUS write( PVOID buffer, ULONG length, ULONG *written );
	NTSTATUS open( object_t *&out, open_info_t& info );
	virtual NTSTATUS fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength );
	inline bool is_connected() { return state == pipe_connected; }
	inline bool is_idle() {return state == pipe_idle; }
	inline bool is_awaiting_connect() {return state == pipe_wait_connect; }
	bool do_connect();
	NTSTATUS connect();
	NTSTATUS disconnect();
	void set_client( pipe_client_t* pipe_client );
	void queue_message_from_client( pipe_message_t *msg );
	void queue_message_to_client( pipe_message_t *msg );
};

// a single client instance
class pipe_client_t : public io_object_t
{
	friend class pipe_container_t;
public:
	pipe_client_element_t entry[1];
	pipe_server_t *server;
	thread_t *thread;
public:
	pipe_client_t( pipe_container_t *container );
	virtual NTSTATUS read( PVOID buffer, ULONG length, ULONG *read );
	virtual NTSTATUS write( PVOID buffer, ULONG length, ULONG *written );
	NTSTATUS set_pipe_info( FILE_PIPE_INFORMATION& pipe_info );
	virtual NTSTATUS fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength );
	NTSTATUS transceive(
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength );
};

// server factory, used by NtCreateNamedPipeFile
class pipe_factory : public object_factory
{
	ULONG MaxInstances;
public:
	pipe_factory( ULONG _MaxInstances );
	NTSTATUS alloc_object(object_t** obj);
	NTSTATUS on_open( object_dir_t* dir, object_t*& obj, open_info_t& info );
};

NTSTATUS pipe_device_t::open( object_t *&out, open_info_t& info )
{
	if (info.path.Length == 0)
		return object_dir_impl_t::open( out, info );

	// appears to be a flat namespace under the pipe device
	dprintf("pipe = %pus\n", &info.path );
	out = lookup( info.path, info.case_insensitive() );

	// not the NtCreateNamedPipeFile case?
	if (out && dynamic_cast<pipe_factory*>(&info) == NULL)
		return out->open( out, info );

	return info.on_open( this, out, info );
}

NTSTATUS pipe_container_t::open( object_t *&out, open_info_t& info )
{
	dprintf("allocating pipe client = %pus\n", &info.path );
	pipe_client_t *pipe = 0;
	NTSTATUS r = create_client( pipe );
	if (r < STATUS_SUCCESS)
		return r;
	out = pipe;
	return r;
}

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

NTSTATUS pipe_device_t::read( PVOID buffer, ULONG length, ULONG *read )
{
	return STATUS_ACCESS_DENIED;
}

NTSTATUS pipe_device_t::write( PVOID buffer, ULONG length, ULONG *written )
{
	return STATUS_ACCESS_DENIED;
}

pipe_message_t::pipe_message_t( ULONG _Length ) :
	Length( _Length )
{
}

pipe_message_t *pipe_message_t::alloc_pipe_message( ULONG _Length )
{
	ULONG sz = _Length + sizeof (pipe_message_t);
	void *mem = (void*) new unsigned char[sz];
	return new(mem) pipe_message_t(_Length);
}

unsigned char *pipe_message_t::data_ptr()
{
	return (unsigned char *) (this+1);
}

class wait_server_info_t
{
	FILE_PIPE_WAIT_FOR_BUFFER info;
public:
	LARGE_INTEGER&  Timeout;
	ULONG&          NameLength;
	BOOLEAN&        TimeoutSpecified;
	unicode_string_t name;
public:
	wait_server_info_t();
	NTSTATUS copy_from_user( PFILE_PIPE_WAIT_FOR_BUFFER pwfb, ULONG Length );
	void dump();
};

wait_server_info_t::wait_server_info_t() :
	Timeout( info.Timeout ),
	NameLength( info.NameLength ),
	TimeoutSpecified( info.TimeoutSpecified )
{
}

NTSTATUS wait_server_info_t::copy_from_user( PFILE_PIPE_WAIT_FOR_BUFFER pwfb, ULONG Length )
{
	NTSTATUS r;
	ULONG sz = FIELD_OFFSET( FILE_PIPE_WAIT_FOR_BUFFER, Name );

	if (Length < sz)
		return STATUS_INVALID_PARAMETER;
	r = ::copy_from_user( &info, pwfb, sz );
	if (r < STATUS_SUCCESS)
		return r;
	if (Length < (sz + NameLength))
		return STATUS_INVALID_PARAMETER;
	return name.copy_wstr_from_user( pwfb->Name, NameLength );
}

void wait_server_info_t::dump()
{
	dprintf("pipe server wait name=%pus\n", &name );
}

NTSTATUS pipe_device_t::wait_server_available( PFILE_PIPE_WAIT_FOR_BUFFER pwfb, ULONG Length )
{
	wait_server_info_t info;

	NTSTATUS r = info.copy_from_user( pwfb, Length );
	if (r < STATUS_SUCCESS)
		return r;

	info.dump();

	object_t* obj = lookup( info.name, true );
	if (!obj)
	{
		dprintf("no pipe server (%pus)\n", &info.name );
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	pipe_container_t *container = dynamic_cast<pipe_container_t*>( obj );
	if (!container)
		return STATUS_UNSUCCESSFUL;

	pipe_server_t* server = container->find_idle_server();
	if (!server)
	{
		//FIXME: timeout
		current->wait();
	}

	return STATUS_SUCCESS;
}

NTSTATUS pipe_device_t::fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
	 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength )
{
	if (FsControlCode == FSCTL_PIPE_WAIT)
		return wait_server_available( (PFILE_PIPE_WAIT_FOR_BUFFER) InputBuffer, InputBufferLength );

	dprintf("unimplemented %08lx\n", FsControlCode);

	return STATUS_NOT_IMPLEMENTED;
}

pipe_container_t::pipe_container_t( ULONG max ) :
	num_instances(0),
	max_instances(max)
{
}

void pipe_container_t::unlink( pipe_server_t *pipe )
{
	servers.unlink( pipe );
	num_instances--;
}

NTSTATUS pipe_container_t::create_server( pipe_server_t *& pipe, ULONG max_inst )
{
	dprintf("creating pipe server\n");
	if (max_inst != max_instances)
		return STATUS_INVALID_PARAMETER;

	if (num_instances >= max_instances )
		return STATUS_ACCESS_DENIED;
	num_instances++;

	pipe = new pipe_server_t( this );

	servers.append( pipe );
	addref( this );

	return STATUS_SUCCESS;
}

NTSTATUS pipe_container_t::create_client( pipe_client_t*& client )
{
	client = new pipe_client_t( this );
	if (!client)
		return STATUS_NO_MEMORY;

	pipe_server_t *server = find_idle_server();
	if (server)
	{
		server->set_client( client );
		thread_t *t = server->thread;
		server->thread = NULL;
		t->start();
	}

	return STATUS_SUCCESS;
}

void pipe_server_t::set_client( pipe_client_t* pipe_client )
{
	assert( pipe_client );
	client = pipe_client;
	client->server = this;
	state = pipe_connected;
	dprintf("connect server %p to client %p\n", this, client );
}

pipe_server_t* pipe_container_t::find_idle_server()
{
	// search for an idle server
	for (pipe_server_iter_t i(servers); i; i.next())
	{
		pipe_server_t *ps = i;
		if (ps->is_awaiting_connect())
			return ps;
	}
	return NULL;
}

pipe_server_t::pipe_server_t( pipe_container_t *_container ) :
	container( _container ),
	state( pipe_idle ),
	client( NULL ),
	thread( NULL )
{
}

pipe_server_t::~pipe_server_t()
{
	pipe_message_t *msg;
	while ((msg = received_messages.head()))
	{
		received_messages.unlink( msg );
		delete msg;
	}
	container->unlink( this );
	release( container );
}

NTSTATUS pipe_server_t::open( object_t *&out, open_info_t& info )
{
	// should return a pointer to a pipe client
	dprintf("implement\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS pipe_server_t::read( PVOID buffer, ULONG length, ULONG *read )
{
	pipe_message_t *msg;

	// only allow reading in the correct state
	if (state != pipe_connected)
		return STATUS_PIPE_BROKEN;

	// only allow one reader at a time
	if (thread)
		return STATUS_PIPE_BUSY;

	// get a message
	msg = received_messages.head();
	if (!msg)
	{
		// wait for a message
		thread = current;
		current->wait();
		if (current->is_terminated())
			return STATUS_THREAD_IS_TERMINATING;
		assert( thread == NULL );
		msg = received_messages.head();
	}

	ULONG len = 0;
	if (msg)
	{
		len = min( length, msg->Length );
		NTSTATUS r = copy_to_user( buffer, msg->data_ptr(), len );
		if (r < STATUS_SUCCESS)
			return r;
		received_messages.unlink( msg );
		delete msg;
	}
	*read = len;
	return STATUS_SUCCESS;
}

NTSTATUS pipe_server_t::write( PVOID buffer, ULONG length, ULONG *written )
{
	pipe_message_t *msg = pipe_message_t::alloc_pipe_message( length );

	NTSTATUS r;
	r = copy_from_user( msg->data_ptr(), buffer, length );
	if (r < STATUS_SUCCESS)
	{
		delete msg;
		return r;
	}

	queue_message_to_client( msg );
	*written = length;
	return STATUS_SUCCESS;
}

bool pipe_server_t::do_connect()
{
	for (pipe_client_iter_t i( container->get_clients() ); i; i.next())
	{
		pipe_client_t *pipe_client = i;

		if (pipe_client->server)
			continue;
		set_client( pipe_client );
		return true;
	}
	return false;
}

NTSTATUS pipe_server_t::connect()
{
	if (state != pipe_idle)
		return STATUS_PIPE_CONNECTED;

	state = pipe_wait_connect;
	do_connect();
	if (!is_connected())
	{
		thread = current;
		current->wait();
		if (current->is_terminated())
			return STATUS_THREAD_IS_TERMINATING;
		assert( thread == NULL );
		assert( is_connected() );
	}

	return STATUS_SUCCESS;
}

NTSTATUS pipe_server_t::disconnect()
{
	if (state != pipe_connected)
		return STATUS_PIPE_BROKEN;

	client->server = 0;
	client = 0;
	state = pipe_disconnected;

	return STATUS_SUCCESS;
}

NTSTATUS pipe_server_t::fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength )
{
	dprintf("pipe_server_t %08lx\n", FsControlCode);
	if (FsControlCode == FSCTL_PIPE_LISTEN)
		return connect();

	if (FsControlCode == FSCTL_PIPE_DISCONNECT)
		return disconnect();

	dprintf("implement\n");
	return STATUS_NOT_IMPLEMENTED;
}

void pipe_server_t::queue_message_from_client( pipe_message_t *msg )
{
	received_messages.append( msg );

	// wakeup readers
	assert( state == pipe_connected );
	if (thread)
	{
		thread_t *t = thread;
		thread = 0;
		t->start();
	}
}

void pipe_server_t::queue_message_to_client( pipe_message_t *msg )
{
	sent_messages.append( msg );
	// wakeup readers
	assert( state == pipe_connected );
	if (client->thread)
	{
		thread_t *t = client->thread;
		client->thread = 0;
		t->start();
	}
}

pipe_client_t::pipe_client_t( pipe_container_t *container ) :
	server( NULL ),
	thread( NULL )
{
}

NTSTATUS pipe_client_t::read( PVOID buffer, ULONG length, ULONG *read )
{
	pipe_message_t *msg;

	// only allow reading in the correct state
	if (server == NULL || server->state != pipe_server_t::pipe_connected)
		return STATUS_PIPE_BROKEN;

	// only allow one reader at a time
	if (thread)
		return STATUS_PIPE_BUSY;

	// get a message
	msg = server->sent_messages.head();
	if (!msg)
	{
		// wait for a message
		thread = current;
		current->wait();
		if (current->is_terminated())
			return STATUS_THREAD_IS_TERMINATING;
		assert( thread == NULL );
		msg = server->sent_messages.head();
	}

	ULONG len = 0;
	if (msg)
	{
		len = min( length, msg->Length );
		NTSTATUS r = copy_to_user( buffer, msg->data_ptr(), len );
		if (r < STATUS_SUCCESS)
			return r;
		server->sent_messages.unlink( msg );
		delete msg;
	}
	*read = len;
	return STATUS_SUCCESS;
}

NTSTATUS pipe_client_t::write( PVOID buffer, ULONG length, ULONG *written )
{
	pipe_message_t *msg = pipe_message_t::alloc_pipe_message( length );

	if (server == NULL || server->state != pipe_server_t::pipe_connected)
		return STATUS_PIPE_BROKEN;

	NTSTATUS r;
	r = copy_from_user( msg->data_ptr(), buffer, length );
	if (r < STATUS_SUCCESS)
	{
		delete msg;
		return r;
	}

	server->queue_message_from_client( msg );
	*written = length;
	return STATUS_SUCCESS;
}

NTSTATUS pipe_client_t::fs_control( event_t* event, IO_STATUS_BLOCK iosb, ULONG FsControlCode,
		 PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength )
{
	dprintf("pipe_client_t %08lx\n", FsControlCode);

	if (FsControlCode == FSCTL_PIPE_TRANSCEIVE)
		return transceive( InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength );

	return STATUS_INVALID_PARAMETER;
}

NTSTATUS pipe_client_t::transceive(
		PVOID InputBuffer, ULONG InputBufferLength,
		PVOID OutputBuffer, ULONG OutputBufferLength )
{
	NTSTATUS r;
	ULONG out = 0;
	r = write( InputBuffer, InputBufferLength, &out );
	if (r < STATUS_SUCCESS)
		return r;

	r = read( OutputBuffer, OutputBufferLength, &out );
	if (r < STATUS_SUCCESS)
		return r;

	return STATUS_SUCCESS;
}

NTSTATUS pipe_client_t::set_pipe_info( FILE_PIPE_INFORMATION& pipe_info )
{
	dprintf("%ld %ld\n", pipe_info.ReadModeMessage, pipe_info.WaitModeBlocking);
	return STATUS_SUCCESS;
}

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
	NTSTATUS r;

	dprintf("pipe_factory()\n");
	pipe_container_t *container = 0;
	if (!obj)
	{
		container = new pipe_container_t( MaxInstances );
		if (!container)
			return STATUS_NO_MEMORY;

		r = container->name.copy( &info.path );
		if (r < STATUS_SUCCESS)
			return r;

		dir->append( container );
	}
	else
	{
		container = dynamic_cast<pipe_container_t*>( obj );
		if (!container)
			return STATUS_OBJECT_TYPE_MISMATCH;
	}

	assert( container );

	pipe_server_t *pipe = 0;
	r = container->create_server( pipe, MaxInstances );
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
	if (r < STATUS_SUCCESS)
		return r;

	pipe_factory factory( MaxInstances );

	return factory.create( PipeHandle, AccessMask, ObjectAttributes );
}
