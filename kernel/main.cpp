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

#include "config.h"

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
#include <getopt.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "mem.h"
#include "object.h"
#include "objdir.h"
#include "ntcall.h"
#include "section.h"
#include "timer.h"
#include "unicode.h"
#include "fiber.h"
#include "file.h"
#include "event.h"
#include "symlink.h"
#include "alloc_bitmap.h"

process_list_t processes;
thread_t *current;
object_t *ntdll_section;
int option_debug = 0;
ULONG KiIntSystemCall = 0;
bool forced_quit;

class default_sleeper_t : public sleeper_t
{
public:
	virtual bool check_events( bool wait );
	virtual ~default_sleeper_t() {}
};

int sleeper_t::get_int_timeout( LARGE_INTEGER& timeout )
{
	timeout.QuadPart = (timeout.QuadPart+9999)/10000;
	int t = INT_MAX;
	if (timeout.QuadPart < t)
		t = timeout.QuadPart;
	return t;
}

bool default_sleeper_t::check_events( bool wait )
{
	LARGE_INTEGER timeout;

	// check for expired timers
	bool timers_left = timeout_t::check_timers(timeout);

	// Check for a deadlock and quit.
	//  This happens if we're the only active thread,
	//  there's no more timers, and we're asked to wait.
	if (!timers_left && wait && fiber_t::last_fiber())
		return true;
	if (!wait)
		return false;

	int t = get_int_timeout( timeout );
	int r = poll( 0, 0, t );
	if (r >= 0)
		return false;
	if (errno != EINTR)
		die("poll failed %d\n", errno);
	return false;
}

default_sleeper_t default_sleeper;
sleeper_t* sleeper = &default_sleeper;

int schedule(void)
{
	/* while there's still a thread running */
	while (processes.head())
	{
		// check if any thing interesting has happened
		sleeper->check_events( false );

		// other fibers are active... schedule run them
		if (!fiber_t::last_fiber())
		{
			fiber_t::yield();
			continue;
		}

		// there's still processes but no active threads ... sleep
		if (sleeper->check_events( true ))
			break;
	}

	return 0;
}

NTSTATUS create_initial_process( thread_t **t, UNICODE_STRING& us )
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

	r = open_file( file, us );
	if (r < STATUS_SUCCESS)
		return r;

	/* load the executable and ntdll */
	r = create_section( &section, file, 0, SEC_IMAGE, PAGE_EXECUTE_READWRITE );
	release( file );
	if (r < STATUS_SUCCESS)
		return r;

	/* create the initial process */
	r = create_process( &p, section );
	release( section );
	section = NULL;

	if (r < STATUS_SUCCESS)
		return r;

	PPEB ppeb = (PPEB) p->peb_section->get_kernel_address();
	p->create_exe_ppb( &ppeb->ProcessParameters, us );

	/* map the stack */
	pstack = NULL;
	r = p->vm->allocate_virtual_memory( &pstack, 0, stack_size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE );
	if (r < STATUS_SUCCESS)
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
	unicode_string_t us;
	file_t *file = 0;
	NTSTATUS r;

	us.set( ntdll );

	r = open_file( file, us );
	if (r < STATUS_SUCCESS)
		die("failed to open ntdll\n");

	r = create_section( &ntdll_section, file, 0, SEC_IMAGE, PAGE_EXECUTE_READWRITE );
	if (r < STATUS_SUCCESS)
		die("failed to create ntdll section\n");

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

static void backtrace_and_quit()
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

static void segv_handler(int)
{
	backtrace_and_quit();
}

static void abort_handler(int)
{
	backtrace_and_quit();
}

bool init_skas();
bool init_tt( const char *loader_path );

struct trace_option {
	const char *name;
	int enabled;
};

trace_option trace_option_list[] = {
	{ "syscall", false },
	{ "tebshm", false },
	{ "pebshm", false },
	{ "ntshm", false },
	{ "gdishm", false },
	{ "usershm", false },
	{ "csrdebug", false },
	{ "ldrsnaps", false },
	{ "core", false },
	{ 0, false },
};

int& option_trace = trace_option_list[0].enabled;

void usage( void )
{
	const char usage[] =
		"Usage: %s [options] [native.exe]\n"
		"Options:\n"
		"  -d,--debug    break into debugger on exceptions\n"
		"  -g,--graphics select screen driver\n"
		"  -h,--help     print this message\n"
		"  -q,--quiet    quiet, suppress debug messages\n"
		"  -t,--trace=<options>    enable tracing\n"
		"  -v,--version  print version\n\n"
		"  smss.exe is started by default\n\n";
	printf( usage, PACKAGE_NAME );

	// list the trace options
	printf("  trace options: ");
	for (int i=0; trace_option_list[i].name; i++)
		printf("%s ", trace_option_list[i].name );
	printf("\n");

	// list the graphics drivers
	printf("  graphics drivers: ");
	list_graphics_drivers();
	printf("\n");

	printf("\n");

	exit(0);
}


void version( void )
{
	const char version[] = "%s\n"
		"Copyright (C) 2008-2009 Mike McCormack\n"
		"Licence LGPL\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n\n";
	printf( version, PACKAGE_STRING );
	exit(0);
}

bool trace_is_enabled( const char *name )
{
	for (int i=0; trace_option_list[i].name; i++)
		if (!strcmp(name, trace_option_list[i].name))
			return trace_option_list[i].enabled;

	return false;
}

void enable_trace( const char *name )
{
	for (int i=0; trace_option_list[i].name; i++)
	{
		const char *optname = trace_option_list[i].name;
		if ( strcmp( optname, name ))
			continue;
		trace_option_list[i].enabled = true;
		return;
	}

	fprintf(stderr, "unknown trace: %s\n\n", name);
	usage();
}

void parse_trace_options( const char *options )
{
	if (!options)
	{
		enable_trace( "syscall" );
		return;
	}

	const char *x, *p = options;
	unsigned int len;
	char str[10];
	while (*p)
	{
		x = strchr( p, ',' );
		if (x)
			len = x - p;
		else
			len = strlen( p );

		len = min( len, sizeof str );
		memcpy( str, p, len );
		str[len] = 0;
		enable_trace( str );
		p += len;
		if ( *p == ',')
			p++;
	}
}

void parse_options(int argc, char **argv)
{
	while (1)
	{
		int option_index;
		static struct option long_options[] = {
			{"debug", no_argument, NULL, 'd' },
			{"graphics", required_argument, NULL, 'g' },
			{"help", no_argument, NULL, 'h' },
			{"trace", optional_argument, NULL, 't' },
			{"version", no_argument, NULL, 'v' },
			{NULL, 0, 0, 0 },
		};

		int ch = getopt_long(argc, argv, "g:dhqt::v?", long_options, &option_index );
		if (ch == -1)
			break;

		switch (ch)
		{
		case 'd':
			option_debug = 1;
			break;
		case 'g':
			if (!set_graphics_driver( optarg ))
			{
				fprintf(stderr, "unknown graphics driver %s\n", optarg);
				usage();
			}
			break;
		case '?':
		case 'h':
			usage();
			break;
		case 't':
			parse_trace_options( optarg );
			break;
		case 'v':
			version();
		}
	}
}

int main(int argc, char **argv)
{
	unicode_string_t us;
	thread_t *initial_thread = NULL;
	const char *exename;

	parse_options( argc, argv );

	if (optind == argc)
	{
		// default to starting smss.exe
		exename = "\\??\\c:\\winnt\\system32\\smss.exe";
	}
	else
	{
		exename = argv[optind];
	}

	// the skas3 patch is deprecated...
	if (0) init_skas();

	// pass our path so thread tracing can find the client stub
	init_tt( argv[0] );
	if (!pcreate_address_space)
		die("no way to manage address spaces found\n");

	if (!trace_is_enabled("core"))
	{
		// enable backtraces
		signal(SIGSEGV, segv_handler);
		signal(SIGABRT, abort_handler);
	}

	// quick sanity test
	allocation_bitmap_t::test();

	// initialize boottime
	SYSTEM_TIME_OF_DAY_INFORMATION dummy;
	get_system_time_of_day( dummy );

	init_registry();
	fiber_t::fibers_init();
	init_root();
	create_directory_object( (PWSTR) L"\\" );
	create_directory_object( (PWSTR) L"\\??" );
	unicode_string_t link_name, link_target;
	link_name.set( L"\\DosDevices" );
	link_target.copy( L"\\??" );
	create_symlink( link_name, link_target );
	create_directory_object( (PWSTR) L"\\Device" );
	create_directory_object( (PWSTR) L"\\Device\\MailSlot" );
	create_directory_object( (PWSTR) L"\\Security" );
	//create_directory_object( (PWSTR) L"\\DosDevices" );
	create_directory_object( (PWSTR) L"\\BaseNamedObjects" );
	create_sync_event( (PWSTR) L"\\Security\\LSA_AUTHENTICATION_INITIALIZED" );
	create_sync_event( (PWSTR) L"\\SeLsaInitEvent" );
	init_random();
	init_pipe_device();
	// XP
	create_directory_object( (PWSTR) L"\\KernelObjects" );
	create_sync_event( (PWSTR) L"\\KernelObjects\\CritSecOutOfMemoryEvent" );
	init_drives();
	init_ntdll();
	create_kthread();

	us.copy( exename );

	int r = create_initial_process( &initial_thread, us );
	if (r < STATUS_SUCCESS)
		die("create_initial_process() failed (%08x)\n", r);

	// run the main loop
	schedule();

	ntgdi_fini();
	r = initial_thread->process->ExitStatus;
	//fprintf(stderr, "process exited (%08x)\n", r);
	release( initial_thread );

	shutdown_kthread();
	do_cleanup();

	free_root();
	fiber_t::fibers_finish();
	free_registry();
	free_ntdll();

	return r;
}
