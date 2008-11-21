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


#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "mem.h"
#include "ntcall.h"
#include "timer.h"

NTSTATUS copy_to_user( void *dest, const void *src, size_t len )
{
	return current->copy_to_user( dest, src, len );
}

NTSTATUS copy_from_user( void *dest, const void *src, size_t len )
{
	return current->copy_from_user( dest, src, len );
}

NTSTATUS verify_for_write( void *dest, size_t len )
{
	return current->verify_for_write( dest, len );
}

NTSTATUS NTAPI NtRaiseHardError(
	NTSTATUS Status,
	ULONG NumberOfArguments,
	ULONG StringArgumentsMask,
	PULONG Arguments,
	HARDERROR_RESPONSE_OPTION ResponseOption,
	PHARDERROR_RESPONSE Response)
{
	NTSTATUS r;
	ULONG i;

	dprintf("%08lx %lu %lu %p %u %p\n", Status, NumberOfArguments,
			StringArgumentsMask, Arguments, ResponseOption, Response);

	if (NumberOfArguments>32)
		return STATUS_INVALID_PARAMETER;

	dprintf("hard error:\n");
	for (i=0; i<NumberOfArguments; i++)
	{
		void *arg;

		r = copy_from_user( &arg, &Arguments[i], sizeof (PUNICODE_STRING) );
		if (r < STATUS_SUCCESS)
			break;

		if (StringArgumentsMask & (1<<i))
		{
			unicode_string_t us;

			r = us.copy_from_user( (UNICODE_STRING*) arg );
			if (r < STATUS_SUCCESS)
				break;

			dprintf("arg[%ld]: %pus\n", i, &us);
		}
		else
			dprintf("arg[%ld]: %08lx\n", i, (ULONG)arg);
	}

	debugger();

	return STATUS_SUCCESS;
}

#define SET_INFO_LENGTH(len, item) \
	do { \
		(len) = sizeof (item); \
		if ((len) < SystemInformationLength) \
			return STATUS_INFO_LENGTH_MISMATCH; \
		(len) = SystemInformationLength; \
	} while (0)

NTSTATUS NTAPI NtQuerySystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength )
{
	NTSTATUS r = STATUS_SUCCESS;
	union {
		SYSTEM_BASIC_INFORMATION basic;
		SYSTEM_CPU_INFORMATION cpu;
		SYSTEM_THREAD_INFORMATION thread;
		SYSTEM_TIME_OF_DAY_INFORMATION time_of_day;
		SYSTEM_RANGE_START_INFORMATION range_start;
		SYSTEM_GLOBAL_FLAG global_flag;
		SYSTEM_KERNEL_DEBUGGER_INFORMATION kernel_debugger_info;
		SYSTEM_PERFORMANCE_INFORMATION performance_info;
		SYSTEM_CRASH_DUMP_STATE_INFORMATION crash_dump_info;
	} info;
	ULONG len = 0;

	dprintf("%d %p %lu %p\n", SystemInformationClass, SystemInformation,
			SystemInformationLength, ReturnLength);

	if (ReturnLength)
	{
		r = copy_to_user( ReturnLength, &len, sizeof len );
		if (r < STATUS_SUCCESS)
			return r;
	}

	memset( &info, 0, sizeof info );

	switch( SystemInformationClass )
	{
	case SystemBasicInformation:
		SET_INFO_LENGTH( len, info.basic );
		info.basic.dwUnknown1 = 0;
		info.basic.uKeMaximumIncrement = 0x18730;
		info.basic.uPageSize = 0x1000;
		info.basic.uMmNumberOfPhysicalPages = 0xbf6c;
		info.basic.uMmLowestPhysicalPage = 1;
		info.basic.uMmHighestPhysicalPage = 0xbfdf;
		info.basic.uAllocationGranularity = 0x1000;
		info.basic.pLowestUserAddress = (void*)0x1000;
		info.basic.pMmHighestUserAddress = (void*)0x7ffeffff;
		info.basic.uKeActiveProcessors = 1;
		info.basic.uKeNumberProcessors = 1;
		break;

	case SystemCpuInformation:
		SET_INFO_LENGTH( len, info.cpu );
		info.cpu.Architecture = 0;
		info.cpu.Level = 6;
		info.cpu.Revision = 0x0801;
		info.cpu.FeatureSet = 0x2fff;
		break;

	case SystemTimeOfDayInformation:
		SET_INFO_LENGTH( len, info.time_of_day );
		get_system_time_of_day( info.time_of_day );
		break;

	case SystemRangeStartInformation:
		SET_INFO_LENGTH( len, info.range_start );
		info.range_start.SystemRangeStart = (PVOID) 0x80000000;
		break;

	case SystemGlobalFlag:
		SET_INFO_LENGTH( len, info.global_flag );
		info.global_flag.GlobalFlag = 0;
		if (option_trace)
			info.global_flag.GlobalFlag |= FLG_SHOW_LDR_SNAPS | FLG_ENABLE_CSRDEBUG;
		break;

	case SystemKernelDebuggerInformation:
		SET_INFO_LENGTH( len, info.kernel_debugger_info );
		info.kernel_debugger_info.DebuggerEnabled = FALSE;
		info.kernel_debugger_info.DebuggerNotPresent = TRUE;
		break;

	case SystemPerformanceInformation:
		SET_INFO_LENGTH( len, info.performance_info );
		info.performance_info.AvailablePages = 0x80000; // 512Mb
		break;

	case SystemCrashDumpStateInformation:
		SET_INFO_LENGTH( len, info.crash_dump_info );
		info.crash_dump_info.CrashDumpSectionExists = 0;
		info.crash_dump_info.Unknown = 0;
		break;

	default:
		dprintf("SystemInformationClass = %d not handled\n", SystemInformationClass);
		r = STATUS_INVALID_INFO_CLASS;
	}

	r = copy_to_user( SystemInformation, &info, len );
	if (ReturnLength)
		r = copy_to_user( ReturnLength, &len, sizeof len );

	return r;
}

NTSTATUS NTAPI NtSetSystemInformation(
	SYSTEM_INFORMATION_CLASS SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength )
{
	dprintf("%d %p %lu\n", SystemInformationClass, SystemInformation, SystemInformationLength );
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtFlushInstructionCache(
	HANDLE Process,
	PVOID BaseAddress,
	SIZE_T NumberOfBytesToFlush )
{
	dprintf("%p %p %08lx\n", Process, BaseAddress, NumberOfBytesToFlush);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtDisplayString( PUNICODE_STRING String )
{
	unicode_string_t us;
	NTSTATUS r;

	r = us.copy_from_user( String );
	if (r < STATUS_SUCCESS)
		return r;

	dprintf("%pus\n", &us );

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreatePagingFile(
	PUNICODE_STRING FileName,
	PULARGE_INTEGER InitialSize,
	PULARGE_INTEGER MaximumSize,
	ULONG Reserved)
{
	unicode_string_t us;
	NTSTATUS r;

	r = us.copy_from_user( FileName );
	if (r < STATUS_SUCCESS)
		return r;

	ULARGE_INTEGER init_sz, max_sz;

	r = copy_from_user( &init_sz, InitialSize, sizeof init_sz );
	if (r < STATUS_SUCCESS)
		return r;

	r = copy_from_user( &max_sz, MaximumSize, sizeof max_sz );
	if (r < STATUS_SUCCESS)
		return r;

	dprintf("unimplemented - %pus %llu %llu %08lx\n",
			 &us, init_sz.QuadPart, max_sz.QuadPart, Reserved);

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtShutdownSystem(
	SHUTDOWN_ACTION Action)
{
	const char *action = 0;
	switch (Action)
	{
	case ShutdownNoReboot:
		action = "ShutdownNoReboot";
		break;
	case ShutdownReboot:
		action = "ShutdownReboot";
		break;
	case ShutdownPowerOff:
		action = "ShutdownPowerOff";
		break;
	default:
		return STATUS_INVALID_PARAMETER;
	}
	dprintf("%s\n", action);
	exit(1);
}

NTSTATUS NTAPI NtQueryPerformanceCounter(
	PLARGE_INTEGER PerformanceCount,
	PLARGE_INTEGER PerformanceFrequency)
{
	LARGE_INTEGER now = timeout_t::current_time();
	LARGE_INTEGER freq;
	NTSTATUS r;
	freq.QuadPart = 1000LL;
	r = copy_to_user( PerformanceCount, &now, sizeof now );
	if (r < STATUS_SUCCESS)
		return r;
	r = copy_to_user( PerformanceFrequency, &freq, sizeof freq );
	return r;
}

NTSTATUS NTAPI NtAllocateLocallyUniqueId(
	PLUID Luid)
{
	static LARGE_INTEGER id;
	LUID luid;
	luid.HighPart = id.QuadPart >> 32;
	luid.LowPart = id.QuadPart & 0xffffffffLL;
	NTSTATUS r = copy_to_user( Luid, &luid, sizeof luid );
	if (r == STATUS_SUCCESS)
		id.QuadPart++;
	return r;
}

NTSTATUS NTAPI NtQueryDebugFilterState(
	ULONG Component,
	ULONG Level)
{
	dprintf("%08lx %08lx\n", Component, Level);
	return STATUS_SUCCESS;
}
