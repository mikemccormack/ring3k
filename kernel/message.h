/*
 * nt loader
 *
 * Copyright 2006-2009 Mike McCormack
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

// a message sent to a window via a callback
// the message information needs to be copied to user space
class message_tt
{
public:
	virtual ULONG get_size() const = 0;
	virtual NTSTATUS copy_to_user( void *ptr ) const = 0;
	virtual ULONG get_callback_num() const = 0;
	virtual void set_window_info( window_tt *win ) = 0;
	virtual ~message_tt() {}
};

// message with the piece of packed information to be sent
// one fixed size piece of data to send named 'info'
template<class Pack> class generic_message_tt : public message_tt
{
public:
	Pack info;
public:
	generic_message_tt();
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const = 0;
	virtual void set_window_info( window_tt *win );
};

// WM_CREATE and WM_NCCREATE
class create_message_tt : public generic_message_tt<NTCREATEPACKEDINFO>
{
protected:
	const UNICODE_STRING& cls;
	const UNICODE_STRING& name;
public:
	create_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name );
	virtual ULONG get_callback_num() const;
};

class nccreate_message_tt : public create_message_tt
{
public:
	nccreate_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name );
};

// WM_GETMINMAXINFO
class getminmaxinfo_tt : public generic_message_tt<NTMINMAXPACKEDINFO>
{
public:
	getminmaxinfo_tt();
	virtual ULONG get_callback_num() const;
};

// WM_NCCALCSIZE
class nccalcsize_message_tt : public generic_message_tt<NTNCCALCSIZEPACKEDINFO>
{
public:
	nccalcsize_message_tt();
	virtual ULONG get_callback_num() const;
};

// basic messages where lparam and wparam aren't pointers
class basicmsg_tt : public generic_message_tt<NTSIMPLEMESSAGEPACKEDINFO>
{
public:
	basicmsg_tt();
	basicmsg_tt( INT message );
	virtual ULONG get_callback_num() const;
};

class showwindowmsg_tt : public basicmsg_tt
{
public:
	showwindowmsg_tt( bool show );
};

// WM_WINDOWPOSCHANGING and WM_WINDOWPOSCHANGED
class winposchange_tt : public generic_message_tt<NTPOSCHANGINGPACKEDINFO>
{
	WINDOWPOS& pos;
public:
	winposchange_tt( ULONG message, WINDOWPOS& _pos );
	virtual ULONG get_callback_num() const = 0;
};

class winposchanging_tt : public winposchange_tt
{
public:
	winposchanging_tt( WINDOWPOS& _pos );
	virtual ULONG get_callback_num() const;
};

class winposchanged_tt : public winposchange_tt
{
public:
	winposchanged_tt( WINDOWPOS& _pos );
	virtual ULONG get_callback_num() const;
};

// WM_ACTIVATEAPP
class appactmsg_tt : public basicmsg_tt
{
public:
	appactmsg_tt( UINT type );
};

// WM_NCACTIVATE
class ncactivate_tt : public basicmsg_tt
{
public:
	ncactivate_tt();
};

// WM_ACTIVATE
class activate_tt : public basicmsg_tt
{
public:
	activate_tt();
};

// WM_SETFOCUS
class setfocusmsg_tt : public basicmsg_tt
{
public:
	setfocusmsg_tt();
};

class ncpaintmsg_tt : public basicmsg_tt
{
public:
	ncpaintmsg_tt();
};

class erasebkgmsg_tt : public basicmsg_tt
{
public:
	erasebkgmsg_tt();
};

class keyup_msg_tt : public basicmsg_tt
{
public:
	keyup_msg_tt( UINT key );
};

class keydown_msg_tt : public basicmsg_tt
{
public:
	keydown_msg_tt( UINT key );
};

#endif // __RING3K_MESSAGE__
