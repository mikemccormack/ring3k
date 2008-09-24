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


#include <unistd.h>

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "mem.h"
#include "object.h"
#include "object.inl"
#include "ntcall.h"
#include "section.h"
#include "timer.h"
#include "file.h"

class thread_impl_t;

class kernel_thread_t :
	public thread_t
{
public:
	kernel_thread_t( process_t *p );
	virtual ~kernel_thread_t();
	virtual void get_context( CONTEXT& c );
	virtual bool win32k_init_complete();
	virtual NTSTATUS do_user_callback( ULONG index, ULONG& length, PVOID& buffer);
	virtual NTSTATUS terminate( NTSTATUS Status );
	virtual bool is_terminated();
	virtual void register_terminate_port( object_t *port );
	//virtual void wait();
	virtual NTSTATUS queue_apc_thread(PKNORMAL_ROUTINE ApcRoutine, PVOID Arg1, PVOID Arg2, PVOID Arg3);
	virtual token_t* get_token();
	virtual NTSTATUS resume( PULONG count );
	virtual NTSTATUS copy_to_user( void *dest, const void *src, size_t count );
	virtual NTSTATUS copy_from_user( void *dest, const void *src, size_t count );
	virtual NTSTATUS verify_for_write( void *dest, size_t count );
	virtual int run();
	virtual BOOLEAN is_signalled( void );
};

kernel_thread_t::kernel_thread_t( process_t *p ) :
	thread_t( p )
{
}

kernel_thread_t::~kernel_thread_t()
{
}

BOOLEAN kernel_thread_t::is_signalled()
{
	return FALSE;
}

void kernel_thread_t::get_context( CONTEXT& c )
{
	assert(0);
}

bool kernel_thread_t::win32k_init_complete()
{
	assert(0);
	return 0;
}

NTSTATUS kernel_thread_t::do_user_callback( ULONG index, ULONG& length, PVOID& buffer)
{
	assert(0);
	return 0;
}

NTSTATUS kernel_thread_t::terminate( NTSTATUS Status )
{
	assert(0);
	return 0;
}

bool kernel_thread_t::is_terminated()
{
	assert(0);
	return 0;
}

void kernel_thread_t::register_terminate_port( object_t *port )
{
	assert(0);
}

/*void kernel_thread_t::wait()
{
	assert(0);
}*/

NTSTATUS kernel_thread_t::queue_apc_thread(PKNORMAL_ROUTINE ApcRoutine, PVOID Arg1, PVOID Arg2, PVOID Arg3)
{
	assert(0);
	return 0;
}

token_t* kernel_thread_t::get_token()
{
	assert(0);
	return 0;
}

NTSTATUS kernel_thread_t::resume( PULONG count )
{
	assert(0);
	return 0;
}

NTSTATUS kernel_thread_t::copy_to_user( void *dest, const void *src, size_t count )
{
	if (!dest)
		return STATUS_ACCESS_VIOLATION;
	memcpy( dest, src, count );
	return STATUS_SUCCESS;
}

NTSTATUS kernel_thread_t::copy_from_user( void *dest, const void *src, size_t count )
{
	if (!dest)
		return STATUS_ACCESS_VIOLATION;
	memcpy( dest, src, count );
	return STATUS_SUCCESS;
}

NTSTATUS kernel_thread_t::verify_for_write( void *dest, size_t count )
{
	if (!dest)
		return STATUS_ACCESS_VIOLATION;
	return STATUS_SUCCESS;
}

int kernel_thread_t::run()
{
	const int maxlen = 0x100;
	//dprintf("starting kthread %p p = %p\n", this, process);
	current = static_cast<thread_t*>( this );
	//dprintf("current->process = %p\n", current->process);
	object_attributes_t rm_oa( (PCWSTR) L"\\SeRmCommandPort" );
	HANDLE port = 0, client = 0;
	NTSTATUS r = NtCreatePort( &port, &rm_oa, 0x100, 0x100, 0 );
	if (r != STATUS_SUCCESS)
	{
		die("NtCreatePort(SeRmCommandPort) failed r = %08lx\n", r);
	}

	BYTE buf[maxlen];
	LPC_MESSAGE *req = (LPC_MESSAGE*) buf;
	r = NtListenPort( port, req );
	if (r != STATUS_SUCCESS)
	{
		die("NtListenPort(SeRmCommandPort) failed r = %08lx\n", r);
	}

	HANDLE conn_port = 0;
	r = NtAcceptConnectPort( &conn_port, 0, req, TRUE, NULL, NULL );
	if (r != STATUS_SUCCESS)
	{
		die("NtAcceptConnectPort(SeRmCommandPort) failed r = %08lx\n", r);
	}

	r = NtCompleteConnectPort( conn_port );
	if (r != STATUS_SUCCESS)
	{
		die("NtCompleteConnectPort(SeRmCommandPort) failed r = %08lx\n", r);
	}

	unicode_string_t lsa;
	lsa.copy( (PCWSTR) L"\\SeLsaCommandPort" );

	SECURITY_QUALITY_OF_SERVICE qos;
	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	r = NtConnectPort( &client, &lsa, &qos, NULL, NULL, NULL, NULL, NULL );
	if (r != STATUS_SUCCESS)
	{
		die("NtConnectPort(SeLsaCommandPort) failed r = %08lx\n", r);
	}

	while (1)
	{
		ULONG client_handle;

		r = NtReplyWaitReceivePort( port, &client_handle, 0, req );
		if (r != STATUS_SUCCESS)
		{
			die("NtReplyWaitReceivePort(SeRmCommandPort) failed r = %08lx\n", r);
		}

		dprintf("got message %ld\n", req->MessageId );

		// send something back...
		r = NtReplyPort( port, req );
		if (r != STATUS_SUCCESS)
		{
			die("NtReplyPort(SeRmCommandPort) failed r = %08lx\n", r);
		}
	}

	dprintf("done\n");

	stop();
	return 0;
}

static kernel_thread_t* kernel_thread;

void create_kthread(void)
{
	// process is for the handle table
	process_t *kernel_process = new process_t;
	kernel_thread = new kernel_thread_t( kernel_process );
	//release( kernel_process );

	kernel_thread->start();
}

void shutdown_kthread(void)
{
	// FIXME: the kernel thread is sleeping at this point
	// resources won't be correctly freed
	if (!kernel_thread)
		return;
	delete kernel_thread;
}
