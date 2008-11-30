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

#ifndef __TEST_LOG_H__
#define __TEST_LOG_H__

NTSTATUS log_init( void );
NTSTATUS log_fini(void);

void init_us( PUNICODE_STRING us, WCHAR *string );
void init_oa( OBJECT_ATTRIBUTES* oa, UNICODE_STRING* us, WCHAR *path );

extern ULONG pass_count, fail_count;

void dprintf(char *string, ...) __attribute__((format (printf,1,2)));
void dump_bin(BYTE *buf, ULONG sz);

#define ok(cond, str, ...) \
    do { \
        if (!(cond)) { \
            dprintf( "%d: " str, __LINE__, ## __VA_ARGS__ ); \
            fail_count++; \
        } else pass_count++; \
    } while (0)

#endif // __TEST_LOG_H__
