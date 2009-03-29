/*
 * native test suite
 *
 * Copyright 2008-2009 Mike McCormack
 *
 * This test is an interactive demonstration of the KernelCallbackTable
 *  and its relation to messages passed from the Windows 2000 Kernel.
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

BOOL quit;

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

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

const int ofs_thread_id_in_teb = 0x24;

ULONG get_current_thread_id(void)
{
	ULONG *teb = get_teb();
	return teb[ofs_thread_id_in_teb/4];
}

const int ofs_thread_user_info_in_teb = 0x6e4;

/*
 *   NTUSERINFO
 *
 *   0x08  PWND DesktopWindow
 *   0x14  hookinfo[]
 *   0x50  PWND ShellWindow
 *   0x5C  PWND TaskmanWindow
 *   0x60  PWND ProgmanWindow
 */
void* get_thread_user_info( void )
{
	void **teb = get_teb();
	return teb[ofs_thread_user_info_in_teb/4];
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

// magic numbers for everybody
void init_callback(void *arg)
{
	NtCallbackReturn( 0, 0, 0 );
}

void do_paint( HWND hwnd )
{
	PAINTSTRUCT ps;
	HDC dc;

	dc = NtUserBeginPaint( hwnd, &ps );
	NtUserEndPaint( hwnd, &ps );
}

void do_erasebk( HWND hwnd, RECT* rect )
{
	HDC dc = NtUserGetDC( hwnd );
	BOOL r;

	ok( dc != 0, "getdc failed\n");
	r = NtGdiRectangle( dc, rect->left, rect->top, rect->right, rect->bottom );
	ok( r == TRUE, "NtGdiRectangle failed\n");
	r = NtGdiRectangle( dc, 0, 0, 100, 100 );
	NtGdiDeleteObjectApp( dc );
}

void do_keydown( HWND hwnd, UINT keycode )
{
	switch (keycode)
	{
	case VK_ESCAPE:
		quit = 1;
		return;
	//case VK_SPACE:
	}
}

void basicmsg_callback(NTSIMPLEMESSAGEPACKEDINFO *pack)
{
	NTWINCALLBACKRETINFO ret;
	INT r = 0;

	switch (pack->msg)
	{
	// messages that may or may not come here
#define msg(x) case x: dprintf( #x "\n" );
#define msg_break(x) case x: dprintf( "" #x "\n" ); break;
	msg_break( WM_KEYUP )
	msg_break( WM_SYSKEYDOWN )
	msg_break( WM_SYSKEYUP )
	msg_break( WM_MOUSEMOVE )
	msg_break( WM_MOUSEWHEEL )
	msg_break( WM_NCHITTEST )
	msg_break( WM_SETCURSOR )
	msg_break( WM_SHOWWINDOW )
	msg_break( WM_ACTIVATE )
	msg_break( WM_ACTIVATEAPP )
	msg_break( WM_NCACTIVATE )
	msg_break( WM_SETFOCUS )
	msg_break( WM_NCPAINT )
	msg_break( WM_NCDESTROY )
	msg_break( WM_DESTROY )
	msg_break( WM_SIZE )
	msg_break( WM_MOVE )
	msg_break( WM_CANCELMODE )
	msg_break( WM_SYNCPAINT )
	msg( WM_KEYDOWN )
		do_keydown( pack->wininfo->handle, pack->wparam );
		break;
	msg( WM_ERASEBKGND )
		do_erasebk(pack->wininfo->handle, &pack->wininfo->rcClient );
		break;
	msg( WM_PAINT )
		do_paint(pack->wininfo->handle);
		break;
#undef msg
#undef msg_break
	default:
		dprintf("msg %04lx\n", pack->msg );
		break;
	}

	ret.val = r;
	ret.size = 0;
	ret.buf = 0;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void create_callback( NTCREATEPACKEDINFO *pack )
{
	NTWINCALLBACKRETINFO ret;

	ret.val = TRUE;
	ret.size = 0;
	ret.buf = 0;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void nccalc_callback( NTNCCALCSIZEPACKEDINFO *pack )
{
	NTWINCALLBACKRETINFO ret;

	ret.val = 0;
	ret.size = sizeof pack->params + sizeof pack->winpos;
	ret.buf = &pack->params;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void getminmax_callback( NTMINMAXPACKEDINFO *pack )
{
	dprintf("WM_GETMINMAXINFO\n");
}

void position_changing_callback( PNTPOSCHANGINGPACKEDINFO pack )
{
	dprintf("WM_WINDOWPOSCHANGING\n");
}

void position_changed_callback( PNTPOSCHANGINGPACKEDINFO pack )
{
	dprintf("WM_WINDOWPOSCHANGED\n");
}

void xcallback27( void *ptr )
{
	dprintf("xcallback27\n");
	dump_bin( ptr, 0x10 );
}

void init_callbacks( void )
{
	callback_table[NTWIN32_BASICMSG_CALLBACK] = &basicmsg_callback;
	callback_table[NTWIN32_THREAD_INIT_CALLBACK] = &init_callback;
	callback_table[NTWIN32_MINMAX_CALLBACK] = &getminmax_callback;
	callback_table[NTWIN32_CREATE_CALLBACK] = &create_callback;
	callback_table[NTWIN32_NCCALC_CALLBACK] = &nccalc_callback;
	callback_table[27] = &xcallback27;
	callback_table[NTWIN32_POSCHANGING_CALLBACK] = &position_changing_callback;
	callback_table[NTWIN32_POSCHANGED_CALLBACK] = &position_changed_callback;

	__asm__ (
		"movl %%fs:0x18, %%eax\n\t"
		"movl 0x30(%%eax), %%eax\n\t"
		"movl %%ebx, 0x2c(%%eax)\n\t"  // set PEB's KernelCallbackTable
		: : "b" (&callback_table) : "eax" );
}

USER_PROCESS_CONNECT_INFO user_info;

void become_gui_thread( void )
{
	NTSTATUS r;
	PVOID p;

	init_callbacks();
	ok( 0 == get_user_pointer_offset(), "user-kernel offset set too soon\n");
	NtGdiInit();
	ok( 0 != get_user_pointer_offset(), "user-kernel offset not set\n");

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
	ok(0, "should never get here\n");
	return 0;
}

WCHAR test_class_name[] = L"TESTCLS";

void register_class( void )
{
	NTCLASSMENUNAMES menu;
	UNICODE_STRING empty;
	UNICODE_STRING name;
	NTWNDCLASSEX wndcls;
	ATOM atom;

	memset( &wndcls, 0, sizeof wndcls );
	memset( &name, 0, sizeof name );
	memset( &menu, 0, sizeof menu );
	memset( &empty, 0, sizeof empty );

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

// argument selects a set of window styles
void create_window( void )
{
	USER32_UNICODE_STRING cls, title;
	WCHAR title_str[] = L"test window";
	HANDLE window;
	MSG msg;
	BOOL r;
	int i = 0;

	cls.Buffer = test_class_name;
	cls.Length = sizeof test_class_name - 2;
	cls.MaximumLength = sizeof test_class_name;

	title.Buffer = title_str;
	title.Length = sizeof title_str - 2;
	title.MaximumLength = 0;

	window = NtUserCreateWindowEx(0x80000000, &cls, &title,
		WS_DLGFRAME | WS_VISIBLE,
		100, 100, 100, 100,
		0, 0, get_exe_base(), 0, 0x400 );
	ok( window != 0, "window handle zero\n");

	//r = NtUserShowWindow( window, SW_SHOW );
	//ok( r == TRUE, "show window failed\n");

	while (!quit && NtUserGetMessage( &msg, 0, 0, 0 ))
	{
		NtUserDispatchMessage( &msg );
		i++;
	}

	r = NtUserDestroyWindow( window );
	ok( TRUE == r, "NtUserDestroyWindow failed\n");
}

void NtProcessStartup( void )
{
	log_init();
	dprintf("Press escape to quit\n");
	become_gui_thread();
	register_class();
	create_window();
	log_fini();
}
