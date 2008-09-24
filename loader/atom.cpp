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

NTSTATUS NTAPI NtAddAtom(
	PWSTR String,
	ULONG StringLength,
	PUSHORT Atom)
{
	dprintf("%p %lu %p\n", String, StringLength, Atom);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtFindAtom(
	PWSTR String,
	ULONG StringLength,
	PUSHORT Atom)
{
	dprintf("%p %lu %p\n", String, StringLength, Atom);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtDeleteAtom(
	USHORT Atom)
{
	dprintf("%u\n", Atom);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueryInformationAtom(
	USHORT Atom,
	ATOM_INFORMATION_CLASS AtomInformationClass,
	PVOID AtomInformation,
	ULONG AtomInformationLength,
	PULONG ReturnLength)
{
	dprintf("%u\n", Atom);
	return STATUS_NOT_IMPLEMENTED;
}
