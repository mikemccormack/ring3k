/*
 * fiber implementation
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

// Fibers implement an infrastructure for cooperative multitasking
//  by allow multiple execution contexts in a single thread.
// Being cooperative, they must be switched explicitly.
//
// Lots of gcc specific code here.
// Tested on Linux.  Windows has a native fiber implementation.

#ifndef __FIBER_H__
#define __FIBER_H__

class fiber_t {
	struct fiber_stack_t {
		long ebp;
		long esi;
		long edi;
		long edx;
		long ecx;
		long ebx;
		long eax;
		long eip;
	};

private:				// offset 0 = vtable pointer
	fiber_stack_t *stack_pointer;	// offset 4
	fiber_t *next;			// offset 8
	fiber_t *prev;
	void *stack;
	unsigned int stack_size;

private:
	void remove_from_runlist();
	void add_to_runlist();
	fiber_t();
	static int run_fiber( fiber_t* fiber );

public:
	static const unsigned int fiber_default_stack_size = 0x10000;
	static const unsigned int guard_size = 0x1000;

public:
	static void fibers_init();
	static void fibers_finish();
	fiber_t( unsigned int size );
	virtual ~fiber_t();
	static void yield();
	static bool last_fiber();
	void start();
	void stop();
	virtual int run();
};

#endif // __FIBER_H__
