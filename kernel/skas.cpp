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
	static unsigned short user_fs;
public:
	static pid_t child_pid;
	skas3_address_space_impl(int _fd);
	virtual pid_t get_child_pid();
	virtual ~skas3_address_space_impl();
	virtual int mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset );
	virtual int munmap( BYTE *address, size_t length );
	virtual void run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec );
	static pid_t create_tracee(void);
	static void init_fs(void);
	virtual unsigned short get_userspace_fs();
};

pid_t skas3_address_space_impl::child_pid = -1;
int skas3_address_space_impl::num_address_spaces;
unsigned short skas3_address_space_impl::user_fs;

// target for timer signals
pid_t skas3_address_space_impl::get_child_pid()
{
	return child_pid;
}

int do_fork_child(void *arg)
{
	_exit(1);
}

/* from Wine */
struct modify_ldt_s
{
    unsigned int  entry_number;
    unsigned long base_addr;
    unsigned int  limit;
    unsigned int  seg_32bit : 1;
    unsigned int  contents : 2;
    unsigned int  read_exec_only : 1;
    unsigned int  limit_in_pages : 1;
    unsigned int  seg_not_present : 1;
    unsigned int  usable : 1;
    unsigned int  garbage : 25;
};

static inline int set_thread_area( struct modify_ldt_s *ptr )
{
    int res;
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %3,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (res), "=m" (*ptr)
                          : "0" (243) /* SYS_set_thread_area */, "q" (ptr), "m" (*ptr) );
    if (res >= 0) return res;
    errno = -res;
    return -1;
}

// allocate fs in the current process
void skas3_address_space_impl::init_fs(void)
{
	unsigned short fs;
	__asm__ __volatile__ ( "\n\tmovw %%fs, %0\n" : "=r"( fs ) : );
	if (fs != 0)
	{
		user_fs = fs;
		return;
	}

	struct modify_ldt_s ldt;
	memset( &ldt, 0, sizeof ldt );
	ldt.entry_number = -1;
	int r = set_thread_area( &ldt );
	if (r<0)
		die("alloc %%fs failed, errno = %d\n", errno);
	user_fs = (ldt.entry_number << 3) | 3;
}

unsigned short skas3_address_space_impl::get_userspace_fs()
{
	return user_fs;
}

pid_t skas3_address_space_impl::create_tracee(void)
{
	pid_t pid;

	// init fs before forking
	init_fs();

	// clone this process
	const int stack_size = 0x1000;
	void *stack = mmap_anon( 0, stack_size, PROT_READ | PROT_WRITE );
	pid = clone( do_fork_child, (char*) stack + stack_size,
		CLONE_FILES | CLONE_STOPPED | SIGCHLD, NULL );
	if (pid == -1)
	{
		dprintf("clone failed (%d)\n", errno);
		return pid;
	}
	if (pid == 0)
	{
		// using CLONE_STOPPED we should never get here
		die("CLONE_STOPPED\n");
	}

	int r = ::ptrace( PTRACE_ATTACH, pid, 0, 0 );
	if (r < 0)
	{
		dprintf("ptrace_attach failed (%d)\n", errno);
		return -1;
	}

	return pid;
}

skas3_address_space_impl::skas3_address_space_impl(int _fd) :
	fd(_fd)
{
	num_address_spaces++;
}

void skas3_address_space_impl::run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec )
{
	/* load the process address space into the child process */
	int r = ptrace_set_address_space( child_pid, fd );
	if (r < 0)
		die("ptrace_set_address_space failed %d (%d)\n", r, errno);

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
		skas3_address_space_impl::child_pid = skas3_address_space_impl::create_tracee();
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
	ptrace_address_space_impl::set_signals();
	pcreate_address_space = &create_skas3_address_space;
	return true;
}

