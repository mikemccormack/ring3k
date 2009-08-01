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

#include "ntcall.h"
#include "object.h"
#include "thread.h"
#include "file.h"
#include "debug.h"
#include "object.inl"

// completion_packet_t holds the data for one I/O completion
class completion_packet_t;

typedef list_anchor<completion_packet_t,0> completion_list_t;
typedef list_element<completion_packet_t> completion_list_element_t;

class completion_packet_t
{
public:
	completion_list_element_t entry[1];
	ULONG key;
	ULONG value;
	NTSTATUS status;
	ULONG info;
	completion_packet_t(ULONG k, ULONG v, NTSTATUS s, ULONG i) :
		key(k),
		value(v),
		status(s),
		info(i)
	{
	}
};

// completion_waiter_t is instantiated on the stack of a thread waiting on an I/O completion
class completion_waiter_t;

typedef list_anchor<completion_waiter_t,0> completion_waiter_list_t;
typedef list_element<completion_waiter_t> completion_waiter_list_element_t;

class completion_waiter_t
{
protected:
	friend class list_anchor<completion_waiter_t,0>;
	friend class list_element<completion_waiter_t>;
	completion_waiter_list_element_t entry[1];
	thread_t *thread;
	completion_packet_t *packet;
public:
	completion_waiter_t(thread_t *t) : thread(t), packet(0) {}
	~completion_waiter_t();
	void stop( completion_waiter_list_t& waiter_list, PLARGE_INTEGER timeout = 0);
	void start();
	completion_packet_t *get_packet();
	void set_packet( completion_packet_t* _packet );
	bool is_linked() {return entry[0].is_linked();}
};

void completion_waiter_t::stop(
			completion_waiter_list_t& waiter_list,
			PLARGE_INTEGER timeout)
{
	waiter_list.append(this);
	//thread->set_timeout( timeout );
	thread->wait();
	//thread->set_timeout( 0 );
}

void completion_waiter_t::start()
{
	thread->start();
}

completion_waiter_t::~completion_waiter_t()
{
	assert(!is_linked());
}

void completion_waiter_t::set_packet( completion_packet_t* _packet )
{
	assert( packet == NULL );
	packet = _packet;
}

completion_packet_t *completion_waiter_t::get_packet()
{
	return packet;
}

class completion_port_impl_t;

typedef list_anchor<completion_port_impl_t,0> completion_port_list_t;
typedef list_element<completion_port_impl_t> completion_port_list_element_t;

// completion_port_impl_t is the implementation of the I/O completion port object
class completion_port_impl_t : public completion_port_t
{
	friend class list_anchor<completion_port_impl_t,0>;
	friend class list_element<completion_port_impl_t>;
	completion_port_list_element_t entry[1];
	static completion_port_list_t waiting_thread_ports;
	friend void check_completions( void );
private:
	ULONG num_threads;
	completion_list_t queue;
	completion_waiter_list_t waiter_list;
public:
	completion_port_impl_t( ULONG num );
	virtual ~completion_port_impl_t();
	virtual BOOLEAN is_signalled( void );
	virtual BOOLEAN satisfy( void );
	virtual void set(ULONG key, ULONG value, NTSTATUS status, ULONG info);
	virtual NTSTATUS remove(ULONG& key, ULONG& value, NTSTATUS& status, ULONG& info, PLARGE_INTEGER timeout);
	virtual bool access_allowed( ACCESS_MASK required, ACCESS_MASK handle );
	void check_waiters();
	void port_wait_idle();
	bool is_linked() {return entry[0].is_linked();}
	void start_waiter( completion_waiter_t *waiter );
};

completion_port_impl_t::completion_port_impl_t( ULONG num ) :
	num_threads(num)
{
}

BOOLEAN completion_port_impl_t::satisfy()
{
	return FALSE;
}

BOOLEAN completion_port_impl_t::is_signalled()
{
	return TRUE;
}

bool completion_port_impl_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 IO_COMPLETION_QUERY_STATE,
			 IO_COMPLETION_MODIFY_STATE,
			 IO_COMPLETION_ALL_ACCESS );
}

completion_port_impl_t::~completion_port_impl_t()
{
	assert(!is_linked());
}

completion_port_t::~completion_port_t()
{
}

completion_port_list_t completion_port_impl_t::waiting_thread_ports;

void check_completions( void )
{
	completion_port_impl_t *port = completion_port_impl_t::waiting_thread_ports.head();
	if (!port)
		return;
	port->check_waiters();
}

void completion_port_impl_t::check_waiters()
{
	// start the first waiter
	completion_waiter_t *waiter = waiter_list.head();
	assert( waiter );

	start_waiter( waiter );
}

void completion_port_impl_t::port_wait_idle()
{
	if (is_linked())
		return;
	waiting_thread_ports.append( this );
}

void completion_port_impl_t::set(ULONG key, ULONG value, NTSTATUS status, ULONG info)
{
	completion_packet_t *packet;
	packet = new completion_packet_t( key, value, status, info );
	queue.append( packet );

	// queue a packet if there's no waiting thread
	completion_waiter_t *waiter = waiter_list.head();
	if (!waiter)
		return;

	// give each thread an I/O completion packet
	// and add it to the list of idle threads
	if (runlist_entry_t::num_active_threads() >= num_threads )
	{
		port_wait_idle();
		return;
	}

	// there should only be one packet in the queue here
	start_waiter( waiter );
	assert( queue.empty() );
}

void completion_port_impl_t::start_waiter( completion_waiter_t *waiter )
{
	// remove a packet from the queue
	completion_packet_t *packet = queue.head();
	assert( packet );
	queue.unlink( packet );

	// pass the packet to the waiting thread
	waiter->set_packet( packet );

	// unlink the waiter, and possibly unlink this port
	waiter_list.unlink( waiter );
	if (waiter_list.empty() && is_linked())
		waiting_thread_ports.unlink( this );

	// restart the waiter last
	waiter->start();
}

NTSTATUS completion_port_impl_t::remove(ULONG& key, ULONG& value, NTSTATUS& status, ULONG& info, PLARGE_INTEGER timeout)
{

	// try remove the completion entry first
	completion_packet_t *packet = queue.head();
	if (!packet)
	{
		// queue thread here
		completion_waiter_t waiter(current);
		waiter.stop( waiter_list, timeout );
		packet = waiter.get_packet();
	}
	// this thread must be active... don't block ourselves
	else if (runlist_entry_t::num_active_threads() > num_threads )
	{
		// there's a packet ready but the system, is busy
		// a completion port isn't a FIFO - leave the packt alone
		// wait for idle, then remove the packet
		port_wait_idle();
		completion_waiter_t waiter(current);
		waiter.stop( waiter_list, timeout );
		//if (queue.empty() && is_linked())
			//waiting_thread_ports.unlink( this );
		packet = waiter.get_packet();
	}
	else
	{
		// there's enough free threads to run, and there's a packet waiting
		// we're ready to go
		queue.unlink( packet );
	}
	if (!packet)
		return STATUS_TIMEOUT;

	key = packet->key;
	value = packet->value;
	status = packet->status;
	info = packet->info;

	delete packet;

	return STATUS_SUCCESS;
}

class completion_factory : public object_factory
{
	static const int num_cpus = 1;
private:
	ULONG num_threads;
public:
	completion_factory(ULONG n) : num_threads(n) {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS completion_factory::alloc_object(object_t** obj)
{
	if (num_threads == 0)
		num_threads = num_cpus;

	*obj = new completion_port_impl_t( num_threads );
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI NtCreateIoCompletion(
	PHANDLE IoCompletionHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG NumberOfConcurrentThreads)
{
	trace("%p %08lx %p %ld\n", IoCompletionHandle, DesiredAccess,
			ObjectAttributes, NumberOfConcurrentThreads);
	completion_factory factory( NumberOfConcurrentThreads );
	return factory.create( IoCompletionHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtOpenIoCompletion(
	PHANDLE IoCompletionHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", IoCompletionHandle, AccessMask,
			ObjectAttributes);
	return nt_open_object<completion_port_t>( IoCompletionHandle, AccessMask, ObjectAttributes );
}

// blocking
NTSTATUS NTAPI NtRemoveIoCompletion(
	HANDLE IoCompletionHandle,
	PULONG IoCompletionKey,
	PULONG IoCompletionValue,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER TimeOut)
{
	NTSTATUS r;

	trace("%p %p %p %p %p\n", IoCompletionHandle, IoCompletionKey,
			IoCompletionValue, IoStatusBlock, TimeOut);

	completion_port_impl_t *port = 0;
	r = object_from_handle( port, IoCompletionHandle, IO_COMPLETION_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	if (IoCompletionKey)
	{
		r = verify_for_write( IoCompletionKey, sizeof *IoCompletionKey );
		if (r < STATUS_SUCCESS)
			return r;
	}

	if (IoCompletionValue)
	{
		r = verify_for_write( IoCompletionValue, sizeof *IoCompletionValue );
		if (r < STATUS_SUCCESS)
			return r;
	}

	if (IoStatusBlock)
	{
		r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
		if (r < STATUS_SUCCESS)
			return r;
	}

	LARGE_INTEGER t;
	t.QuadPart = 0LL;
	if (TimeOut)
	{
		r = copy_from_user( &t, TimeOut, sizeof t );
		if (r < STATUS_SUCCESS)
			return r;
		TimeOut = &t;
	}

	ULONG key = 0, val = 0;
	IO_STATUS_BLOCK iosb;
	iosb.Status = 0;
	iosb.Information = 0;
	port->remove( key, val, iosb.Status, iosb.Information, TimeOut );

	if (IoCompletionKey)
		copy_to_user( IoCompletionKey, &key, sizeof key );

	if (IoCompletionValue)
		copy_to_user( IoCompletionValue, &val, sizeof val );

	if (IoStatusBlock)
		copy_to_user( IoStatusBlock, &iosb, sizeof iosb );

	return r;
}

// nonblocking
NTSTATUS NTAPI NtSetIoCompletion(
	HANDLE IoCompletionHandle,
	ULONG IoCompletionKey,
	ULONG IoCompletionValue,
	NTSTATUS Status,
	ULONG Information)
{
	NTSTATUS r;

	trace("%p %08lx %08lx %08lx %08lx\n", IoCompletionHandle, IoCompletionKey,
			IoCompletionValue, Status, Information);

	completion_port_impl_t *port = 0;
	r = object_from_handle( port, IoCompletionHandle, IO_COMPLETION_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	port->set( IoCompletionKey, IoCompletionValue, Status, Information );

	return STATUS_SUCCESS;
}
