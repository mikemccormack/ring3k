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
static void *user_shared_mem = 0;

void *init_user_shared_memory()
{
	// read/write for the kernel and read only for processes
	if (!user_shared_mem)
	{
		LARGE_INTEGER sz;
		NTSTATUS r;

		sz.QuadPart = 0x10000;
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
	r = process_from_handle( Process, &proc );
	if (r < STATUS_SUCCESS)
		return r;

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

	BYTE *p = 0;
	r = user_shared_section->mapit( proc->vm, p, 0,
					MEM_COMMIT, PAGE_READONLY );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	if (0) proc->vm->set_tracer( p, ntusershm_trace );

	info.win2k.Ptr[0] = (void*)p;
	info.win2k.Ptr[1] = (void*)0xbee20000;
	info.win2k.Ptr[2] = (void*)0xbee30000;
	info.win2k.Ptr[3] = (void*)0xbee40000;

	dprintf("user shared at %p\n", p);

	r = copy_to_user( Buffer, &info, BufferSize );
	if (r < STATUS_SUCCESS)
		return STATUS_UNSUCCESSFUL;

	return STATUS_SUCCESS;
}

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
	dprintf("%p %p %p %p\n", Funcs, FuncsW, FuncsA, Base);
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

ATOM NTAPI NtUserRegisterClassExWOW(PNTWNDCLASSEX ClassInfo, PUNICODE_STRING ClassName, PVOID, USHORT, ULONG, ULONG)
{
	NTWNDCLASSEX clsinfo;

	NTSTATUS r;
	r = copy_from_user( &clsinfo, ClassInfo, sizeof clsinfo );
	if (r < STATUS_SUCCESS)
		return 0;

	if (clsinfo.Size != sizeof clsinfo)
		return 0;

	unicode_string_t us;
	r = us.copy_from_user( ClassName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("Name  = %pus\n", &us);

	static ATOM atom = 0xc001;
	return atom++;
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
	Length = str.Length;
	MaximumLength = str.MaximumLength;
	Buffer = str.Buffer;
	return copy_wstr_from_user();
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

	user32_unicode_string_t class_name;
	r = class_name.copy_from_user( ClassName );
	if (r < STATUS_SUCCESS)
		return 0;

	dprintf("ClassName = %pus\n", &class_name );

	static ULONG window = 0x0001bb01;
	return (HANDLE) window++;
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
