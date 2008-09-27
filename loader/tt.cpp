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

class tt_address_space_impl: public ptrace_address_space_impl
{
	enum stub_state {
		stub_running,
		stub_suspended,
	};
	long stub_regs[FRAME_SIZE];
	char buffer[64]; // same size as tt stub's buffer
	stub_state state;
	int in_fd;
	int out_fd;
	pid_t child_pid;
protected:
	//int ptrace_run( PCONTEXT ctx, int single_step );
	int readwrite( struct tt_req *req );
	void run_stub();
	void suspend();
public:
	static void wait_for_signal( pid_t pid, int signal );
	static pid_t create_tracee( int& in_fd, int& out_fd );
	tt_address_space_impl( pid_t pid, int in_fd, int out_fd );
	virtual pid_t get_child_pid();
	virtual ~tt_address_space_impl();
	virtual int mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset );
	virtual int munmap( BYTE *address, size_t length );
	virtual void run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec );
	virtual unsigned short get_userspace_fs();
};

pid_t tt_address_space_impl::get_child_pid()
{
	return child_pid;
}

void tt_address_space_impl::wait_for_signal( pid_t pid, int signal )
{
	while (1)
	{
		int r, status = 0;
		r = wait4( pid, &status, WUNTRACED, NULL );
		if (r < 0)
			die("wait_for_signal: wait4() failed %d\n", errno);
		if (r != pid)
			continue;
		if (WIFEXITED(status) )
			die("Client died\n");

		if (WIFSTOPPED(status) && WEXITSTATUS(status) == signal)
			return;

		dprintf("stray signal %d\n", WEXITSTATUS(status));

		// start the child again so we can get the next signal
		ptrace( PTRACE_CONT, pid, 0, 0 );
	}
}

pid_t tt_address_space_impl::create_tracee( int& in_fd, int& out_fd )
{
	int r;
	pid_t pid;
	int infds[2], outfds[2];

	r = pipe( infds );
	if (r != 0)
		return -1;

	r = pipe( outfds );
	if (r != 0)
		return -1;

	pid = fork();
	if (pid == -1)
	{
		dprintf("fork() failed %d\n", errno);
		return pid;
	}
	if (pid == 0)
	{
		// close stdin and stdout
		close(0);
		close(1);

		dup2( infds[0], STDIN_FILENO );
		close( infds[1] );

		dup2( outfds[1], STDOUT_FILENO );
		close( outfds[0] );

		::ptrace( PTRACE_TRACEME, 0, 0, 0 );
		r = ::execl("./ttclient", "./ttclient", NULL );
		// the next line should not be reached
		die("exec failed %d\n", r);
	}

	close( infds[0] );
	close( outfds[1] );
	in_fd = infds[1];
	out_fd = outfds[0];

	wait_for_signal( pid, SIGTRAP );

	r = ::ptrace( PTRACE_CONT, pid, 0, 0 );

	// read a single character from the stub to synchronize
	do {
		char dummy;
		r = read(out_fd, &dummy, 1);
	} while (r == -1 && errno == EINTR);

	//fprintf(stderr, "created process %d\n", pid );

	return pid;
}

tt_address_space_impl::tt_address_space_impl(pid_t pid, int in, int out) :
	state(stub_running),
	in_fd(in),
	out_fd(out),
	child_pid(pid)
{
	//dprintf("tt_address_space_impl()\n");
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
	int in_fd = -1, out_fd = -1;
	pid_t pid = tt_address_space_impl::create_tracee( in_fd, out_fd );
	if (pid < 0)
		return 0;

	return new tt_address_space_impl( pid, in_fd, out_fd );
}

int tt_address_space_impl::readwrite( struct tt_req *req )
{
	int r;

	// getting a signal here can deadlock
	assert( sig_target == 0 );

	// send the buffer to the stub
	do {
		r = write(in_fd, req, sizeof *req);
	} while (r == -1 && errno == EINTR);
	if (r != sizeof *req)
	{
		fprintf(stderr, "write failed %d\n", r);
		return r;
	}

	// read the stub's response
	struct tt_reply reply;
	do {
		r = read(out_fd, &reply, sizeof reply);
	} while (r == -1 && errno == EINTR);
	if (r != sizeof reply)
	{
		fprintf(stderr, "read failed\n");
		return r;
	}

	return reply.r;
}

int tt_address_space_impl::mmap( BYTE *address, size_t length, int prot, int flags, int file, off_t offset )
{
	//dprintf("tt_address_space_impl::mmap()\n");
	run_stub();

	struct tt_req req;
	struct tt_req_map &map = req.u.map;

	req.type = tt_req_map;
	map.pid = getpid();
	map.fd = file;
	map.addr = (int) address;
	map.len = length;
	map.ofs = offset;
	map.prot = prot;
	// send our pid to the stub
	//dprintf("tt_address_space_impl::mmap()\n");
	return readwrite( &req );
}

int tt_address_space_impl::munmap( BYTE *address, size_t length )
{
	//dprintf("tt_address_space_impl::munmap()\n");
	run_stub();
	struct tt_req req;
	struct tt_req_umap &umap = req.u.umap;

	req.type = tt_req_umap;
	umap.addr = (int) address;
	umap.len = length;

	return readwrite( &req );
}

void tt_address_space_impl::run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec )
{
	//dprintf("tt_address_space_impl::run()\n");

	suspend();
	ptrace_address_space_impl::run( TebBaseAddress, ctx, single_step, timeout, exec );
}

unsigned short tt_address_space_impl::get_userspace_fs()
{
	suspend();
	return stub_regs[FS];
}

void tt_address_space_impl::suspend()
{
	if (state == stub_suspended)
		return;

	// no signals!
	assert( sig_target == 0 );

	assert( child_pid != -1 );
	kill( child_pid, SIGSTOP );

	wait_for_signal( child_pid, SIGSTOP );

	if (state == stub_running)
	{
		//dprintf("saving registers\n");
		int r = ptrace_get_regs( child_pid, stub_regs );
		if (r < 0)
			die("failed to save stub registers\n");
	}

	state = stub_suspended;
}

void tt_address_space_impl::run_stub()
{
	int r;
	suspend();
	r = ptrace_set_regs( child_pid, stub_regs );
	if (r < 0)
		die("ptrace_set_regs failed\n");
	r = ::ptrace( PTRACE_CONT, child_pid, 0, 0 );
	if (r < 0)
		die("ptrace( PTRACE_CONT ) failed\n");
	state = stub_running;
}

bool init_tt()
{
	dprintf("using tt\n");
	ptrace_address_space_impl::set_signals();
	pcreate_address_space = &create_tt_address_space;
	return true;
}
