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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"

NTSTATUS NTAPI NtCreateJobObject(
	PHANDLE JobHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", JobHandle, AccessMask, ObjectAttributes);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtOpenJobObject(
	PHANDLE JobHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", JobHandle, AccessMask, ObjectAttributes);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtAssignProcessToJobObject(
	HANDLE JobHandle,
	HANDLE ProcessHandle)
{
	trace("%p %p\n", JobHandle, ProcessHandle);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtTerminateJobObject(
	HANDLE JobHandle,
	NTSTATUS ExitStatus)
{
	trace("%p %08lx\n", JobHandle, ExitStatus);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueryInformationJobObject(
	HANDLE JobHandle,
	JOBOBJECTINFOCLASS InfoClass,
	PVOID JobObjectInformation,
	ULONG JobObjectInformationSize,
	PULONG ReturnLength)
{
	trace("\n");
	return STATUS_ACCESS_DENIED;
}

NTSTATUS NTAPI NtSetInformationJobObject(
	HANDLE JobHandle,
	JOBOBJECTINFOCLASS InfoClass,
	PVOID JobObjectInformation,
	ULONG JobObjectInformationSize)
{
	return STATUS_NOT_IMPLEMENTED;
}
