/*
 * native test suite
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

#include <stdarg.h>
#include "ntapi.h"
#include "rtlapi.h"
#include "log.h"

static WCHAR slotname[] = L"\\device\\mailslot\\nthost";
static WCHAR eventname[] = L"\\BaseNamedObjects\\nthost";

HANDLE slot, event;
ULONG pass_count, fail_count;

NTSTATUS log_init( void )
{
	UNICODE_STRING name;
	OBJECT_ATTRIBUTES oa;
	IO_STATUS_BLOCK iosb;
	NTSTATUS r;

	pass_count = 0;
	fail_count = 0;

	/* open the event */
	name.Buffer = eventname;
	name.Length = sizeof eventname - 2;
	name.MaximumLength = name.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = NULL;
	oa.SecurityQualityOfService = NULL;

	r = NtOpenEvent( &event, EVENT_ALL_ACCESS, &oa );
	if (r != STATUS_SUCCESS)
		return r;

	/* open the mailslot */
	name.Buffer = slotname;
	name.Length = sizeof slotname - 2;
	name.MaximumLength = name.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = NULL;
	oa.SecurityQualityOfService = NULL;

	iosb.Status = 0;
	iosb.Information = 0;

	return NtOpenFile( &slot, GENERIC_WRITE, &oa, &iosb, FILE_SHARE_WRITE | FILE_SHARE_READ, 0 );
}

NTSTATUS log_fini(void)
{
	dprintf("%lu failed, %lu passed\n", fail_count, pass_count);
	NtClose( slot );
	NtClose( event );

	return NtTerminateProcess( NtCurrentProcess(), fail_count ? 1 : 0 );
}

UINT log_strlen( char *p )
{
	UINT n = 0;
	while( p[n] ) n++;
	return n;
}

// compatible with win2k int 2d
struct log_buffer {
	USHORT len;
	USHORT pad;
	ULONG empty[3];
	char buffer[0x100];
};

void log_debug( struct log_buffer *lb )
{
	__asm__ __volatile ( "\tint $0x2d\n" : : "c"(lb), "a"(1) );
}

NTSTATUS log_write( struct log_buffer *lb )
{
	NTSTATUS r;
	IO_STATUS_BLOCK iosb;

	iosb.Status = 0;
	iosb.Information = 0;
	r = NtWriteFile( slot, NULL, NULL, NULL, &iosb, lb->buffer, lb->len, NULL, NULL );
	if (r != STATUS_SUCCESS)
		return r;

	r = NtSetEvent( event, NULL );

	return r;
}

void dprintf(char *string, ...)
{
	struct log_buffer x;
	va_list va;

	x.empty[0] = 0;
	x.empty[1] = 0;
	x.empty[2] = 0;

	va_start( va, string );
	vsprintf( x.buffer, string, va );
	va_end( va );

	x.len = log_strlen( x.buffer );
	if (slot && event)
		log_write( &x );
	else
		log_debug( &x );
}

void *memset( void *ptr, int val, size_t len )
{
	unsigned char *p = ptr;
	size_t n = 0;
	while (n<len)
		p[n++] = val;
	return ptr;
}

void init_us( PUNICODE_STRING us, WCHAR *string )
{
	int i = 0;
	while (string[i])
		i++;
	us->Length = i*2;
	us->MaximumLength = 0;
	us->Buffer = string;
}

void init_oa( OBJECT_ATTRIBUTES* oa, UNICODE_STRING* us, PWSTR path )
{
	init_us( us, path );

	oa->Length = sizeof *oa;
	oa->RootDirectory = 0;
	oa->ObjectName = us;
	oa->Attributes = OBJ_CASE_INSENSITIVE;
	oa->SecurityDescriptor = 0;
	oa->SecurityQualityOfService = 0;
}

char hex(BYTE x)
{
	if (x<10)
		return x+'0';
	return x+'A'-10;
}

void dump_bin(void *ptr, ULONG sz)
{
	BYTE *buf = ptr;
	char str[0x33];
	int i;
	for (i=0; i<sz; i++)
	{
		str[(i%16)*3] = hex(buf[i]>>4);
		str[(i%16)*3+1] = hex(buf[i]&0x0f);
		str[(i%16)*3+2] = ' ';
		str[(i%16)*3+3] = 0;
		if ((i+1)%16 == 0 || (i+1) == sz)
			dprintf("%s\n", str);
	}
}

