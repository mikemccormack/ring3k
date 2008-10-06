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
#include "mem.h"
#include "object.h"
#include "ntcall.h"
#include "section.h"
#include "object.inl"

class message_t;

typedef list_anchor<message_t, 0> message_list_t;
typedef list_element<message_t> message_entry_t;
typedef list_iter<message_t, 0> message_iter_t;

class message_t {
public:
	ULONG destination_id;
protected:
	friend class list_anchor<message_t, 0>;
	friend class list_iter<message_t, 0>;
	message_entry_t entry[1];
public:
	void *operator new(size_t n, size_t len);
	explicit message_t();
	bool is_linked() { return entry[0].is_linked(); }
	~message_t();
	void dump();
	const char* msg_type();
public:
	LPC_MESSAGE req;
};

struct listener_t;
struct port_queue_t;

typedef list_anchor<listener_t, 0> listener_list_t;
typedef list_element<listener_t> listener_entry_t;
typedef list_iter<listener_t, 0> listener_iter_t;

struct listener_t {
	listener_entry_t entry[1];
	port_t *port;
	thread_t *thread;
	BOOLEAN want_connect;
	ULONG message_id;
public:
	explicit listener_t(port_t *p, thread_t *t, BOOLEAN wc, ULONG id);
	~listener_t();
	bool is_linked() { return entry[0].is_linked(); }
};

struct port_queue_t : public object_t {
	ULONG refs;
	ULONG max_connect;
	ULONG max_data;
	message_list_t messages;
	listener_list_t listeners;
public:
	explicit port_queue_t( ULONG max_connect, ULONG max_data );
	~port_queue_t();
	message_t *find_connection_request();
};

struct port_t : public object_t {
	port_queue_t *queue;
	BOOLEAN server;
	thread_t *thread;
	port_t *other;
	section_t *section; // our section
	BYTE *other_section_base;	// mapped address of other port's section
	BYTE *our_section_base;	// mapped address of other port's section
	ULONG view_size;
	ULONG identifier;
	message_t *received_msg;
public:
	explicit port_t( BOOLEAN s, thread_t *t, port_queue_t *q );
	~port_t();
	void send_message( message_t *msg );
	void send_close_message( void );
	NTSTATUS send_reply( message_t *reply );
	void listen( message_t *&msg );
	NTSTATUS send_request( message_t *msg );
	void request_wait_reply( message_t *msg, message_t *&reply );
	NTSTATUS reply_wait_receive( message_t *reply, message_t *&received );
	NTSTATUS accept_connect( thread_t *t, message_t *reply, PLPC_SECTION_WRITE server_write_sec );
};

struct exception_msg_data_t {
	ULONG EventCode;
	ULONG Status;
	EXCEPTION_RECORD ExceptionRecord;
};

static int unique_message_id = 0x101;

void *message_t::operator new(size_t msg_size, size_t extra)
{
	return new unsigned char[msg_size + extra];
}

message_t::message_t() :
	destination_id(0)
{
	memset( &req, 0, sizeof req );
}

message_t::~message_t()
{
	assert( !is_linked() );
}

void unlink_and_free_message( message_list_t *list, message_t *msg )
{
	assert( msg->is_linked() );
	list->unlink( msg );
	delete msg;
}

void msg_free_unlinked( message_t *msg )
{
	if (!msg)
		return;
	if (msg->is_linked())
		return;
	delete msg;
}

const char* message_t::msg_type()
{
	switch (req.MessageType)
	{
#define M(x) case x: return #x;
	M(LPC_NEW_MESSAGE)
	M(LPC_REQUEST)
	M(LPC_REPLY)
	M(LPC_DATAGRAM)
	M(LPC_LOST_REPLY)
	M(LPC_PORT_CLOSED)
	M(LPC_CLIENT_DIED)
	M(LPC_EXCEPTION)
	M(LPC_DEBUG_EVENT)
	M(LPC_ERROR_EVENT)
	M(LPC_CONNECTION_REQUEST)
#undef M
	default: return "unknown";
	}
}

void message_t::dump()
{
	if (!option_trace)
		return;
	dprintf("DataSize    = %d\n", req.DataSize);
	dprintf("MessageSize = %d\n", req.MessageSize);
	dprintf("MessageType = %d (%s)\n", req.MessageType, msg_type());
	dprintf("Offset      = %d\n", req.VirtualRangesOffset);
	dprintf("ClientId    = %04x, %04x\n",
			 (int)req.ClientId.UniqueProcess, (int)req.ClientId.UniqueThread);
	dprintf("MessageId   = %ld\n", req.MessageId);
	dprintf("SectionSize = %08lx\n", req.SectionSize);
	dump_mem(&req.Data, req.DataSize);
}

port_t *port_from_obj( object_t *obj )
{
	return dynamic_cast<port_t*>( obj );
}

NTSTATUS port_from_handle( HANDLE handle, port_t *& port )
{
	return object_from_handle( port, handle, 0 );
}

void send_terminate_message(
	thread_t *thread,
	object_t *terminate_port,
	LARGE_INTEGER& create_time )
{
	port_t *port = dynamic_cast<port_t*>( terminate_port );
	assert(port);
	create_time.QuadPart = 0;

	dprintf("thread = %p port = %p\n", thread, port);

	ULONG data_size = sizeof create_time;
	ULONG msg_size = FIELD_OFFSET(LPC_MESSAGE, Data) + data_size;

	message_t *msg = new(msg_size) message_t;

	msg->req.MessageSize = msg_size;
	msg->req.MessageType = LPC_CLIENT_DIED;
	msg->req.DataSize = data_size;
	thread->get_client_id( &msg->req.ClientId );
	msg->req.MessageId = unique_message_id++;
	memcpy( &msg->req.Data, &create_time, sizeof create_time );

	port->send_message( msg );
	release(terminate_port);
}

NTSTATUS set_exception_port( process_t *process, object_t *obj )
{
	port_t *port = port_from_obj( obj );
	if (!port)
		return STATUS_OBJECT_TYPE_MISMATCH;
	if (process->exception_port)
		return STATUS_PORT_ALREADY_SET;
	// no addref here, destructors searchs processes...
	process->exception_port = port;
	return STATUS_SUCCESS;
}

bool send_exception( thread_t *thread, EXCEPTION_RECORD& rec )
{
	if (!thread->process->exception_port)
		return false;

	port_t *port = static_cast<port_t*>(thread->process->exception_port);

	dprintf("thread = %p port = %p\n", thread, port);

	ULONG status = STATUS_PENDING;
	while (1)
	{
		message_t *msg = new(0x78) message_t;

		msg->req.MessageSize = 0x78;
		msg->req.MessageType = LPC_EXCEPTION;
		msg->req.DataSize = 0x5c;
		thread->get_client_id( &msg->req.ClientId );
		msg->req.MessageId = unique_message_id++;

		exception_msg_data_t *x;

		x = (typeof x) &msg->req.Data[0];
		x->Status = status;
		x->EventCode = 0;
		x->ExceptionRecord = rec;

		// send the message and block waiting for a response
		message_t *reply = 0;
		port->request_wait_reply( msg, reply );
		x = (typeof x) &reply->req.Data[0];
		status = x->Status;
		delete reply;

		switch (status)
		{
		case DBG_CONTINUE:
		case DBG_EXCEPTION_HANDLED:
			return false;
		case DBG_TERMINATE_THREAD:
			thread->terminate(rec.ExceptionCode);
			return true;
		case DBG_TERMINATE_PROCESS:
			thread->process->terminate(rec.ExceptionCode);
			return true;
		default:
			dprintf("status = %08lx\n", status);
			continue;
		}
	}

	return false;
}

listener_t::listener_t(port_t *p, thread_t *t, BOOLEAN connect, ULONG id) :
	port(p),
	thread(t),
	want_connect(connect),
	message_id(id)
{
	addref( t );
	port->queue->listeners.append( this );
}

listener_t::~listener_t()
{
	// should be unlinked by whoever restarts this thread
	assert( !is_linked() );
	release( thread );
}

static inline ULONG round_up( ULONG len )
{
	return (len + 3) & ~3;
}

NTSTATUS copy_msg_from_user( message_t **message, LPC_MESSAGE *Reply, ULONG max_data )
{
	LPC_MESSAGE reply_hdr;
	message_t *msg;
	NTSTATUS r;

	r = copy_from_user( &reply_hdr, Reply, sizeof reply_hdr );
	if (r != STATUS_SUCCESS)
		return r;

	if (reply_hdr.DataSize > max_data)
		return STATUS_INVALID_PARAMETER;

	if (reply_hdr.MessageSize > max_data)
		return STATUS_PORT_MESSAGE_TOO_LONG;

	ULONG len = round_up(reply_hdr.DataSize);
	msg = new(len) message_t;
	if (!msg)
		return STATUS_NO_MEMORY;

	memcpy( &msg->req, &reply_hdr, sizeof reply_hdr );
	r = copy_from_user( &msg->req.Data, &Reply->Data[0], len );
	if (r != STATUS_SUCCESS)
		delete msg;
	else
		*message = msg;

	return r;
}

NTSTATUS copy_msg_to_user( LPC_MESSAGE *addr, message_t *msg )
{
	return copy_to_user( addr, &msg->req, round_up(msg->req.MessageSize) );
}

port_queue_t::~port_queue_t()
{
	message_t *m;

	//dprintf("%p\n", this);

	while ((m = messages.head() ))
		unlink_and_free_message( &messages, m );

	assert( listeners.empty() );
}

void port_t::send_close_message( void )
{
	message_t *msg;

	// FIXME: what's in the two words of data?
	ULONG data_size = sizeof (ULONG) * 2;
	ULONG msg_size = FIELD_OFFSET(LPC_MESSAGE, Data) + data_size;

	msg = new(data_size) message_t;

	// FIXME: Should we queue the message?
	//		What if new() fails?
	//		Should we send the message to every listener?

	msg->req.MessageSize = msg_size;
	msg->req.MessageType = LPC_PORT_CLOSED;
	msg->req.DataSize = data_size;
	current->get_client_id( &msg->req.ClientId );
	msg->req.MessageId = unique_message_id++;

	send_message( msg );
	//msg_free_unlinked( msg );
}

port_t::~port_t()
{
	// check if this is the exception port for any processes
	for ( process_iter_t i(processes); i; i.next() )
	{
		process_t *p = i;
		if (p->exception_port == this)
			p->exception_port = 0;
	}

	if (other_section_base)
		thread->process->vm->unmap_view( other_section_base );
	if (our_section_base)
		thread->process->vm->unmap_view( our_section_base );
	if (section)
		release(section);
	if (other)
	{
		other->other = 0;
		other = 0;
		send_close_message();
	}
	release( queue );
	release( thread );
}

port_t::port_t( BOOLEAN s, thread_t *t, port_queue_t *q ) :
	queue(q),
	server(s),
	thread(t),
	other(0),
	section(0),
	other_section_base(0),
	our_section_base(0),
	view_size(0),
	identifier(0),
	received_msg(0)
{
	if (q)
		addref(q);
	addref(thread);
}

port_queue_t::port_queue_t( ULONG _max_connect, ULONG _max_data ) :
	max_connect( _max_connect ),
	max_data( _max_data )
{
}

NTSTATUS create_named_port(
	object_t **obj,
	OBJECT_ATTRIBUTES *oa,
	ULONG max_connect,
	ULONG max_data )
{
	port_t *port;
	NTSTATUS r = STATUS_SUCCESS;

	*obj = NULL;

	port = new port_t( TRUE, current, 0 );
	if (!port)
		return STATUS_NO_MEMORY;

	port->queue = new port_queue_t( max_connect, max_data );
	if (port->queue)
	{
		r = name_object( port, oa );
		if (r == STATUS_SUCCESS)
		{
			addref( port );
			*obj = port;
		}
	}
	release( port );

	return r;
}

message_t *port_queue_t::find_connection_request()
{
	// check for existing connect requests
	for ( message_iter_t i(messages); i ; i.next() )
	{
		message_t *msg = i;
		if (msg->req.MessageType == LPC_CONNECTION_REQUEST)
			return msg;
	}
	return 0;
}

NTSTATUS port_t::send_reply( message_t *reply )
{
	reply->req.MessageType = LPC_REPLY;
	current->get_client_id( &reply->req.ClientId );

	reply->dump();

	for (listener_iter_t i(queue->listeners); i; i.next())
	{
		listener_t *l = i;

		if (l->want_connect)
			continue;

		if (l->message_id == reply->req.MessageId)
		{
			l->port->received_msg = reply;
			queue->listeners.unlink( l );
			l->thread->start();
			return STATUS_SUCCESS;
		}
	}
	return STATUS_REPLY_MESSAGE_MISMATCH;
}

// Wake a thread that called NtListenPort or NtReplyWaitReceive
void port_t::send_message( message_t *msg )
{
	msg->dump();

	msg->destination_id = identifier;
	queue->messages.append( msg );

	for (listener_iter_t i(queue->listeners); i; i.next())
	{
		listener_t *l = i;
		if (l->message_id)
			continue;
		if (!l->want_connect || msg->req.MessageType == LPC_CONNECTION_REQUEST)
		{
			//dprintf("queue %p has listener %p\n", queue, l->thread);
			queue->listeners.unlink( l );
			l->thread->start();
			return;
		}
	}
}

void port_t::listen( message_t *&msg )
{
	msg = queue->find_connection_request();
	if (!msg)
	{
		// Block until somebody connects to this port.
		listener_t l( this, current, TRUE, 0 );

		current->wait();

		msg = queue->find_connection_request();
		assert(msg);
	}
	queue->messages.unlink(msg);
}

NTSTATUS connect_port(
	PHANDLE out_handle,
	UNICODE_STRING *name,
	message_t *msg,
	message_t *&reply,
	PULONG MaximumMessageLength,
	PLPC_SECTION_WRITE write_sec,
	PLPC_SECTION_READ ServerSharedMemory )
{
	OBJECT_ATTRIBUTES oa;
	NTSTATUS r;
	object_t *obj = NULL;
	port_t *port;

	dprintf("%pus\n", name);

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = name;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = get_named_object( &obj, &oa );
	if (r != STATUS_SUCCESS)
		return r;

	// maybe the object should just be a queue...
	// try a test using ntconnectport on a result of itself
	port_queue_t *queue;

	port = port_from_obj( obj );
	if (!port)
		return STATUS_OBJECT_TYPE_MISMATCH;
	queue = port->queue;
	release(port);

	// check the connect data isn't too big
	if (queue->max_connect < msg->req.DataSize)
		return STATUS_INVALID_PARAMETER;

	// set the thread's MessageId so that complete_connect can find it
	current->MessageId = msg->req.MessageId;

	port = new port_t( FALSE, current, queue );
	if (!port)
		return STATUS_NO_MEMORY;

	// FIXME: use the section properly
	if (write_sec->SectionHandle)
	{
		r = object_from_handle( port->section, write_sec->SectionHandle, 0 );
		if (r != STATUS_SUCCESS)
			return r;
		addref(port->section);
	}
	port->view_size = write_sec->ViewSize;
	port->send_message( msg );

	// port_t::accept_connect will set the port->other pointer
	assert(port->other == 0);
	assert(port->received_msg == 0);

	// expect to be restarted by NtCompleteConnectPort when t->port is set
	assert( 0 == current->port);
	current->port = port;
	current->wait();
	assert( current->port == 0 );
	if (port->received_msg)
	{
		reply = port->received_msg;
		port->received_msg = 0;
	}

	// failing to fill the "other" port is a connection refusal
	if (port->other == 0)
	{
		release( port );
		return STATUS_PORT_CONNECTION_REFUSED;
	}

	r = alloc_user_handle( port, 0, out_handle );
	release( port );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("ServerSharedMemory = %p\n", ServerSharedMemory);
	if (ServerSharedMemory)
	{
		LPC_SECTION_READ read_sec;

		// Length seems to be always set to zero on output...
		read_sec.Length = 0;
		read_sec.ViewBase = port->other_section_base;
		read_sec.ViewSize = port->other->view_size;

		copy_to_user( ServerSharedMemory, &read_sec, sizeof read_sec );
	}

	write_sec->ViewBase = port->our_section_base;
	write_sec->TargetViewBase = port->other->other_section_base;

	if (MaximumMessageLength)
		copy_to_user( MaximumMessageLength, &port->queue->max_data, sizeof (ULONG));

	return STATUS_SUCCESS;
}

NTSTATUS complete_connect_port( port_t *port )
{
	//dprintf("%p\n", port);

	if (port->server)
		return STATUS_INVALID_PORT_HANDLE;

	if (!port->other)
		return STATUS_INVALID_PARAMETER;

	// allow starting threads where t->port is set
	thread_t *t = port->other->thread;
	if (!t->port)
		return STATUS_INVALID_PARAMETER;

	// make sure we don't try restart the thread twice
	t->port = 0;

	// restart the thread that was blocked on connect
	t->start();

	return STATUS_SUCCESS;
}

NTSTATUS port_t::accept_connect(
	thread_t *t,
	message_t *reply,
	PLPC_SECTION_WRITE server_write_sec )
{
	//dprintf("%p %p %08lx\n", req->ClientId.UniqueProcess, req->ClientId.UniqueThread, req->MessageId);

	// set the other pointer for connect_port
	other = t->port;
	other->other = this;
	other->received_msg = reply;

	NTSTATUS r;
	// map our section into the other process
	if (server_write_sec->SectionHandle)
	{
		r = object_from_handle( section, server_write_sec->SectionHandle, 0 );
		if (r != STATUS_SUCCESS)
			return r;
		addref(section);
		view_size = server_write_sec->ViewSize;

		// map our section into their process
		assert(t->port->other_section_base == 0);
		r = section->mapit( t->process->vm, t->port->other_section_base, 0,
						MEM_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return r;

		// map our section into our process
		assert(our_section_base == 0);
		r = section->mapit( current->process->vm, our_section_base, 0,
						MEM_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return r;

		dprintf("ours=%p theirs=%p\n", t->port->other_section_base, our_section_base);
	}

	// map the other side's section into our process
	if (other->section)
	{
		assert(other_section_base == 0);
		// map their section into our process
		r = other->section->mapit( current->process->vm, other_section_base, 0,
						MEM_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return r;

		// map their section into their process
		r = other->section->mapit( t->process->vm, t->port->our_section_base, 0,
						MEM_COMMIT, PAGE_READWRITE );
		if (r != STATUS_SUCCESS)
			return r;
		dprintf("theirs=%p ours=%p\n", other_section_base, t->port->our_section_base);
	}

	server_write_sec->ViewBase = our_section_base;
	server_write_sec->TargetViewBase = other->other_section_base;

	return STATUS_SUCCESS;
}

NTSTATUS port_t::send_request( message_t *msg )
{
	// queue a message into the port
	//msg->req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data);
	current->get_client_id( &msg->req.ClientId );
	msg->req.MessageId = unique_message_id++;
	msg->req.SectionSize = 0;
	msg->destination_id = identifier;

	current->MessageId = msg->req.MessageId;

	send_message( msg );
	// receiver frees the message

	return STATUS_SUCCESS;
}

void port_t::request_wait_reply( message_t *msg, message_t *&reply )
{
	send_request( msg );

	listener_t l( this, current, FALSE, msg->req.MessageId );

	// put the thread to sleep while we wait for a reply
	assert( received_msg == 0 );
	current->wait();
	reply = received_msg;
	received_msg = 0;
	assert( !reply->is_linked() );
}

NTSTATUS port_t::reply_wait_receive( message_t *reply, message_t *& received )
{
	dprintf("%p %p %p\n", this, reply, received );

	if (reply)
	{
		NTSTATUS r = send_reply( reply );
		if (r != STATUS_SUCCESS)
			return r;
	}

	received = queue->messages.head();
	if (!received)
	{
		listener_t l( this, current, FALSE, 0 );

		current->wait();
		received = queue->messages.head();
	}
	assert( received );
	queue->messages.unlink(received);
	assert( !received->is_linked() );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreatePort(
	PHANDLE Port,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG MaxConnectInfoLength,
	ULONG MaxDataLength,
	PULONG Reserved )
{
	object_attributes_t oa;
	NTSTATUS r;
	object_t *p = NULL;

	dprintf("%p %p %lu %lu %p\n", Port, ObjectAttributes, MaxConnectInfoLength, MaxDataLength, Reserved);

	r = verify_for_write( Port, sizeof *Port );
	if (r != STATUS_SUCCESS)
		return r;

	r = oa.copy_from_user( ObjectAttributes );
	if (r != STATUS_SUCCESS)
		return r;

	if (MaxDataLength > 0x148)
		return STATUS_INVALID_PARAMETER_4;

	if (MaxConnectInfoLength > 0x104)
		return STATUS_INVALID_PARAMETER_3;

	dprintf("root = %p port = %pus\n", oa.RootDirectory, oa.ObjectName );

	r = create_named_port( &p, &oa, MaxConnectInfoLength, MaxDataLength );
	if (r == STATUS_SUCCESS)
	{
		r = alloc_user_handle( p, 0, Port );
		release( p );
	}

	return r;
}

NTSTATUS NTAPI NtListenPort(
	HANDLE PortHandle,
	PLPC_MESSAGE ConnectionRequest )
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p %p\n", PortHandle, ConnectionRequest);

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	if ((ULONG)ConnectionRequest&3)
		return STATUS_DATATYPE_MISALIGNMENT;

	r = verify_for_write( ConnectionRequest, sizeof *ConnectionRequest );
	if (r != STATUS_SUCCESS)
		return r;

	message_t *msg = 0;
	port->listen( msg );
	r = copy_msg_to_user( ConnectionRequest, msg );
	delete msg;

	return r;
}

NTSTATUS NTAPI NtConnectPort(
	PHANDLE ClientPortHandle,
	PUNICODE_STRING ServerPortName,
	PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	PLPC_SECTION_WRITE ClientSharedMemory,
	PLPC_SECTION_READ ServerSharedMemory,
	PULONG MaximumMessageLength,
	PVOID ConnectionInfo,
	PULONG ConnectionInfoLength )
{
	dprintf("%p %p %p %p %p %p %p %p\n", ClientPortHandle, ServerPortName,
			SecurityQos, ClientSharedMemory, ServerSharedMemory,
			MaximumMessageLength, ConnectionInfo, ConnectionInfoLength);

	return NtSecureConnectPort( ClientPortHandle, ServerPortName,
			SecurityQos, ClientSharedMemory, NULL, ServerSharedMemory,
			MaximumMessageLength, ConnectionInfo, ConnectionInfoLength);
}

NTSTATUS NTAPI NtSecureConnectPort(
	PHANDLE ClientPortHandle,
	PUNICODE_STRING ServerPortName,
	PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	PLPC_SECTION_WRITE ClientSharedMemory,
	PSID ServerSid,
	PLPC_SECTION_READ ServerSharedMemory,
	PULONG MaximumMessageLength,
	PVOID ConnectionInfo,
	PULONG ConnectionInfoLength )
{
	unicode_string_t name;
	NTSTATUS r;

	dprintf("%p %p %p %p %p %p %p %p %p\n", ClientPortHandle, ServerPortName,
			SecurityQos, ClientSharedMemory, ServerSid, ServerSharedMemory,
			MaximumMessageLength, ConnectionInfo, ConnectionInfoLength);

	r = verify_for_write( ClientPortHandle, sizeof (HANDLE) );
	if (r != STATUS_SUCCESS)
		return r;

	LPC_SECTION_WRITE write_sec;
	memset( &write_sec, 0, sizeof write_sec );
	if (ClientSharedMemory)
	{
		r = copy_from_user( &write_sec, ClientSharedMemory, sizeof write_sec );
		if (r != STATUS_SUCCESS)
			return r;
		if (write_sec.Length != sizeof write_sec)
			return STATUS_INVALID_PARAMETER;
	}

	LPC_SECTION_READ read_sec;
	memset( &read_sec, 0, sizeof read_sec );
	if (ServerSharedMemory)
	{
		r = copy_from_user( &read_sec, ServerSharedMemory, sizeof read_sec );
		if (r != STATUS_SUCCESS)
			return r;
		if (read_sec.Length != sizeof read_sec)
			return STATUS_INVALID_PARAMETER;
	}

	PSECURITY_QUALITY_OF_SERVICE qos;
	r = copy_from_user( &qos, SecurityQos, sizeof qos );
	if (r != STATUS_SUCCESS)
		return r;

	r = name.copy_from_user( ServerPortName );
	if (r != STATUS_SUCCESS)
		return r;

	if (!name.Buffer)
		return STATUS_OBJECT_NAME_INVALID;

	if (ClientSharedMemory)
	{
		section_t* sec = 0;
		r = object_from_handle( sec, write_sec.SectionHandle, 0 );
		if (r != STATUS_SUCCESS)
			return STATUS_INVALID_HANDLE;
	}

	// get the length
	ULONG info_length = 0;
	if (ConnectionInfoLength)
	{
		r = copy_from_user( &info_length, ConnectionInfoLength, sizeof info_length );
		if (r != STATUS_SUCCESS)
			return r;
	}

	if (MaximumMessageLength)
	{
		r = verify_for_write( MaximumMessageLength, sizeof *MaximumMessageLength );
		if (r != STATUS_SUCCESS)
			return r;
	}

	// build a connect message
	message_t* msg = new(info_length) message_t;
	if (!msg)
		return STATUS_NO_MEMORY;

	// hack to avoid copying too much data then failing later
	if (info_length > 0x104)
		info_length = 0x108;

	msg->req.DataSize = info_length;
	msg->req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + info_length;
	msg->req.MessageType = LPC_CONNECTION_REQUEST;
	current->get_client_id( &msg->req.ClientId );
	msg->req.MessageId = unique_message_id++;
	msg->req.SectionSize = write_sec.ViewSize;

	r = copy_from_user( msg->req.Data, ConnectionInfo, info_length );
	if (r == STATUS_SUCCESS)
	{
		message_t *reply = 0;
		r = connect_port( ClientPortHandle, &name, msg, reply, MaximumMessageLength, &write_sec, ServerSharedMemory );
		if (r == STATUS_SUCCESS && ClientSharedMemory)
			copy_to_user( ClientSharedMemory, &write_sec, sizeof write_sec );

		// copy the received connect info back to the caller
		if (reply)
		{
			if (ConnectionInfoLength )
			{
				// the buffer can't be assumed to be bigger info_length
				// so truncate the received data
				if (info_length > reply->req.DataSize)
					info_length = reply->req.DataSize;
				if (ConnectionInfo)
					copy_to_user( ConnectionInfo, reply->req.Data, info_length );
				copy_to_user( ConnectionInfoLength, &info_length, sizeof info_length );
			}
			delete reply;
		}
	}

	return r;
}

NTSTATUS NTAPI NtReplyWaitReceivePort(
	HANDLE PortHandle,
	PULONG ReceivePortHandle,
	PLPC_MESSAGE Reply,
	PLPC_MESSAGE IncomingRequest )
{
	port_t *port = 0;
	message_t *reply_msg = NULL;
	NTSTATUS r;

	dprintf("%p %p %p %p\n", PortHandle, ReceivePortHandle, Reply, IncomingRequest);

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	if (ReceivePortHandle)
	{
		r = verify_for_write( ReceivePortHandle, sizeof *ReceivePortHandle );
		if (r != STATUS_SUCCESS)
			return r;
	}

	if (Reply)
	{
		r = copy_msg_from_user( &reply_msg, Reply, port->queue->max_data );
		if (r != STATUS_SUCCESS)
			return r;
	}

	r = verify_for_write( IncomingRequest, sizeof *IncomingRequest );
	if (r != STATUS_SUCCESS)
	{
		delete reply_msg;
		return r;
	}

	message_t *received = 0;
	r = port->reply_wait_receive( reply_msg, received );
	if (r != STATUS_SUCCESS)
		return r;

	r = copy_msg_to_user( IncomingRequest, received );
	if (r == STATUS_SUCCESS)
	{
		if (ReceivePortHandle)
			copy_to_user( ReceivePortHandle, &received->destination_id, sizeof (ULONG) );
		delete received;
	}

	return r;
}

NTSTATUS NTAPI NtRequestWaitReplyPort(
	HANDLE PortHandle,
	PLPC_MESSAGE Request,
	PLPC_MESSAGE Reply )
{
	message_t *msg = NULL;
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p %p %p\n", PortHandle, Request, Reply );

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	r = copy_msg_from_user( &msg, Request, port->queue->max_data );
	if (r != STATUS_SUCCESS)
		return r;

	msg->req.MessageType = LPC_REQUEST;

	message_t *reply_msg = 0;
	port->request_wait_reply( msg, reply_msg );

	r = copy_msg_to_user( Reply, reply_msg );
	if (r == STATUS_SUCCESS)
		delete reply_msg;

	return r;
}

NTSTATUS NTAPI NtAcceptConnectPort(
	PHANDLE ServerPortHandle,
	ULONG PortIdentifier,
	PLPC_MESSAGE ConnectionReply,
	BOOLEAN AcceptConnection,
	PLPC_SECTION_WRITE ServerSharedMemory,
	PLPC_SECTION_READ ClientSharedMemory )
{
	NTSTATUS r;

	dprintf("%p %lx %p %u %p %p\n", ServerPortHandle, PortIdentifier,
			ConnectionReply, AcceptConnection, ServerSharedMemory, ClientSharedMemory );

	message_t *reply = 0;
	r = copy_msg_from_user( &reply, ConnectionReply, 0x148 );
	if (r != STATUS_SUCCESS)
		return r;

	r = verify_for_write( ServerPortHandle, sizeof *ServerPortHandle );
	if (r != STATUS_SUCCESS)
		return r;

	LPC_SECTION_WRITE write_sec;
	memset( &write_sec, 0, sizeof write_sec );
	if (ServerSharedMemory)
	{
		r = copy_from_user( &write_sec, ServerSharedMemory, sizeof write_sec );
		if (r != STATUS_SUCCESS)
			return r;
		if (write_sec.Length != sizeof write_sec)
			return STATUS_INVALID_PARAMETER;
	}

	if (ClientSharedMemory)
	{
		LPC_SECTION_READ read_sec;
		r = copy_from_user( &read_sec, ClientSharedMemory, sizeof read_sec );
		if (r != STATUS_SUCCESS)
			return r;
		if (read_sec.Length != sizeof read_sec)
			return STATUS_INVALID_PARAMETER;
		r = verify_for_write( ClientSharedMemory, sizeof *ClientSharedMemory );
		if (r != STATUS_SUCCESS)
			return r;
	}

	thread_t *t = find_thread_by_client_id( &reply->req.ClientId );
	if (!t)
		return STATUS_INVALID_CID;

	dprintf("%08lx %08lx\n", t->MessageId, reply->req.MessageId);
	if (t->MessageId != reply->req.MessageId)
	{
		dprintf("reply message mismatch\n");
		return STATUS_REPLY_MESSAGE_MISMATCH;
	}

	// avoid accepting the same connection twice
	if (!t->port)
		return STATUS_REPLY_MESSAGE_MISMATCH;
	if (t->port->other)
		return STATUS_REPLY_MESSAGE_MISMATCH;

	if (!AcceptConnection)
	{
		// restart the thread that was blocked on connect
		t->port = 0;
		t->start();
		return STATUS_SUCCESS;
	}

	assert( t->port );
	assert( !t->port->other );
	assert( t->port->queue );

	port_t *port = new port_t( FALSE, current, t->port->queue );
	if (!port)
		return STATUS_NO_MEMORY;

	// tie the ports together
	r = port->accept_connect( t, reply, &write_sec );
	if (r != STATUS_SUCCESS)
	{
		delete port;
		return r;
	}

	// allocate a handle
	HANDLE handle = 0;
	r = alloc_user_handle( port, 0, ServerPortHandle, &handle );

	// write out information on the sections we just created
	if (ClientSharedMemory)
	{
		LPC_SECTION_READ read_sec;

		if (port->other->section)
		{
			read_sec.Length = sizeof read_sec;
			read_sec.ViewBase = port->other_section_base;
			read_sec.ViewSize = port->other->view_size;
		}
		else
			memset( &read_sec, 0, sizeof read_sec);
		copy_to_user( ClientSharedMemory, &read_sec, sizeof read_sec );
	}

	if (ServerSharedMemory)
		copy_to_user( ServerSharedMemory, &write_sec, sizeof write_sec );

	// use the port's handle as its identifier
	if (PortIdentifier)
		port->other->identifier = PortIdentifier;
	else
		port->other->identifier = (ULONG) handle;

	release( port );

	return r;
}

NTSTATUS NTAPI NtCompleteConnectPort(
	HANDLE PortHandle )
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p\n", PortHandle);

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	return complete_connect_port( port );
}

NTSTATUS NTAPI NtReplyPort(
	HANDLE PortHandle,
	PLPC_MESSAGE reply )
{
	port_t *port = 0;
	message_t *msg = NULL;
	NTSTATUS r;

	dprintf("%p %p\n", PortHandle, reply );

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	r = copy_msg_from_user( &msg, reply, port->queue->max_data );
	if (r != STATUS_SUCCESS)
		goto error;

	r = port->send_reply( msg );

error:
	if (r != STATUS_SUCCESS)
		msg_free_unlinked( msg );

	return r;
}

NTSTATUS NTAPI NtRegisterThreadTerminatePort(
	HANDLE PortHandle)
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p\n", PortHandle);

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	current->register_terminate_port( port );

	return r;
}

NTSTATUS NTAPI NtRequestPort(
	HANDLE PortHandle,
	PLPC_MESSAGE Request )
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p %p\n", PortHandle, Request);

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	message_t *msg = 0;
	r = copy_msg_from_user( &msg, Request, port->queue->max_data );
	if (r != STATUS_SUCCESS)
		return r;

	msg->req.MessageType = LPC_DATAGRAM;
	return port->send_request( msg );
}

NTSTATUS NTAPI NtSetDefaultHardErrorPort(
	HANDLE PortHandle )
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p\n", PortHandle );

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("does nothing\n");

	return STATUS_SUCCESS;
}

//syscall NtQueryInformationPort (9a) not implemented
NTSTATUS NTAPI NtQueryInformationPort(
	HANDLE PortHandle,
	PORT_INFORMATION_CLASS InformationClass,
	PVOID Buffer,
	ULONG Length,
	PULONG ReturnLength )
{
	port_t *port = 0;
	NTSTATUS r;

	dprintf("%p\n", PortHandle );

	r = port_from_handle( PortHandle, port );
	if (r != STATUS_SUCCESS)
		return r;

	switch (InformationClass)
	{
	default:
		return STATUS_INVALID_INFO_CLASS;
	}

	return STATUS_SUCCESS;
}

