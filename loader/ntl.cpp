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


#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <poll.h>
#include <signal.h>
#include <execinfo.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "mem.h"
#include "object.h"
#include "ntcall.h"
#include "section.h"
#include "timer.h"
#include "unicode.h"
#include "fiber.h"
#include "file.h"
#include "event.h"

process_list_t processes;
thread_t *current;
object_t *ntdll_section;
int option_debug = 0;
ULONG KiIntSystemCall = 0;

void sleep_timeout(LARGE_INTEGER timeout)
{
	timeout.QuadPart = (timeout.QuadPart+9999)/10000;
	int t = INT_MAX;
	if (timeout.QuadPart < INT_MAX)
		t = timeout.QuadPart;
	int r = poll(0, 0, t);
	if (r < 0 && errno != EINTR)
		die("poll failed\n");
}

int schedule(void)
{
	/* while there's still a thread running */
	while (processes.head())
	{
		// check if any timers have expired
		LARGE_INTEGER timeout;
		timeout_t::check_timers(timeout);

		// other fibers are active... schedule run them
		if (!fiber_t::last_fiber())
		{
			fiber_t::yield();
			continue;
		}

		// there's still processes but no active threads ... sleep
		if (timeout_t::check_timers(timeout))
			sleep_timeout(timeout);
		else if (fiber_t::last_fiber())
			break;
	}

	return 0;
}

NTSTATUS create_initial_process( thread_t **t, OBJECT_ATTRIBUTES *oa )
{
	BYTE *pstack;
	const unsigned int stack_size = 0x100 * PAGE_SIZE;
	process_t *p = NULL;
	CONTEXT ctx;
	INITIAL_TEB init_teb;
	CLIENT_ID id;
	object_t *section = NULL;
	file_t *file = 0;
	int r;

	r = open_file( file, oa );
	if (r != STATUS_SUCCESS)
		return r;

	/* load the executable and ntdll */
	r = create_section( &section, file, 0, SEC_IMAGE, PAGE_EXECUTE_READWRITE );
	release( file );
	if (r != STATUS_SUCCESS)
		return r;

	/* create the initial process */
	r = create_process( &p, section );
	release( section );
	section = NULL;

	if (r != STATUS_SUCCESS)
		return r;

	PPEB ppeb = (PPEB) p->peb_section->get_kernel_address();
	p->create_exe_ppb( &ppeb->ProcessParameters, oa );

	/* map the stack */
	pstack = NULL;
	r = p->vm->allocate_virtual_memory( &pstack, 0, stack_size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	/* teb initialization data */
	memset( &init_teb, 0, sizeof init_teb );
	init_teb.StackReserved = pstack;
	init_teb.StackCommit = (BYTE*)init_teb.StackReserved + stack_size;
	init_teb.StackCommitMax = (BYTE*)init_teb.StackCommit - PAGE_SIZE;

	/* initialize the first thread's context */
	p->vm->init_context( ctx );
	ctx.Eip = (DWORD) get_entry_point( p );
	ctx.Esp = (DWORD) pstack + stack_size - 8;

	dprintf("entry point = %08lx\n", ctx.Eip);

	/* when starting nt processes, make the PEB the first arg of NtProcessStartup */
	r = p->vm->copy_to_user( (BYTE*) ctx.Esp + 4, &p->PebBaseAddress, sizeof p->PebBaseAddress );

	if (r == STATUS_SUCCESS)
		r = create_thread( t, p, &id, &ctx, &init_teb, FALSE );

	release( p );

	return r;
}

NTSTATUS init_ntdll( void )
{
	WCHAR ntdll[] = {
		'\\','?','?','\\','c',':','\\','w','i','n','n','t','\\',
		's','y','s','t','e','m','3','2','\\','n','t','d','l','l','.','d','l','l',0 };
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	file_t *file = 0;
	NTSTATUS r;

	us.Buffer = ntdll;
	us.Length = sizeof ntdll - 2;
	us.MaximumLength = 0;

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.ObjectName = &us;

	r = open_file( file, &oa );
	if (r != STATUS_SUCCESS)
		die("failed to open ntdll\n");

	r = create_section( &ntdll_section, file, 0, SEC_IMAGE, PAGE_EXECUTE_READWRITE );
	if (r != STATUS_SUCCESS)
		die("failed to map ntdll\n");

	KiIntSystemCall = get_proc_address( ntdll_section, "KiIntSystemCall" );
	dprintf("KiIntSystemCall = %08lx\n", KiIntSystemCall);
	init_syscalls(KiIntSystemCall != 0);

	release( file );

	return r;
}

void free_ntdll( void )
{
	release( ntdll_section );
	ntdll_section = NULL;
}

void do_cleanup( void )
{
	int num_threads = 0, num_processes = 0;

	for ( process_iter_t pi(processes); pi; pi.next() )
	{
		process_t *p = pi;
		if (p->is_signalled())
			continue;
		num_processes++;
		fprintf(stderr, "process %04lx\n", p->id);
		for ( sibling_iter_t ti(p->threads); ti; ti.next() )
		{
			thread_t *t = ti;
			if (t->is_signalled())
				continue;
			fprintf(stderr, "\tthread %04lx\n", t->trace_id());
			num_threads++;
		}
	}
	if (num_threads || num_processes)
		fprintf(stderr, "%d threads %d processes left\n", num_threads, num_processes);
}

int usage( const char *prog )
{
	fprintf(stderr, "%s [-d] [-t] [-m] [-q] native.exe\n", prog );
	fprintf(stderr, "   [-d] break into debugger on exceptions\n");
	fprintf(stderr, "   [-m] memory reference counting\n");
	fprintf(stderr, "   [-t] trace syscall entry and exit\n");
	fprintf(stderr, "   [-q] quiet, suppress debug messages\n");
	return 0;
}

static void segv_handler(int)
{
	const int max_frames = 20;
	void *bt[max_frames];
	char **names;
	int n=0, size;
	ULONG id = 0;

	if (current)
		id = current->trace_id();

	size = backtrace(bt, max_frames);
	names = backtrace_symbols(bt, size);

	fprintf(stderr, "%04lx: caught kernel SEGV (%d frames):\n", id, size);
	for (n=0; n<size; n++)
	{
		fprintf(stderr, "%d: %s\n", n, names[n]);
	}
	exit(1);
}

bool init_skas();
bool init_tt();

bool option_skas = 1;

int main(int argc, char **argv)
{
	OBJECT_ATTRIBUTES oa;
	unicode_string_t us;
	thread_t *initial_thread = NULL;
	int r, n;

	// enable backtraces
	signal(SIGSEGV, segv_handler);

	for (n=1; n<argc; n++)
	{
		if (argv[n][0] != '-')
			break;
		switch (argv[n][1])
		{
		case 'd':
			option_debug = 1;
			break;
		case 't':
			option_trace = 1;
			break;
		case 'q':
			option_quiet = 1;
			break;
		case 's':
			option_skas = 0;
			break;
		default:
			return usage( argv[0] );
		}
	}

	if (n == argc)
		return usage( argv[0] );

	(option_skas && init_skas()) || init_tt();
	if (!pcreate_address_space)
		die("no way to manage address spaces found\n");

	// initialize boottime
	SYSTEM_TIME_OF_DAY_INFORMATION dummy;
	get_system_time_of_day( dummy );

	init_registry();
	init_ntdll();
	fiber_t::fibers_init();
	create_directory_object( (PWSTR) L"\\" );
	create_directory_object( (PWSTR) L"\\??" );
	create_directory_object( (PWSTR) L"\\Device" );
	create_directory_object( (PWSTR) L"\\Global" );
	create_directory_object( (PWSTR) L"\\Security" );
	create_sync_event( (PWSTR) L"\\Security\\LSA_AUTHENTICATION_INITIALIZED" );
	create_sync_event( (PWSTR) L"\\SeLsaInitEvent" );
	// XP
	create_sync_event( (PWSTR) L"\\KernelObjects\\CritSecOutOfMemoryEvent" );
	create_kthread();

	us.copy( argv[n] );

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.ObjectName = &us;

	r = create_initial_process( &initial_thread, &oa );
	if (r != STATUS_SUCCESS)
		die("create_initial_process() failed (%08x)\n", r);

	// run the main loop
	schedule();

	r = initial_thread->process->ExitStatus;
	//fprintf(stderr, "process exitted (%08x)\n", r);
	release( initial_thread );

	shutdown_kthread();

	do_cleanup();

	fiber_t::fibers_finish();
	delete us.Buffer;
	free_registry();
	free_ntdll();

	return r;
}
