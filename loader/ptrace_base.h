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

#ifndef __NTNATIVE_PTRACE_BASE_H
#define __NTNATIVE_PTRACE_BASE_H

#include "config.h"

class ptrace_address_space_impl: public address_space_impl
{
protected:
	static ptrace_address_space_impl *sig_target;
	static void cancel_timer();
	static void sigitimer_handler(int signal);
	int get_context( PCONTEXT ctx );
	int set_context( PCONTEXT ctx );
	int ptrace_run( PCONTEXT ctx, int single_step );
	virtual pid_t get_child_pid() = 0;
	virtual void handle( int signal );
	virtual void run( void *TebBaseAddress, PCONTEXT ctx, int single_step, LARGE_INTEGER& timeout, execution_context_t *exec );
	virtual void alarm_timeout(LARGE_INTEGER& timeout);
	virtual int set_userspace_fs(void *TebBaseAddress, ULONG fs);
	virtual void init_context( CONTEXT& ctx );
	virtual unsigned short get_userspace_fs() = 0;
	virtual unsigned short get_userspace_data_seg();
	virtual unsigned short get_userspace_code_seg();
	virtual int get_fault_info( void *& addr );
	static void wait_for_signal( pid_t pid, int signal );
public:
	static void set_signals();
};


#endif // __NTNATIVE_PTRACE_BASE_H
