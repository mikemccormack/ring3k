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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "file.h"
#include "debug.h"

class mailslot_t : public io_object_t
{
public:
	virtual NTSTATUS read( PVOID Buffer, ULONG Length, ULONG *Read );
	virtual NTSTATUS write( PVOID Buffer, ULONG Length, ULONG *Written );
};

NTSTATUS mailslot_t::read( PVOID Buffer, ULONG Length, ULONG *Read )
{
	trace("\n");
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS mailslot_t::write( PVOID Buffer, ULONG Length, ULONG *Written )
{
	trace("\n");
	return STATUS_NOT_IMPLEMENTED;
}

class mailslot_factory : public object_factory
{
public:
	mailslot_factory() {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS mailslot_factory::alloc_object(object_t** obj)
{
	*obj = new mailslot_t;
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateMailslotFile(
	PHANDLE MailslotHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG CreateOptions,
	ULONG InBufferSize,
	ULONG MaxMessageSize,
	PLARGE_INTEGER ReadTimeout)
{
	trace("%p %08lx %p %p %08lx %lu %lu %p\n", MailslotHandle, AccessMask,
			ObjectAttributes, IoStatusBlock, CreateOptions,
			InBufferSize, MaxMessageSize, ReadTimeout);

	NTSTATUS r;

	r = verify_for_write( IoStatusBlock, sizeof *IoStatusBlock );
	if (r < STATUS_SUCCESS)
		return r;

	if (!ObjectAttributes)
		return STATUS_INVALID_PARAMETER;

	object_attributes_t oa;
	r = oa.copy_from_user( ObjectAttributes );
	if (r < STATUS_SUCCESS)
		return r;

	if (!oa.ObjectName)
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	// FIXME: these checks should be done in the object manager
	UNICODE_STRING &us = *oa.ObjectName;
	if (us.Length<2)
		return STATUS_OBJECT_PATH_SYNTAX_BAD;

	if (us.Length&1)
		return STATUS_OBJECT_NAME_INVALID;

	if (us.Length == 2 && us.Buffer[0] == '\\')
		return STATUS_OBJECT_TYPE_MISMATCH;

	PCWSTR ptr = (PCWSTR) L"\\??\\mailslot\\";
	if (us.Length < 26 || memcmp(ptr, us.Buffer, 26))
		return STATUS_OBJECT_NAME_NOT_FOUND;

	mailslot_factory factory;
	return factory.create( MailslotHandle, AccessMask, ObjectAttributes );
}

