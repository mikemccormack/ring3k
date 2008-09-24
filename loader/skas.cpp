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

#include "ptrace_base.h"

class skas3_address_space_impl: public ptrace_address_space_impl
{
	static int num_address_spaces;
	int fd;
public:
	static pid_t child_pid;
	skas3_address_space_impl(int _fd);
	virtual pid_t get_child_pid();
	virtual ~skas3_address_space_impl();
	virtual int mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset );
	virtual int munmap( BYTE *address, size_t length );
	virtual void run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec );
};

pid_t skas3_address_space_impl::child_pid = -1;
int skas3_address_space_impl::num_address_spaces;

// target for timer signals
pid_t skas3_address_space_impl::get_child_pid()
{
	return child_pid;
}

int do_fork_child(void *arg)
{
	_exit(1);
}

pid_t create_tracee(void)
{
	const int stack_size = 0x1000;
	void *stack;
	int status;
	pid_t pid;

	stack = mmap_anon( 0, stack_size, PROT_READ | PROT_WRITE );

#ifdef HAVE_CLONE
	pid = clone( do_fork_child, (char*) stack + stack_size,
					   CLONE_FILES | CLONE_VM | CLONE_STOPPED | SIGCHLD, NULL );
#else
	pid = fork();
#endif

	if (pid == -1)
		return pid;
	if (pid == 0)
	{
		// parent will take over here
		::ptrace( PTRACE_TRACEME, 0, 0, 0 );
		kill( getpid(), SIGCHLD );
		// the next line should not be reached
		_exit(1);
	}

	::ptrace( PTRACE_ATTACH, pid, 0, 0 );
	if (0 && pid != wait4( pid, &status, WUNTRACED, NULL ))
		return -1;

	return pid;
}

skas3_address_space_impl::skas3_address_space_impl(int _fd) :
	fd(_fd)
{
	if (num_address_spaces == 0)
		set_itimer_signal();
	num_address_spaces++;
}

void skas3_address_space_impl::run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec )
{
	/* load the process address space into the child process */
	int r = ptrace_set_address_space( child_pid, fd );
	if (r < 0)
		die("ptrace_set_address_space failed %d\n", r);

	ptrace_address_space_impl::run( TebBaseAddress, ctx, single_step, timeout, exec );
}

skas3_address_space_impl::~skas3_address_space_impl()
{
	destroy();
	close( fd );
	assert(num_address_spaces>0);
	num_address_spaces--;
	if (num_address_spaces == 0)
	{
		ptrace( PTRACE_KILL, child_pid, 0, 0 );
		kill( child_pid, SIGTERM );
		child_pid = -1;
	}
}

address_space_impl* create_skas3_address_space()
{
	if (skas3_address_space_impl::child_pid == -1)
	{
		// Set up the signal handler and unmask it first.
		// The child's signal handler will be unmasked too.
		skas3_address_space_impl::child_pid = create_tracee();
	}

	int fd = ptrace_alloc_address_space_fd();
	if (fd < 0)
		return NULL;

	return new skas3_address_space_impl( fd );
}

int skas3_address_space_impl::mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset )
{
	return remote_mmap( fd, address, length, prot, flags, file, offset );
}

int skas3_address_space_impl::munmap( BYTE *address, size_t length )
{
	return remote_munmap( fd, address, length );
}

bool init_skas()
{
	int fd = ptrace_alloc_address_space_fd();
	if (fd < 0)
	{
		dprintf("skas3 patch not present\n");
		return false;
	}
	close( fd );
	dprintf("using skas3\n");
	pcreate_address_space = &create_skas3_address_space;
	return true;
}

