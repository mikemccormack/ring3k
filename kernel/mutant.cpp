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

#include "object.h"
#include "thread.h"
#include "ntcall.h"
#include "object.inl"

class mutant_t : public sync_object_t
{
	thread_t *owner;
	ULONG count;
public:
	mutant_t(BOOLEAN InitialOwner);
	NTSTATUS take_ownership();
	NTSTATUS release_ownership(ULONG& prev);
	virtual BOOLEAN is_signalled();
	virtual BOOLEAN satisfy();
};

mutant_t *mutant_from_obj( object_t *obj )
{
	return dynamic_cast<mutant_t*>( obj );
}

mutant_t::mutant_t(BOOLEAN InitialOwner) :
	owner(0),
	count(0)
{
	if (InitialOwner)
		take_ownership();
}

BOOLEAN mutant_t::is_signalled()
{
	return current != NULL;
}

BOOLEAN mutant_t::satisfy()
{
	take_ownership();
	return TRUE;
}

NTSTATUS mutant_t::take_ownership()
{
	if (owner && owner != current)
		return STATUS_MUTANT_NOT_OWNED;
	owner = current;
	count++;
	return STATUS_SUCCESS;
}

NTSTATUS mutant_t::release_ownership(ULONG& prev)
{
	if (owner != current)
		return STATUS_MUTANT_NOT_OWNED;
	prev = count;
	if (!--count)
		owner = 0;
	return STATUS_SUCCESS;
}

class mutant_factory : public object_factory
{
private:
	BOOLEAN InitialOwner;
public:
	mutant_factory(BOOLEAN io) : InitialOwner(io) {};
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS mutant_factory::alloc_object(object_t** obj)
{
	*obj = new mutant_t(InitialOwner);
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateMutant(
	PHANDLE MutantHandle,
	ACCESS_MASK AccessMask,
	POBJECT_ATTRIBUTES ObjectAttributes,
	BOOLEAN InitialOwner)
{
	trace("%p %08lx %p %u\n", MutantHandle,
			AccessMask, ObjectAttributes, InitialOwner);

	mutant_factory factory( InitialOwner );
	return factory.create( MutantHandle, AccessMask, ObjectAttributes );
}

NTSTATUS NTAPI NtReleaseMutant(
	HANDLE MutantHandle,
	PULONG PreviousState)
{
	mutant_t *mutant = 0;
	ULONG prev = 0;
	NTSTATUS r;

	trace("%p %p\n", MutantHandle, PreviousState);

	r = object_from_handle( mutant, MutantHandle, MUTEX_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	if (PreviousState)
	{
		r = verify_for_write( PreviousState, sizeof PreviousState );
		if (r < STATUS_SUCCESS)
			return r;
	}

	r = mutant->release_ownership( prev );
	if (r == STATUS_SUCCESS && PreviousState)
		r = copy_to_user( PreviousState, &prev, sizeof prev );

	return r;
}

NTSTATUS NTAPI NtOpenMutant(
	PHANDLE MutantHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", MutantHandle, DesiredAccess, ObjectAttributes );
	return nt_open_object<mutant_t>( MutantHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtQueryMutant(
	HANDLE MutantHandle,
	MUTANT_INFORMATION_CLASS MutantInformationClass,
	PVOID MutantInformation,
	ULONG MutantInformationLength,
	PULONG ReturnLength)
{
	return STATUS_NOT_IMPLEMENTED;
}
