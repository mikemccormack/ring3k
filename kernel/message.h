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

#ifndef __RING3K_MESSAGE__
#define __RING3K_MESSAGE__

#include "ntwin32.h"
#include "win.h"

class message_tt
{
public:
	virtual ULONG get_size() const = 0;
	virtual NTSTATUS copy_to_user( void *ptr ) const = 0;
	virtual ULONG get_callback_num() const = 0;
	virtual void set_window_info( window_tt *win ) = 0;
	virtual ~message_tt() {}
};

class wmessage_ptr_tt : public message_tt
{
protected:
	struct pointer_info_tt
	{
		ULONG sz;
		ULONG x;
		ULONG count;
		PVOID kernel_address;
		ULONG adjust_info_ofs;
		BOOL  no_adjust;
	};
	pointer_info_tt &pi;
public:
	wmessage_ptr_tt( pointer_info_tt& pointer_info );
	virtual ULONG get_size() const = 0;
	virtual NTSTATUS copy_to_user( void *ptr ) const = 0;
	virtual ULONG get_callback_num() const = 0;
	virtual void set_window_info( window_tt *win ) = 0;
};

class create_message_tt : public wmessage_ptr_tt
{
protected:
	struct create_client_data : public pointer_info_tt
	{
		PVOID wininfo;
		ULONG msg;
		WPARAM wparam;
		BOOL cs_nonnull;
		NTCREATESTRUCT cs;
		PVOID wndproc;
		ULONG (CALLBACK *func)(PVOID/*PWININFO*/,ULONG,WPARAM,PVOID/*LPCREATESTRUCT*/,PVOID);
	} info;
	const UNICODE_STRING& cls;
	const UNICODE_STRING& name;
public:
	create_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name );
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

class nccreate_message_tt : public create_message_tt
{
public:
	nccreate_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name );
};

class getminmaxinfo_tt : public message_tt
{
	NTMINMAXPACKEDINFO info;
public:
	getminmaxinfo_tt();
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

class nccalcsize_message_tt : public message_tt
{
	NTNCCALCSIZEPACKEDINFO info;
public:
	nccalcsize_message_tt();
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

class basicmsg_tt : public message_tt
{
protected:
	NTSIMPLEMESSAGEPACKEDINFO info;
public:
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

class showwindowmsg_tt : public basicmsg_tt
{
public:
	showwindowmsg_tt( bool show );
};

#endif // __RING3K_MESSAGE__
