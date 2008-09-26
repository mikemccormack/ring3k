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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>

#include "client.h"

int sys_open( const char *path, int flags )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_open), "b" (path), "c" (flags) );
}

int sys_read( int fd, void *buf, size_t count )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_read), "b" (fd), "c" (buf), "d"(count) );
	return r;
}

int sys_write( int fd, const void *buf, size_t count )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_write), "b" (fd), "c" (buf), "d"(count) );
	return r;
}

int sys_close( int fd )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_close), "b" (fd) );
	return r;
}

void* sys_mmap( void *start, size_t length, int prot, int flags, int fd, off_t offset )
{
	void* r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_mmap), "b" (&start) );
	return r;
}

int sys_munmap( void *start, size_t length )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_munmap), "b" (start), "c"(length) );
	return r;
}

int sys_mprotect( const void *start, size_t len, int prot )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_mprotect), "b" (start), "c"(len), "d"(prot) );
	return r;
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
    return res;
}

void dprintf(const char *string)
{
	int n = 0;
	while (string[n])
		n++;
	sys_write( 2, string, n );
}

// allocate fs in the current process
void init_fs(void)
{
	unsigned short fs;

	// check is somebody set fs already
	__asm__ __volatile__ ( "\n\tmovw %%fs, %0\n" : "=r"( fs ) : );
	if (fs != 0)
		return;

	// allocate fs
	struct modify_ldt_s ldt;
	ldt.entry_number = 0;
	ldt.base_addr = 0;
	ldt.limit = 0;
	ldt.seg_32bit = 0;
	ldt.contents = 0;
	ldt.read_exec_only = 0;
	ldt.limit_in_pages = 0;
	ldt.seg_not_present = 0;
	ldt.usable = 0;
	ldt.garbage = 0;
	ldt.entry_number = -1;

	int r = set_thread_area( &ldt );
	if (r<0)
	{
		dprintf("failed to allocate an ldt\n");
		return;
	}

	// set fs to the selector we allocated
	fs = (ldt.entry_number << 3) | 3;
	__asm__ __volatile__ ( "\n\tmovw %0, %%fs\n" : : "r"( fs ) );
}

char *append_string( char *target, const char *source )
{
	while ((*target = *source))
		source++, target++;
	return target;
}

char *append_number( char *target, int num )
{
	int n = 0, i = 0;
	// write out the number backwards
	do {
		target[n++] = (num%10) + '0';
		num /= 10;
	} while (num);
	target[n] = 0;
	// reverse it
	while ((n-1) > i)
	{
		// swap
		char x = target[--n];
		target[n] = target[i];
		target[i++] = x;
	}
	return target + i + n;
}

int do_mmap( struct tt_req_map *req )
{
	char str[32], *s;
	void *p;
	int r, fd;

	//sprintf( str, "/proc/%d/fd/%d", req->pid, req->fd );
	s = append_string( str, "/proc/" );
	s = append_number( s, req->pid );
	s = append_string( s, "/fd/" );
	s = append_number( s, req->fd );

	fd = sys_open( str, (req->prot & PROT_WRITE) ? O_RDWR : O_RDONLY );
	if (fd < 0)
	{
		dprintf("sys_open failed\n");
		return fd;
	}
	p = sys_mmap( (void*) req->addr, req->len, req->prot, MAP_SHARED | MAP_FIXED, fd, req->ofs );
	r = (p == (void*) req->addr) ? 0 : -1;
	sys_close( fd );
	return r;
}

int do_umap( struct tt_req_umap *req )
{
	return sys_munmap( (void*) req->addr, req->len );
}

int do_prot( struct tt_req_prot *req )
{
	return sys_mprotect( (void*) req->addr, req->len, req->prot );
}

int _start( void )
{
	struct tt_req req;
	struct tt_reply reply;
	int r, finished = 0;

	init_fs();

	// write a single character to synchronize
	do {
		char ch = '.';
		r = sys_write(1, &ch, sizeof ch);
	} while (r == -EINTR);

	while (!finished)
	{
		do {
			r = sys_read( 0, &req, sizeof req );
		} while (r == -EINTR);
		if (r != sizeof req)
		{
			dprintf("sys_read failed\n");
			break;
		}

		switch (req.type)
		{
		case tt_req_map:
			reply.r = do_mmap( &req.u.map );
			break;
		case tt_req_umap:
			reply.r = do_umap( &req.u.umap );
			break;
		case tt_req_prot:
			reply.r = do_prot( &req.u.prot );
			break;
		case tt_req_exit:
			reply.r = 0;
			finished = 1;
		}

		do {
			r = sys_write(1, &reply, sizeof reply);
		} while (r == -EINTR);
		if (r != sizeof reply)
		{
			dprintf("write failed\n");
			break;
		}
	}

	dprintf("exit!\n");

	return !finished;
}
