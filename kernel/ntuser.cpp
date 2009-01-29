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
#include <new>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "ntcall.h"
#include "section.h"
#include "objdir.h"
#include "ntwin32.h"
#include "win32mgr.h"
#include "mem.h"
#include "debug.h"
#include "object.inl"
#include "alloc_bitmap.h"
#include "queue.h"
#include "message.h"
#include "win.h"

wndcls_list_tt wndcls_list;
window_tt *active_window;

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

// see http://winterdom.com/dev/ui/wnd.html

#define USER_HANDLE_WINDOW 1

struct user_handle_entry_t {
	union {
		void *object;
		USHORT next_free;
	};
	void *owner;
	USHORT type;
	USHORT highpart;
};

struct user_shared_mem_t {
	ULONG x1;
	ULONG x2;
	ULONG max_window_handle;
};

static const ULONG user_shared_mem_size = 0x20000;
static const ULONG user_shared_mem_reserve = 0x10000;

// section for user handle table
static section_t *user_handle_table_section = 0;

// kernel address for user handle table (shared)
static user_handle_entry_t *user_handle_table;

// section for user shared memory
static section_t *user_shared_section = 0;

// kernel address for memory shared with the user process
static user_shared_mem_t *user_shared;

// bitmap of free memory
allocation_bitmap_t user_shared_bitmap;

static USHORT next_user_handle = 1;

#define MAX_USER_HANDLES 0x200

void check_max_window_handle( ULONG n )
{
	n++;
	if (user_shared->max_window_handle<n)
		user_shared->max_window_handle = n;
	dprintf("max_window_handle = %04lx\n", user_shared->max_window_handle);
}

void init_user_handle_table()
{
	USHORT i;
	next_user_handle = 1;
	for ( i=next_user_handle; i<(MAX_USER_HANDLES-1); i++ )
	{
		user_handle_table[i].object = (void*) (i+1);
		user_handle_table[i].owner = 0;
		user_handle_table[i].type = 0;
		user_handle_table[i].highpart = 1;
	}
}

ULONG alloc_user_handle( void* obj, ULONG type )
{
	ULONG ret = next_user_handle;
	ULONG next = user_handle_table[ret].next_free;
	assert( user_handle_table[ret].type == 0 );
	assert( user_handle_table[ret].owner == 0 );
	assert( next <= MAX_USER_HANDLES );
	user_handle_table[ret].object = obj;
	user_handle_table[ret].type = type;
	user_handle_table[ret].owner = obj;
	next_user_handle = next;
	check_max_window_handle( ret );
	return (user_handle_table[ret].highpart << 16) | ret;
}

void* user_obj_from_handle( HANDLE handle, ULONG type )
{
	UINT n = (UINT) handle;
	USHORT lowpart = n&0xffff;
	//USHORT highpart = (n>>16);

	if (lowpart == 0 || lowpart > user_shared->max_window_handle)
		return NULL;
	//FIXME: check high part and type
	//if (user_handle_table[].highpart != highpart)
	return user_handle_table[lowpart].object;
}

window_tt *window_from_handle( HANDLE handle )
{
	void *obj = user_obj_from_handle( handle, 1 );
	if (!obj)
		return NULL;
	return (window_tt*) obj;
}

void *init_user_shared_memory()
{
	// read/write for the kernel and read only for processes
	if (!user_shared)
	{
		LARGE_INTEGER sz;
		NTSTATUS r;

		sz.QuadPart = sizeof (user_handle_entry_t) * MAX_USER_HANDLES;
		r = create_section( &user_handle_table_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return 0;

		user_handle_table = (user_handle_entry_t*) user_handle_table_section->get_kernel_address();

		init_user_handle_table();

		sz.QuadPart = user_shared_mem_size;
		r = create_section( &user_shared_section, NULL, &sz, SEC_COMMIT, PAGE_READWRITE );
		if (r < STATUS_SUCCESS)
			return 0;

		user_shared = (user_shared_mem_t*) user_shared_section->get_kernel_address();

		// setup the allocation bitmap for user objects (eg. windows)
		void *object_area = (void*) ((BYTE*) user_shared + user_shared_mem_reserve);
		user_shared_bitmap.set_area( object_area,
			user_shared_mem_size - user_shared_mem_reserve );

		// create the window stations directory too
		create_directory_object( (PWSTR) L"\\Windows\\WindowStations" );
	}

	dprintf("user_handle_table at %p\n", user_handle_table );
	dprintf("user_shared at %p\n", user_shared );

	return user_shared;
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

class ntuserhandle_tracer : public block_tracer
{
public:
	virtual void on_access( mblock *mb, BYTE *address, ULONG eip );
};

void ntuserhandle_tracer::on_access( mblock *mb, BYTE *address, ULONG eip )
{
	ULONG ofs = address - mb->get_base_address();
	const int sz = sizeof (user_handle_entry_t);
	ULONG number = ofs/sz;
	const char *field = "unknown";
	switch (ofs % sz)
	{
#define f(n, x) case n: field = #x; break;
	f( 0, owner )
	f( 4, object )
	f( 8, type )
	f( 10, highpart )
#undef f
	default:
		field = "unknown";
	}

	fprintf(stderr, "%04lx: accessed user handle[%04lx]+%s (%ld) from %08lx\n",
		current->trace_id(), number, field, ofs%sz, eip);
}
static ntuserhandle_tracer ntuserhandle_trace;

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
	BYTE*& user_handles = proc->win32k_info->user_handles;

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

	if (option_trace)
	{
		proc->vm->set_tracer( user_shared_mem, ntusershm_trace );
		proc->vm->set_tracer( user_handles, ntuserhandle_trace );
	}

	// map the shared handle table
	r = user_handle_table_section->mapit( proc->vm, user_handles, 0,
					MEM_COMMIT, PAGE_READONLY );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	info.win2k.Ptr[0] = (void*)user_shared_mem;
	info.win2k.Ptr[1] = (void*)user_handles;
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

BOOLEAN NtPostQuitMessage( ULONG ret )
{
	if (current->queue)
		current->queue->post_quit_message( ret );
	return TRUE;
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
	case NTUCOP_POSTQUITMESSAGE:
		return NtPostQuitMessage( Param );
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
	return win32k_manager->alloc_screen_dc();
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
	current->process->window_station = WindowStation;
	return TRUE;
}

HANDLE NTAPI NtUserGetProcessWindowStation(void)
{
	dprintf("\n");
	return current->process->window_station;
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

window_tt::window_tt( thread_t* t, wndcls_tt *_wndcls, unicode_string_t& _name, ULONG _style, ULONG _exstyle,
		 LONG x, LONG y, LONG width, LONG height, PVOID instance )
{
	parent = 0;
	get_win_thread() = t;
	self = this;
	wndcls = _wndcls;
	style = _style;
	exstyle = _exstyle;
	rcWnd.left = x;
	rcWnd.top = y;
	rcWnd.right = x + width;
	rcWnd.bottom = y + height;
	hInstance = instance;
}

PWND window_tt::get_wininfo()
{
	ULONG ofs = (BYTE*)this - (BYTE*)user_shared;
	return (PWND) (current->process->win32k_info->user_shared_mem + ofs);
}

NTSTATUS window_tt::send( message_tt& msg )
{
	thread_t*& thread = get_win_thread();
	if (thread->is_terminated())
		return STATUS_THREAD_IS_TERMINATING;

	PTEB teb = thread->get_teb();
	teb->CachedWindowHandle = handle;

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

BOOLEAN window_tt::show( INT Show )
{
	// send a WM_SHOWWINDOW message
	showwindowmsg_tt sw( TRUE );
	send( sw );

	return TRUE;
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

	// tweak the styles
	Style |= WS_CLIPSIBLINGS;
	ExStyle |= WS_EX_WINDOWEDGE;
	ExStyle &= ~0x80000000;

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

	window_tt *win;
	void *mem = user_shared_bitmap.alloc( sizeof *win );
	if (!mem)
		return 0;

	win = new(mem) window_tt( current, wndcls, window_name, Style, ExStyle, x, y, Width, Height, Instance );

	win->handle = (HWND) alloc_user_handle( win, USER_HANDLE_WINDOW );
	win->wndproc = wndcls->get_wndproc();

	// check set the offset
	// FIXME: there might be a better place to do this
	ULONG ofs = (BYTE*) user_shared - current->process->win32k_info->user_shared_mem;
	current->get_teb()->KernelUserPointerOffset = ofs;

	// create a thread message queue if necessary
	if (!current->queue)
		current->queue = new thread_message_queue_tt;

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

	if (win->style & WS_VISIBLE)
	{
		win->show( SW_SHOW );

		WINDOWPOS wp;
		memset( &wp, 0, sizeof wp );
		winposchanging_tt poschanging( wp );
		win->send( poschanging );

		// send activate messages
		win->activate();

		// painting probably should be done elsewhere
		ncpaintmsg_tt ncpaint;
		win->send( ncpaint );

		erasebkgmsg_tt erase;
		win->send( erase );

		winposchanged_tt poschanged( wp );
		win->send( poschanged );
	}

	return win->handle;
}

void window_tt::activate()
{
	if (active_window == this)
		return;

	if (active_window)
	{
		appactmsg_tt aa( WA_INACTIVE );
		active_window->send( aa );
	}

	active_window = this;
	appactmsg_tt aa( WA_ACTIVE );
	send( aa );

	ncactivate_tt ncact;
	send( ncact );

	activate_tt act;
	send( act );

	setfocusmsg_tt setfocus;
	send( setfocus );
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
	window_tt *win = window_from_handle( Window );
	if (!win)
		return FALSE;

	return win->show( Show );
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

BOOLEAN NTAPI NtUserMoveWindow( HANDLE Window, int x, int y, int width, int height, BOOLEAN repaint )
{
	return TRUE;
}

BOOLEAN NTAPI NtUserRedrawWindow( HANDLE Window, RECT *Update, HANDLE Region, UINT Flags )
{
	return TRUE;
}

ULONG NTAPI NtUserGetAsyncKeyState( ULONG Key )
{
	return win32k_manager->get_async_key_state( Key );
}

LRESULT NTAPI NtUserDispatchMessage( PMSG Message )
{
	return 0;
}

BOOLEAN NTAPI NtUserInvalidateRect( HWND Window, const RECT* Rectangle, BOOLEAN Erase )
{
	return TRUE;
}
