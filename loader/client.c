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
#include <stdarg.h>

#include "client.h"

/* refer to wine's loader/preloader.c for explanation of following */

/* required by -fprofile-arcs -ftest-coverage */
void __bb_init_func(void) { return; }

/* required by -fstack-protector */
void *__stack_chk_guard = 0;
void __stack_chk_fail(void) { return; }

static int sys_open( const char *path, int flags )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_open), "b" (path), "c" (flags) );
	return r;
}

static int sys_write( int fd, const void *buf, size_t count )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_write), "b" (fd), "c" (buf), "d"(count) );
	return r;
}

static int sys_close( int fd )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_close), "b" (fd) );
	return r;
}

static int sys_exit( int ret )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_exit), "b" (ret) );
	return r;
}

static void *sys_mmap( void *start, size_t len, int prot, int flags, int fd, off_t offset )
{
    void *r;

    struct
    {
        void        *addr;
        unsigned int length;
        unsigned int prot;
        unsigned int flags;
        unsigned int fd;
        unsigned int offset;
    } args;

    args.addr   = start;
    args.length = len;
    args.prot   = prot;
    args.flags  = flags;
    args.fd     = fd;
    args.offset = offset;
    __asm__ __volatile__( "pushl %%ebx; movl %2,%%ebx; int $0x80; popl %%ebx"
                          : "=a" (r) : "0" (SYS_mmap), "q" (&args) : "memory" );
    return r;
}

static int sys_munmap( void *start, size_t length )
{
	int r;
	__asm__ __volatile__(
		"\tint $0x80\n"
	: "=a" (r) : "a" (SYS_munmap), "b" (start), "c"(length) );
	return r;
}

static int sys_mprotect( const void *start, size_t len, int prot )
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

/* from wine/loader/preloader.c */
/*
 * wld_printf - just the basics
 *
 *  %x prints a hex number
 *  %s prints a string
 *  %p prints a pointer
 */
static int wld_vsprintf(char *buffer, const char *fmt, va_list args )
{
    static const char hex_chars[16] = "0123456789abcdef";
    const char *p = fmt;
    char *str = buffer;
    int i;

    while( *p )
    {
        if( *p == '%' )
        {
            p++;
            if( *p == 'x' )
            {
                unsigned int x = va_arg( args, unsigned int );
                for(i=7; i>=0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
            }
            else if (p[0] == 'l' && p[1] == 'x')
            {
                unsigned long x = va_arg( args, unsigned long );
                for(i=7; i>=0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
                p++;
            }
            else if( *p == 'p' )
            {
                unsigned long x = (unsigned long)va_arg( args, void * );
                for(i=7; i>=0; i--)
                    *str++ = hex_chars[(x>>(i*4))&0xf];
            }
            else if( *p == 's' )
            {
                char *s = va_arg( args, char * );
                while(*s)
                    *str++ = *s++;
            }
            else if( *p == 0 )
                break;
            p++;
        }
        *str++ = *p++;
    }
    *str = 0;
    return str - buffer;
}

static __attribute__((format(printf,1,2))) void dprintf(const char *fmt, ... )
{
    va_list args;
    char buffer[256];
    int len;

    va_start( args, fmt );
    len = wld_vsprintf(buffer, fmt, args );
    va_end( args );
    sys_write(2, buffer, len);
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

static int do_mmap( struct tt_req_map *req )
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

static int do_umap( struct tt_req_umap *req )
{
	return sys_munmap( (void*) req->addr, req->len );
}

static int do_prot( struct tt_req_prot *req )
{
	return sys_mprotect( (void*) req->addr, req->len, req->prot );
}

int _start( void )
{
	struct tt_req req;
	int r = 0, finished = 0;

	init_fs();

	while (!finished)
	{
		// throw an exception
		// the trace client will trap and fill req
		__asm__ __volatile__(
                          "int $3\n\t"
                          : : "a"(r), "b"(&req) : "memory" );

		switch (req.type)
		{
		case tt_req_map:
			r = do_mmap( &req.u.map );
			break;
		case tt_req_umap:
			r = do_umap( &req.u.umap );
			break;
		case tt_req_prot:
			r = do_prot( &req.u.prot );
			break;
		case tt_req_exit:
			r = 0;
			finished = 1;
		default:
			dprintf("protocol error\n");
			sys_exit(1);
		}

		// exit on the next iteration if something goes wrong
		req.type = tt_req_exit;
	}

	dprintf("exit!\n");
	sys_exit(1);
	return 0;
}
