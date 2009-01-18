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

#ifndef __THREAD_H__
#define __THREAD_H__

#include "object.h"
#include "list.h"
#include "timer.h"
#include "fiber.h"
#include "token.h"
#include "mem.h"

#define PAGE_SIZE 0x1000

class thread_t;

typedef list_anchor<thread_t,0> sibling_list_t;
typedef list_iter<thread_t,0> sibling_iter_t;
typedef list_element<thread_t> thread_element_t;

struct port_t;
struct process_t;

struct exception_stack_frame
{
	PEXCEPTION_RECORD prec;
	PCONTEXT pctx;
	CONTEXT ctx;
	EXCEPTION_RECORD rec;
};

const ULONG context_all =
		CONTEXT_FLOATING_POINT |
		CONTEXT_DEBUG_REGISTERS |
		CONTEXT_EXTENDED_REGISTERS |
		CONTEXT_FULL ;

struct kernel_debug_string_output {
	USHORT length;
	USHORT pad;
	ULONG address;
	ULONG unknown1;
	ULONG unknown2;
};

struct section_t;
class mblock;

class runlist_entry_t
{
	friend class list_anchor<runlist_entry_t,0>;
	friend class list_element<runlist_entry_t>;
	list_element<runlist_entry_t> entry[1];
	static list_anchor<runlist_entry_t,0> running_threads;
	static ULONG num_running_threads;
public:
	static ULONG num_active_threads();
	void runlist_add();
	void runlist_remove();
};

class thread_t :
	public sync_object_t,
	public fiber_t,
	public runlist_entry_t
{
	friend class list_anchor<thread_t,0>;
	friend class list_element<thread_t>;
	friend class list_iter<thread_t,0>;
	thread_element_t entry[1];

protected:
	ULONG id;

public:
	process_t *process;

	// LPC information
	ULONG MessageId;
	port_t *port;

public:
	thread_t( process_t *p );
	virtual ~thread_t();
	virtual ULONG trace_id();
	ULONG get_id() { return id; }
	virtual void get_client_id( CLIENT_ID *id );
	virtual void wait();
	virtual void stop();

public:
	virtual void get_context( CONTEXT& c ) = 0;
	virtual bool win32k_init_complete() = 0;
	virtual NTSTATUS do_user_callback( ULONG index, ULONG& length, PVOID& buffer) = 0;
	virtual NTSTATUS terminate( NTSTATUS Status ) = 0;
	virtual bool is_terminated() = 0;
	virtual void register_terminate_port( object_t *port ) = 0;
	virtual NTSTATUS queue_apc_thread(PKNORMAL_ROUTINE ApcRoutine, PVOID Arg1, PVOID Arg2, PVOID Arg3) = 0;
	virtual token_t* get_token() = 0;
	virtual NTSTATUS resume( PULONG count ) = 0;
	virtual NTSTATUS copy_to_user( void *dest, const void *src, size_t count ) = 0;
	virtual NTSTATUS copy_from_user( void *dest, const void *src, size_t count ) = 0;
	virtual NTSTATUS verify_for_write( void *dest, size_t count ) = 0;
	virtual void* push( ULONG count ) = 0;
	virtual void pop( ULONG count ) = 0;
	virtual PTEB get_teb() = 0;
};

NTSTATUS create_thread( thread_t **pthread, process_t *p, PCLIENT_ID id, CONTEXT *ctx, INITIAL_TEB *init_teb, BOOLEAN suspended );
int run_thread(fiber_t *arg);

extern thread_t *current;

void send_terminate_message( thread_t *thread, object_t *port, LARGE_INTEGER& create_time );
bool send_exception( thread_t *thread, EXCEPTION_RECORD &rec );

#endif // __THREAD_H__
