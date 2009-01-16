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


#include <unistd.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "thread.h"
#include "debug.h"

#include "types.h"
#include "extern.h"

char printable( WCHAR x )
{
	if (x>=0x20 && x<0x7f)
		return x;
	return '.';
}

int sprint_wide_string( char *output, int len, unsigned short *str )
{
	int n;
	for (n=0; n<len && str[n]; n++)
		*output++ = printable( str[n] );
	*output++ = 0;
	return n;
}

int sprint_unicode_string( char *output, int len, UNICODE_STRING *us )
{
	int n;

	if (!us || !us->Buffer)
		return snprintf(output, len, "(null us)");

	for (n=0; n<len && n<us->Length/2; n++)
		*output++ = printable( us->Buffer[n] );
	*output++ = 0;

	return n;
}

void debugprintf(const char *file, const char *func, int line, const char *fmt, ...)
{
	char buffer[0x100], fstr[16], *p;
	int sz, n, i, is_longlong;
	va_list va;

	if (!option_trace)
		return;

	va_start( va, fmt );

	p = buffer;
	sz = sizeof buffer - 1;

	while (*fmt && sz>0)
	{
		if (fmt[0] != '%')
		{
			*p++ = *fmt++;
			sz--;
			continue;
		}

		if (fmt[0] == '%' && fmt[1] == '%')
		{
			*p++ = *fmt;
			fmt += 2;
			sz--;
			continue;
		}

		i = 0;
		fstr[i++] = *fmt++;

		if (*fmt == '-')
			fstr[i++] = *fmt++;

		while (*fmt >= '0' && *fmt <= '9')
			fstr[i++] = *fmt++;

		is_longlong = 0;
		if (*fmt == 'l')
			fstr[i++] = *fmt++;
		if (*fmt == 'l')
		{
			fstr[i++] = *fmt++;
			is_longlong = 1;
		}

		fstr[i++] = *fmt;
		fstr[i++] = 0;

		n = 0;
		switch (*fmt)
		{
		case 'p':
			if (fmt[1] == 'w' && fmt[2] == 's')
			{
				n = sprint_wide_string( p, sz, va_arg(va, unsigned short * ) );
				fmt += 2;
			}
			else if (fmt[1] == 'u' && fmt[2] == 's')
			{
				n = sprint_unicode_string( p, sz, va_arg(va, UNICODE_STRING* ) );
				fmt += 2;
			}
			else
				n = snprintf( p, sz, "%p", va_arg( va, void* ) );
			break;
		case 'x':
			if (is_longlong)
				n = snprintf( p, sz, fstr, va_arg( va, long long ) );
			else
				n = snprintf( p, sz, fstr, va_arg( va, int ) );
			break;
		case 'd':
		case 'u':
		case 'o':
			if (is_longlong)
				n = snprintf( p, sz, fstr, va_arg( va, long long ) );
			else
				n = snprintf( p, sz, fstr, va_arg( va, int ) );
			break;
		case 's':
			n = snprintf( p, sz, fstr, va_arg( va, char * ) );
			break;
		case 'S':
			n = sprint_wide_string( p, sz, va_arg(va, unsigned short * ) );
			break;
		case 'c':
			n = snprintf( p, sz, fstr, va_arg( va, int ) );
			break;
		case 0:
			break;
		default:
			n = snprintf( p, sz, "(?%c=%x)", *fmt, va_arg( va, unsigned int ) );
			break;
		}
		if (n<0)
			break;
		fmt++;
		p += n;
		sz -= n;
	}
	*p = 0;
	va_end( va );

	fprintf( stderr, "%s %s", func, buffer );
}

void dump_mem(void *p, unsigned int len)
{
	unsigned char *x = (unsigned char*) p;
	unsigned int i;
	char line[0x11];

	fprintf(stderr,"address %p\n", p);
	for(i=0; i<len; i++)
	{
		if (i%0x10 == 0)
		{
			memset( line, ' ', sizeof line );
			line[0x10] = 0;
		}
		line[i%0x10] = printable(x[i]);
		fprintf(stderr,"%02x ", x[i] );
		if ((i+1)%0x10 == 0 || (i+1) == len)
			fprintf(stderr, "%*s\n", ((15 - i)%16)*3 + 20, line);
	}
}

//extern pid_t child_pid;

void die(const char *fmt, ...)
{
	//char maps[0x30];
	va_list va;

	va_start( va, fmt );
	vfprintf(stderr, fmt, va);
	va_end( va );
	//sprintf(maps, "cat /proc/%d/maps", child_pid);
	//system(maps);
	exit(1);
}

void do_dump_regs(CONTEXT *ctx)
{
	fprintf(stderr, "eax %08lx ebx %08lx ecx %08lx edx %08lx\n"
			"esi %08lx edi %08lx ebp %08lx efl %08lx\n"
			"cs:eip %04lx:%08lx ss:esp %04lx:%08lx\n"
			"ds %04lx es %04lx fs %04lx gs %04lx\n",
			ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx,
			ctx->Esi, ctx->Edi, ctx->Ebp, ctx->EFlags,
			ctx->SegCs, ctx->Eip, ctx->SegSs, ctx->Esp,
			ctx->SegDs, ctx->SegEs, ctx->SegFs, ctx->SegGs);
}

void dump_regs(CONTEXT *ctx)
{
	if (option_trace)
		do_dump_regs( ctx );
}

BYTE* dump_user_mem( BYTE *address )
{
	BYTE mem[0x80];
	unsigned int len = sizeof mem;
	unsigned char *x = mem;
	unsigned int i;
	char line[0x11];

	NTSTATUS r = copy_from_user( mem, address, len );
	if (r)
	{
		fprintf(stderr, "address %p: invalid\n", address);
		return address;
	}

	fprintf(stderr,"address %p\n", address);
	for (i=0; i<len; i++)
	{
		if (i%0x10 == 0)
		{
			memset( line, ' ', sizeof line );
			line[0x10] = 0;
		}
		line[i%0x10] = printable(x[i]);
		fprintf(stderr,"%02x ", x[i] );
		if ((i+1)%0x10 == 0 || (i+1) == len)
			fprintf(stderr, "%*s\n", ((15 - i)%16)*3 + 20, line);
	}
	return address + sizeof mem;
}

void debugger_backtrace(PCONTEXT ctx)
{
	ULONG frame, stack, x[2], i;
	NTSTATUS r;

	frame = ctx->Ebp;
	stack = ctx->Esp;

	r = copy_from_user( &x[0], (void*) stack, sizeof x );
	if (r == STATUS_SUCCESS)
	{
		fprintf(stderr, "sysret = %08lx\n", x[0]);
	}

	for (i=0; i<0x10; i++)
	{
		fprintf(stderr, "%2ld: %08lx %08lx  ", i, stack, frame);
		if (stack > frame)
		{
			fprintf(stderr, "<invalid frame>\n");
			break;
		}

		r = copy_from_user( &x[0], (void*) frame, sizeof x );
		if (r < STATUS_SUCCESS)
		{
			fprintf(stderr, "<invalid address>\n");
			break;
		}

		fprintf(stderr, "ret = %08lx\n", x[1]);
		if (!x[1])
			break;

		// next frame
		stack = frame;
		frame = x[0];
	}
}

typedef struct _ud_info {
	ud_t ud_obj;
	BYTE *src;
} ud_info;

int ud_input_hook(struct ud* ud_obj)
{
	ud_info *info = (ud_info*) ud_obj;
	BYTE b = 0;
	NTSTATUS r = copy_from_user( &b, info->src, 1 );
	if (r < STATUS_SUCCESS)
		return UD_EOI;
	info->src++;
	return b;
}

BYTE *unassemble( BYTE *address )
{
	ud_info info;
	int i = 10;
	BYTE *insn_addr = address;

	ud_init(&info.ud_obj);
	ud_set_input_hook(&info.ud_obj, ud_input_hook);
	ud_set_mode(&info.ud_obj, 32);
	ud_set_syntax(&info.ud_obj, UD_SYN_INTEL);
	ud_set_pc(&info.ud_obj, (ULONG_PTR) address);

	info.src = address;

	while (ud_disassemble(&info.ud_obj))
	{
		insn_addr = (BYTE*)(ULONG) ud_insn_off(&info.ud_obj);
		fprintf(stderr,"%p %20s %s\n", insn_addr,
				 ud_insn_hex(&info.ud_obj), ud_insn_asm(&info.ud_obj));
		insn_addr += ud_insn_len(&info.ud_obj);
		if (!--i)
			break;
	}

	return insn_addr;
}

void chomp( char *buf )
{
	int len = strlen(buf);

	if (len && buf[len - 1 ] == '\n')
		buf[ len - 1 ] = 0;
}

void debugger_help( void )
{
	const char *help_text =
		"ring3k debugger\n\n"
		" b          backtrace\n"
		" c          continue\n"
		" d <addr>   dump the contents of memory\n"
		" h          help (this text)\n"
		" r          display registers\n"
		" q          quit ring3k\n"
		" u <addr>   disassemble\n";
	fprintf( stderr, "%s\n", help_text);
}

void debugger( void )
{
	CONTEXT ctx;

	ctx.ContextFlags = context_all;
	current->get_context( ctx );
	BYTE *d_address = (BYTE*) ctx.Esp;
	BYTE *u_address = (BYTE*) ctx.Eip;
	char buf[100];
	int errors = 0;
	static bool help_displayed;

	if (!help_displayed)
	{
		debugger_help();
		help_displayed = true;
	}
	do_dump_regs( &ctx );

	while (errors < 3)
	{
		fprintf( stderr, "-");
		if (!fgets(buf, sizeof buf, stdin))
		{
			errors++;
			continue;
		}

		chomp( buf );

		switch (buf[0])
		{
		case 'q': // quit
			exit(1);

		case 'd': // dump, like DOS debug :)
			if (buf[1])
				d_address = (BYTE*)strtol(buf+1, NULL, 0x10);
			d_address = dump_user_mem( d_address );
			break;

		case 'r': // registers
			do_dump_regs( &ctx );
			break;

		case 'h': // continue
			debugger_help();
			break;
		case 'c': // continue
			return;

		case 'b': // backtrace
			debugger_backtrace( &ctx );
			break;

		case 'u': // unassemble
			if (buf[1])
				u_address = (BYTE*)strtol(buf+1, NULL, 0x10);
			u_address = unassemble( u_address );
			break;

		default:
			fprintf(stderr, "unknown command %s\n", buf);
		}
	}
}
