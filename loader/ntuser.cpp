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
#include <stdio.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "section.h"
#include "objdir.h"
#include "ntuser.h"
#include "ntgdi.h"
#include "mem.h"
#include "debug.h"

class window_tt;
class wndcls_tt;

typedef list_anchor<wndcls_tt, 0> wndcls_list_tt;
typedef list_element<wndcls_tt> wndcls_entry_tt;
typedef list_iter<wndcls_tt, 0> wndcls_iter_tt;

wndcls_list_tt wndcls_list;

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

class message_tt
{
public:
	virtual ULONG get_size() const = 0;
	virtual NTSTATUS copy_to_user( void *ptr ) const = 0;
	virtual ULONG get_callback_num() const = 0;
	virtual void set_window_info( window_tt *win ) = 0;
	virtual ~message_tt() {}
};

class window_tt
{
	void *shared_mem;
	thread_t *thread;
	wndcls_tt *cls;
	unicode_string_t name;
	ULONG style;
	ULONG exstyle;
	LONG x;
	LONG y;
	LONG width;
	LONG height;
	window_tt* parent;
public:
	window_tt( void *winshm, thread_t* t, wndcls_tt* wndcls, unicode_string_t& name, ULONG _style, ULONG _exstyle,
		 LONG x, LONG y, LONG width, LONG height );
	NTSTATUS send( message_tt& msg );
	void *get_shared_address() {return shared_mem;}
	void *get_wndproc() { return cls->get_wndproc(); }
	void *get_wininfo();
};

ULONG NTAPI NtUserGetThreadState( ULONG InfoClass )
{
	switch (InfoClass)
	{
	case 0: // GetFocus
	case 1: // GetActiveWindow
	case 2: // GetCapture
	case 5: // GetInputState
	case 6: // GetCursor
	case 8: // used in PeekMessageW
	case 9: // GetMessageExtraInfo
	case 0x0a: // used in InSendMessageEx
	case 0x0b: // GetMessageTime
	case 0x0c: // ?
		return 0;
	case 0x10: // ?
		return 0;
	case 0x11: // sets TEB->Win32ThreadInfo for the current thread
		return 1;
	default:
		dprintf("%ld\n", InfoClass );
	}
	return 0;
}

static section_t *user_shared_section = 0;
static const ULONG user_shared_mem_size = 0x20000;
static void *user_shared_mem = 0;

// quick hack for the moment
void **find_free_user_shared()
{
	ULONG i;

	//use top half of shared memory
	ULONG sz = user_shared_mem_size - 0x10000;
	void** x = (void**)user_shared_mem + 0x10000/sizeof (void**);

	// use blocks of 0x100 bytes
	for (i=0; i<sz; i += 0x40)
	{
		if (x[i] == NULL)
			return &x[i];
	}
	return NULL;
}

void *init_user_shared_memory()
{
	// read/write for the kernel and read only for processes
	if (!user_shared_mem)
	{
		LARGE_INTEGER sz;
		NTSTATUS r;

		sz.QuadPart = user_shared_mem_size;
		r = create_section( &user_shared_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return 0;

		user_shared_mem = (BYTE*) user_shared_section->get_kernel_address();

		// create the window stations directory too
		create_directory_object( (PWSTR) L"\\Windows\\WindowStations" );
	}

	dprintf("user_shared_mem at %p\n", user_shared_mem );

	return user_shared_mem;
}

class ntusershm_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void ntusershm_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	fprintf(stderr, "%04lx: accessed ushm[%04lx] from %08lx\n", current->trace_id(), ofs, eip);
}

static ntusershm_tracer ntusershm_trace;

NTSTATUS NTAPI NtUserProcessConnect(HANDLE Process, PVOID Buffer, ULONG BufferSize)
{
	union {
		USER_PROCESS_CONNECT_INFO win2k;
		USER_PROCESS_CONNECT_INFO_XP winxp;
	} info;
	const ULONG version = 0x50000;
	NTSTATUS r;

	process_t *proc = 0;
	r = object_from_handle( proc, Process, 0 );
	if (r < STATUS_SUCCESS)
		return r;

	// check if we're already connected
	r = win32k_process_init( proc );
	if (r < STATUS_SUCCESS)
		return r;

	assert( proc->win32k_info );
	BYTE*& user_shared_mem = proc->win32k_info->user_shared_mem;
	if (user_shared_mem)
		return STATUS_SUCCESS;

	dprintf("%p %p %lu\n", Process, Buffer, BufferSize);
	if (BufferSize != sizeof info.winxp && BufferSize != sizeof info.win2k)
	{
		dprintf("buffer size wrong %ld (not WinXP or Win2K?)\n", BufferSize);
		return STATUS_UNSUCCESSFUL;
	}

	r = copy_from_user( &info, Buffer, BufferSize );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	if (info.winxp.Version != version)
	{
		dprintf("version wrong %08lx %08lx\n", info.winxp.Version, version);
		return STATUS_UNSUCCESSFUL;
	}

	// map the user shared memory block into the process's memory
	if (!init_user_shared_memory())
		return STATUS_UNSUCCESSFUL;

	r = user_shared_section->mapit( proc->vm, user_shared_mem, 0,
					MEM_COMMIT, PAGE_READONLY );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	if (0) proc->vm->set_tracer( user_shared_mem, ntusershm_trace );

	info.win2k.Ptr[0] = (void*)user_shared_mem;
	info.win2k.Ptr[1] = (void*)0xbee20000;
	info.win2k.Ptr[2] = (void*)0xbee30000;
	info.win2k.Ptr[3] = (void*)0xbee40000;

	dprintf("user shared at %p\n", user_shared_mem);

	r = copy_to_user( Buffer, &info, BufferSize );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

PVOID g_funcs[9];
PVOID g_funcsW[20];
PVOID g_funcsA[20];

// Funcs array has 9 function pointers
// FuncsW array has 20 function pointers
// FuncsA array has 20 function pointers
// Base is the Base address of the DLL containing the functions
BOOLEAN NTAPI NtUserInitializeClientPfnArrays(
	PVOID Funcs,
	PVOID FuncsW,
	PVOID FuncsA,
	PVOID Base)
{
	NTSTATUS r;

	r = copy_from_user( &g_funcs, Funcs, sizeof g_funcs );
	if (r < 0)
		return r;
	r = copy_from_user( &g_funcsW, FuncsW, sizeof g_funcsW );
	if (r < 0)
		return r;
	r = copy_from_user( &g_funcsA, FuncsA, sizeof g_funcsA );
	if (r < 0)
		return r;
	return 0;
}

BOOLEAN NTAPI NtUserInitialize(ULONG u_arg1, ULONG u_arg2, ULONG u_arg3)
{
	dprintf("%08lx %08lx %08lx\n", u_arg1, u_arg2, u_arg3);
	return TRUE;
}

ULONG NTAPI NtUserCallNoParam(ULONG Index)
{
	dprintf("%lu\n", Index);
	switch (Index)
	{
	case 0:
		return 0; // CreateMenu
	case 1:
		return 0; // CreatePopupMenu
	case 2:
		return 0; // DestroyCaret
	case 3:
		return 0; // ?
	case 4:
		return 0; // GetInputDesktop
	case 5:
		return 0; // GetMessagePos
	case 6:
		return 0; // ?
	case 7:
		return 0xfeed0007;
	case 8:
		return 0; // ReleaseCapture
	case 0x0a:
		return 0; // EndDialog?
	case 0x12:
		return 0; // ClientThreadSetup?
	case 0x15:
		return 0; // MsgWaitForMultipleObjects
	default:
		return FALSE;
	}
}

BOOLEAN NtReleaseDC( HANDLE hdc )
{
	return win32k_manager->release_dc( hdc );
}

ULONG NTAPI NtUserCallOneParam(ULONG Param, ULONG Index)
{
	dprintf("%lu (%08lx)\n", Index, Param);
	switch (Index)
	{
	case 0x16: // BeginDeferWindowPos
		return TRUE;
	case 0x17: // WindowFromDC
		return TRUE;
	case 0x18: // AllowSetForegroundWindow
		return TRUE;
	case 0x19: // used by CreateIconIndirect
		return TRUE;
	case 0x1a: // used by DdeUnitialize
		return TRUE;
	case 0x1b: // used by MsgWaitForMultipleObjectsEx
		return TRUE;
	case 0x1c: // EnumClipboardFormats
		return TRUE;
	case 0x1d: // used by MsgWaitForMultipleObjectsEx
		return TRUE;
	case 0x1e: // GetKeyboardLayout
		return TRUE;
	case 0x1f: // GetKeyboardType
		return TRUE;
	case 0x20: // GetQueueStatus
		return TRUE;
	case 0x21: // SetLockForegroundWindow
		return TRUE;
	case 0x22: // LoadLocalFonts, used by LoadRemoteFonts
		return TRUE;
	case 0x23: // ?
		return TRUE;
	case 0x24: // MessageBeep
		return TRUE;
	case 0x25: // used by SoftModalMessageBox
		return TRUE;
	case 0x26: // PostQuitMessage
		return TRUE;
	case 0x27: // RealizeUserPalette
		return TRUE;
	case 0x28: // used by ClientThreadSetup
		return TRUE;
	case 0x29: // used by ReleaseDC + DeleteDC (deref DC?)
		return NtReleaseDC( (HANDLE) Param );
	case 0x2a: // ReplyMessage
		return TRUE;
	case 0x2b: // SetCaretBlinkTime
		return TRUE;
	case 0x2c: // SetDoubleClickTime
		return TRUE;
	case 0x2d: // ShowCursor
		return TRUE;
	case 0x2e: // StartShowGlass
		return TRUE;
	case 0x2f: // SwapMouseButton
		return TRUE;
	case 0x30: // SetMessageExtraInfo
		return TRUE;
	case 0x31: // used by UserRegisterWowHandlers
		return TRUE;
	case 0x33: // GetProcessDefaultLayout
		return TRUE;
	case 0x34: // SetProcessDefaultLayout
		return TRUE;
	case 0x37: // GetWinStationInfo
		return TRUE;
	case 0x38: // ?
		return TRUE;
	default:
		return FALSE;
	}
}

// should be PASCAL calling convention?
ULONG NTAPI NtUserCallTwoParam(ULONG Param2, ULONG Param1, ULONG Index)
{
	switch (Index)
	{
	case 0x53:  // EnableWindow
		dprintf("EnableWindow (%08lx, %08lx)\n", Param1, Param2);
		break;
	case 0x55:  // ShowOwnedPopups
		dprintf("ShowOwnedPopups (%08lx, %08lx)\n", Param1, Param2);
		break;
	case 0x56:  // SwitchToThisWindow
		dprintf("SwitchToThisWindow (%08lx, %08lx)\n", Param1, Param2);
		break;
	case 0x57:  // ValidateRgn
		dprintf("ValidateRgn (%08lx, %08lx)\n", Param1, Param2);
		break;
	default:
		dprintf("%lu (%08lx, %08lx)\n", Index, Param1, Param2);
		break;
	}
	return TRUE;
}

// returns a handle to the thread's desktop
HANDLE NTAPI NtUserGetThreadDesktop(
	ULONG ThreadId,
	ULONG u_arg2)
{
	dprintf("%lu %lu\n", ThreadId, u_arg2);
	return (HANDLE) 0xde5;
}

HANDLE NTAPI NtUserFindExistingCursorIcon(PUNICODE_STRING Library, PUNICODE_STRING str2, PVOID p_arg3)
{
	ULONG index;

	dprintf("%p %p %p\n", Library, str2, p_arg3);

	unicode_string_t us;
	NTSTATUS r;

	r = us.copy_from_user( Library );
	if (r == STATUS_SUCCESS)
		dprintf("Library=\'%pus\'\n", &us);

	r = us.copy_from_user( str2 );
	if (r == STATUS_SUCCESS)
		dprintf("str2=\'%pus\'\n", &us);

	r = copy_from_user( &index, p_arg3, sizeof index );
	if (r == STATUS_SUCCESS)
		dprintf("index = %lu\n", index);

	return 0;
}

HANDLE NTAPI NtUserGetDC(HANDLE Window)
{
	dprintf("%p\n", Window);
	return win32k_manager->alloc_dc();
}

HGDIOBJ NtUserSelectPalette(HGDIOBJ hdc, HPALETTE palette, BOOLEAN force_bg)
{
	dprintf("%p %p %d\n", hdc, palette, force_bg);
	return alloc_gdi_object( FALSE, GDI_OBJECT_PALETTE );
}

BOOLEAN NTAPI NtUserSetCursorIconData(
	HANDLE Handle,
	PVOID Module,
	PUNICODE_STRING ResourceName,
	PICONINFO IconInfo)
{
	dprintf("%p %p %p %p\n", Handle, Module, ResourceName, IconInfo);
	return TRUE;
}

BOOLEAN NTAPI NtUserGetIconInfo(
	HANDLE Icon,
	PICONINFO IconInfo,
	PUNICODE_STRING lpInstName,
	PUNICODE_STRING lpResName,
	LPDWORD pbpp,
	BOOL bInternal)
{
	dprintf("\n");
	return TRUE;
}

wndcls_tt::wndcls_tt( NTWNDCLASSEX& ClassInfo, const UNICODE_STRING& ClassName, const UNICODE_STRING& MenuName, ATOM a ) :
	name( ClassName ),
	menu( MenuName ),
	info( ClassInfo ),
	atom( a ),
	refcount( 0 )
{
}

wndcls_tt* wndcls_tt::from_name( const UNICODE_STRING& wndcls_name )
{
	for (wndcls_iter_tt i(wndcls_list); i; i.next())
	{
		wndcls_tt *cls = i;
		if (cls->get_name().is_equal( wndcls_name ))
			return cls;
	}
	return NULL;
}

ATOM NTAPI NtUserRegisterClassExWOW(
	PNTWNDCLASSEX ClassInfo,
	PUNICODE_STRING ClassName,
	PNTCLASSMENUNAMES MenuNames,
	USHORT,
	ULONG Flags,
	ULONG)
{
	NTWNDCLASSEX clsinfo;

	NTSTATUS r;
	r = copy_from_user( &clsinfo, ClassInfo, sizeof clsinfo );
	if (r < STATUS_SUCCESS)
		return 0;

	if (clsinfo.Size != sizeof clsinfo)
		return 0;

	unicode_string_t clsstr;
	r = clsstr.copy_from_user( ClassName );
	if (r < STATUS_SUCCESS)
		return 0;

	// for some reason, a structure with three of the same name is passed...
	NTCLASSMENUNAMES menu_strings;
	r = copy_from_user( &menu_strings, MenuNames, sizeof menu_strings );
	if (r < STATUS_SUCCESS)
		return 0;

	unicode_string_t menuname;
	r = menuname.copy_from_user( menu_strings.name_us );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("window class = %pus  menu = %pus\n", &clsstr, &menuname);

	static ATOM atom = 0xc001;
	wndcls_tt* cls = new wndcls_tt( clsinfo, clsstr, menuname, atom );
	if (!cls)
		return 0;

	wndcls_list.append( cls );

	return cls->get_atom();
}

NTSTATUS NTAPI NtUserSetInformationThread(
	HANDLE ThreadHandle,
	ULONG InfoClass,
	PVOID Buffer,
	ULONG BufferLength)
{
	dprintf("%p %08lx %p %08lx\n", ThreadHandle, InfoClass, Buffer, BufferLength);
	return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtUserGetKeyboardLayoutList(ULONG x1, ULONG x2)
{
	dprintf("%08lx, %08lx\n", x1, x2);
	return STATUS_SUCCESS;
}

HANDLE NTAPI NtUserCreateWindowStation(
	POBJECT_ATTRIBUTES WindowStationName,
	ACCESS_MASK DesiredAccess,
	HANDLE ObjectDirectory,
	ULONG x1,
	PVOID x2,
	ULONG Locale)
{
	dprintf("%p %08lx %p %08lx %p %08lx\n",
		WindowStationName, DesiredAccess, ObjectDirectory, x1, x2, Locale);

	// print out the name
	OBJECT_ATTRIBUTES oa;

	NTSTATUS r;
	r = copy_from_user( &oa, WindowStationName, sizeof oa );
	if (r < STATUS_SUCCESS)
		return 0;

	unicode_string_t us;
	r = us.copy_from_user( oa.ObjectName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("name = %pus\n", &us );

	return (HANDLE) 0xf00d2000;
}

HANDLE NTAPI NtUserCreateDesktop(
	POBJECT_ATTRIBUTES DesktopName,
	ULONG x1,
	ULONG x2,
	ULONG x3,
	ACCESS_MASK DesiredAccess)
{
	dprintf("\n");

	// print out the name
	object_attributes_t oa;
	NTSTATUS r;
	r = oa.copy_from_user( DesktopName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("name = %pus\n", oa.ObjectName );

	return (HANDLE) 0xf00d2001;
}

BOOLEAN NTAPI NtUserSetProcessWindowStation(HANDLE WindowStation)
{
	dprintf("\n");
	return TRUE;
}

HANDLE NTAPI NtUserGetProcessWindowStation(void)
{
	dprintf("\n");
	return (HANDLE) 0xf00d2000;
}

BOOLEAN NTAPI NtUserSetThreadDesktop(HANDLE Desktop)
{
	dprintf("\n");
	return TRUE;
}

BOOLEAN NTAPI NtUserSetImeHotKey(ULONG x1, ULONG x2, ULONG x3, ULONG x4, ULONG x5)
{
	dprintf("\n");
	return TRUE;
}

BOOLEAN NTAPI NtUserLoadKeyboardLayoutEx(
	HANDLE File,
	ULONG x1,
	ULONG x2,
	PVOID x3,
	ULONG locale,
	ULONG flags)
{
	dprintf("\n");
	return TRUE;
}

BOOLEAN NTAPI NtUserUpdatePerUserSystemParameters(ULONG x1, ULONG x2)
{
	dprintf("\n");
	return TRUE;
}

BOOLEAN NTAPI NtUserSystemParametersInfo(ULONG x1, ULONG x2, ULONG x3, ULONG x4)
{
	dprintf("\n");
	return TRUE;
}

BOOLEAN NTAPI NtUserSetWindowStationUser(HANDLE WindowStation, PVOID, ULONG, ULONG)
{
	dprintf("\n");
	return TRUE;
}

ULONG NTAPI NtUserGetCaretBlinkTime(void)
{
	dprintf("\n");
	return 100;
}

ULONG message_no = 0xc001;

ULONG NTAPI NtUserRegisterWindowMessage(PUNICODE_STRING Message)
{
	dprintf("\n");
	unicode_string_t us;

	NTSTATUS r = us.copy_from_user( Message );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("message = %pus -> %04lx\n", &us, message_no);

	return message_no++;
}

BOOLEAN NTAPI NtUserGetMessage(PMSG Message, HANDLE Window, ULONG MinMessage, ULONG MaxMessage)
{
	dprintf("\n");
	// hmm... nothing to return yet...
	current->stop();
	return FALSE;
}

class user32_unicode_string_t : public unicode_string_t
{
public:
	NTSTATUS copy_from_user( PUSER32_UNICODE_STRING String );
};

NTSTATUS user32_unicode_string_t::copy_from_user( PUSER32_UNICODE_STRING String )
{
	USER32_UNICODE_STRING str;
	NTSTATUS r = ::copy_from_user( &str, String, sizeof str );
	if (r < STATUS_SUCCESS)
		return r;
	return copy_wstr_from_user( str.Buffer, str.Length );
}

window_tt::window_tt( void *winshm, thread_t* t, wndcls_tt *wndcls, unicode_string_t& _name, ULONG _style, ULONG _exstyle,
		 LONG _x, LONG _y, LONG _width, LONG _height ) :
	shared_mem( winshm ),
	thread( t ),
	cls( wndcls ),
	style( _style ),
	exstyle( _exstyle ),
	x( _x ),
	y( _y ),
	width( _width ),
	height( _height ),
	parent( 0 )
{
	name.copy( &_name );
}

void *window_tt::get_wininfo()
{
	ULONG ofs = (BYTE*)shared_mem - (BYTE*)user_shared_mem;
	return (void*) (thread->process->win32k_info->user_shared_mem + ofs);
}

static const ULONG num_win_handles = 0x10;
static const ULONG handle_offset = 0x10000;
window_tt *winhandles[num_win_handles];

HANDLE alloc_win_handle( window_tt *win )
{
	for (ULONG n=0; n<num_win_handles; n++)
	{
		if (!winhandles[n])
		{
			winhandles[n] = win;
			return (HANDLE)(n+handle_offset);
		}
	}
	return 0;
}

window_tt *window_from_handle( HANDLE handle )
{
	UINT n = (UINT) handle;

	if (n < handle_offset)
		return NULL;
	n -= handle_offset;
	if (n >= num_win_handles)
		return NULL;
	return winhandles[n];
}

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

wmessage_ptr_tt::wmessage_ptr_tt( pointer_info_tt& pointer_info ) :
	pi( pointer_info )
{
	pi.x = 0;
	pi.count = 0;
	pi.kernel_address = 0;
	pi.adjust_info_ofs = 0;
	pi.no_adjust = 0;
}

class create_message_tt : public wmessage_ptr_tt
{
	static const ULONG NTWIN32_CREATE_CALLBACK = 9;
	static const ULONG WM_CREATE = 0x0001;
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
	static const ULONG WM_NCCREATE = 0x0081;
public:
	nccreate_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name );
};

nccreate_message_tt::nccreate_message_tt( NTCREATESTRUCT& cs, const UNICODE_STRING& cls, const UNICODE_STRING& name ) :
	create_message_tt( cs, cls, name )
{
	info.msg = WM_NCCREATE;
}

create_message_tt::create_message_tt( NTCREATESTRUCT& cs,
			const UNICODE_STRING& _cls, const UNICODE_STRING& _name ) :
	wmessage_ptr_tt( info ),
	cls( _cls ),
	name( _name )
{
	info.sz = sizeof info;
	info.wininfo = NULL;
	info.msg = WM_CREATE;
	info.wparam = 0;
	info.cs_nonnull = TRUE;
	info.cs = cs;
}

ULONG create_message_tt::get_callback_num() const
{
	return NTWIN32_CREATE_CALLBACK;
}

NTSTATUS create_message_tt::copy_to_user( void *ptr ) const
{
	BYTE *p = (BYTE*) ptr;
	NTSTATUS r;
	r = ::copy_to_user( p, &info, sizeof info );
	return r;
}

ULONG create_message_tt::get_size() const
{
	return sizeof info;
}

void create_message_tt::set_window_info( window_tt *win )
{
	info.wininfo = win->get_wininfo();
	info.wndproc = win->get_wndproc();
}

typedef struct tagMINMAXINFO {
	POINT	ptReserved;
	POINT	ptMaxSize;
	POINT	ptMaxPosition;
	POINT	ptMinTrackSize;
	POINT	ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

typedef struct _NTMINMAXPACKEDINFO {
	PVOID	wininfo;
	ULONG	msg;
	WPARAM	wparam;
	MINMAXINFO minmax;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,MINMAXINFO*,PVOID);
} NTMINMAXPACKEDINFO;

class getminmaxinfo_tt : public message_tt
{
	static const ULONG NTWIN32_MINMAX_CALLBACK = 17;
	static const ULONG WM_GETMINMAXINFO = 0x24;
	NTMINMAXPACKEDINFO info;
public:
	getminmaxinfo_tt();
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

getminmaxinfo_tt::getminmaxinfo_tt()
{
	memset( &info, 0, sizeof info );
	info.msg = WM_GETMINMAXINFO;
}

ULONG getminmaxinfo_tt::get_size() const
{
	return sizeof info;
}

NTSTATUS getminmaxinfo_tt::copy_to_user( void *ptr ) const
{
	return ::copy_to_user( ptr, &info, sizeof info );
}

ULONG getminmaxinfo_tt::get_callback_num() const
{
	return NTWIN32_MINMAX_CALLBACK;
}

void getminmaxinfo_tt::set_window_info( window_tt *win )
{
	info.wininfo = win->get_wininfo();
	info.wndproc = win->get_wndproc();
}

typedef struct _WINDOWPOS {
	HANDLE	hwnd;
	HANDLE	hwndInsertAfter;
	INT	x;
	INT	y;
	INT	cx;
	INT	cy;
	UINT	flags;
} WINDOWPOS;

typedef struct _NCCALCSIZE_PARAMS {
	RECT	rgrc[3];
	WINDOWPOS *lppos;
} NCCALCSIZE_PARAMS;

typedef struct _NTNCCALCSIZEPACKEDINFO {
	PVOID	wininfo;
	ULONG	msg;
	BOOL	wparam;
	PVOID	wndproc;
	ULONG	(CALLBACK *func)(PVOID,ULONG,WPARAM,NCCALCSIZE_PARAMS*,PVOID);
	NCCALCSIZE_PARAMS params;
	WINDOWPOS winpos;
} NTNCCALCSIZEPACKEDINFO;

class nccalcsize_message_tt : public message_tt
{
	static const ULONG NTWIN32_NCCALC_CALLBACK = 20;
	static const ULONG WM_NCCALCSIZE = 0x0083;
	NTNCCALCSIZEPACKEDINFO info;
public:
	nccalcsize_message_tt();
	virtual ULONG get_size() const;
	virtual NTSTATUS copy_to_user( void *ptr ) const;
	virtual ULONG get_callback_num() const;
	virtual void set_window_info( window_tt *win );
};

nccalcsize_message_tt::nccalcsize_message_tt()
{
	memset( &info, 0, sizeof info );
	info.msg = WM_NCCALCSIZE;
}

ULONG nccalcsize_message_tt::get_size() const
{
	return sizeof info;
}

NTSTATUS nccalcsize_message_tt::copy_to_user( void *ptr ) const
{
	return ::copy_to_user( ptr, &info, sizeof info );
}

ULONG nccalcsize_message_tt::get_callback_num() const
{
	return NTWIN32_NCCALC_CALLBACK;
}

void nccalcsize_message_tt::set_window_info( window_tt *win )
{
	info.wininfo = win->get_wininfo();
	info.wndproc = win->get_wndproc();
}

NTSTATUS window_tt::send( message_tt& msg )
{
	if (thread->is_terminated())
		return STATUS_THREAD_IS_TERMINATING;

	msg.set_window_info( this );

	void *address = thread->push( msg.get_size() );

	NTSTATUS r = msg.copy_to_user( address );
	if (r >= STATUS_SUCCESS)
	{
		ULONG ret_len = 0;
		PVOID ret_buf = 0;

		r = thread->do_user_callback( msg.get_callback_num(), ret_len, ret_buf );
	}

	thread->pop( msg.get_size() );

	return r;
}

HANDLE NTAPI NtUserCreateWindowEx(
	ULONG ExStyle,
	PUSER32_UNICODE_STRING ClassName,
	PUSER32_UNICODE_STRING WindowName,
	ULONG Style,
	LONG x,
	LONG y,
	LONG Width,
	LONG Height,
	HANDLE Parent,
	HANDLE Menu,
	PVOID Instance,
	PVOID Param,
	//ULONG ShowMode,
	BOOL UnicodeWindow)
{
	dprintf("\n");

	NTSTATUS r;

	user32_unicode_string_t window_name;
	r = window_name.copy_from_user( WindowName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("WindowName = %pus\n", &window_name );

	user32_unicode_string_t wndcls_name;
	r = wndcls_name.copy_from_user( ClassName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("ClassName = %pus\n", &wndcls_name );

	wndcls_tt* wndcls = wndcls_tt::from_name( wndcls_name );
	if (!wndcls)
		return 0;

	NTCREATESTRUCT cs;

	cs.lpCreateParams = Param;
	cs.hInstance = Instance;
	cs.hMenu = Menu;
	cs.cy = Width;
	cs.cx = Height;
	cs.x = x;
	cs.y = y;
	cs.style = Style;
	cs.lpszName = NULL;
	cs.lpszClass = NULL;
	cs.dwExStyle = ExStyle;

	void **winshm = find_free_user_shared();
	if (!winshm)
		return 0;

	window_tt *win = new window_tt( winshm, current, wndcls, window_name, Style, ExStyle, x, y, Width, Height );
	if (!win)
		return 0;

	HANDLE ret = alloc_win_handle( win );
	if (!ret)
		return 0;

	*winshm = ret;

	// send WM_GETMINMAXINFO
	getminmaxinfo_tt minmax;
	win->send( minmax );

	// send WM_NCCREATE
	nccreate_message_tt nccreate( cs, wndcls_name, window_name );
	win->send( nccreate );

	// send WM_NCCALCSIZE
	nccalcsize_message_tt nccalcsize;
	win->send( nccalcsize );

	// send WM_CREATE
	create_message_tt create( cs, wndcls_name, window_name );
	win->send( create );

	return ret;
}

BOOLEAN NTAPI NtUserSetLogonNotifyWindow( HANDLE Window )
{
	return TRUE;
}

LONG NTAPI NtUserGetClassInfo(
	PVOID Module,
	PUNICODE_STRING ClassName,
	PVOID Buffer,
	PULONG Length,
	ULONG Unknown)
{
	unicode_string_t class_name;
	NTSTATUS r = class_name.copy_from_user( ClassName );
	if (r < STATUS_SUCCESS)
		return r;
	dprintf("%pus\n", &class_name );

	return 0;
}

BOOLEAN NTAPI NtUserNotifyProcessCreate( ULONG NewProcessId, ULONG CreatorId, ULONG, ULONG )
{
	return TRUE;
}

BOOLEAN NTAPI NtUserConsoleControl( ULONG Id, PVOID Information, ULONG Length )
{
	return TRUE;
}

BOOLEAN NTAPI NtUserGetObjectInformation( HANDLE Object, ULONG InformationClass, PVOID Buffer, ULONG Length, PULONG ReturnLength)
{
	return TRUE;
}

BOOLEAN NTAPI NtUserResolveDesktop(HANDLE Process, PVOID, PVOID, PHANDLE Desktop )
{
	return TRUE;
}

BOOLEAN NTAPI NtUserShowWindow( HANDLE Window, INT Show )
{
	return TRUE;
}

UINT NTAPI NtUserSetTimer( HANDLE Window, UINT Identifier, UINT Elapse, PVOID TimerProc )
{
	static UINT timer = 1;
	return timer++;
}

HANDLE NTAPI NtUserCreateAcceleratorTable( PVOID Accelerators, UINT Count )
{
	static UINT accelerator = 1;
	return (HANDLE) accelerator++;
}
