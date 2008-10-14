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

class thread_obj_wait_t;
typedef list_anchor<thread_obj_wait_t, 0> thread_obj_wait_list_t;
typedef list_element<thread_obj_wait_t> thread_obj_wait_element_t;
typedef list_iter<thread_obj_wait_t, 0> thread_obj_wait_iter_t;

struct thread_obj_wait_t : public watch_t
{
	thread_obj_wait_element_t entry[1];
	sync_object_t *obj;
	thread_impl_t *thread;
public:
	thread_obj_wait_t( thread_impl_t* t, sync_object_t* o);
	virtual void notify();
	virtual ~thread_obj_wait_t();
	BOOLEAN is_signalled() { return obj->is_signalled(); }
	BOOLEAN satisfy() { return obj->satisfy(); }
};

class callback_frame_t
{
	CONTEXT ctx;
	callback_frame_t *prev;
	// values from NtCallbackReturn
	NTSTATUS status;
	ULONG length;
	PVOID buffer;
	BOOLEAN complete;
public:
	callback_frame_t(thread_impl_t *t);
	void do_return(NTSTATUS s, ULONG l, PVOID b);
	void get_return(NTSTATUS& s, ULONG& l, PVOID& b);
	BOOLEAN is_complete() {return complete;}
	void pop( thread_impl_t *t );
};

struct thread_apc_t;
typedef list_anchor<thread_apc_t,0> thread_apc_list_t;
typedef list_element<thread_apc_t> thread_apc_element_t;

struct thread_apc_t
{
	thread_apc_t(PKNORMAL_ROUTINE ApcRoutine, PVOID Arg1, PVOID Arg2, PVOID Arg3) :
		proc(ApcRoutine)
	{
		arg[0] = Arg1;
		arg[1] = Arg2;
		arg[2] = Arg3;
	}
	thread_apc_element_t entry[1];
	PKNORMAL_ROUTINE proc;
	PVOID arg[3];
};

class thread_impl_t :
	public thread_t,
	public execution_context_t,
	public timeout_t
{
	THREAD_STATE ThreadState;
	ULONG SuspendCount;

	//SYSTEM_THREAD_INFORMATION members
	NTSTATUS ExitStatus;
	section_t *teb_section;
	PVOID TebBaseAddress;	// user
	PTEB teb;		// kernel

	// list of APCs
	thread_apc_list_t apc_list;
	BOOLEAN alerted;

	// blocking objects
	thread_obj_wait_list_t wait_list;
	BOOLEAN Alertable;
	WAIT_TYPE WaitType;
	BOOLEAN in_wait;

	KERNEL_USER_TIMES times;

	CONTEXT ctx;
	BOOLEAN context_changed;

	object_t *terminate_port;
	token_t *token;

	// win32 callback stack
	callback_frame_t *callback_frame;
	PVOID Win32StartAddress;
	bool win32k_init_done;

	// memory tracing
#ifdef MEM_TRACE
	mblock *accessed;
	BYTE *accessed_addr;
#endif

public:
	thread_impl_t( process_t *p );
	~thread_impl_t();
	NTSTATUS create( CONTEXT *ctx, INITIAL_TEB *init_teb, BOOLEAN suspended );
	virtual BOOLEAN is_signalled( void );
	void set_state( THREAD_STATE state );
	bool is_terminated() { return ThreadState == StateTerminated; }
	void query_information( THREAD_BASIC_INFORMATION& info );
	void query_information( KERNEL_USER_TIMES& info );
	NTSTATUS zero_tls_cells( ULONG index );
	NTSTATUS kernel_debugger_output_string( struct kernel_debug_string_output *hdr );
	NTSTATUS kernel_debugger_call( ULONG func, void *arg1, void *arg2 );
	BOOLEAN software_interrupt( BYTE number );
	void handle_user_segv();
	void start_exception_handler(exception_stack_frame& frame);
	NTSTATUS raise_exception( exception_stack_frame& info, BOOLEAN SearchFrames );
	NTSTATUS do_user_callback( ULONG index, ULONG& length, PVOID& buffer);
	NTSTATUS user_callback_return(PVOID Result, ULONG ResultLength, NTSTATUS Status );
	NTSTATUS terminate( NTSTATUS Status );
	NTSTATUS test_alert();
	NTSTATUS queue_apc_thread(PKNORMAL_ROUTINE ApcRoutine, PVOID Arg1, PVOID Arg2, PVOID Arg3);
	BOOLEAN deliver_apc( NTSTATUS status );
	NTSTATUS resume( PULONG count );
	int set_initial_regs( void *start, void *stack);
	void copy_registers( CONTEXT& dest, CONTEXT &src, ULONG flags );
	void set_context( CONTEXT& c, bool override_return=true );
	void get_context( CONTEXT& c );
	void set_token( token_t *tok );
	token_t* get_token();
	callback_frame_t* set_callback( callback_frame_t *cb );
	PVOID& win32_start_address();
	void register_terminate_port( object_t *port );
	bool win32k_init_complete();

	virtual int run();

	// wait related functions
	NTSTATUS wait_on_handles( ULONG count, PHANDLE handles, WAIT_TYPE type, BOOLEAN alert, PLARGE_INTEGER timeout );
	NTSTATUS check_wait();
	NTSTATUS wait_on( sync_object_t *obj );
	NTSTATUS check_wait_all();
	NTSTATUS check_wait_any();
	void end_wait();
	virtual void signal_timeout(); // timeout_t
	NTSTATUS delay_execution( LARGE_INTEGER& timeout );
	void start();
	void wait();
	void notify();
	NTSTATUS alert();
	ULONG is_last_thread();

	virtual void handle_fault();
	virtual void handle_breakpoint();

	virtual NTSTATUS copy_to_user( void *dest, const void *src, size_t count );
	virtual NTSTATUS copy_from_user( void *dest, const void *src, size_t count );
	virtual NTSTATUS verify_for_write( void *dest, size_t count );
};

list_anchor<runlist_entry_t,0> runlist_entry_t::running_threads;
ULONG runlist_entry_t::num_running_threads;

void runlist_entry_t::runlist_add()
{
	assert( (num_running_threads == 0) ^ !running_threads.empty() );
	running_threads.prepend( this );
	num_running_threads++;
}

void runlist_entry_t::runlist_remove()
{
	running_threads.unlink( this );
	num_running_threads--;
	assert( (num_running_threads == 0) ^ !running_threads.empty() );
}

ULONG runlist_entry_t::num_active_threads()
{
	return num_running_threads;
}

int thread_impl_t::set_initial_regs( void *start, void *stack)
{
	process->vm->init_context( ctx );

	ctx.Eip = (DWORD) start;
	ctx.Esp = (DWORD) stack;

	context_changed = TRUE;

	return 0;
}

BOOLEAN thread_impl_t::is_signalled( void )
{
	return (ThreadState == StateTerminated);
}

void thread_impl_t::set_state( THREAD_STATE state )
{
	ULONG prev_state = ThreadState;

	if (prev_state == StateTerminated)
		return;

	ThreadState = state;
	switch (state)
	{
	case StateWait:
		runlist_remove();
		break;
	case StateRunning:
		runlist_add();
		break;
	case StateTerminated:
		notify_watchers();
		if (prev_state == StateRunning)
			runlist_remove();
		break;
	default:
		die("switch to unknown thread state\n");
	}

	// ready to complete some I/O?
	if (num_active_threads() == 0)
		check_completions();
}


NTSTATUS thread_impl_t::kernel_debugger_output_string( struct kernel_debug_string_output *hdr )
{
	struct kernel_debug_string_output header;
	char *string;
	NTSTATUS r;

	r = copy_from_user( &header, hdr, sizeof header );
	if (r != STATUS_SUCCESS)
	{
		dprintf("debug string output header invalid\n");
		return r;
	}

	string = new char[ header.length + 1 ];
	memset( string, 0, header.length + 1 );
	r = copy_from_user( string, hdr+1, header.length );
	if (r == STATUS_SUCCESS)
	{
		if (header.address == 0 && header.unknown1 == 0 && header.unknown2 == 0)
			fprintf(stderr, "%04lx (debug) : %s\n", trace_id(), string);
		else
			fprintf(stderr, "%04lx (debug %lx,%lx,%lx) : %s\n", trace_id(), header.address, header.unknown1, header.unknown2, string);
	}
	else
		fprintf(stderr, "%04lx (debug %lx,%lx,%lx) <invalid>\n", trace_id(), header.address, header.unknown1, header.unknown2);

	delete string;
	return r;
}

ULONG thread_t::trace_id()
{
	if (!process)
		return id;
	return id | (process->id<<8);
}

NTSTATUS thread_impl_t::kernel_debugger_call( ULONG func, void *arg1, void *arg2 )
{
	NTSTATUS r;

	switch (func)
	{
	case 1:
		r = kernel_debugger_output_string( (struct kernel_debug_string_output *)arg1 );
		break;
	case 0x101:
		{
		const char *sym = process->vm->get_symbol( (BYTE*) arg1 );
		if (sym)
			fprintf(stderr, "%04lx: %s called\n", trace_id(), sym);
		else
			fprintf(stderr, "%04lx: %p called\n", trace_id(), arg1);
		}
		r = 0;
		break;
	default:
		dump_regs( &ctx );
		dprintf("unhandled function %ld\n", func );
		r = STATUS_NOT_IMPLEMENTED;
	}
	if (r == STATUS_SUCCESS)
	{
		// skip breakpoints after debugger calls
		BYTE inst[1];
		if (STATUS_SUCCESS == copy_from_user( inst, (void*) ctx.Eip, 1 ) &&
			inst[0] == 0xcc)
		{
			ctx.Eip++;
		}
	}
	return r;
}

BOOLEAN thread_impl_t::software_interrupt( BYTE number )
{
	if (number > 0x2e || number < 0x2b)
	{
		dprintf("Unhandled software interrupt %02x\n", number);
		return FALSE;
	}

	ctx.Eip += 2;
	context_changed = FALSE;

	NTSTATUS r;
	switch (number)
	{
	case 0x2b:
		r = NtCallbackReturn( (void*) ctx.Ecx, ctx.Edx, ctx.Eax );
		break;

	case 0x2c:
		r = NtSetLowWaitHighThread();
		break;

	case 0x2d:
		kernel_debugger_call( ctx.Eax, (void*) ctx.Ecx, (void*) ctx.Edx );
		r = ctx.Eax;  // check if this returns a value
		break;

	case 0x2e:
		r = do_nt_syscall( trace_id(), ctx.Eax, (ULONG*) ctx.Edx, ctx.Eip );
		break;

	default:
		assert(0);
	}

	if (!context_changed)
		 ctx.Eax = r;

	return TRUE;
}

void thread_impl_t::handle_user_segv()
{
	if (!option_quiet)
	{
		fprintf(stderr, "%04lx: exception at %08lx\n", trace_id(), ctx.Eip);
		dump_regs( &ctx );
		debugger_backtrace(&ctx);
	}

	exception_stack_frame info;

	memset( &info, 0, sizeof info );
	memcpy( &info.ctx, &ctx, sizeof ctx );

	// FIXME: might not be an access violation
	info.rec.ExceptionCode = STATUS_ACCESS_VIOLATION;
	info.rec.ExceptionFlags = EXCEPTION_CONTINUABLE;
	info.rec.ExceptionRecord = 0;
	info.rec.ExceptionAddress = (void*) ctx.Eip;
	info.rec.NumberParameters = 0;
	//ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];

	start_exception_handler( info );
}

void thread_impl_t::start_exception_handler(exception_stack_frame& info)
{
	if (0)
	{
		dprintf("ExceptionCode	%08lx\n", info.rec.ExceptionCode);
		dprintf("ExceptionFlags   %08lx\n", info.rec.ExceptionFlags);
		dprintf("ExceptionRecord  %p\n", info.rec.ExceptionRecord);
		dprintf("ExceptionAddress %p\n", info.rec.ExceptionAddress);
		dprintf("NumberParameters %ld\n", info.rec.NumberParameters);
	}

	if (send_exception( this, info.rec ))
		return;

	info.ctx.ContextFlags = context_all;

	info.ctx.SegCs &= 0xffff;
	info.ctx.SegEs &= 0xffff;
	info.ctx.SegSs &= 0xffff;
	info.ctx.SegDs &= 0xffff;
	info.ctx.SegFs &= 0xffff;
	info.ctx.SegGs &= 0xffff;

	// hack to stop looping
	if (info.ctx.Eip & 0x80000000)
	{
		dprintf("Eip invalid %08lx\n", info.ctx.Eip);
		terminate(STATUS_ACCESS_VIOLATION);
		return;
	}

	// push the context, exception record and KiUserExceptionDispatcher args
	exception_stack_frame *stack = (exception_stack_frame*)((BYTE*) ctx.Esp - sizeof info);
	info.pctx = &stack->ctx;
	info.prec = &stack->rec;
	ctx.Esp = (LONG) stack;

	NTSTATUS r = copy_to_user( stack, &info, sizeof info );
	if (r != STATUS_SUCCESS)
	{
		dprintf("%04lx: invalid stack handling exception at %08lx\n", id, ctx.Eip);
		terminate(r);
		return;
	}

	// get the address of the user side handler
	// FIXME: this should be stored in the process_t structure
	BYTE *pKiExceptionDispatcher = (BYTE*)process->pntdll +
				get_proc_address( ntdll_section, "KiUserExceptionDispatcher" );
	if (!pKiExceptionDispatcher)
		die("failed to find KiExceptionDispatcher in ntdll\n");

	context_changed = 1;
	ctx.Eip = (ULONG) pKiExceptionDispatcher;
}

callback_frame_t* thread_impl_t::set_callback( callback_frame_t *cb )
{
	callback_frame_t *old = callback_frame;
	callback_frame = cb;
	return old;
}

callback_frame_t::callback_frame_t(thread_impl_t *t) :
	status(STATUS_PENDING),
	length(0),
	buffer(0),
	complete(FALSE)
{
	ctx.ContextFlags = context_all;
	t->get_context( ctx );
	prev = t->set_callback( this );
}

void callback_frame_t::do_return( NTSTATUS s, ULONG l, PVOID b )
{
	assert(!complete);
	status = s;
	length = l;
	buffer = b;
	complete = TRUE;
}

void callback_frame_t::pop( thread_impl_t *t )
{
	assert( complete );
	t->set_callback( prev );
	// clear context_changed so eax is set
	t->set_context( ctx, false );
}

void callback_frame_t::get_return(NTSTATUS& s, ULONG& l, PVOID& b)
{
	assert(complete);
	s = status;
	l = length;
	b = buffer;
}

NTSTATUS thread_impl_t::do_user_callback( ULONG index, ULONG &length, PVOID &buffer)
{
	struct {
		ULONG x[4];
	} frame;

	if (index == 0)
		die("zero index in win32 callback\n");
	frame.x[0] = 0;
	frame.x[1] = index - 1;
	frame.x[2] = ctx.Esp;
	frame.x[3] = 0;

	//callback_stack.push( &ctx, fn );

	ULONG new_esp = ctx.Esp - sizeof frame;
	NTSTATUS r = copy_to_user( (void*) new_esp, &frame, sizeof frame );
	if (r != STATUS_SUCCESS)
	{
		dprintf("%04lx: invalid stack handling exception at %08lx\n", id, ctx.Eip);
		terminate(r);
		return r;
	}

	// FIXME: limit recursion so we don't blow the stack

	// store the current user context
	callback_frame_t old_frame(this);

	// setup the new execution context
	BYTE *pKiUserCallbackDispatcher = (BYTE*)process->pntdll +
				get_proc_address( ntdll_section, "KiUserCallbackDispatcher" );

	context_changed = 1;
	ctx.Eip = (ULONG) pKiUserCallbackDispatcher;
	ctx.Esp = new_esp;

	// recurse, resume user execution here
	dprintf("continuing execution at %08lx\n", ctx.Eip);
	run();

	// fetch return values out of the frame
	old_frame.get_return(r, length, buffer);
	context_changed = 0;
	dprintf("callback returned %08lx\n", r);

	return r;
}

NTSTATUS thread_impl_t::user_callback_return( PVOID Result, ULONG ResultLength, NTSTATUS Status )
{
	if (!callback_frame)
		return STATUS_UNSUCCESSFUL;
	callback_frame->do_return( Status, ResultLength, Result );
	return STATUS_SUCCESS;
}

NTSTATUS thread_impl_t::queue_apc_thread(
	PKNORMAL_ROUTINE ApcRoutine,
	PVOID Arg1,
	PVOID Arg2,
	PVOID Arg3)
{
	thread_apc_t *apc;

	if (ThreadState == StateTerminated)
		return STATUS_ACCESS_DENIED;

	if (!ApcRoutine)
		return STATUS_SUCCESS;

	apc = new thread_apc_t(ApcRoutine, Arg1, Arg2, Arg3);
	if (!apc)
		return STATUS_NO_MEMORY;

	apc_list.append( apc );
	if (in_wait)
		notify();

	return STATUS_SUCCESS;
}

BOOLEAN thread_impl_t::deliver_apc(NTSTATUS thread_return)
{
	// NOTE: can use this to start a thread...
	thread_apc_t *apc = apc_list.head();
	if (!apc)
		return FALSE;

	apc_list.unlink( apc );

	// set the return code in Eax
	ctx.Eax = thread_return;

	NTSTATUS r = STATUS_SUCCESS;
	ULONG new_esp = ctx.Esp;

	// push current context ... for NtContinue
	new_esp -= sizeof ctx;
	r = copy_to_user( (void*) new_esp, &ctx, sizeof ctx );
	if (r != STATUS_SUCCESS)
		goto end;

	// setup APC
	void *apc_stack[4];
	apc_stack[0] = (void*) apc->proc;
	apc_stack[1] = apc->arg[0];
	apc_stack[2] = apc->arg[1];
	apc_stack[3] = apc->arg[2];

	new_esp -= sizeof apc_stack;
	r = copy_to_user( (void*) new_esp, apc_stack, sizeof apc_stack );
	if (r != STATUS_SUCCESS)
		goto end;

	void *pKiUserApcDispatcher;
	pKiUserApcDispatcher = (BYTE*)process->pntdll + get_proc_address( ntdll_section, "KiUserApcDispatcher" );
	if (!pKiUserApcDispatcher)
		die("failed to find KiUserApcDispatcher in ntdll\n");

	ctx.Esp = new_esp;
	ctx.Eip = (ULONG) pKiUserApcDispatcher;
	context_changed = 1;

end:
	if (r != STATUS_SUCCESS)
		terminate( r );
	delete apc;
	return TRUE;
}

void thread_impl_t::copy_registers( CONTEXT& dest, CONTEXT &src, ULONG flags )
{
#define SET(reg) dest.reg = src.reg
#define SETSEG(reg) dest.reg = src.reg&0xffff
	flags &= 0x1f; // remove CONTEXT_X86
	if (flags & CONTEXT_CONTROL)
	{
		SET( Ebp );
		SET( Eip );
		SETSEG( SegCs );
		SET( EFlags );
		SET( Esp );
		SETSEG( SegSs );
	}

	if (flags & CONTEXT_SEGMENTS)
	{
		SETSEG( SegDs );
		SETSEG( SegEs );
		SETSEG( SegFs );
		SET( SegGs );
	}

	if (flags & CONTEXT_INTEGER)
	{
		SET( Ebx );
		SET( Ecx );
		SET( Edx );
		SET( Esi );
		SET( Edi );
		SET( Eax );
	}

	if (flags & CONTEXT_FLOATING_POINT)
	{
		SET( FloatSave );
	}

	if (flags & CONTEXT_DEBUG_REGISTERS)
	{
		SET( Dr0 );
		SET( Dr1 );
		SET( Dr2 );
		SET( Dr3 );
		SET( Dr6 );
		SET( Dr7 );
	}
#undef SET
#undef SETSEG
}

void thread_impl_t::get_context( CONTEXT& c )
{
	copy_registers( c, ctx, c.ContextFlags );
}

// when override_return is true, Eax will not be set on syscall return
void thread_impl_t::set_context( CONTEXT& c, bool override_return )
{
	copy_registers( ctx, c, c.ContextFlags );
	context_changed = override_return;
	dump_regs( &ctx );
}

NTSTATUS thread_impl_t::copy_to_user( void *dest, const void *src, size_t count )
{
	return process->vm->copy_to_user( dest, src, count );
}

NTSTATUS thread_impl_t::copy_from_user( void *dest, const void *src, size_t count )
{
	return process->vm->copy_from_user( dest, src, count );
}

NTSTATUS thread_impl_t::verify_for_write( void *dest, size_t count )
{
	return process->vm->verify_for_write( dest, count );
}

NTSTATUS thread_impl_t::zero_tls_cells( ULONG index )
{
	if (index >= (sizeof teb->TlsSlots/sizeof teb->TlsSlots[0]))
		return STATUS_INVALID_PARAMETER;
	teb->TlsSlots[index] = 0;
	return STATUS_SUCCESS;
}

void thread_impl_t::register_terminate_port( object_t *port )
{
	if (terminate_port)
		release(terminate_port);
	addref( port );
	terminate_port = port;
}

NTSTATUS thread_impl_t::terminate( NTSTATUS status )
{
	if (ThreadState == StateTerminated)
		return STATUS_INVALID_PARAMETER;

	ExitStatus = status;
	set_state( StateTerminated );

	// store the exit time
	times.ExitTime = timeout_t::current_time();

	// send the thread terminate message if necessary
	if (terminate_port)
	{
		send_terminate_message( this, terminate_port, times.CreateTime );
		terminate_port = 0;
	}

	// if we just killed the last thread in the process, kill the process too
	if (process->is_signalled())
	{
		dprintf("last thread in process exitted %08lx\n", status);
		process->terminate( status );
	}

	if (this == current)
		stop();

	return STATUS_SUCCESS;
}

void thread_t::stop()
{
	fiber_t::stop();
	current = this;
}

int thread_impl_t::run()
{
	while (1)
	{
		current = this;

		if (ThreadState != StateRunning)
		{
			dprintf("%04lx: thread state wrong (%d)!\n", trace_id(), ThreadState);
			assert (0);
		}

		// run for 10ms
		LARGE_INTEGER timeout;
		timeout.QuadPart = 10L; // 10ms

		process->vm->run( TebBaseAddress, &ctx, 0, timeout, this );

		if (callback_frame && callback_frame->is_complete())
		{
			callback_frame->pop( this );
			return 0;
		}

		fiber_t::yield();
	}
	return 0;
}

void thread_impl_t::handle_fault()
{
	unsigned char inst[8];
	NTSTATUS r;

	memset( inst, 0, sizeof inst );
	r = copy_from_user( inst, (void*) ctx.Eip, 2 );
	if (r != STATUS_SUCCESS ||
		inst[0] != 0xcd ||
		!software_interrupt( inst[1] ))
	{
		if (option_debug)
			debugger();
		handle_user_segv();
	}
}

void thread_impl_t::handle_breakpoint()
{
	fprintf(stderr,"stopped\n");
	debugger();
}

thread_t::thread_t(process_t *p) :
	fiber_t( fiber_default_stack_size ),
	process( p ),
	MessageId(0),
	port(0)
{
	id = unique_counter++;
	addref( process );
	process->threads.append( this );
}

thread_t::~thread_t()
{
	process->threads.unlink( this );
	release( process );
}

thread_impl_t::thread_impl_t( process_t *p ) :
	thread_t( p ),
	ThreadState(StateInitialized),
	SuspendCount(1),
	ExitStatus(STATUS_PENDING),
	teb_section(0),
	TebBaseAddress(0),
	teb(0),
	alerted(0),
	Alertable(0),
	WaitType(WaitAny),
	in_wait(0),
	context_changed(0),
	terminate_port(0),
	token(0),
	callback_frame(0),
	win32k_init_done(false)
{

	times.CreateTime = timeout_t::current_time();
	times.ExitTime.QuadPart = 0;
	times.UserTime.QuadPart = 0;
	times.KernelTime.QuadPart = 0;

#ifdef MEM_TRACE
	accessed_addr = 0;
	accessed = false;
#endif
}

bool thread_impl_t::win32k_init_complete()
{
	if (win32k_init_done)
		return true;
	win32k_init_done = true;
	return false;
}

void thread_t::get_client_id( CLIENT_ID *client_id )
{
	client_id->UniqueProcess = (HANDLE) (process->id);
	client_id->UniqueThread = (HANDLE) id;
}

void thread_impl_t::query_information( THREAD_BASIC_INFORMATION& info )
{
	info.ExitStatus = ExitStatus;
	info.TebBaseAddress = TebBaseAddress;
	get_client_id( &info.ClientId );
	// FIXME: AffinityMask, Priority, BasePriority
}

void thread_impl_t::query_information( KERNEL_USER_TIMES& info )
{
	info = times;
}

thread_impl_t::~thread_impl_t()
{
	// delete outstanding APCs
	while (apc_list.empty())
	{
		thread_apc_t *apc = apc_list.head();
		apc_list.unlink( apc );
		delete apc;
	}
}

NTSTATUS thread_impl_t::resume( PULONG count )
{
	if (count)
		*count = SuspendCount;
	if (!SuspendCount)
		return STATUS_SUCCESS;
	if (!--SuspendCount)
	{
		set_state( StateRunning );
		start();
	}
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtResumeThread(
	HANDLE ThreadHandle,
	PULONG PreviousSuspendCount )
{
	thread_t *thread;
	ULONG count = 0;
	NTSTATUS r;

	dprintf("%p %p\n", ThreadHandle, PreviousSuspendCount );

	r = object_from_handle( thread, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	r = thread->resume( &count );

	if (r == STATUS_SUCCESS && PreviousSuspendCount)
		r = copy_to_user( PreviousSuspendCount, &count, sizeof count );

	return r;
}

NTSTATUS NTAPI NtSuspendThread(
	HANDLE ThreadHandle,
	PULONG PreviousSuspendCount)
{
	dprintf("%p %p\n", ThreadHandle, PreviousSuspendCount );
	return STATUS_NOT_IMPLEMENTED;
}

thread_obj_wait_t::thread_obj_wait_t( thread_impl_t* t, sync_object_t* o):
	obj(o),
	thread(t)
{
	addref(obj);
	obj->add_watch( this );
}

void thread_obj_wait_t::notify()
{
	//dprintf("waking %p\n", thread);
	thread->notify();
}

void thread_impl_t::start()
{
	// check we weren't terminated
	if (ThreadState != StateTerminated)
		fiber_t::start();
}

void thread_t::wait()
{
	stop();
}

void thread_impl_t::wait()
{
	set_state( StateWait );
	thread_t::wait();
	set_state( StateRunning );
}

NTSTATUS thread_impl_t::check_wait()
{
	NTSTATUS r;

	// check for satisfied waits first, then APCs
	r = (WaitType == WaitAll) ? check_wait_all() : check_wait_any();
	if (r == STATUS_PENDING && Alertable && deliver_apc(STATUS_USER_APC))
		return STATUS_USER_APC;
	return r;
}

thread_obj_wait_t::~thread_obj_wait_t()
{
	obj->remove_watch( this );
	release(obj);
}

NTSTATUS thread_impl_t::wait_on( sync_object_t *obj )
{
	thread_obj_wait_t *wait = new thread_obj_wait_t( this, obj );
	if (!wait)
		return STATUS_NO_MEMORY;

	// Append to list so value in check_wait_any() is right.
	wait_list.append( wait );
	return STATUS_SUCCESS;
}

void thread_impl_t::end_wait()
{
	thread_obj_wait_iter_t i(wait_list);

	while (i)
	{
		thread_obj_wait_t *wait = i;
		i.next();
		wait_list.unlink( wait );
		delete wait;
	}
}

NTSTATUS thread_impl_t::check_wait_all()
{
	thread_obj_wait_iter_t i(wait_list);

	while (i)
	{
		thread_obj_wait_t *wait = i;

		if (!wait->obj->is_signalled())
			return STATUS_PENDING;
		i.next();
	}

	i.reset();
	while (i)
	{
		thread_obj_wait_t *wait = i;
		wait->obj->satisfy();
		i.next();
	}
	return STATUS_SUCCESS;
}

NTSTATUS thread_impl_t::check_wait_any()
{
	thread_obj_wait_iter_t i(wait_list);
	ULONG ret = 0; // return handle index to thread

	while (i)
	{
		thread_obj_wait_t *wait = i;

		i.next();
		if (wait->is_signalled())
		{
			wait->satisfy();
			return ret;
		}
		ret++;
	}
	return STATUS_PENDING;
}

void thread_impl_t::notify()
{
	assert(in_wait);
	in_wait = FALSE;
	start();
}

void thread_impl_t::signal_timeout()
{
	notify();
}

NTSTATUS thread_impl_t::wait_on_handles(
	ULONG count,
	PHANDLE handles,
	WAIT_TYPE type,
	BOOLEAN alert,
	PLARGE_INTEGER timeout)
{
	NTSTATUS r = STATUS_SUCCESS;

	Alertable = alert;
	WaitType = type;

	// iterate the array and wait on each handle
	for (ULONG i=0; i<count; i++)
	{
		dprintf("handle[%ld] = %08lx\n", i, (ULONG) handles[i]);
		object_t *any = 0;
		r = object_from_handle( any, handles[i], SYNCHRONIZE );
		if (r != STATUS_SUCCESS)
		{
			end_wait();
			return r;
		}

		sync_object_t *obj = dynamic_cast<sync_object_t*>( any );
		if (!obj)
		{
			end_wait();
			return STATUS_INVALID_HANDLE;
		}

		r = wait_on( obj );
		if (r != STATUS_SUCCESS)
		{
			end_wait();
			return r;
		}
	}

	// make sure we wait for a little bit every time
	LARGE_INTEGER t;
	if (timeout && timeout->QuadPart <= 0 && timeout->QuadPart> -100000LL)
	{
		t.QuadPart = -100000LL;
		timeout = &t;
	}

	set_timeout( timeout );
	while (1)
	{
		r = check_wait();
		if (r != STATUS_PENDING)
			break;

		if (alerted)
		{
			alerted = FALSE;
			r = STATUS_ALERTED;
			break;
		}

		if (timeout && has_expired())
		{
			r = STATUS_TIMEOUT;
			break;
		}

		in_wait = TRUE;
		wait();
		assert( in_wait == FALSE );
	}

	end_wait();
	set_timeout( 0 );

	return r;
}

NTSTATUS thread_impl_t::alert()
{
	alerted = TRUE;
	if (in_wait)
		notify();
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtWaitForSingleObject(
	HANDLE Handle,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout)
{
	NTSTATUS r;

	dprintf("%p %d %p\n", Handle, Alertable, Timeout);

	object_t *any = 0;
	r = object_from_handle( any, Handle, SYNCHRONIZE );
	if (r != STATUS_SUCCESS)
		return r;

	sync_object_t *obj = dynamic_cast<sync_object_t*>( any );
	if (!obj)
		return STATUS_INVALID_HANDLE;

	LARGE_INTEGER time;
	if (Timeout)
	{
		r = copy_from_user( &time, Timeout, sizeof *Timeout );
		if (r != STATUS_SUCCESS)
			return r;
		Timeout = &time;
	}

	thread_impl_t *t = dynamic_cast<thread_impl_t*>( current );
	assert( t );
	return t->wait_on_handles( 1, &Handle, WaitAny, Alertable, Timeout );
}

NTSTATUS NTAPI NtWaitForMultipleObjects(
	ULONG HandleCount,
	PHANDLE Handles,
	WAIT_TYPE WaitType,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout)
{
	NTSTATUS r;

	dprintf("%lu %p %u %u %p\n", HandleCount, Handles, WaitType, Alertable, Timeout);

	if (HandleCount < 1 || HandleCount > MAXIMUM_WAIT_OBJECTS)
		return STATUS_INVALID_PARAMETER_1;

	LARGE_INTEGER t;
	if (Timeout)
	{
		r = copy_from_user( &t, Timeout, sizeof t );
		if (r != STATUS_SUCCESS)
			return r;
		Timeout = &t;
	}

	// copy the array of handles
	HANDLE hcopy[MAXIMUM_WAIT_OBJECTS];
	r = copy_from_user( hcopy, Handles, HandleCount * sizeof (HANDLE) );
	if (r != STATUS_SUCCESS)
		return r;

	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	return thread->wait_on_handles( HandleCount, hcopy, WaitType, Alertable, Timeout );
}

NTSTATUS NTAPI NtDelayExecution( BOOLEAN Alertable, PLARGE_INTEGER Interval )
{
	LARGE_INTEGER timeout;
	NTSTATUS r;

	r = copy_from_user( &timeout, Interval, sizeof timeout );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("timeout = %llx\n", timeout.QuadPart);
	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	r = thread->wait_on_handles( 0, 0, WaitAny, Alertable, &timeout );
	if (r == STATUS_TIMEOUT)
		r = STATUS_SUCCESS;
	return r;
}

class teb_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void teb_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	fprintf(stderr, "%04lx: accessed teb[%04lx] from %08lx\n",
			current->trace_id(), ofs, eip);
}

teb_tracer teb_trace;

NTSTATUS create_thread( thread_t **pthread, process_t *p, PCLIENT_ID id, CONTEXT *ctx, INITIAL_TEB *init_teb, BOOLEAN suspended )
{
	thread_impl_t *t = new thread_impl_t( p );
	if (!t)
		return STATUS_INSUFFICIENT_RESOURCES;
	NTSTATUS r = t->create( ctx, init_teb, suspended );
	if (r != STATUS_SUCCESS)
	{
		dprintf("releasing partially built thread\n");
		release( t );
		t = 0;
	}
	else
	{
		*pthread = t;
		// FIXME: does a thread die when its last handle is closed?
		addref( t );

		t->get_client_id( id );
	}

	return r;
}

NTSTATUS thread_impl_t::create( CONTEXT *ctx, INITIAL_TEB *init_teb, BOOLEAN suspended )
{
	void *pLdrInitializeThunk;
	void *pKiUserApcDispatcher;
	PTEB pteb = NULL;
	BYTE *addr = 0;
	void *stack;
	NTSTATUS r;
	struct {
		void *pLdrInitializeThunk;
		void *unk1;
		void *pntdll;  /* set to pexe if running a win32 program */
		void *unk2;
		CONTEXT ctx;
		void *ret;	 /* return address (to KiUserApcDispatcher?) */
	} init_stack;

	/* allocate the TEB */
	LARGE_INTEGER sz;
	sz.QuadPart = PAGE_SIZE;
	r = create_section( &teb_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	r = teb_section->mapit( process->vm, addr, 0, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	teb = (PTEB) teb_section->get_kernel_address();

	pteb = (PTEB) addr;
	teb->Peb = (PPEB) process->PebBaseAddress;
	teb->Tib.Self = &pteb->Tib;
	teb->StaticUnicodeString.Buffer = pteb->StaticUnicodeBuffer;
	teb->StaticUnicodeString.MaximumLength = sizeof pteb->StaticUnicodeBuffer;
	teb->StaticUnicodeString.Length = sizeof pteb->StaticUnicodeBuffer;

	// FIXME: need a good thread test for these
	teb->DeallocationStack = init_teb->StackReserved;
	teb->Tib.StackBase = init_teb->StackCommit;
	teb->Tib.StackLimit = init_teb->StackReserved;

	get_client_id( &teb->ClientId );

	/* setup fs in the user address space */
	TebBaseAddress = pteb;

	/* find entry points */
	pLdrInitializeThunk = (BYTE*)process->pntdll + get_proc_address( ntdll_section, "LdrInitializeThunk" );
	if (!pLdrInitializeThunk)
		die("failed to find LdrInitializeThunk in ntdll\n");

	pKiUserApcDispatcher = (BYTE*)process->pntdll + get_proc_address( ntdll_section, "KiUserApcDispatcher" );
	if (!pKiUserApcDispatcher)
		die("failed to find KiUserApcDispatcher in ntdll\n");

	dprintf("LdrInitializeThunk = %p pKiUserApcDispatcher = %p\n",
		pLdrInitializeThunk, pKiUserApcDispatcher );

	// FIXME: should set initial registers then queue an APC

	/* set up the stack */
	stack = (BYTE*) ctx->Esp - sizeof init_stack;

	/* setup the registers */
	int err = set_initial_regs( pKiUserApcDispatcher, stack );
	if (0>err)
		dprintf("set_initial_regs failed (%d)\n", err);

	memset( &init_stack, 0, sizeof init_stack );
	init_stack.pntdll = process->pntdll;  /* set to pexe if running a win32 program */
	init_stack.pLdrInitializeThunk = pLdrInitializeThunk;

	/* copy the context onto the stack for NtContinue */
	memcpy( &init_stack.ctx, ctx, sizeof *ctx );
	init_stack.ret  = (void*) 0xf00baa;

	r = process->vm->copy_to_user( stack, &init_stack, sizeof init_stack );
	if (r != STATUS_SUCCESS)
		dprintf("failed to copy initial stack data\n");

	if (!suspended)
		resume( NULL );

	if (0) trace_memory( process->vm, addr, teb_trace );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateThread(
	PHANDLE Thread,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	HANDLE Process,
	PCLIENT_ID ClientId,
	PCONTEXT Context,
	PINITIAL_TEB InitialTeb,
	BOOLEAN CreateSuspended )
{
	INITIAL_TEB init_teb;
	CONTEXT ctx;
	NTSTATUS r;
	process_t *p;
	thread_t *t = NULL;
	CLIENT_ID id;

	dprintf("%p %08lx %p %p %p %p %p %d\n", Thread, DesiredAccess, ObjectAttributes,
			Process, ClientId, Context, InitialTeb, CreateSuspended);

	r = copy_from_user( &ctx, Context, sizeof ctx );
	if (r != STATUS_SUCCESS)
		return r;

	r = copy_from_user( &init_teb, InitialTeb, sizeof init_teb );
	if (r != STATUS_SUCCESS)
		return r;

	r = process_from_handle( Process, &p );
	if (r != STATUS_SUCCESS)
		return r;

	memset( &id, 0, sizeof id );
	r = create_thread( &t, p, &id, &ctx, &init_teb, CreateSuspended );

	if (r == STATUS_SUCCESS)
	{
		r = alloc_user_handle( t, DesiredAccess, Thread );
		release( t );
	}

	if (r == STATUS_SUCCESS)
		r = copy_to_user( ClientId, &id, sizeof id );

	return r;
}

NTSTATUS NTAPI NtContinue(
	PCONTEXT Context,
	BOOLEAN RaiseAlert)
{
	NTSTATUS r;

	dprintf("%p %d\n", Context, RaiseAlert);

	CONTEXT c;
	r = copy_from_user( &c, Context, sizeof c );
	if (r != STATUS_SUCCESS)
		return r;

	c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	thread->set_context( c );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtYieldExecution( void )
{
	thread_t *t = current;
	for (int i=0; i<0x10; i++)
		fiber_t::yield();
	current = t;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtTerminateThread(
	HANDLE ThreadHandle,
	NTSTATUS Status)
{
	thread_t *t;
	NTSTATUS r;

	dprintf("%p %08lx\n", ThreadHandle, Status);

	if (ThreadHandle == 0)
		t = current;
	else
	{
		r = object_from_handle( t, ThreadHandle, 0 );
		if (r != STATUS_SUCCESS)
			return r;
	}

	// If we killed ourselves we'll return the the scheduler but never run again.
	return t->terminate( Status );
}

ULONG thread_impl_t::is_last_thread()
{
	for ( sibling_iter_t i(process->threads); i; i.next() )
	{
		thread_t *t = i;
		if (t != this && !t->is_terminated())
			return 0;
	}
	return 1;
}

NTSTATUS NTAPI NtQueryInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength,
	PULONG ReturnLength)
{
	union {
		THREAD_BASIC_INFORMATION basic;
		KERNEL_USER_TIMES times;
		ULONG last_thread;
	} info;
	ULONG sz = 0;
	NTSTATUS r;
	thread_impl_t *t;

	dprintf("%p %d %p %lu %p\n", ThreadHandle,
			ThreadInformationClass, ThreadInformation, ThreadInformationLength, ReturnLength);

	switch( ThreadInformationClass )
	{
	case ThreadBasicInformation:
		sz = sizeof info.basic;
		break;
	case ThreadTimes:
		sz = sizeof info.times;
		break;
	case ThreadAmILastThread:
		sz = sizeof info.last_thread;
		break;
	default:
		 dprintf("info class %d\n", ThreadInformationClass);
		 return STATUS_INVALID_INFO_CLASS;
	}

	if (sz != ThreadInformationLength)
		return STATUS_INFO_LENGTH_MISMATCH;

	memset( &info, 0, sizeof info );

	r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	if (ReturnLength)
	{
		r = verify_for_write( ReturnLength, sizeof *ReturnLength );
		if (r != STATUS_SUCCESS)
			return r;
	}

	switch( ThreadInformationClass )
	{
	case ThreadBasicInformation:
		t->query_information( info.basic );
		break;
	case ThreadTimes:
		t->query_information( info.times );
		break;
	case ThreadAmILastThread:
		info.last_thread = t->is_last_thread();
		break;
	default:
		assert(0);
	}

	r = copy_to_user( ThreadInformation, &info, sz );

	if (r == STATUS_SUCCESS && ReturnLength)
		copy_to_user( ReturnLength, &sz, sizeof sz );

	return r;
}

NTSTATUS NTAPI NtAlertThread(
	HANDLE ThreadHandle)
{
	NTSTATUS r;
	thread_impl_t *t = 0;

	dprintf("%p\n", ThreadHandle);

	r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	return t->alert();
}

NTSTATUS NTAPI NtAlertResumeThread(
	HANDLE ThreadHandle,
	PULONG PreviousSuspendCount)
{
	dprintf("%p %p\n", ThreadHandle, PreviousSuspendCount);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtTestAlert(void)
{
	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	return thread->test_alert();
}

NTSTATUS thread_impl_t::test_alert()
{
	if (alerted)
	{
		alerted = FALSE;
		return STATUS_ALERTED;
	}

	deliver_apc(STATUS_SUCCESS);
	return STATUS_SUCCESS;
}

void thread_impl_t::set_token( token_t *tok )
{
	if (token)
		release( token );
	addref( tok );
	token = tok;
}

token_t *thread_impl_t::get_token()
{
	return token;
}

PVOID& thread_impl_t::win32_start_address()
{
	return Win32StartAddress;
}

NTSTATUS NTAPI NtSetInformationThread(
	HANDLE ThreadHandle,
	THREADINFOCLASS ThreadInformationClass,
	PVOID ThreadInformation,
	ULONG ThreadInformationLength)
{
	dprintf("%p %u %p %lu\n", ThreadHandle, ThreadInformationClass,
			ThreadInformation, ThreadInformationLength);

	thread_impl_t *t = 0;
	NTSTATUS r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	switch (ThreadInformationClass)
	{
	case ThreadPriority:
		return STATUS_SUCCESS;
	case ThreadBasePriority:
		return STATUS_SUCCESS;
	case ThreadImpersonationToken:
		{
		HANDLE TokenHandle = 0;
		if (ThreadInformationLength != sizeof TokenHandle)
			return STATUS_INFO_LENGTH_MISMATCH;
		NTSTATUS r = copy_from_user( &TokenHandle, ThreadInformation, sizeof TokenHandle );
		if (r != STATUS_SUCCESS)
			return r;
		token_t *token = 0;
		r = object_from_handle(token, TokenHandle, 0);
		if (r != STATUS_SUCCESS)
			return r;
		t->set_token( token );
		return STATUS_SUCCESS;
		}
	case ThreadZeroTlsCell:
		{
		ULONG index = 0;
		NTSTATUS r = copy_from_user( &index, ThreadInformation, sizeof index );
		if (r != STATUS_SUCCESS)
			return r;
		return t->zero_tls_cells( index );
		}
		return STATUS_SUCCESS;
	case ThreadQuerySetWin32StartAddress:
		{
		PVOID& Win32StartAddress = t->win32_start_address();
		if (ThreadInformationLength != sizeof Win32StartAddress)
			return STATUS_INFO_LENGTH_MISMATCH;
		return copy_from_user( &Win32StartAddress, ThreadInformation, sizeof Win32StartAddress );
		}
	default:
		break;
	}
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueueApcThread(
	HANDLE ThreadHandle,
	PKNORMAL_ROUTINE ApcRoutine,
	PVOID Arg1,
	PVOID Arg2,
	PVOID Arg3)
{
	dprintf("%p %p %p %p %p\n", ThreadHandle, ApcRoutine, Arg1, Arg2, Arg3);

	thread_t *t = 0;
	NTSTATUS r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	return t->queue_apc_thread(ApcRoutine, Arg1, Arg2, Arg3);
}

NTSTATUS NTAPI NtRaiseException( PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context, BOOL SearchFrames )
{
	exception_stack_frame info;
	NTSTATUS r;

	dprintf("%p %p %d\n", ExceptionRecord, Context, SearchFrames);

	r = copy_from_user( &info.rec, ExceptionRecord, sizeof info.rec );
	if (r != STATUS_SUCCESS)
		return r;

	r = copy_from_user( &info.ctx, Context, sizeof info.ctx );
	if (r != STATUS_SUCCESS)
		return r;

	// FIXME: perhaps we should blow away everything pushed on after the current frame

	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	return thread->raise_exception( info, SearchFrames );
}

NTSTATUS thread_impl_t::raise_exception(
	exception_stack_frame& info,
	BOOLEAN SearchFrames )
{
	// pop our args
	ctx.Esp += 12;

	// NtRaiseException probably just pushes two pointers on the stack
	// rather than copying the full context and exception record...

	if (!SearchFrames)
		terminate( info.rec.ExceptionCode );
	else
		start_exception_handler( info );

	return STATUS_SUCCESS;  // not used
}

NTSTATUS NTAPI NtCallbackReturn(
	PVOID Result,
	ULONG ResultLength,
	NTSTATUS Status)
{
	dprintf("%p %lu %08lx\n", Result, ResultLength, Status);
	thread_impl_t *thread = dynamic_cast<thread_impl_t*>( current );
	assert( thread );
	return thread->user_callback_return(Result, ResultLength, Status);
}

NTSTATUS NTAPI NtSetThreadExecutionState(
	EXECUTION_STATE ExecutionState,
	PEXECUTION_STATE PreviousExecutionState )
{
	dprintf("%ld %p\n", ExecutionState, PreviousExecutionState );
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtGetContextThread(
	HANDLE ThreadHandle,
	PCONTEXT Context)
{
	dprintf("%p %p\n", ThreadHandle, Context );

	thread_t *t = 0;
	NTSTATUS r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	CONTEXT c;
	r = copy_from_user( &c, Context, sizeof c );
	if (r != STATUS_SUCCESS)
		return r;

	t->get_context( c );

	r = copy_to_user( Context, &c, sizeof c );
	return r;
}

NTSTATUS NTAPI NtSetContextThread(
	HANDLE ThreadHandle,
	PCONTEXT Context)
{
	dprintf("%p %p\n", ThreadHandle, Context );

	thread_impl_t *t = 0;
	NTSTATUS r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	CONTEXT c;
	r = copy_from_user( &c, Context, sizeof c );
	if (r != STATUS_SUCCESS)
		return r;

	t->set_context( c );
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryDefaultLocale(
	BOOLEAN ThreadOrSystem,
	PLCID Locale)
{
	dprintf("%x %p\n", ThreadOrSystem, Locale);

	LCID lcid = MAKELCID( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT );

	return copy_to_user( Locale, &lcid, sizeof lcid );
}

NTSTATUS NTAPI NtQueryDefaultUILanguage(
	LANGID* Language)
{
	LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
	return copy_to_user( Language, &lang, sizeof lang );
}

NTSTATUS NTAPI NtQueryInstallUILanguage(
	LANGID* Language)
{
	LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
	return copy_to_user( Language, &lang, sizeof lang );
}

NTSTATUS NTAPI NtImpersonateThread(
	HANDLE ThreadHandle,
	HANDLE TargetThreadHandle,
	PSECURITY_QUALITY_OF_SERVICE SecurityQoS)
{
	dprintf("\n");

	thread_t *t = 0;
	NTSTATUS r = object_from_handle( t, ThreadHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	thread_t *target = 0;
	r = object_from_handle( target, TargetThreadHandle, THREAD_DIRECT_IMPERSONATION );
	if (r != STATUS_SUCCESS)
		return r;

	SECURITY_QUALITY_OF_SERVICE qos;
	r = copy_from_user( &qos, SecurityQoS, sizeof qos );
	if (r != STATUS_SUCCESS)
		return r;

	return STATUS_SUCCESS;
}
