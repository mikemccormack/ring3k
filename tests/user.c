/*
 * native test suite
 *
 * Copyright 2008 Mike McCormack
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

#include "ntapi.h"
#include "ntwin32.h"
#include "log.h"

ULONG NTAPI testWndProc( HANDLE Window, UINT Message, UINT Wparam, ULONG Lparam );
void *get_exe_base( void );

// declare functions
#define ucb(type, number) \
void NTAPI ucb##type##number() {			\
	ok( 0, "callback " #type #number " called\n");	\
}

ucb(N,0) ucb(N,1) ucb(N,2) ucb(N,3) ucb(N,4) ucb(N,5) ucb(N,6) ucb(N,7) ucb(N,8)

ucb(W,0) ucb(W,1) ucb(W,2) ucb(W,3) ucb(W,4) ucb(W,5) ucb(W,6) ucb(W,7) ucb(W,8) ucb(W,9)
ucb(W,10) ucb(W,11) ucb(W,12) ucb(W,13) ucb(W,14) ucb(W,15) ucb(W,16) ucb(W,17) ucb(W,18) ucb(W,19)

ucb(A,0) ucb(A,1) ucb(A,2) ucb(A,3) ucb(A,4) ucb(A,5) ucb(A,6) ucb(A,7) ucb(A,8) ucb(A,9)
ucb(A,10) ucb(A,11) ucb(A,12) ucb(A,13) ucb(A,14) ucb(A,15) ucb(A,16) ucb(A,17) ucb(A,18) ucb(A,19)
#undef ucb

// declare the tables of functions
#define ucb(type, number) ucb##type##number,
void *ucbN_funcs[9] = {
	ucb(N,0) ucb(N,1) ucb(N,2) ucb(N,3) ucb(N,4) ucb(N,5) ucb(N,6) ucb(N,7) ucb(N,8)
};
void *ucbW_funcs[20] = {
	ucb(W,0) ucb(W,1) ucb(W,2) ucb(W,3) ucb(W,4) ucb(W,5) ucb(W,6) ucb(W,7) ucb(W,8) ucb(W,9)
	ucb(W,10) ucb(W,11) ucb(W,12) ucb(W,13) ucb(W,14) ucb(W,15) ucb(W,16) ucb(W,17) ucb(W,18) ucb(W,19)
};
void *ucbA_funcs[20] = {
	ucb(A,0) ucb(A,1) ucb(A,2) ucb(A,3) ucb(A,4) ucb(A,5) ucb(A,6) ucb(A,7) ucb(A,8) ucb(A,9)
	ucb(A,10) ucb(A,11) ucb(A,12) ucb(A,13) ucb(A,14) ucb(A,15) ucb(A,16) ucb(A,17) ucb(A,18) ucb(A,19)
};
#undef ucb

void uninit_callback()
{
	ok( 0, "uninit_callback called\n" );
	NtCallbackReturn( 0, 0, 0 );
}

BOOL cb_called[NUM_USER32_CALLBACKS];

#define cb(x)						\
void callback##x(void *arg) {			\
	ok( 0, "unexpected callback " #x " called\n" );	\
	NtCallbackReturn( 0, 0, 0 );			\
}

cb(0) cb(1) cb(2) cb(3) cb(4) cb(5) cb(6) cb(7) cb(8) cb(9) cb(10)
cb(11) cb(12) cb(13) cb(14) cb(15) cb(16) cb(17) cb(18) cb(19) cb(20)
cb(21) cb(22) cb(23) cb(24) cb(25) cb(26) cb(27) cb(28) cb(29) cb(30)
cb(31) cb(32) cb(33) cb(34) cb(35) cb(36) cb(37) cb(38) cb(39) cb(40)
cb(41) cb(42) cb(43) cb(44) cb(45) cb(46) cb(47) cb(48) cb(49) cb(50)
cb(51) cb(52) cb(53) cb(54) cb(55) cb(56) cb(57) cb(58) cb(59) cb(60)
cb(61) cb(62) cb(63) cb(64) cb(65) cb(66) cb(67) cb(68) cb(69) cb(70)
cb(71) cb(72) cb(73) cb(74) cb(75) cb(76) cb(77) cb(78) cb(79) cb(80)
cb(81) cb(82) cb(83) cb(84) cb(85) cb(86) cb(87) cb(88) cb(89)

#undef cb

#define cbt(x) callback##x

void *callback_table[NUM_USER32_CALLBACKS] = {
	cbt(0),  cbt(1),  cbt(2),  cbt(3),  cbt(4),  cbt(5),  cbt(6),  cbt(7),  cbt(8),  cbt(9),
	cbt(10), cbt(11), cbt(12), cbt(13), cbt(14), cbt(15), cbt(16), cbt(17), cbt(18), cbt(19),
	cbt(20), cbt(21), cbt(22), cbt(23), cbt(24), cbt(25), cbt(26), cbt(27), cbt(28), cbt(29),
	cbt(30), cbt(31), cbt(32), cbt(33), cbt(34), cbt(35), cbt(36), cbt(37), cbt(38), cbt(39),
	cbt(40), cbt(41), cbt(42), cbt(43), cbt(44), cbt(45), cbt(46), cbt(47), cbt(48), cbt(49),
	cbt(50), cbt(51), cbt(52), cbt(53), cbt(54), cbt(55), cbt(56), cbt(57), cbt(58), cbt(59),
	cbt(60), cbt(61), cbt(62), cbt(63), cbt(64), cbt(65), cbt(66), cbt(67), cbt(68), cbt(69),
	cbt(70), cbt(71), cbt(72), cbt(73), cbt(74), cbt(75), cbt(76), cbt(77), cbt(78), cbt(79),
	cbt(80), cbt(81), cbt(82), cbt(83), cbt(84), cbt(85), cbt(86), cbt(87), cbt(88), cbt(89),
};

// magic numbers for everybody
void init_callback(void *arg)
{
	NtCallbackReturn( 0, 0, 0 );
}

#define MAX_RECEIVED_MESSAGES 0x1000
ULONG sequence;
ULONG received_msg[MAX_RECEIVED_MESSAGES];

static inline void record_received( ULONG msg )
{
	if (sequence < MAX_RECEIVED_MESSAGES)
		received_msg[ sequence ++ ] = msg;
}

void basicmsg_callback(NTSIMPLEMESSAGEPACKEDINFO *pack)
{
	ok( pack->wininfo != NULL && *(pack->wininfo) != NULL, "*wininfo NULL\n" );
	record_received( pack->msg );
	switch (pack->msg)
	{
	case WM_SHOWWINDOW:
	case WM_ACTIVATE:
		break;
	default:
		dprintf("msg %04lx\n", pack->msg );
		break;
	}
}

// magic numbers for everybody
void getminmax_callback(NTMINMAXPACKEDINFO *pack)
{
	//dprintf("wininfo = %p\n", pack->wininfo );
	ok( pack->wininfo != NULL && *(pack->wininfo) != NULL, "*wininfo NULL\n" );
	ok( pack->wparam == 0, "wparam wrong %08x\n", pack->wparam );
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	ok( pack->msg == WM_GETMINMAXINFO, "message wrong %08lx\n", pack->msg );
	ok( pack->wininfo != NULL && *(pack->wininfo) != NULL, "*wininfo NULL\n" );
	ok( pack->func != NULL, "func NULL\n" );

	record_received( pack->msg );

	NtCallbackReturn( 0, 0, 0 );
}

void create_callback( NTCREATEPACKEDINFO *pack )
{
	NTWINCALLBACKRETINFO ret;

	ok( pack->wininfo != NULL && *(pack->wininfo) != NULL, "*wininfo NULL\n" );
	ok( pack->func != NULL, "func NULL\n" );
	record_received( pack->msg );

	switch (pack->msg)
	{
	case WM_NCCREATE:
	case WM_CREATE:
		break;

	default:
		ok( 0, "message wrong %08lx\n", pack->msg );
	}

	ret.val = TRUE;
	ret.size = 0;
	ret.buf = 0;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void nccalc_callback( NTNCCALCSIZEPACKEDINFO *pack )
{
	record_received( pack->msg );
	ok( pack->func != NULL, "func NULL\n" );
	ok( pack->msg == WM_NCCALCSIZE, "message wrong %08lx\n", pack->msg );
	ok( pack->wininfo != NULL && *(pack->wininfo) != NULL, "*wininfo NULL\n" );
	NtCallbackReturn( 0, 0, 0 );
}

void init_callbacks( void )
{
	callback_table[NTWIN32_BASICMSG_CALLBACK] = &basicmsg_callback;
	callback_table[NTWIN32_THREAD_INIT_CALLBACK] = &init_callback;
	callback_table[NTWIN32_MINMAX_CALLBACK] = &getminmax_callback;
	callback_table[NTWIN32_CREATE_CALLBACK] = &create_callback;
	callback_table[NTWIN32_NCCALC_CALLBACK] = &nccalc_callback;
	ok( NTWIN32_MINMAX_CALLBACK == 17, "wrong!\n");

	__asm__ (
		"movl %%fs:0x18, %%eax\n\t"
		"movl 0x30(%%eax), %%eax\n\t"
		"movl %%ebx, 0x2c(%%eax)\n\t"  // set PEB's KernelCallbackTable
		: : "b" (&callback_table) : "eax" );
}

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

const int ofs_kernel_pointer_delta_in_teb = 0x6e8;

ULONG get_user_pointer_offset(void)
{
	ULONG *teb = get_teb();
	return teb[ofs_kernel_pointer_delta_in_teb/4];
}

const int ofs_peb_in_teb = 0x30;

void* get_peb( void )
{
	void **p = get_teb();
	return p[ofs_peb_in_teb/sizeof (*p)];
}

const int ofs_exe_base_in_peb = 0x08;

void *get_exe_base( void )
{
	void** peb = get_peb();
	return peb[ofs_exe_base_in_peb/4];
}

USER_PROCESS_CONNECT_INFO user_info;

void become_gui_thread( void )
{
	NTSTATUS r;
	PVOID p;

	init_callbacks();
	NtGdiInit();

	memset( &user_info, 0, sizeof user_info );
	user_info.Version = 0x00050000;
	r = NtUserProcessConnect( NtCurrentProcess(), &user_info, sizeof user_info );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	p = get_exe_base();
	r = NtUserInitializeClientPfnArrays( ucbN_funcs, ucbW_funcs, ucbA_funcs, p );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );
}

ULONG NTAPI testWndProc( HANDLE Window, UINT Message, UINT Wparam, ULONG Lparam )
{
	switch (Message)
	{
	case WM_NCCREATE:
		return 0;
	case WM_CREATE:
		return 1;
	default:
		return 0;
	}
}

struct menu_names
{
	LPSTR name_a;
	LPWSTR name_w;
	PUNICODE_STRING name_us;
};

WCHAR test_class_name[] = L"TESTCLS";

void register_class( void )
{
	struct menu_names menu;
	UNICODE_STRING empty;
	UNICODE_STRING name;
	NTWNDCLASSEX wndcls;
	ATOM atom;

	memset( &wndcls, 0, sizeof wndcls );
	memset( &name, 0, sizeof name );
	memset( &menu, 0, sizeof menu );
	memset( &empty, 0, sizeof empty );

	atom = NtUserRegisterClassExWOW( 0, 0, 0, 0, 0, 0 );
	ok( atom == 0, "return %04x\n", atom );

	atom = NtUserRegisterClassExWOW( &wndcls, &name, 0, 0, 0, 0 );
	ok( atom == 0, "return %04x\n", atom );

	wndcls.Size = sizeof wndcls;

	atom = NtUserRegisterClassExWOW( &wndcls, &name, 0, 0, 0, 0);
	ok( atom == 0, "return %04x\n", atom );

	init_us( &name, test_class_name );

	atom = NtUserRegisterClassExWOW( &wndcls, &name, 0, 0, 0, 0);
	ok( atom == 0, "return %04x\n", atom );

	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0, 0);
	ok( atom == 0, "return %04x\n", atom );

	wndcls.ClassName = test_class_name;
	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0, 0);
	ok( atom == 0, "return %04x\n", atom );

	wndcls.WndProc = testWndProc;
	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0x80, 0);
	ok( atom == 0, "return %04x\n", atom );

	wndcls.Instance = get_exe_base();
	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0x80, 0);
	ok( atom == 0, "return %04x\n", atom );

	wndcls.Style = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	ok(wndcls.Size == 0x30, "size wrong\n");
	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0x80, 0);
	ok( atom == 0, "return %04x\n", atom );

	wndcls.WndExtra = 4;
	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0x80, 0);
	ok( atom == 0, "return %04x\n", atom );

	init_us( &name, test_class_name );
	name.MaximumLength = name.Length + 2;

	wndcls.Size = sizeof wndcls;
	wndcls.Style = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	wndcls.WndProc = testWndProc;
	wndcls.ClsExtra = 0;
	wndcls.WndExtra = 4;
	wndcls.Instance = get_exe_base();
	wndcls.Icon = 0;
	wndcls.Cursor = 0;
	wndcls.Background = 0;
	wndcls.MenuName = 0;
	wndcls.ClassName = test_class_name;
	wndcls.IconSm = 0;

	// menu name is repeated 3 times in different formats...
	menu.name_a = 0;
	menu.name_w = 0;
	menu.name_us = &empty;

	atom = NtUserRegisterClassExWOW( &wndcls, &name, &menu, 0, 0x80, 0 );
	ok( atom != 0, "return %04x\n", atom );
}

static inline void check_msg( ULONG msg, ULONG* n )
{
	ULONG val = received_msg[*n];
	ok( val == msg, "%ld sequence %ld != %ld\n", *n, val, msg );
	(*n)++;
}

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

ULONG handle_highpart( HANDLE handle )
{
	return ((ULONG) handle) >> 16;
}

ULONG handle_lowpart( HANDLE handle )
{
	return ((ULONG) handle) & 0xffff;
}

void* check_user_handle( HANDLE handle, USHORT type )
{
	struct user_handle_entry_t *user_handle_table;
	struct user_shared_mem_t *user_shared_mem;
	ULONG highpart = handle_highpart( handle );
	ULONG lowpart = handle_lowpart( handle );

	user_shared_mem = user_info.Ptr[0];
	user_handle_table = (struct user_handle_entry_t *) user_info.Ptr[1];
	ok( user_shared_mem->max_window_handle >= lowpart, "max_window_handle zero\n");
	ok( user_handle_table[lowpart].type == type, "type wrong\n");
	ok( user_handle_table[lowpart].highpart == highpart, "highpart wrong\n");
	ok( user_handle_table[lowpart].owner != 0, "owner empty\n");
	ULONG kptr = (ULONG) user_handle_table[lowpart].object;
	ok( (kptr&~0xffff) != 0, "not a pointer\n");
	return (void*) kptr;
}

void* kernel_to_user( void *kptr )
{
	return (void*) ((ULONG)kptr - get_user_pointer_offset());
}

/* see http://winterdom.com/dev/ui/wnd.html */
typedef struct _WND WND;

struct _WND
{
      HWND        hWnd;
      ULONG       unk1;
      ULONG       unk2;
      ULONG       unk3;
      WND*        pSelf;
      DWORD       dwFlags;
      ULONG       unk6;
      DWORD       dwStyleEx;
      DWORD       dwStyle;
      HINSTANCE   hInstance;
      ULONG       unk10;
      WND*        pNextWnd;
      WND*        pParentWnd;
      WND*        pFirstChild;
      WND*        pOwnerWnd;
      RECT        rcWnd;
      RECT        rcClient;
      PVOID       pWndProc;
      PVOID       pWndClass;
      ULONG       unk25;
      ULONG       unk26;
      ULONG       unk27;
      ULONG       unk28;
      union {
         DWORD    dwWndID;
         PVOID    pMenu;
      } id;
      ULONG       unk30;
      ULONG       unk31;
      ULONG       unk32;
      WCHAR*      pText;
      DWORD       dwWndBytes;
      ULONG       unk35;
      ULONG       unk36;
      ULONG       wlUserData;
      ULONG       wlWndExtra[1];
};

void create_window( void )
{
	USER32_UNICODE_STRING cls, title;
	WCHAR title_str[] = L"test window";
	HANDLE window;
	ULONG n = 0;
	WND *wndptr, *kernel_wndptr;
	ULONG style, exstyle;
	void *instance;

	cls.Buffer = test_class_name;
	cls.Length = sizeof test_class_name - 2;
	cls.MaximumLength = sizeof test_class_name;

	title.Buffer = title_str;
	title.Length = sizeof title_str - 2;
	title.MaximumLength = sizeof title_str;

	instance = get_exe_base();
	style = WS_CAPTION |WS_SYSMENU |WS_GROUP;
	exstyle = 0x80000000;
	window = NtUserCreateWindowEx(exstyle, &cls, &title, style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, get_exe_base(), 0, 0x400 );
	ok( window != 0, "window handle zero\n");

	kernel_wndptr = check_user_handle( window, USER_HANDLE_WINDOW );
	wndptr = kernel_to_user( kernel_wndptr );
	//dprintf("wininfo = %p\n", wndptr );
	ok( wndptr->pSelf == kernel_wndptr, "self pointer wrong\n");
	ok( wndptr->hWnd == window, "window handle wrong\n");
	ok( wndptr->dwStyle == (style | WS_CLIPSIBLINGS), "style wrong %08lx %08lx\n", wndptr->dwStyle, style);
	ok( wndptr->dwStyleEx == WS_EX_WINDOWEDGE, "exstyle wrong %08lx %08lx\n", wndptr->dwStyleEx, exstyle);
	ok( wndptr->hInstance == instance, "instance wrong\n");

	check_msg( WM_GETMINMAXINFO, &n );
	check_msg( WM_NCCREATE, &n );
	check_msg( WM_NCCALCSIZE, &n );
	check_msg( WM_CREATE, &n );
}

void test_window()
{
	register_class();
	create_window();
}

void NtProcessStartup( void )
{
	log_init();
	become_gui_thread();
	test_window();
	log_fini();
}
