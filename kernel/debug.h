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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

void debugprintf(const char *file, const char *func, int line, const char *fmt, ...) __attribute__((format (printf,4,5)));
void dump_mem(void *p, unsigned int len);
void die(const char *fmt, ...) __attribute__((format (printf,1,2))) __attribute__((noreturn));
int dump_instruction(unsigned char *inst);
void print_wide_string( unsigned short *str, int len );

extern int option_quiet;
extern int option_debug;
void dump_regs(CONTEXT *ctx);

void debugger( void );
void debugger_backtrace(PCONTEXT ctx);

#ifdef __cplusplus
}
#endif

#define kalloc( size ) _kalloc( __FILE__, __LINE__, (size) )
#define kfree( mem ) _kfree( __FILE__, __LINE__, (mem) )

#define dprintf(...) debugprintf(__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)

#endif // _DEBUG_H_
