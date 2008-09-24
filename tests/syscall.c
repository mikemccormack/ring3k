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
#include "log.h"

static inline NTSTATUS ntcall(ULONG num, void *args)
{
	NTSTATUS ret;
	__asm__( "int $0x2e" : "=a" (ret) : "a" (num), "d" (args) );
	return ret;
}

void test_syscall( void )
{
	NTSTATUS r;
	ULONG arg[0x10];

	r = ntcall(0xf00, &arg[0]);
	ok(r == STATUS_INVALID_SYSTEM_SERVICE, "return wrong %08lx\n", r);
}

void test_system_info_range_start( void )
{
	SYSTEM_RANGE_START_INFORMATION info;
	ULONG ret = 0;
	NTSTATUS r;

	r = NtQuerySystemInformation( SystemRangeStartInformation, &info, sizeof info, &ret );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( ret == sizeof info, "return wrong %08lx\n", ret);
	// won't pass on 3G user space systems...
	ok( info.SystemRangeStart == (void*)0x80000000, "return wrong %p\n", info.SystemRangeStart );
}

void test_system_info_time_of_day( void )
{
	//SYSTEM_TIME_OF_DAY_INFORMATION *time_of_day;
	BYTE info[0x100];
	ULONG ret = 0;
	NTSTATUS r;
	ULONG len;

	len = 28;
	r = NtQuerySystemInformation( SystemTimeOfDayInformation, &info, len, &ret );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( ret == len, "return wrong %ld\n", ret);

	len = 36;
	r = NtQuerySystemInformation( SystemTimeOfDayInformation, &info, len, &ret );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( ret == len, "return wrong %ld\n", ret);

	len = 48;
	r = NtQuerySystemInformation( SystemTimeOfDayInformation, &info, len, &ret );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( ret == len, "return wrong %ld\n", ret);

	/*ULONG i;
	for (i=0; i<(ret/sizeof (ULONG)); i++)
		dprintf("info[%02x] = %08lx\n", i, ((ULONG*)&info)[i] ); */

	len = sizeof info;
	r = NtQuerySystemInformation( SystemTimeOfDayInformation, &info, len, &ret );
	ok( r == STATUS_INFO_LENGTH_MISMATCH, "return wrong %08lx\n", r);
	ok( ret == 0, "return wrong %ld\n", ret);
}

void test_get_locale( void )
{
	NTSTATUS r;
	LCID lcid;
	LCID eng = MAKELCID( MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT );

	r = NtQueryDefaultLocale( 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "return wrong %08lx\n", r);

	lcid = ~0;
	r = NtQueryDefaultLocale( ~0, &lcid );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( lcid == eng, "return wrong %04lx %04lx\n", lcid, eng);

	lcid = ~0;
	r = NtQueryDefaultLocale( 0, &lcid );
	ok( r == STATUS_SUCCESS, "return wrong %08lx\n", r);
	ok( lcid == eng, "return wrong %04lx %04lx\n", lcid, eng);
}

void test_nt_shared_mem( void )
{
	const KUSER_SHARED_DATA * const kshm = (void*) 0x7ffe0000;

	ok( kshm->ProductIsValid == 1, "product type not valid %08x\n", kshm->ProductIsValid);
	ok( kshm->NtProductType == 1, "product type wrong %08x\n", kshm->NtProductType);
	ok( kshm->ImageNumberLow == 0x14c, "image number low wrong %08x\n", kshm->ImageNumberLow);
	ok( kshm->ImageNumberHigh == 0x14c, "image number high wrong %08x\n", kshm->ImageNumberHigh);
	ok( kshm->NtMajorVersion == 5, "major version wrong %08x\n", kshm->NtMajorVersion);
	ok( kshm->NtMinorVersion == 0, "minor version wrong %08x\n", kshm->NtMinorVersion);
}

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

const int ofs_peb_in_teb = 0x30;

void* get_peb( void )
{
	void **p = get_teb();
	return p[ofs_peb_in_teb/sizeof (*p)];
}

void test_versions( void )
{
	ULONG *peb = get_peb();
	ULONG x;

	// these values are correct for Windows 2000
	x = peb[0xa4/4];
	ok( x == 5, "major version wrong %ld\n", x);

	x = peb[0xa8/4];
	ok( x == 0, "minor version wrong %ld\n", x);

	x = peb[0xac/4];
	ok( x == 0x01000893, "build number wrong %08lx\n", x);

	x = peb[0xb0/4];
	ok( x == 2, "platform id wrong %08lx\n", x);
}

void NtProcessStartup( void )
{
	log_init();
	test_syscall();
	test_system_info_range_start();
	test_system_info_time_of_day();
	test_get_locale();
	test_nt_shared_mem();
	test_versions();
	log_fini();
}
