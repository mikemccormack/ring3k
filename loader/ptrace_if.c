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

#include <sys/ptrace.h>
#ifdef HAVE_ASM_PTRACE_H
#include <asm/ptrace.h>
#endif

#include "ptrace_if.h"

int remote_mmap( int proc_fd, void *start, size_t length,
				 int prot, int flags, int fd, off_t offset)
{
	struct proc_mm_op msg;

	msg.op = MM_MMAP;
	msg.u.mmap.addr = (unsigned int) start;
	msg.u.mmap.len = length;
	msg.u.mmap.prot = prot;
	msg.u.mmap.flags = flags;
	msg.u.mmap.fd = fd;
	msg.u.mmap.offset = offset;

	return write( proc_fd, &msg, sizeof msg );
}

int remote_munmap( int proc_fd, void *start, size_t length )
{
	struct proc_mm_op msg;

	msg.op = MM_MUNMAP;
	msg.u.munmap.addr = (unsigned int) start;
	msg.u.munmap.len = length;
	return write( proc_fd, &msg, sizeof msg );
};

int remote_mprotect( int proc_fd, void *start, size_t length, int prot )
{
	struct proc_mm_op msg;

	msg.op = MM_MPROTECT;
	msg.u.mprotect.addr = (unsigned int) start;
	msg.u.mprotect.len = length;
	msg.u.mprotect.prot = prot;

	return write( proc_fd, &msg, sizeof msg );
}

int ptrace_set_user_ldt( pid_t pid, struct user_desc *ldt )
{
	struct ptrace_ldt pl;

	pl.func = SET_USER_LDT;
	pl.ptr = ldt;
	pl.bytecount = sizeof *ldt;

	return ptrace( PTRACE_LDT, pid, 0, &pl );
}

int ptrace_set_thread_area( pid_t pid, struct user_desc *ldt )
{
	return ptrace( PTRACE_SET_THREAD_AREA, pid, ldt->entry_number, ldt );
}

int ptrace_get_thread_area( pid_t pid, struct user_desc *ldt )
{
	return ptrace( PTRACE_GET_THREAD_AREA, pid, ldt->entry_number, ldt );
}

int ptrace_arch_prctl( pid_t pid, void *address )
{
	return ptrace( PTRACE_ARCH_PRCTL, pid, ARCH_SET_FS, address );
}

int ptrace_set_regs( pid_t pid, long *regs )
{
	return ptrace( PTRACE_SETREGS, pid, 0, regs );
}

int ptrace_set_fpregs( pid_t pid, struct user_i387_struct *fpregs )
{
	return ptrace( PTRACE_SETFPREGS, pid, 0, fpregs );
}

int ptrace_get_regs( pid_t pid, long *regs )
{
	return ptrace( PTRACE_GETREGS, pid, 0, regs );
}

int ptrace_get_fpregs( pid_t pid, struct user_i387_struct *fpregs )
{
	return ptrace( PTRACE_GETFPREGS, pid, 0, fpregs );
}

int ptrace_get_exception_info( pid_t pid, struct ptrace_ex_faultinfo *info )
{
	return ptrace( PTRACE_EX_FAULTINFO, pid, 0, info );
}

int ptrace_get_signal_info( pid_t pid, siginfo_t *info)
{
	return ptrace( PTRACE_GETSIGINFO, pid, 0, info );
}

int ptrace_set_address_space( pid_t pid, int fd )
{
	return ptrace( PTRACE_SWITCH_MM, pid, 0, fd );
}

int ptrace_alloc_address_space_fd( void )
{
	return open( "/proc/mm", O_WRONLY );
}
