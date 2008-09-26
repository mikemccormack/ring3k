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

#define CTX_HAS_CONTROL(flags) ((flags)&1)
#define CTX_HAS_INTEGER(flags) ((flags)&2)
#define CTX_HAS_SEGMENTS(flags) ((flags)&4)
#define CTX_HAS_FLOAT(flags) ((flags)&8)
#define CTX_HAS_DEBUG(flags) ((flags)&0x10)

#define CTX_HAS_INTEGER_CONTROL_OR_SEGMENTS(flags) ((flags)&7)

int ptrace_address_space_impl::set_context( PCONTEXT ctx )
{
	long regs[FRAME_SIZE];

	memset( regs, 0, sizeof regs );

	regs[EBX] = ctx->Ebx;
	regs[ECX] = ctx->Ecx;
	regs[EDX] = ctx->Edx;
	regs[ESI] = ctx->Esi;
	regs[EDI] = ctx->Edi;
	regs[EAX] = ctx->Eax;

	regs[DS] = ctx->SegDs;
	regs[ES] = ctx->SegEs;
	regs[FS] = ctx->SegFs;
	regs[GS] = ctx->SegGs;
	regs[SS] = ctx->SegSs;

	regs[CS] = ctx->SegCs;
	regs[SS] = ctx->SegSs;
	regs[UESP] = ctx->Esp;
	regs[EIP] = ctx->Eip;
	regs[EBP] = ctx->Ebp;
	regs[EFL] = ctx->EFlags;

	return ptrace_set_regs( get_child_pid(), regs );
}

int ptrace_address_space_impl::get_context( PCONTEXT ctx )
{
	long regs[FRAME_SIZE];
	int r;

	memset( ctx, 0, sizeof *ctx );

	r = ptrace_get_regs( get_child_pid(), regs );
	if (r < 0)
		return r;

	ctx->ContextFlags = CONTEXT_INTEGER | CONTEXT_SEGMENTS | CONTEXT_CONTROL;

	// CONTEXT_INTEGER
	ctx->Ebx = regs[EBX];
	ctx->Ecx = regs[ECX];
	ctx->Edx = regs[EDX];
	ctx->Esi = regs[ESI];
	ctx->Edi = regs[EDI];
	ctx->Eax = regs[EAX];

	// CONTEXT_SEGMENTS
	ctx->SegDs = regs[DS];
	ctx->SegEs = regs[ES];
	ctx->SegFs = regs[FS];
	ctx->SegGs = regs[GS];

	// CONTEXT_CONTROL
	ctx->SegSs = regs[SS];
	ctx->Esp = regs[UESP];
	ctx->SegCs = regs[CS];
	ctx->Eip = regs[EIP];
	ctx->Ebp = regs[EBP];
	ctx->EFlags = regs[EFL];

#if 0
	if (CTX_HAS_FLOAT(flags))
	{
		struct user_i387_struct fpregs;

		r = ptrace_get_fpregs( get_child_pid(), &fpregs );
		if (r < 0)
			return r;

		ctx->ContextFlags |= CONTEXT_FLOATING_POINT;

		ctx->FloatSave.ControlWord = fpregs.cwd;
		ctx->FloatSave.StatusWord = fpregs.swd;
		ctx->FloatSave.TagWord = fpregs.twd;
		//FloatSave. = fpregs.fip;
		//FloatSave. = fpregs.fcs;
		//FloatSave. = fpregs.foo;
		//FloatSave. = fpregs.fos;
		//FloatSave. = fpregs.fip;
		assert( sizeof fpregs.st_space == sizeof ctx->FloatSave.RegisterArea );
		memcpy( ctx->FloatSave.RegisterArea, fpregs.st_space, sizeof fpregs.st_space );
		//ErrorOffset;
		//ErrorSelector;
		//DataOffset;
		//DataSelector;
		//Cr0NpxState;
		dprintf("not complete\n");
	}

	if (flags & ~CONTEXT86_FULL)
		die( "invalid context flags\n");
#endif

	return 0;
}

int ptrace_address_space_impl::ptrace_run( PCONTEXT ctx, int single_step )
{
	pid_t r_pid;
	int r, status = 0;

	sig_target = this;
	/* set the current thread's context */
	r = set_context( ctx );
	if (r<0)
		die("set_thread_context failed\n");

	/* run it */
	r = ptrace( single_step ? PTRACE_SINGLESTEP : PTRACE_CONT, get_child_pid(), 0, 0 );
	if (r<0)
		die("PTRACE_CONT failed (%d)\n", errno);

	/* wait until it needs our attention */
	do {
		r_pid = wait4( get_child_pid(), &status, WUNTRACED, NULL );
	} while (r_pid == -1 && errno == EINTR);
	if (r_pid == -1)
		die("wait failed %d\n", errno);

	r = get_context( ctx );
	if (r < 0)
		die("failed to get registers\n");
	sig_target = 0;

	return status;
}

void ptrace_address_space_impl::alarm_timeout(LARGE_INTEGER &timeout)
{
	/* set the timeout */
	struct itimerval val;
	val.it_value.tv_sec = timeout.QuadPart/1000LL;
	val.it_value.tv_usec = timeout.QuadPart%1000LL;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	int r = setitimer(ITIMER_REAL, &val, NULL);
	if (r < 0)
		die("couldn't set itimer\n");
}

void ptrace_address_space_impl::cancel_timer()
{
	struct itimerval val;
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	int r = setitimer(ITIMER_REAL, &val, NULL);
	if (r < 0)
		die("couldn't cancel itimer\n");
}

ptrace_address_space_impl* ptrace_address_space_impl::sig_target;

void ptrace_address_space_impl::sigitimer_handler(int signal)
{
	if (sig_target)
		sig_target->handle( signal );
}

void ptrace_address_space_impl::handle( int signal )
{
	//dprintf("signal %d\n", signal);
	pid_t pid = get_child_pid();
#ifdef HAVE_SIGQUEUE
	sigval val;
	val.sival_int = 0;
	sigqueue(pid, SIGALRM, val);
#else
	kill(pid, SIGALRM);
#endif
}

void ptrace_address_space_impl::set_itimer_signal()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = ptrace_address_space_impl::sigitimer_handler;
	sigemptyset(&sa.sa_mask);

	if (0 > sigaction(SIGALRM, &sa, NULL))
		die("unable to set action for SIGALRM\n");

	// turn the signal on
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	if (0 > sigprocmask(SIG_UNBLOCK, &sigset, NULL))
		die("unable to unblock SIGALRM\n");
}

void ptrace_address_space_impl::run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec )
{
	set_userspace_fs(TebBaseAddress, ctx->SegFs);

	alarm_timeout( timeout );

	while (1)
	{
		int status = ptrace_run( ctx, 0 );
		if (WIFSIGNALED(status))
			break;

		if (WIFEXITED(status))
			break;

		if (WIFSTOPPED(status) && WEXITSTATUS(status) == SIGSEGV)
		{
			exec->handle_fault();
			break;
		}

		if (WIFSTOPPED(status) && WEXITSTATUS(status) == SIGSTOP)
			break;

		if (WIFSTOPPED(status) && WEXITSTATUS(status) == SIGCONT)
		{
			dprintf("got SIGCONT\n");
			continue;
		}

		if (WIFSTOPPED(status) && WEXITSTATUS(status) == SIGALRM)
			break;

		if (WIFSTOPPED(status))
		{
			exec->handle_breakpoint();
			break;
		}
	}

	cancel_timer();
}

unsigned short ptrace_address_space_impl::get_userspace_code_seg()
{
	unsigned short cs;
	__asm__ __volatile__ ( "\n\tmovw %%cs, %0\n" : "=r"( cs ) : );
	return cs;
}

unsigned short ptrace_address_space_impl::get_userspace_data_seg()
{
	unsigned short cs;
	__asm__ __volatile__ ( "\n\tmovw %%ds, %0\n" : "=r"( cs ) : );
	return cs;
}

void ptrace_address_space_impl::init_context( CONTEXT& ctx )
{
	memset( &ctx, 0, sizeof ctx );
	ctx.SegFs = get_userspace_fs();
	ctx.SegDs = get_userspace_data_seg();
	ctx.SegEs = get_userspace_data_seg();
	ctx.SegSs = get_userspace_data_seg();
	ctx.SegCs = get_userspace_code_seg();
	ctx.EFlags = 0x00000296;
}

int ptrace_address_space_impl::set_userspace_fs(void *TebBaseAddress, ULONG fs)
{
	struct user_desc ldt;
	int r;

	memset( &ldt, 0, sizeof ldt );
	ldt.entry_number = (fs >> 3);
	ldt.base_addr = (unsigned long) TebBaseAddress;
	ldt.limit = 0xfff;
	ldt.seg_32bit = 1;

	r = ptrace_set_thread_area( get_child_pid(), &ldt );
	if (r<0)
		die("set %%fs failed, fs = %ld errno = %d child = %d\n", fs, errno, get_child_pid());
	return r;
}
