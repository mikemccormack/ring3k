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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>

#include <sys/wait.h>
#include <sched.h>

#include <sys/ptrace.h>
#ifdef HAVE_ASM_PTRACE_H
#include <asm/ptrace.h>
#endif

#include "windef.h"
#include "winnt.h"
#include "mem.h"
#include "thread.h"

#include "ptrace_if.h"
#include "debug.h"
#include "platform.h"

#include "client.h"
#include "ptrace_base.h"

const char stub_name[] = "ring3k-client";
char stub_path[MAX_PATH];

class tt_address_space_impl: public ptrace_address_space_impl
{
	long stub_regs[FRAME_SIZE];
	pid_t child_pid;
protected:
	int userside_req( int type );
public:
	tt_address_space_impl();
	virtual pid_t get_child_pid();
	virtual ~tt_address_space_impl();
	virtual int mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset );
	virtual int munmap( BYTE *address, size_t length );
	virtual unsigned short get_userspace_fs();
};

pid_t tt_address_space_impl::get_child_pid()
{
	return child_pid;
}

tt_address_space_impl::tt_address_space_impl()
{
	int r;
	pid_t pid;

	pid = fork();
	if (pid == -1)
		die("fork() failed %d\n", errno);

	if (pid == 0)
	{
		::ptrace( PTRACE_TRACEME, 0, 0, 0 );
		r = ::execl( stub_path, stub_name, NULL );
		// the next line should not be reached
		die("exec failed (%d) - %s missing?\n", r, stub_path);
	}

	// trace through exec after traceme
	wait_for_signal( pid, SIGTRAP );
	r = ::ptrace( PTRACE_CONT, pid, 0, 0 );
	if (r < 0)
		die("PTRACE_CONT failed (%d)\n", errno);

	// client should hit a breakpoint
	wait_for_signal( pid, SIGTRAP );
	r = ptrace_get_regs( pid, stub_regs );
	if (r < 0)
		die("constructor: ptrace_get_regs failed (%d)\n", errno);

	child_pid = pid;
}

tt_address_space_impl::~tt_address_space_impl()
{
	assert( sig_target == 0 );
	//dprintf(stderr,"~tt_address_space_impl()\n");
	destroy();
	ptrace( PTRACE_KILL, child_pid, 0, 0 );
	assert( child_pid != -1 );
	kill( child_pid, SIGTERM );
	child_pid = -1;
}

address_space_impl* create_tt_address_space()
{
	//dprintf("create_tt_address_space\n");
	// Set up the signal handler and unmask it first.
	// The child's signal handler will be unmasked too.
	return new tt_address_space_impl();
}

int tt_address_space_impl::userside_req( int type )
{
	struct tt_req *ureq = (struct tt_req *) stub_regs[EBX];
	int r;

	ptrace( PTRACE_POKEDATA, child_pid, &ureq->type, type );

	r = ptrace_set_regs( child_pid, stub_regs );
	if (r < 0)
		die("ptrace_set_regs failed\n");
	r = ::ptrace( PTRACE_CONT, child_pid, 0, 0 );
	if (r < 0)
		die("ptrace( PTRACE_CONT ) failed\n");

	wait_for_signal( child_pid, SIGTRAP );
	r = ptrace_get_regs( child_pid, stub_regs );
	if (r < 0)
		die("ptrace_get_regs failed (%d)\n", errno);

	return stub_regs[EAX];
}

int tt_address_space_impl::mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset )
{
	//dprintf("tt_address_space_impl::mmap()\n");

	// send our pid to the stub
	struct tt_req *ureq = (struct tt_req *) stub_regs[EBX];
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.pid, getpid() );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.fd, file );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.addr, (int) address );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.len, length );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.ofs, offset );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.prot, prot );
	return userside_req( tt_req_map );
}

int tt_address_space_impl::munmap( BYTE *address, size_t length )
{
	//dprintf("tt_address_space_impl::munmap()\n");
	struct tt_req *ureq = (struct tt_req *) stub_regs[EBX];
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.addr, (int) address );
	ptrace( PTRACE_POKEDATA, child_pid, &ureq->u.map.len, length );
	return userside_req( tt_req_umap );
}

unsigned short tt_address_space_impl::get_userspace_fs()
{
	return stub_regs[FS];
}

void get_stub_path( const char *kernel_path )
{
	// FIXME: handle loader in path too
	const char *p = strrchr( kernel_path, '/' );
	int len;
	if (p)
	{
		len = p - kernel_path + 1;
	}
	else
	{
		static const char current_dir[] = "./";
		p = current_dir;
		len = sizeof current_dir - 1;
	}

	memcpy( stub_path, kernel_path, len );
	stub_path[len] = 0;
	if ((len + sizeof stub_name) > sizeof stub_path)
		die("path too long\n");
	strcat( stub_path, stub_name );
}

// quick check that /proc is mounted
void check_proc()
{
	int fd = open("/proc/self/fd/0", O_RDONLY);
	if (fd < 0)
		die("/proc not mounted\n");
	close( fd );
}

bool init_tt( const char *kernel_path )
{
	get_stub_path( kernel_path );
	check_proc();
	dprintf("using thread tracing, kernel %s, client %s\n", kernel_path, stub_path );
	ptrace_address_space_impl::set_signals();
	pcreate_address_space = &create_tt_address_space;
	return true;
}
