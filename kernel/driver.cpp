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

NTSTATUS NTAPI NtLoadDriver(
	PUNICODE_STRING DriverServiceName)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtUnloadDriver(
	PUNICODE_STRING DriverServiceName)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtCancelDeviceWakeupRequest(
	HANDLE DeviceHandle)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtGetDevicePowerState(
	HANDLE DeviceHandle,
	PDEVICE_POWER_STATE DevicePowerState)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtInitiatePowerAction(
	POWER_ACTION SystemAction,
	SYSTEM_POWER_STATE MinSystemState,
	ULONG Flags,
	BOOLEAN Asynchronous)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtPowerInformation(
	POWER_INFORMATION_LEVEL PowerInformationLevel,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtSetSystemPowerState(
	POWER_ACTION SystemAction,
	SYSTEM_POWER_STATE MinSystemState,
	ULONG Flags)
{
	dprintf("\n");
	return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN NTAPI NtIsSystemResumeAutomatic(void)
{
	dprintf("\n");
	return FALSE;
}
