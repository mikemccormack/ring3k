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

#ifndef __RING3K_WIN_H__
#define __RING3K_WIN_H__

struct window_tt;
class wndcls_tt;
class message_tt;

typedef list_anchor<wndcls_tt, 0> wndcls_list_tt;
typedef list_element<wndcls_tt> wndcls_entry_tt;
typedef list_iter<wndcls_tt, 0> wndcls_iter_tt;

class wndcls_tt
{
	friend class list_anchor<wndcls_tt, 0>;
	friend class list_iter<wndcls_tt, 0>;
	wndcls_entry_tt entry[1];
	unicode_string_t name;
	unicode_string_t menu;
	NTWNDCLASSEX info;
	ATOM atom;
	ULONG refcount;
public:
	wndcls_tt( NTWNDCLASSEX& ClassInfo, const UNICODE_STRING& ClassName, const UNICODE_STRING& MenuName, ATOM a );
	static wndcls_tt* from_name( const UNICODE_STRING& wndcls_name );
	ATOM get_atom() const {return atom;}
	const unicode_string_t& get_name() const {return name;}
	void addref() {refcount++;}
	void release() {refcount--;}
	PVOID get_wndproc() const { return info.WndProc; }
};

// shared memory structure!!
// see http://winterdom.com/dev/ui/wnd.html
struct window_tt
{
	HANDLE handle;
	thread_t *thread; // ULONG unk1;
	ULONG unk2;
	ULONG unk3;
	window_tt* self;
	ULONG dwFlags;
	ULONG unk6;
	ULONG exstyle;
	ULONG style;
	PVOID hInstance;
	ULONG unk10;
	window_tt* next;
	window_tt* parent;
	window_tt* first_child;
	window_tt* owner;
	RECT rcWnd;
	RECT rcClient;
	void* wndproc;
	void* wndcls;
	ULONG unk25;
	ULONG unk26;
	ULONG unk27;
	ULONG unk28;
	union {
		ULONG dwWndID;
		void* Menu;
	} id;
	ULONG unk30;
	ULONG unk31;
	ULONG unk32;
	PWSTR pText;
	ULONG dwWndBytes;
	ULONG unk35;
	ULONG unk36;
	ULONG wlUserData;
	ULONG wlWndExtra[1];

public:
	window_tt( thread_t* t, wndcls_tt* wndcls, unicode_string_t& name, ULONG _style, ULONG _exstyle,
		 LONG x, LONG y, LONG width, LONG height, PVOID instance );
	NTSTATUS send( message_tt& msg );
	void *get_wndproc() { return wndproc; }
	void *get_wininfo();
};

// system wide callback functions registered with kernel by user32.dll
extern PVOID g_funcs[9];
extern PVOID g_funcsW[20];
extern PVOID g_funcsA[20];

window_tt *window_from_handle( HANDLE handle );

#endif // __RING3K_WIN_H__
