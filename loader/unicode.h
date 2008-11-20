/*
 * UNICODE_STRING helper
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

#ifndef __UNICODE_H__
#define __UNICODE_H__

#include "winternl.h"

class unicode_string_t : public UNICODE_STRING
{
	WCHAR *buf;
protected:
	NTSTATUS copy_wstr_from_user();
	ULONG utf8_to_wchar( const unsigned char *str, ULONG len, WCHAR *buf );
public:
	explicit unicode_string_t();
	void set( PCWSTR str );
	void set( const wchar_t* str ) { set( (PCWSTR) str ); }
	void set( UNICODE_STRING& us );
	NTSTATUS copy_from_user(PUNICODE_STRING ptr);
	NTSTATUS copy( const UNICODE_STRING* ptr );
	NTSTATUS copy( const char *ptr );
	NTSTATUS copy( const unsigned char *ptr );
	NTSTATUS copy( PCWSTR str );
	NTSTATUS copy( const wchar_t* str ) {return copy( (PCWSTR) str );}
	bool is_equal( PUNICODE_STRING ptr );
	bool compare( PUNICODE_STRING b, BOOLEAN case_insensitive );
	~unicode_string_t();
	unicode_string_t& operator=(const unicode_string_t& in);
	void clear();
	NTSTATUS copy_wstr_from_user( PWSTR String, ULONG Length );
};

class object_attributes_t : public OBJECT_ATTRIBUTES
{
	unicode_string_t us;
public:
	explicit object_attributes_t();
	explicit object_attributes_t( const WCHAR *str );
	~object_attributes_t();
	NTSTATUS copy_from_user( POBJECT_ATTRIBUTES oa );
	object_attributes_t& operator=(const object_attributes_t& in);
};

UINT strlenW( LPCWSTR str );
LPWSTR strcpyW( LPWSTR dest, LPCWSTR src );
LPWSTR strcatW( LPWSTR dest, LPCWSTR src );

#endif // __UNICODE_H__
