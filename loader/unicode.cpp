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
#include "unicode.h"
#include "ntcall.h"
#include "debug.h"

UINT strlenW(LPCWSTR str)
{
	UINT n = 0;
	while(str[n]) n++;
	return n;
}

LPWSTR strcpyW( LPWSTR dest, LPCWSTR src )
{
	while ((*dest++ = *src++))
		;
	return dest;
}

LPWSTR strcatW( LPWSTR dest, LPCWSTR src )
{
	while (*dest) dest++;
	while ((*dest++ = *src++))
		;
	return dest;
}

unicode_string_t::unicode_string_t() :
	buf(0)
{
	Buffer = 0;
	Length = 0;
	MaximumLength = 0;
}

void unicode_string_t::set( UNICODE_STRING& us )
{
	clear();
	Buffer = us.Buffer;
	Length = us.Length;
	MaximumLength = us.MaximumLength;
}

void unicode_string_t::set( PCWSTR str )
{
	clear();
	Buffer = const_cast<PWSTR>( str );
	Length = strlenW( str ) * 2;
	MaximumLength = 0;
}

NTSTATUS unicode_string_t::copy_from_user(PUNICODE_STRING ptr)
{
	NTSTATUS r = ::copy_from_user( static_cast<UNICODE_STRING*>(this), ptr, sizeof (UNICODE_STRING) );
	if (r != STATUS_SUCCESS)
		return r;
	return copy_wstr_from_user();
}

NTSTATUS unicode_string_t::copy_wstr_from_user()
{
	if (buf)
		delete buf;
	buf = 0;
	if (Buffer)
	{
		buf = new WCHAR[ Length/2 ];
		if (!buf)
			return STATUS_NO_MEMORY;
		NTSTATUS r = ::copy_from_user( buf, Buffer, Length );
		if (r != STATUS_SUCCESS)
		{
			delete buf;
			buf = 0;
			return r;
		}
		Buffer = buf;
	}
	return STATUS_SUCCESS;
}

NTSTATUS unicode_string_t::copy_wstr_from_user( PWSTR String, ULONG _Length )
{
	Buffer = String;
	Length = _Length;
	return copy_wstr_from_user();
}

NTSTATUS unicode_string_t::copy( const UNICODE_STRING* ptr )
{
	clear();
	Length = ptr->Length;
	MaximumLength = ptr->MaximumLength;
	if (ptr->Buffer)
	{
		buf = new WCHAR[ Length ];
		if (!buf)
			return STATUS_NO_MEMORY;
		memcpy( buf, ptr->Buffer, Length );
		Buffer = buf;
	}
	else
		Buffer = 0;
	return STATUS_SUCCESS;
}

// returned size include nul terminator
ULONG unicode_string_t::utf8_to_wchar( const unsigned char *str, ULONG len, WCHAR *buf )
{
	unsigned int i, n;

	i = 0;
	n = 0;
	while (str[i])
	{
		if ((str[i]&0x80) == 0)
		{
			if (buf)
				buf[n] = str[i];
			n++;
			i++;
			continue;
		}
		if ((str[i]&0xc8) == 0xc0 &&
			(str[i+1]&0xc0) == 0x80)
		{
			if (buf)
				buf[n] = ((str[i]&0x3f)<<6) | (str[i+1]&0x3f);
			i+=2;
			n++;
			continue;
		}
		if ((str[i]&0xf0) == 0xe0 &&
			(str[i+1]&0xc0) == 0x80 &&
			(str[i+2]&0xc0) == 0x80)
		{
			if (buf)
				buf[n] = ((str[i]&0x3f)<<12) | ((str[i+1]&0x3f)<<6) | (str[i+2]&0x3f);
			i+=3;
			n++;
			continue;
		}
		dprintf("invalid utf8 string %02x %02x %02x\n", str[i], str[i+1], str[i+2]);
		break;
	}
	if (buf)
		buf[n] = 0;

	assert( len == 0 || n <= len );

	return n;
}

NTSTATUS unicode_string_t::copy( const char *str )
{
	const unsigned char *ustr = reinterpret_cast<const unsigned char*>(str);
	return copy( ustr );
}

void unicode_string_t::clear()
{
	if (buf)
		delete buf;
	buf = 0;
	Length = 0;
	MaximumLength = 0;
	Buffer = 0;
}

NTSTATUS unicode_string_t::copy( const unsigned char *ustr )
{
	clear();
	if (!ustr)
		return STATUS_SUCCESS;
	ULONG len = utf8_to_wchar( ustr, 0, 0 );
	Length = len * sizeof (WCHAR);
	buf = new WCHAR[ len + 1 ];
	if (!buf)
		return STATUS_NO_MEMORY;
	utf8_to_wchar( ustr, len, buf );
	Buffer = buf;
	MaximumLength = 0;
	return STATUS_SUCCESS;
}

NTSTATUS unicode_string_t::copy( PCWSTR str )
{
	clear();
	ULONG n = 0;
	while (str[n])
		n++;
	buf = new WCHAR[n];
	if (!buf)
		return STATUS_NO_MEMORY;
	Length = n*2;
	MaximumLength = Length;
	Buffer = buf;
	memcpy( Buffer, str, Length );
	return STATUS_SUCCESS;
}

bool unicode_string_t::is_equal( PUNICODE_STRING ptr )
{
	if (Length != ptr->Length)
		return false;

	return !memcmp(ptr->Buffer, Buffer, Length);
}

unicode_string_t::~unicode_string_t()
{
	clear();
}

unicode_string_t& unicode_string_t::operator=(const unicode_string_t& in)
{
	// free the old buffer
	if (buf)
		delete buf;

	// copy the other string
	Length = in.Length;
	MaximumLength = in.MaximumLength;
	if (in.buf)
	{
		buf = new WCHAR[ Length ];
		memcpy( buf, in.buf, Length );
		Buffer = buf;
	}
	else
		Buffer = 0;
	return *this;
}

// returns TRUE if strings are the same
bool unicode_string_t::compare( PUNICODE_STRING b, BOOLEAN case_insensitive )
{
	if (Length != b->Length)
		return FALSE;
	if (!case_insensitive)
		return (0 == memcmp( Buffer, b->Buffer, Length ));

	// FIXME: should use windows case table
	for ( ULONG i = 0; i < Length/2; i++ )
	{
		WCHAR ai, bi;
		ai = tolower( Buffer[i] );
		bi = tolower( b->Buffer[i] );
		if (ai == bi)
			continue;
		return FALSE;
	}
	return TRUE;
}

object_attributes_t::object_attributes_t()
{
	POBJECT_ATTRIBUTES oa = static_cast<OBJECT_ATTRIBUTES*>( this );
	memset( oa, 0, sizeof *oa );
}

object_attributes_t::~object_attributes_t()
{
}

NTSTATUS object_attributes_t::copy_from_user( POBJECT_ATTRIBUTES oa )
{
	NTSTATUS r;

	r = ::copy_from_user( static_cast<OBJECT_ATTRIBUTES*>( this ), oa, sizeof *oa );
	if (r != STATUS_SUCCESS)
		return r;

	if (Length != sizeof (OBJECT_ATTRIBUTES))
		return STATUS_INVALID_PARAMETER;

	if (ObjectName)
	{
		r = us.copy_from_user( ObjectName );
		if (r != STATUS_SUCCESS)
			return r;
		ObjectName = &us;
	}

	return STATUS_SUCCESS;
}

object_attributes_t& object_attributes_t::operator=(const object_attributes_t& in)
{
	Length = in.Length;
	RootDirectory = in.RootDirectory;
	Attributes = in.Attributes;
	SecurityDescriptor = in.SecurityDescriptor;
	SecurityQualityOfService = in.SecurityQualityOfService;

	us = in.us;
	if (in.ObjectName)
		ObjectName = &us;
	else
		ObjectName = 0;

	return *this;
}

object_attributes_t::object_attributes_t( const WCHAR *str )
{
	us.copy( str );
	Length = sizeof (OBJECT_ATTRIBUTES);
	RootDirectory = 0;
	Attributes = OBJ_CASE_INSENSITIVE;
	SecurityDescriptor = 0;
	SecurityQualityOfService = 0;
	ObjectName = &us;
}
