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


#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "object.h"
#include "object.inl"
#include "mem.h"
#include "ntcall.h"
#include "unicode.h"

#include "list.h"

struct regval_t;
struct regkey_t;

typedef list_anchor<regval_t,0> regval_anchor, regval_anchor_t;
typedef list_anchor<regkey_t,0> regkey_anchor, regkey_anchor_t;
typedef list_iter<regval_t,0> regval_iter, regval_iter_t;
typedef list_iter<regkey_t,0> regkey_iter, regkey_iter_t;
typedef list_element<regval_t> regval_element, regval_element_t;
typedef list_element<regkey_t> regkey_element, regkey_element_t;

struct regval_t {
	regval_element entry[1];
	unicode_string_t name;
	ULONG type;
	ULONG size;
	BYTE *data;
public:
	regval_t( UNICODE_STRING *name, ULONG _type, ULONG _size );
	~regval_t();
};

struct regkey_t : public object_t {
	regkey_t *parent;
	unicode_string_t name;
	unicode_string_t cls;
	regkey_element entry[1];
	regkey_anchor children;
	regval_anchor values;
public:
	regkey_t( regkey_t *_parent, UNICODE_STRING *_name );
	~regkey_t();
	void query( KEY_FULL_INFORMATION& info, UNICODE_STRING& keycls );
	void query( KEY_BASIC_INFORMATION& info, UNICODE_STRING& namestr );
	ULONG num_values(ULONG& max_name_len, ULONG& max_data_len);
	ULONG num_subkeys(ULONG& max_name_len, ULONG& max_class_len);
	void delkey();
	regkey_t *get_child( ULONG Index );
	NTSTATUS query(KEY_INFORMATION_CLASS KeyInformationClass, PVOID KeyInformation, ULONG KeyInformationLength, PULONG ReturnLength);
	virtual bool access_allowed( ACCESS_MASK required, ACCESS_MASK handle );
};

regkey_t *root_key;

// FIXME: should use windows case table
INT strncmpW( WCHAR *a, WCHAR *b, ULONG n )
{
	ULONG i;
	WCHAR ai, bi;

	for ( i = 0; i < n; i++ )
	{
		ai = tolower( a[i] );
		bi = tolower( b[i] );
		if (ai == bi)
			continue;
		return ai < bi ? -1 : 1;
	}
	return 0;
}

BOOLEAN unicode_string_equal( PUNICODE_STRING a, PUNICODE_STRING b, BOOLEAN case_insensitive )
{
	if (a->Length != b->Length)
		return FALSE;
	if (case_insensitive)
		return (0 == strncmpW( a->Buffer, b->Buffer, a->Length/2 ));
	return (0 == memcmp( a->Buffer, b->Buffer, a->Length ));
}

regkey_t::regkey_t( regkey_t *_parent, UNICODE_STRING *_name ) :
	parent( _parent)
{
	name.copy( _name );
	if (parent)
		parent->children.append( this );
}

regkey_t::~regkey_t()
{
	regkey_iter i(children);
	while (i)
	{
		regkey_t *tmp = i;
		i.next();
		tmp->parent = NULL;
		children.unlink( tmp );
		release( tmp );
	}

	regval_iter j(values);
	while (j)
	{
		regval_t *tmp = j;
		j.next();
		values.unlink( tmp );
		delete tmp;
	}
}

bool regkey_t::access_allowed( ACCESS_MASK required, ACCESS_MASK handle )
{
	return check_access( required, handle,
			 KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS|KEY_NOTIFY,
			 KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_CREATE_LINK,
			 KEY_ALL_ACCESS );
}

ULONG regkey_t::num_values(ULONG& max_name_len, ULONG& max_data_len)
{
	ULONG n = 0;
	regval_iter i(values);
	max_name_len = 0;
	max_data_len = 0;
	while (i)
	{
		regval_t *val = i;
		max_name_len = max(max_name_len, val->name.Length );
		max_data_len = max(max_data_len, val->size );
		i.next();
		n++;
	}
	return n;
}

ULONG regkey_t::num_subkeys(ULONG& max_name_len, ULONG& max_class_len)
{
	ULONG n = 0;
	regkey_iter i(children);
	max_name_len = 0;
	while (i)
	{
		regkey_t *subkey = i;
		max_name_len = max(max_name_len, subkey->name.Length );
		max_class_len = max(max_class_len, subkey->cls.Length );
		i.next();
		n++;
	}
	return n;
}

void regkey_t::query( KEY_FULL_INFORMATION& info, UNICODE_STRING& keycls )
{
	dprintf("full information\n");
	info.LastWriteTime.QuadPart = 0LL;
	info.TitleIndex = 0;
	info.ClassOffset = FIELD_OFFSET( KEY_FULL_INFORMATION, Class );
	info.ClassLength = cls.Length;
	info.SubKeys = num_subkeys(info.MaxNameLen, info.MaxClassLen);
	info.Values = num_values(info.MaxValueNameLen, info.MaxValueDataLen);
	keycls = cls;
	dprintf("class = %pus\n", &cls );
}

void regkey_t::query( KEY_BASIC_INFORMATION& info, UNICODE_STRING& namestr )
{
	dprintf("basic information\n");
	info.LastWriteTime.QuadPart = 0LL;
	info.TitleIndex = 0;
	info.NameLength = name.Length;

	namestr = name;
}

void regkey_t::delkey()
{
	if ( parent )
	{
		parent->children.unlink( this );
		parent = NULL;
		release( this );
	}
}

regkey_t *regkey_t::get_child( ULONG Index )
{
	regkey_iter_t i(children);
	regkey_t *child;
	while ((child = i) && Index)
	{
		i.next();
		Index--;
	}
	return child;
}

regval_t::regval_t( UNICODE_STRING *_name, ULONG _type, ULONG _size ) :
	type(_type),
	size(_size)
{
	name.copy(_name);
	data = new BYTE[size];
}

regval_t::~regval_t()
{
	delete[] data;
}

ULONG skip_slashes( UNICODE_STRING *str )
{
	ULONG len;

	// skip slashes
	len = 0;
	while ((len*2) < str->Length && str->Buffer[len] == '\\')
	{
		str->Buffer ++;
		str->Length -= sizeof (WCHAR);
		len++;
	}
	return len;
}

ULONG get_next_segment( UNICODE_STRING *str )
{
	ULONG n = 0;

	while ( n < str->Length/sizeof(WCHAR) && str->Buffer[ n ] != '\\' )
		n++;

	return n * sizeof (WCHAR);
}

ULONG do_open_subkey( regkey_t *&key, UNICODE_STRING *name, bool case_insensitive )
{
	ULONG len;

	skip_slashes( name );

	len = get_next_segment( name );
	if (!len)
		return len;

	for (regkey_iter i(key->children); i; i.next())
	{
		regkey_t *subkey = i;
		if (len != subkey->name.Length)
			continue;
		if (case_insensitive)
		{
			if (strncmpW( name->Buffer, subkey->name.Buffer, len/sizeof(WCHAR) ))
				continue;
		}
		else
		{
			if (memcmp( name->Buffer, subkey->name.Buffer, len/sizeof(WCHAR) ))
				continue;
		}

		// advance
		key = subkey;
		name->Buffer += len/2;
		name->Length -= len;
		return len;
	}

	return 0;
}

NTSTATUS open_parse_key( regkey_t *&key, UNICODE_STRING *name, bool case_insensitive  )
{
	while (name->Length && do_open_subkey( key, name, case_insensitive ))
		/* repeat */ ;

	if (name->Length)
	{
		dprintf("remaining = %pus\n", name);
		if (name->Length == get_next_segment( name ))
			return STATUS_OBJECT_NAME_NOT_FOUND;

		return STATUS_OBJECT_PATH_NOT_FOUND;
	}

	return STATUS_SUCCESS;
}

NTSTATUS create_parse_key( regkey_t *&key, UNICODE_STRING *name, bool& opened_existing )
{
	while (name->Length && do_open_subkey( key, name, true ))
		/* repeat */ ;

	opened_existing = (name->Length == 0);

	while (name->Length)
	{
		UNICODE_STRING seg;

		skip_slashes( name );

		seg.Length = get_next_segment( name );
		seg.Buffer = name->Buffer;

		key = new regkey_t( key, &seg );
		if (!key)
			return STATUS_NO_MEMORY;

		name->Buffer += seg.Length/2;
		name->Length -= seg.Length;
	}

	return STATUS_SUCCESS;
}

NTSTATUS open_key( regkey_t **out, OBJECT_ATTRIBUTES *oa )
{
	UNICODE_STRING parsed_name;
	regkey_t *key = root_key;
	NTSTATUS r;

	if (oa->RootDirectory)
	{
		r = object_from_handle( key, oa->RootDirectory, 0 );
		if (r != STATUS_SUCCESS)
			return r;
	}
	else
		key = root_key;

	memcpy( &parsed_name, oa->ObjectName, sizeof parsed_name );
	r = open_parse_key( key, &parsed_name, oa->Attributes & OBJ_CASE_INSENSITIVE );

	if (r == STATUS_SUCCESS)
		*out = key;

	return r;
}

NTSTATUS create_key( regkey_t **out, OBJECT_ATTRIBUTES *oa, bool& opened_existing )
{
	UNICODE_STRING parsed_name;
	regkey_t *key = root_key;
	NTSTATUS r;

	if (!oa->ObjectName)
		return STATUS_ACCESS_VIOLATION;

	if (oa->RootDirectory)
	{
		r = object_from_handle( key, oa->RootDirectory, 0 );
		if (r != STATUS_SUCCESS)
			return r;
	}
	else
		key = root_key;

	memcpy( &parsed_name, oa->ObjectName, sizeof parsed_name );
	r = create_parse_key( key, &parsed_name, opened_existing );

	if (r == STATUS_SUCCESS)
		*out = key;

	return r;
}

regval_t *key_find_value( regkey_t *key, UNICODE_STRING *us )
{

	for (regval_iter i(key->values); i; i.next())
	{
		regval_t *val = i;
		if (unicode_string_equal( &val->name, us, TRUE ))
			return val;
	}

	return NULL;
}

NTSTATUS delete_value( regkey_t *key, UNICODE_STRING *us )
{
	regval_t *val;

	//dprintf("%p %pus\n", key, us);

	val = key_find_value( key, us );
	if (!val)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	dprintf("deleting %pus\n", &val->name);
	key->values.unlink( val );
	delete val;
	return STATUS_SUCCESS;
}

/* this doesn't set STATUS_MORE_DATA */
NTSTATUS reg_query_value(
	regval_t* val,
	ULONG KeyValueInformationClass,
	PVOID KeyValueInformation,
	ULONG KeyValueInformationLength,
	ULONG& len )
{
	NTSTATUS r = STATUS_SUCCESS;
	union {
		KEY_VALUE_FULL_INFORMATION full;
		KEY_VALUE_PARTIAL_INFORMATION partial;
	} info;
	ULONG info_sz;

	len = 0;

	dprintf("%pus\n", &val->name );

	memset( &info, 0, sizeof info );

	switch( KeyValueInformationClass )
	{
	case KeyValueFullInformation:
		info_sz = FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name );
		// include nul terminator at the end of the name
		info.full.DataOffset = info_sz + val->name.Length + 2;
		len = info.full.DataOffset + val->size;
		if (KeyValueInformationLength < info_sz)
			return STATUS_BUFFER_TOO_SMALL;

		info.full.Type = val->type;
		info.full.DataLength = val->size;
		info.full.NameLength = val->name.Length;

		r = copy_to_user( KeyValueInformation, &info.full, info_sz );
		if (r != STATUS_SUCCESS)
			break;

		if (len > KeyValueInformationLength)
			return STATUS_BUFFER_OVERFLOW;

		r = copy_to_user( (BYTE*)KeyValueInformation + info_sz,
						  val->name.Buffer, val->name.Length );
		if (r != STATUS_SUCCESS)
			break;

		r = copy_to_user( (BYTE*)KeyValueInformation + info.full.DataOffset,
						  val->data, val->size );
		break;

	case KeyValuePartialInformation:
		info_sz = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );
		len = info_sz + val->size;
		if (KeyValueInformationLength < info_sz)
			return STATUS_BUFFER_TOO_SMALL;

		info.partial.Type = val->type;
		info.partial.DataLength = val->size;

		r = copy_to_user( KeyValueInformation, &info.partial, info_sz );
		if (r != STATUS_SUCCESS)
			break;

		if (len > KeyValueInformationLength)
			return STATUS_BUFFER_OVERFLOW;

		r = copy_to_user( (BYTE*)KeyValueInformation + info_sz, val->data, val->size );
		break;

	case KeyValueBasicInformation:
	case KeyValueFullInformationAlign64:
	case KeyValuePartialInformationAlign64:
	default:
		r = STATUS_NOT_IMPLEMENTED;
	}

	return r;
}

NTSTATUS NTAPI NtCreateKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG Disposition )
{
	object_attributes_t oa;
	NTSTATUS r;
	regkey_t *key = NULL;

	dprintf("%p %08lx %p %lu %p %lu %p\n", KeyHandle, DesiredAccess,
			ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition );

	if (Disposition)
	{
		r = verify_for_write( Disposition, sizeof *Disposition );
		if (r != STATUS_SUCCESS)
			return r;
	}

	r = oa.copy_from_user( ObjectAttributes );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("len %08lx root %p attr %08lx %pus\n",
			oa.Length, oa.RootDirectory, oa.Attributes, oa.ObjectName);

	unicode_string_t cls;
	if (Class)
	{
		r = cls.copy_from_user( Class );
		if (r != STATUS_SUCCESS)
			return r;
	}

	bool opened_existing = false;
	r = create_key( &key, &oa, opened_existing );
	if (r == STATUS_SUCCESS)
	{
		if (Disposition)
		{
			ULONG dispos = opened_existing ? REG_OPENED_EXISTING_KEY : REG_CREATED_NEW_KEY;
			copy_to_user( Disposition, &dispos, sizeof *Disposition );
		}
		key->cls.copy( &cls );
		r = alloc_user_handle( key, DesiredAccess, KeyHandle );
		//release( event );
	}
	return r;
}

NTSTATUS NTAPI NtOpenKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes )
{
	OBJECT_ATTRIBUTES oa;
	unicode_string_t us;
	NTSTATUS r;
	regkey_t *key = NULL;

	dprintf("%p %08lx %p\n", KeyHandle, DesiredAccess, ObjectAttributes );

	// copies the unicode string before validating object attributes struct
	r = copy_from_user( &oa, ObjectAttributes, sizeof oa );
	if (r != STATUS_SUCCESS)
		return r;

	r = us.copy_from_user( oa.ObjectName );
	if (r != STATUS_SUCCESS)
		return r;
	oa.ObjectName = &us;

	if (oa.Length != sizeof oa)
		return STATUS_INVALID_PARAMETER;

	dprintf("len %08lx root %p attr %08lx %pus\n",
			oa.Length, oa.RootDirectory, oa.Attributes, oa.ObjectName);

	r = open_key( &key, &oa );

	dprintf("open_key returned %08lx\n", r);

	if (r == STATUS_SUCCESS)
	{
		r = alloc_user_handle( key, DesiredAccess, KeyHandle );
		//release( event );
	}

	return r;
}

NTSTATUS NTAPI NtInitializeRegistry(
	BOOLEAN Setup )
{
	dprintf("%d\n", Setup);
	return STATUS_SUCCESS;
}

NTSTATUS check_key_value_info_class( KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass )
{
	switch( KeyValueInformationClass )
	{
	case KeyValueFullInformation:
		break;
	case KeyValuePartialInformation:
		break;
	case KeyValueBasicInformation:
		break;
	case KeyValueFullInformationAlign64:
	case KeyValuePartialInformationAlign64:
		dprintf("not implemented %d\n", KeyValueInformationClass);
		return STATUS_NOT_IMPLEMENTED;
	default:
		return STATUS_INVALID_PARAMETER;
	}
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtQueryValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID KeyValueInformation,
	ULONG KeyValueInformationLength,
	PULONG ResultLength )
{
	unicode_string_t us;
	NTSTATUS r;
	ULONG len;
	regkey_t *key;
	regval_t *val;

	dprintf("%p %p %d %p %lu %p\n", KeyHandle, ValueName, KeyValueInformationClass,
			KeyValueInformation, KeyValueInformationLength, ResultLength );

	r = check_key_value_info_class( KeyValueInformationClass );
	if (r != STATUS_SUCCESS)
		return r;

	r = object_from_handle( key, KeyHandle, KEY_QUERY_VALUE );
	if (r != STATUS_SUCCESS)
		return r;

	r = us.copy_from_user( ValueName );
	if (r != STATUS_SUCCESS)
		return r;

	r = verify_for_write( ResultLength, sizeof *ResultLength );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("%pus\n", &us );

	val = key_find_value( key, &us );
	if (!val)
		return STATUS_OBJECT_NAME_NOT_FOUND;

	r = reg_query_value( val, KeyValueInformationClass, KeyValueInformation,
						 KeyValueInformationLength, len );

	copy_to_user( ResultLength, &len, sizeof len );

	return r;
}

NTSTATUS NTAPI NtSetValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	ULONG TitleIndex,
	ULONG Type,
	PVOID Data,
	ULONG DataSize )
{
	unicode_string_t us;
	regkey_t *key;
	NTSTATUS r;

	dprintf("%p %p %lu %lu %p %lu\n", KeyHandle, ValueName, TitleIndex, Type, Data, DataSize );

	r = object_from_handle( key, KeyHandle, KEY_SET_VALUE );
	if (r != STATUS_SUCCESS)
		return r;

	r = us.copy_from_user( ValueName );
	if (r == STATUS_SUCCESS)
	{
		regval_t *val;

		val = new regval_t( &us, Type, DataSize );
		if (val)
		{
			r = copy_from_user( val->data, Data, DataSize );
			if (r == STATUS_SUCCESS)
			{
				delete_value( key, &us );
				key->values.append( val );
			}
			else
				delete val;
		}
		else
			r = STATUS_NO_MEMORY;
	}

	return r;
}

NTSTATUS NTAPI NtEnumerateValueKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID KeyValueInformation,
	ULONG KeyValueInformationLength,
	PULONG ResultLength )
{
	regkey_t *key;
	ULONG len = 0;
	NTSTATUS r = STATUS_SUCCESS;

	dprintf("%p %lu %u %p %lu %p\n", KeyHandle, Index, KeyValueInformationClass,
			KeyValueInformation, KeyValueInformationLength, ResultLength );

	r = object_from_handle( key, KeyHandle, KEY_QUERY_VALUE );
	if (r != STATUS_SUCCESS)
		return r;

	r = verify_for_write( ResultLength, sizeof *ResultLength );
	if (r != STATUS_SUCCESS)
		return r;

	regval_iter i(key->values);
	for ( ; i && Index; i.next())
		Index--;

	if (!i)
		return STATUS_NO_MORE_ENTRIES;

	r = reg_query_value( i, KeyValueInformationClass, KeyValueInformation,
						 KeyValueInformationLength, len );

	copy_to_user( ResultLength, &len, sizeof len );

	return r;
}

NTSTATUS NTAPI NtDeleteValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName )
{
	unicode_string_t us;
	NTSTATUS r;
	regkey_t *key;

	dprintf("%p %p\n", KeyHandle, ValueName);

	r = us.copy_from_user( ValueName );
	if (r != STATUS_SUCCESS)
		return r;

	r = object_from_handle( key, KeyHandle, KEY_SET_VALUE );
	if (r != STATUS_SUCCESS)
		return r;
	r = delete_value( key, &us );

	return r;
}

NTSTATUS NTAPI NtDeleteKey(
	HANDLE KeyHandle)
{
	NTSTATUS r;
	regkey_t *key = 0;
	r = object_from_handle( key, KeyHandle, DELETE );
	if (r != STATUS_SUCCESS)
		return r;

	key->delkey();

	return r;
}

NTSTATUS NTAPI NtFlushKey(
	HANDLE KeyHandle)
{
	regkey_t *key = 0;
	NTSTATUS r = object_from_handle( key, KeyHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;
	dprintf("flush!\n");
	return r;
}

NTSTATUS NTAPI NtSaveKey(
	HANDLE KeyHandle,
	HANDLE FileHandle)
{
	dprintf("%p %p\n", KeyHandle, FileHandle);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtSaveMergedKeys(
	HANDLE KeyHandle1,
	HANDLE KeyHandle2,
	HANDLE FileHandle)
{
	dprintf("%p %p %p\n", KeyHandle1, KeyHandle2, FileHandle);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG Flags)
{
	dprintf("%p %p %08lx\n", KeyHandle, FileHandle, Flags);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtLoadKey(
	POBJECT_ATTRIBUTES KeyObjectAttributes,
	POBJECT_ATTRIBUTES FileObjectAttributes)
{
	dprintf("%p %p\n", KeyObjectAttributes, FileObjectAttributes);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtUnloadKey(
	POBJECT_ATTRIBUTES KeyObjectAttributes)
{
	dprintf("%p\n", KeyObjectAttributes);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueryOpenSubKeys(
	POBJECT_ATTRIBUTES KeyObjectAttributes,
	PULONG NumberOfKeys)
{
	dprintf("%p %p\n", KeyObjectAttributes, NumberOfKeys);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtReplaceKey(
	POBJECT_ATTRIBUTES NewFileObjectAttributes,
	HANDLE KeyHandle,
	POBJECT_ATTRIBUTES OldFileObjectAttributes)
{
	dprintf("%p %p %p\n", NewFileObjectAttributes,
			KeyHandle, OldFileObjectAttributes);
	return STATUS_NOT_IMPLEMENTED;
}

bool key_info_class_valid( KEY_INFORMATION_CLASS cls )
{
	switch (cls)
	{
	case KeyNodeInformation:
	case KeyBasicInformation:
	case KeyFullInformation:
		return true;
	}
	return false;
}

NTSTATUS NTAPI NtEnumerateKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG KeyInformationLength,
	PULONG ResultLength)
{
	regkey_t *key = 0;
	NTSTATUS r = object_from_handle( key, KeyHandle, KEY_ENUMERATE_SUB_KEYS );
	if (r != STATUS_SUCCESS)
		return r;

	if (ResultLength)
	{
		r = verify_for_write( ResultLength, sizeof *ResultLength );
		if (r != STATUS_SUCCESS)
			return r;
	}

	if (!key_info_class_valid(KeyInformationClass))
		return STATUS_INVALID_INFO_CLASS;

	regkey_t *child = key->get_child( Index );
	if (!child)
		return STATUS_NO_MORE_ENTRIES;

	return child->query( KeyInformationClass, KeyInformation, KeyInformationLength, ResultLength );
}

NTSTATUS NTAPI NtNotifyChangeKey(
	HANDLE KeyHandle,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG NotifyFilter,
	BOOLEAN WatchSubtree,
	PVOID Buffer,
	ULONG BufferLength,
	BOOLEAN Asynchronous)
{
	regkey_t *key = 0;
	NTSTATUS r = object_from_handle( key, KeyHandle, 0 );
	if (r != STATUS_SUCCESS)
		return r;

	dprintf("does nothing...\n");

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtNotifyChangeMultipleKeys(
	HANDLE KeyHandle,
	ULONG Flags,
	POBJECT_ATTRIBUTES KeyObjectAttributes,
	HANDLE EventHandle,
	PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	ULONG NotifyFilter,
	BOOLEAN WatchSubtree,
	PVOID Buffer,
	ULONG BufferLength,
	BOOLEAN Asynchronous)
{
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI NtQueryKey(
	HANDLE KeyHandle,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG KeyInformationLength,
	PULONG ReturnLength)
{
	if (!key_info_class_valid(KeyInformationClass))
		return STATUS_INVALID_INFO_CLASS;

	NTSTATUS r;
	r = verify_for_write( ReturnLength, sizeof *ReturnLength );
	if (r != STATUS_SUCCESS)
		return r;

	regkey_t *key = 0;
	r = object_from_handle( key, KeyHandle, KEY_QUERY_VALUE );
	if (r != STATUS_SUCCESS)
		return r;

	return key->query( KeyInformationClass, KeyInformation, KeyInformationLength, ReturnLength );
}

NTSTATUS regkey_t::query(
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG KeyInformationLength,
	PULONG ReturnLength)
{
	union {
		KEY_BASIC_INFORMATION basic;
		KEY_FULL_INFORMATION full;
	} info;
	NTSTATUS r;

	memset( &info, 0, sizeof info );
	ULONG sz = 0;
	UNICODE_STRING keycls, keyname;
	keyname.Length = 0;
	keyname.Buffer = 0;
	keycls.Length = 0;
	keycls.Buffer = 0;

	switch (KeyInformationClass)
	{
	case KeyBasicInformation:
		query( info.basic, keyname );
		sz = sizeof info.basic + keyname.Length;
		if (sz > KeyInformationLength)
			return STATUS_INFO_LENGTH_MISMATCH;

		r = copy_to_user( KeyInformation, &info, sz );
		if (r != STATUS_SUCCESS)
			break;

		r = copy_to_user( (BYTE*)KeyInformation + FIELD_OFFSET( KEY_BASIC_INFORMATION, Name ), keyname.Buffer, keyname.Length );

		break;

	case KeyFullInformation:
		query( info.full, keycls );
		sz = sizeof info.full + keycls.Length;
		if (sz > KeyInformationLength)
			return STATUS_INFO_LENGTH_MISMATCH;

		r = copy_to_user( KeyInformation, &info, sz );
		if (r != STATUS_SUCCESS)
			break;

		dprintf("keycls = %pus\n", &keycls);
		r = copy_to_user( (BYTE*)KeyInformation + FIELD_OFFSET( KEY_FULL_INFORMATION, Class ), keycls.Buffer, keycls.Length );

		break;

	case KeyNodeInformation:
		dprintf("KeyNodeInformation\n");
	default:
		assert(0);
	}

	if (r == STATUS_SUCCESS)
		copy_to_user( ReturnLength, &sz, sizeof sz );

	return r;
}

regkey_t *build_key( regkey_t *root, unicode_string_t *name )
{
	regkey_t *key;

	key = root;
	bool opened_existing;
	create_parse_key( key, name, opened_existing );

	return key;
}

BYTE hexchar( xmlChar x )
{
	if (x>='0' && x<='9') return x - '0';
	if (x>='A' && x<='F') return x - 'A' + 10;
	if (x>='a' && x<='f') return x - 'a' + 10;
	return 0xff;
}

ULONG hex_to_binary( xmlChar *str, ULONG len, BYTE *buf )
{
	unsigned int i, n;
	BYTE msb, lsb;

	i = 0;
	n = 0;
	while (str[i] && str[i+1])
	{
		msb = hexchar( str[i++] );
		if (msb == 0xff)
			break;
		lsb = hexchar( str[i++] );
		if (lsb == 0xff)
			break;
		if (buf)
			buf[n] = (msb<<4) | lsb;
		//dprintf("%02x ", (msb<<4) | lsb);
		n++;
	}
	//dprintf("\n");
	assert( len == 0 || n <= len );
	return n;
}

void number_to_binary( xmlChar *str, ULONG len, BYTE *buf )
{
	char *valstr = (char*) str;
	ULONG base = 0;
	ULONG val = 0;
	ULONG i;
	BYTE ch;

	if (str[0] == '0' && str[1] == 'x' || str[1] == 'X')
	{
		// hex
		base = 0x10;
		i = 2;
	}
	else if (str[0] == '0')
	{
		// octal
		base = 8;
		i = 1;
	}
	else
	{
		// decimal
		base = 10;
		i = 0;
	}

	while (str[i])
	{
		ch = hexchar(str[i]);
		if (ch >= base)
			die("invalid registry value %s\n", valstr);
		val *= base;
		val += ch;
		i++;
	}

	*((ULONG*) buf) = val;
}

void dump_val( regval_t *val )
{
	ULONG i;

	dprintf("%pus = ", &val->name );
	switch( val->type )
	{
	case 7:
		for (i=0; i<val->size; i+=2)
		{
			if ((val->size - i)>1 && !val->data[i+1] &&
				val->data[i] >= 0x20 && val->data[i]<0x80)
			{
				fprintf(stderr,"%c", val->data[i]);
				continue;
			}
			fprintf(stderr,"\\%02x%02x", val->data[i+1], val->data[i]);
		}
		fprintf(stderr,"\n");
		break;
	case 1:
	case 2:
		dprintf("%pws\n", val->data );
		break;
	}
}

void load_reg_key( regkey_t *parent, xmlNode *node )
{
	xmlAttr *e;
	xmlChar *contents = NULL;
	const char *type = NULL;
	const char *keycls = NULL;
	unicode_string_t name, data;
	xmlNode *n;
	regval_t *val;
	ULONG size;
	regkey_t *key;

	if (!node->name[0] || node->name[1])
		return;

	for ( e = node->properties; e; e = e->next )
	{
		if (!strcmp( (const char*)e->name, "n"))
			contents = xmlNodeGetContent( (xmlNode*) e );
		else if (!strcmp( (const char*)e->name, "t"))
			type = (const char*) xmlNodeGetContent( (xmlNode*) e );
		else if (!strcmp( (const char*)e->name, "c"))
			keycls = (const char*) xmlNodeGetContent( (xmlNode*) e );
	}

	if (!contents)
		return;

	name.copy( contents );

	switch (node->name[0])
	{
	case 'x': // value stored as hex
		// default type is binary
		if (type == NULL)
			type = "3";
		contents = xmlNodeGetContent( node );
		size = hex_to_binary( contents, 0, NULL );
		val = new regval_t( &name, atoi(type), size );
		hex_to_binary( contents, size, val->data );
		parent->values.append( val );
		break;

	case 'n': // number
		// default type is REG_DWORD
		if (type == NULL)
			type = "4";
		contents = xmlNodeGetContent( node );
		size = sizeof (ULONG);
		val = new regval_t( &name, atoi(type), size );
		number_to_binary( contents, size, val->data );
		parent->values.append( val );
		break;

	case 's': // value stored as a string
		// default type is REG_SZ
		if (type == NULL)
			type = "1";

		data.copy( xmlNodeGetContent( node ) );
		val = new regval_t( &name, atoi(type), data.Length + 2 );
		memcpy( val->data, data.Buffer, data.Length );
		memset( val->data + data.Length, 0, 2 );
		parent->values.append( val );
		break;

	case 'k': // key
		key = build_key( parent, &name );
		key->cls.copy( keycls );
		for (n = node->children; n; n = n->next)
			load_reg_key( key, n );

		break;
	}
}

void init_registry( void )
{
	xmlDoc *doc;
	xmlNode *root;
	const char *regfile = "reg.xml";
	UNICODE_STRING name;

	memset( &name, 0, sizeof name );
	root_key = new regkey_t( NULL, &name );

	doc = xmlReadFile( regfile, NULL, 0 );
	if (!doc)
		die("failed to load registry (%s)\n", regfile );

	root = xmlDocGetRootElement( doc );
	load_reg_key( root_key, root );

	xmlFreeDoc( doc );
}

void free_registry( void )
{
	release( root_key );
	root_key = NULL;
}
