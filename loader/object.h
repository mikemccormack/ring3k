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

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "list.h"
#include "unicode.h"

class object_t;

typedef list_anchor<object_t, 0> object_list_t;
typedef list_element<object_t> object_entry_t;
typedef list_iter<object_t, 0> object_iter_t;

class object_t {
	friend class list_anchor<object_t, 0>;
	friend class list_element<object_t>;
	friend class list_iter<object_t, 0>;
	object_entry_t entry[1];
	ULONG refcount;
public:
	ULONG attr;
	unicode_string_t name;
public:
	object_t();
	virtual bool access_allowed( ACCESS_MASK required, ACCESS_MASK handle );
	virtual ~object_t();
	static bool check_access( ACCESS_MASK required, ACCESS_MASK handle, ACCESS_MASK read, ACCESS_MASK write, ACCESS_MASK all );
	static void addref( object_t *obj );
	static void release( object_t *obj );
};

class object_factory
{
protected:
	virtual NTSTATUS alloc_object(object_t** obj) = 0;
public:
	NTSTATUS create(
		PHANDLE Handle,
		ACCESS_MASK AccessMask,
		POBJECT_ATTRIBUTES ObjectAttributes);
	virtual ~object_factory();
};

class watch_t;

typedef list_anchor<watch_t, 0> watch_list_t;
typedef list_element<watch_t> watch_entry_t;
typedef list_iter<watch_t, 0> watch_iter_t;

class watch_t {
public:
	watch_entry_t entry[1];
	virtual void notify() = 0;
	virtual ~watch_t();
};

class sync_object_t : public object_t {
private:
	watch_list_t watchers;
public:
	sync_object_t();
	virtual ~sync_object_t();
	virtual BOOLEAN is_signalled( void ) = 0;
	virtual BOOLEAN satisfy( void );
	void add_watch( watch_t* watcher );
	void remove_watch( watch_t* watcher );
	void notify_watchers();
};

class object_info_t {
public:
	object_t *object;
	ACCESS_MASK access;
};

class handle_table_t {
	static const unsigned int max_handles = 0x100;

	//int num_objects;
	object_info_t info[max_handles];
protected:
	static HANDLE index_to_handle( ULONG index );
	static ULONG handle_to_index( HANDLE handle );
public:
	~handle_table_t();
	void free_all_handles();
	HANDLE alloc_handle( object_t *obj, ACCESS_MASK access );
	NTSTATUS free_handle( HANDLE handle );
	NTSTATUS object_from_handle( object_t*& obj, HANDLE handle, ACCESS_MASK access );
};

static inline void addref( object_t *obj )
{
	object_t::addref( obj );
}

static inline void release( object_t *obj )
{
	object_t::release( obj );
}

object_t *create_directory_object( PCWSTR name );
NTSTATUS name_object( object_t *obj, const OBJECT_ATTRIBUTES *oa );
NTSTATUS get_named_object( object_t **out, const OBJECT_ATTRIBUTES *oa );
NTSTATUS find_object_by_name( object_t **out, const OBJECT_ATTRIBUTES *oa );

template<typename T> NTSTATUS object_from_handle(T*& out, HANDLE handle, ACCESS_MASK access);

#endif //__OBJECT_H__
