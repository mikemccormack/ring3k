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
#include "ntcall.h"
#include "unicode.h"
#include "object.inl"
#include "event.h"

class event_impl_t : public event_t {
protected:
	BOOLEAN state;
public:
	event_impl_t( BOOLEAN _state );
	virtual ~event_impl_t();
	virtual BOOLEAN is_signalled( void );
	virtual BOOLEAN satisfy( void ) = 0;
	void set( PULONG prev );
	void reset( PULONG prev );
	void pulse( PULONG prev );
	virtual void query(EVENT_BASIC_INFORMATION &info) = 0;
	virtual bool access_allowed( ACCESS_MASK required, ACCESS_MASK handle );
};

class auto_event_t : public event_impl_t
{
public:
	auto_event_t( BOOLEAN state );
	virtual BOOLEAN satisfy( void );
	virtual void query(EVENT_BASIC_INFORMATION &info);
};

class manual_event_t : public event_impl_t
{
public:
	manual_event_t( BOOLEAN state );
	virtual BOOLEAN satisfy( void );
	virtual void query(EVENT_BASIC_INFORMATION &info);
};

event_impl_t::event_impl_t( BOOLEAN _state ) :
	state( _state )
{
}

bool event_impl_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 EVENT_QUERY_STATE,
			 EVENT_MODIFY_STATE,
			 EVENT_ALL_ACCESS );
}

auto_event_t::auto_event_t( BOOLEAN _state) :
	event_impl_t( _state )
{
}

manual_event_t::manual_event_t( BOOLEAN _state) :
	event_impl_t( _state )
{
}

BOOLEAN event_impl_t::is_signalled( void )
{
	return state;
}

BOOLEAN auto_event_t::satisfy( void )
{
	state = 0;
	return TRUE;
}

BOOLEAN manual_event_t::satisfy( void )
{
	return TRUE;
}

void auto_event_t::query( EVENT_BASIC_INFORMATION &info )
{
	info.EventType = SynchronizationEvent;
	info.SignalState = state;
}

void manual_event_t::query( EVENT_BASIC_INFORMATION &info )
{
	info.EventType = NotificationEvent;
	info.SignalState = state;
}

event_t::~event_t( )
{
}

event_impl_t::~event_impl_t( )
{
}

void event_impl_t::set( PULONG prev )
{
	*prev = state;
	state = 1;

	notify_watchers();
}

void event_impl_t::reset( PULONG prev )
{
	*prev = state;
	state = 0;
}

void event_impl_t::pulse( PULONG prev )
{
	set(prev);
	ULONG dummy;
	reset(&dummy);
}

class event_factory : public object_factory
{
private:
	EVENT_TYPE Type;
	BOOLEAN InitialState;
public:
	event_factory(EVENT_TYPE t, BOOLEAN s) : Type(t), InitialState(s) {}
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS event_factory::alloc_object(object_t** obj)
{
	switch (Type)
	{
	case NotificationEvent:
		*obj = new manual_event_t( InitialState );
		break;
	case SynchronizationEvent:
		*obj = new auto_event_t( InitialState );
		break;
	default:
		return STATUS_INVALID_PARAMETER;
	}
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

event_t* create_sync_event( PWSTR name, BOOL InitialState )
{
	event_t *event = new auto_event_t( InitialState );
	if (event)
	{
		OBJECT_ATTRIBUTES oa;
		UNICODE_STRING us;
		us.Buffer = name;
		us.Length = strlenW( name ) * 2;
		us.MaximumLength = 0;

		memset( &oa, 0, sizeof oa );
		oa.Length = sizeof oa;
		oa.Attributes = OBJ_CASE_INSENSITIVE;
		oa.ObjectName = &us;

		NTSTATUS r = name_object( event, &oa );
		if (r < STATUS_SUCCESS)
		{
			trace("name_object failed\n");
			release( event );
			event = 0;
		}
	}
	return event;
}

NTSTATUS NTAPI NtCreateEvent(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	EVENT_TYPE EventType,
	BOOLEAN InitialState )
{
	trace("%p %08lx %p %u %u\n", EventHandle, DesiredAccess,
			ObjectAttributes, EventType, InitialState);

	event_factory factory( EventType, InitialState );
	return factory.create( EventHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtOpenEvent(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", EventHandle, DesiredAccess, ObjectAttributes );
	return nt_open_object<event_t>( EventHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS nteventfunc( HANDLE Handle, PULONG PreviousState, void (event_t::*fn)(PULONG) )
{
	NTSTATUS r;
	ULONG prev;

	if (PreviousState)
	{
		r = verify_for_write( PreviousState, sizeof PreviousState );
		if (r < STATUS_SUCCESS)
			return r;
	}

	event_t *event = 0;
	r = object_from_handle( event, Handle, EVENT_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	(event->*fn)( &prev );

	if (PreviousState)
		copy_to_user( PreviousState, &prev, sizeof prev );

	return r;
}

NTSTATUS NTAPI NtSetEvent(
	HANDLE Handle,
	PULONG PreviousState )
{
	return nteventfunc( Handle, PreviousState, &event_t::set );
}

NTSTATUS NTAPI NtResetEvent(
	HANDLE Handle,
	PULONG PreviousState )
{
	return nteventfunc( Handle, PreviousState, &event_t::reset );
}

NTSTATUS NTAPI NtPulseEvent(
	HANDLE Handle,
	PULONG PreviousState )
{
	return nteventfunc( Handle, PreviousState, &event_t::pulse );
}

NTSTATUS NTAPI NtClearEvent(
	HANDLE Handle)
{
	event_t *event;
	NTSTATUS r;

	trace("%p\n", Handle);

	r = object_from_handle( event, Handle, EVENT_MODIFY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	ULONG prev;
	event->reset( &prev );
	return r;
}

NTSTATUS NTAPI NtQueryEvent(
	HANDLE Handle,
	EVENT_INFORMATION_CLASS EventInformationClass,
	PVOID EventInformation,
	ULONG EventInformationLength,
	PULONG ReturnLength)
{
	event_t *event;
	NTSTATUS r;

	r = object_from_handle( event, Handle, EVENT_QUERY_STATE );
	if (r < STATUS_SUCCESS)
		return r;

	union {
		EVENT_BASIC_INFORMATION basic;
	} info;
	ULONG sz = 0;

	switch (EventInformationClass)
	{
	case EventBasicInformation:
		sz = sizeof info.basic;
		break;
	default:
		return STATUS_INVALID_INFO_CLASS;
	}

	if (sz != EventInformationLength)
		return STATUS_INFO_LENGTH_MISMATCH;

	event->query( info.basic );

	r = copy_to_user( EventInformation, &info, sz );
	if (r < STATUS_SUCCESS)
		return r;

	if (ReturnLength)
		copy_to_user( ReturnLength, &sz, sizeof sz );

	return r;
}

class event_pair_t : public object_t
{
	auto_event_t low, high;
public:
	event_pair_t();
	NTSTATUS set_low();
	NTSTATUS set_high();
	NTSTATUS set_low_wait_high();
	NTSTATUS set_high_wait_low();
	NTSTATUS wait_high();
	NTSTATUS wait_low();
};

event_pair_t::event_pair_t() :
	low(FALSE), high(FALSE)
{
}

NTSTATUS event_pair_t::set_low()
{
	ULONG prev;
	low.set( &prev );
	return STATUS_SUCCESS;
}

NTSTATUS event_pair_t::set_high()
{
	ULONG prev;
	high.set( &prev );
	return STATUS_SUCCESS;
}

NTSTATUS event_pair_t::wait_high()
{
	/*wait_watch_t *ww = new wait_watch_t(&high, current, FALSE, NULL);
	if (!ww)
		return STATUS_NO_MEMORY;

	return ww->notify();*/
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS event_pair_t::wait_low()
{
	/*wait_watch_t *ww = new wait_watch_t(&low, current, FALSE, NULL);
	if (!ww)
		return STATUS_NO_MEMORY;

	return ww->notify();*/
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS event_pair_t::set_low_wait_high()
{
	set_low();
	return wait_high();
}

NTSTATUS event_pair_t::set_high_wait_low()
{
	set_high();
	return wait_low();
}

NTSTATUS event_pair_operation( HANDLE handle, NTSTATUS (event_pair_t::*op)() )
{
	event_pair_t *eventpair = 0;
	NTSTATUS r;
	r = object_from_handle( eventpair, handle, GENERIC_WRITE );
	if (r < STATUS_SUCCESS)
		return r;

	return (eventpair->*op)();
}

class event_pair_factory : public object_factory
{
public:
	virtual NTSTATUS alloc_object(object_t** obj);
};

NTSTATUS event_pair_factory::alloc_object(object_t** obj)
{
	*obj = new event_pair_t();
	if (!*obj)
		return STATUS_NO_MEMORY;
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateEventPair(
	PHANDLE EventPairHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", EventPairHandle, DesiredAccess, ObjectAttributes);
	event_pair_factory factory;
	return factory.create( EventPairHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtOpenEventPair(
	PHANDLE EventPairHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	trace("%p %08lx %p\n", EventPairHandle, DesiredAccess, ObjectAttributes );
	return nt_open_object<event_pair_t>( EventPairHandle, DesiredAccess, ObjectAttributes );
}

NTSTATUS NTAPI NtSetHighEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::set_high );
}

NTSTATUS NTAPI NtSetHighWaitLowEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::set_high_wait_low );
}

NTSTATUS NTAPI NtSetLowEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::set_low );
}

NTSTATUS NTAPI NtSetLowWaitHighEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::set_low_wait_high );
}

NTSTATUS NTAPI NtWaitHighEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::wait_high );
}

NTSTATUS NTAPI NtWaitLowEventPair(
	HANDLE EventPairHandle)
{
	trace("%p\n", EventPairHandle );
	return event_pair_operation( EventPairHandle, &event_pair_t::wait_low );
}

NTSTATUS NTAPI NtSetLowWaitHighThread(void)
{
	trace("\n");
	return STATUS_NO_EVENT_PAIR;
}

NTSTATUS NTAPI NtSetHighWaitLowThread(void)
{
	trace("\n");
	return STATUS_NO_EVENT_PAIR;
}

NTSTATUS NTAPI NtOpenKeyedEvent(
	PHANDLE EventHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes)
{
	// hack: just open an event for the moment
	return nt_open_object<event_t>( EventHandle, DesiredAccess, ObjectAttributes );
}
