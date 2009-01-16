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

#ifndef _PTRACE_IF_H
#define _PTRACE_IF_H

#include <unistd.h>
#include <signal.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* from linux-2.6.16.11/arch/i386/kernel/ldt.c */
struct user_desc {
	unsigned int  entry_number;
	unsigned long base_addr;
	unsigned int  limit;
	unsigned int  seg_32bit:1;
	unsigned int  contents:2;
	unsigned int  read_exec_only:1;
	unsigned int  limit_in_pages:1;
	unsigned int  seg_not_present:1;
	unsigned int  useable:1;
};

struct user_i387_struct {
		long	cwd;
		long	swd;
		long	twd;
		long	fip;
		long	fcs;
		long	foo;
		long	fos;
		long	st_space[20];   /* 8*10 bytes for each FP-reg = 80 bytes */
};

/**
 * Taken from arch/um/kernel/skas/include/proc_mm.h
 */
struct ptrace_faultinfo {
	int is_write;
	unsigned long addr;
};


/* from linux 2.6.17.7  include/asm-x86_64/ptrace.h */
struct ptrace_ex_faultinfo {
	int is_write;
	unsigned long addr;
	int trap_no;
};

struct ptrace_ldt {
	int func;
	void *ptr;
	unsigned long bytecount;
};

#define SET_USER_LDT 0x11

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

#define PTRACE_GET_THREAD_AREA 25
#define PTRACE_SET_THREAD_AREA 26
#define PTRACE_ARCH_PRCTL	  30
#define PTRACE_FAULTINFO	   52
#define PTRACE_LDT			 54
#define PTRACE_SWITCH_MM	   55
#define PTRACE_EX_FAULTINFO	56

#ifndef PTRACE_GETSIGINFO
#define PTRACE_GETSIGINFO	   0x4202
#endif

#ifndef PTRACE_SETSIGINFO
#define PTRACE_SETSIGINFO	   0x4203
#endif

#ifndef CLONE_STOPPED
#define CLONE_STOPPED 0x02000000
#endif

/**
 * Taken from arch/um/kernel/skas/include/proc_mm.h
 */
#define MM_MMAP 54
#define MM_MUNMAP 55
#define MM_MPROTECT 56
#define MM_COPY_SEGMENTS 57

struct mm_mmap {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long offset;
};

struct mm_munmap {
	unsigned long addr;
	unsigned long len;
};

struct mm_mprotect {
	unsigned long addr;
	unsigned long len;
	unsigned int prot;
};

struct proc_mm_op {
	int op;
	union {
		struct mm_mmap mmap;
		struct mm_munmap munmap;
		struct mm_mprotect mprotect;
		int copy_segments;
	} u;
};

int remote_mmap( int proc_fd, void *start, size_t length,
				 int prot, int flags, int fd, off_t offset);
int remote_munmap( int proc_fd, void *start, size_t length );
int remote_mprotect( int proc_fd, void *start, size_t length, int prot );
int ptrace_set_user_ldt( pid_t pid, struct user_desc *ldt );
int ptrace_set_thread_area( pid_t pid, struct user_desc *ldt );
int ptrace_get_thread_area( pid_t pid, struct user_desc *ldt );
int ptrace_arch_prctl( pid_t pid, void *address );
int ptrace_set_regs( pid_t pid, long *regs );
int ptrace_get_regs( pid_t pid, long *regs );
int ptrace_set_fpregs( pid_t pid, struct user_i387_struct *fpregs );
int ptrace_get_fpregs( pid_t pid, struct user_i387_struct *fpregs );
int ptrace_get_exception_info( pid_t pid, struct ptrace_ex_faultinfo *info );
int ptrace_get_signal_info( pid_t pid, siginfo_t *info);
int ptrace_set_address_space( pid_t pid, int fd );
int ptrace_alloc_address_space_fd( void );

#ifdef __cplusplus
}
#endif

#endif  // _PTRACE_IF_H
