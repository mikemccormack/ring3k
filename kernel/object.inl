#ifndef __OBJECT_INL__
#define __OBJECT_INL__

#include "ntcall.h"

template<class T> NTSTATUS nt_open_object(
	PHANDLE Handle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	OBJECT_ATTRIBUTES oa;
	unicode_string_t us;
	NTSTATUS r;

	r = copy_from_user( &oa, ObjectAttributes, sizeof oa );
	if (r != STATUS_SUCCESS)
		return r;

	if (oa.ObjectName)
	{
		r = us.copy_from_user( oa.ObjectName );
		if (r != STATUS_SUCCESS)
			return r;
		oa.ObjectName = &us;
	}

	dprintf("object = %pus\n", oa.ObjectName );

	object_t *object = NULL;

	r = get_named_object( &object, &oa );
	if (r != STATUS_SUCCESS)
		return r;

	if (dynamic_cast<T*>( object ))
	{
		r = alloc_user_handle( object, DesiredAccess, Handle );
	}
	else
		r = STATUS_OBJECT_TYPE_MISMATCH;

	release( object );

	return r;
}

template<typename T> NTSTATUS object_from_handle(T*& out, HANDLE handle, ACCESS_MASK access)
{
	NTSTATUS r;
	object_t *obj = 0;

	r = current->process->handle_table.object_from_handle( obj, handle, access );
	if (r != STATUS_SUCCESS)
		return r;

	out = dynamic_cast<T*>(obj);
	if (!out)
		return STATUS_OBJECT_TYPE_MISMATCH;

	return STATUS_SUCCESS;
}

#endif // __OBJECT_INL__
