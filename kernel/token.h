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

#ifndef __TOKEN_H__
#define __TOKEN_H__

class token_privileges_t;
class sid_t;
class acl_t;
class sid_and_attributes_t;
class token_groups_t;

class token_t : public object_t {
public:
	virtual ~token_t() = 0;
	virtual token_privileges_t& get_privs() = 0;
	virtual sid_t& get_owner() = 0;
	virtual sid_and_attributes_t& get_user() = 0;
	virtual sid_t& get_primary_group() = 0;
	virtual token_groups_t& get_groups() = 0;
	virtual acl_t& get_default_dacl() = 0;
	virtual NTSTATUS adjust(token_privileges_t& privs) = 0;
};

#endif // __TOKEN_H__
