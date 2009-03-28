/*
 * native test suite
 *
 * Copyright 2008-2009 Mike McCormack
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

const int ofs_cached_window_handle_in_teb = 0x6f4;

HANDLE get_cached_window_handle( void )
{
	void **p = get_teb();
	return p[ofs_cached_window_handle_in_teb/sizeof (*p)];
}

const int ofs_cached_window_pointer_in_teb = 0x6f8;

HANDLE get_cached_window_pointer( void )
{
	void **p = get_teb();
	return p[ofs_cached_window_pointer_in_teb/sizeof (*p)];
}

// magic numbers for everybody
void init_callback(void *arg)
{
	NtCallbackReturn( 0, 0, 0 );
}

#define MAX_RECEIVED_MESSAGES 0x1000
ULONG sequence;
ULONG received_msg[MAX_RECEIVED_MESSAGES];
HWND received_hwnd[MAX_RECEIVED_MESSAGES];

#define TEST_XPOS 110
#define TEST_YPOS 120
#define TEST_WIDTH 230
#define TEST_HEIGHT 240

CREATESTRUCTW test_cs;
WINDOWPOS test_winpos;
BOOL testCalcSizeWparam;

void clear_msg_sequence(void)
{
	memset( &received_msg, 0, sizeof received_msg );
	memset( &received_hwnd, 0, sizeof received_hwnd );
	sequence = 0;
}

static inline void record_received( HWND hwnd, ULONG msg )
{
	if (sequence < MAX_RECEIVED_MESSAGES)
	{
		received_hwnd[ sequence ] = hwnd;
		received_msg[ sequence ++ ] = msg;
	}
}

void basicmsg_callback(NTSIMPLEMESSAGEPACKEDINFO *pack)
{
	NTWINCALLBACKRETINFO ret;
	PAINTSTRUCT ps;
	HDC dc;
	BOOL record = TRUE;

	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	ok( get_cached_window_handle() == pack->wininfo->handle, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );

	ret.val = 0;
	ret.size = 0;
	ret.buf = 0;
	switch (pack->msg)
	{
	// messages that may or may not come
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_MOUSEMOVE:
	case WM_NCHITTEST:
	case WM_SETCURSOR:
		record = FALSE;
		break;
	case WM_SHOWWINDOW:
	case WM_ACTIVATE:
	case WM_ACTIVATEAPP:
	case WM_NCACTIVATE:
	case WM_SETFOCUS:
	case WM_NCPAINT:
	case WM_NCDESTROY:
	case WM_DESTROY:
		break;
	case WM_SIZE:
		ok( LOWORD( pack->lparam ) == test_cs.cx, "width wrong\n");
		ok( HIWORD( pack->lparam ) == test_cs.cy, "height wrong\n");
		break;
	case WM_MOVE:
		ok( LOWORD( pack->lparam ) == test_cs.x, "x wrong\n");
		ok( HIWORD( pack->lparam ) == test_cs.y, "y wrong\n");
		break;
	case WM_ERASEBKGND:
		ok( pack->wparam != 0, "hdc was null\n");
		ret.val = 1;
		break;
	case WM_PAINT:
		ok( pack->lparam == 0, "lparam wrong\n");
		ok( pack->wparam == 0, "wparam wrong\n");
		dc = NtUserBeginPaint( pack->wininfo->handle, &ps );
		ok(ps.rcPaint.left == 0, "left wrong\n");
		ok(ps.rcPaint.right == test_cs.cx, "right wrong %ld %d\n",
			ps.rcPaint.right, test_cs.cx);
		ok(ps.rcPaint.top == 0, "top wrong\n");
		ok(ps.rcPaint.bottom == test_cs.cy, "bottom wrong %ld %d\n",
			ps.rcPaint.bottom, test_cs.cy);
		NtUserEndPaint( pack->wininfo->handle, &ps );
		break;
	default:
		dprintf("msg %04lx\n", pack->msg );
		break;
	}
	if (record)
		record_received( pack->wininfo->handle, pack->msg );

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

HANDLE get_cached_window_handle(void);

// magic numbers for everybody
void getminmax_callback(NTMINMAXPACKEDINFO *pack)
{
	NTWINCALLBACKRETINFO ret;
	HANDLE window = pack->wininfo->handle;
	//dprintf("wininfo = %p\n", pack->wininfo );
	//dprintf("handle = %p\n", window );
	//dprintf("cached handle = %p\n", get_cached_window_handle() );
	//dprintf("cached pointer = %p\n", get_cached_window_pointer() );
	ok( get_cached_window_handle() == window, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");
	ok( pack->wininfo != NULL, "wininfo NULL\n" );
	ok( window != NULL, "handle NULL\n" );
	ok( pack->wparam == 0, "wparam wrong %08x\n", pack->wparam );
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	ok( pack->msg == WM_GETMINMAXINFO, "message wrong %08lx\n", pack->msg );
	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	ok( pack->func != NULL, "func NULL\n" );

	record_received( pack->wininfo->handle, pack->msg );

	ret.val = 0;
	ret.size = sizeof pack->minmax;
	ret.buf = &pack->minmax;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void create_callback( NTCREATEPACKEDINFO *pack )
{
	NTWINCALLBACKRETINFO ret;

	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	ok( pack->func != NULL, "func NULL\n" );
	ok( get_cached_window_handle() == pack->wininfo->handle, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	record_received( pack->wininfo->handle, pack->msg );

	ok(pack->cs.cx == test_cs.cx, "width wrong %d\n", pack->cs.cx);
	ok(pack->cs.cy == test_cs.cy, "height wrong %d\n", pack->cs.cy);
	ok(pack->cs.x == test_cs.x, "x pos wrong %d\n", pack->cs.x);
	ok(pack->cs.y == test_cs.y, "y pos wrong %d\n", pack->cs.y);
	ok(pack->cs.style == test_cs.style, "style wrong %08lx\n", pack->cs.style);
	ok(pack->cs.dwExStyle == test_cs.dwExStyle, "exstyle wrong %08lx\n", pack->cs.dwExStyle);

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
	NTWINCALLBACKRETINFO ret;

	ok( pack->func != NULL, "func NULL\n" );
	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	record_received( pack->wininfo->handle, pack->msg );
	ok( pack->msg == WM_NCCALCSIZE, "message wrong %08lx\n", pack->msg );
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	ok( pack->wparam == testCalcSizeWparam, "wparam wrong\n");
	ok( get_cached_window_handle() == pack->wininfo->handle, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");

	ok( pack->params.rgrc[0].left == test_cs.x, "x position wrong %ld %d\n", pack->params.rgrc[0].left, test_cs.x);
	ok( pack->params.rgrc[0].top == test_cs.y, "y position wrong %ld %d\n", pack->params.rgrc[0].top, test_cs.y);
	ret.val = 0;
	ret.size = sizeof pack->params + sizeof pack->winpos;
	ret.buf = &pack->params;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void position_changing_callback( PNTPOSCHANGINGPACKEDINFO pack )
{
	NTWINCALLBACKRETINFO ret;

	//dprintf("position changing\n");
	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	record_received( pack->wininfo->handle, pack->msg );
	ok( pack->msg == WM_WINDOWPOSCHANGING, "message wrong %08lx\n", pack->msg );
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	ok( pack->func != NULL, "func NULL\n" );
	ok( get_cached_window_handle() == pack->wininfo->handle, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");
	ok( pack->winpos.hwnd == pack->wininfo->handle, "handle wrong\n");
	//ok( test_cs.style & WS_VISIBLE, "invisible window received WM_POSCHANGING\n");

	// check the position
	//dprintf("WM_WINDOWPOSCHANGING: %d,%d-%d,%d %08x\n",
		//pack->winpos.x, pack->winpos.y, pack->winpos.cx, pack->winpos.cy, pack->winpos.flags );
	ok( pack->winpos.x == test_winpos.x, "x pos wrong %d\n", pack->winpos.x);
	ok( pack->winpos.y == test_winpos.y, "y pos wrong %d\n", pack->winpos.y);
	ok( pack->winpos.cx == test_winpos.cx, "cx pos wrong %d\n", pack->winpos.cx);
	ok( pack->winpos.cy == test_winpos.cy, "cy pos wrong %d\n", pack->winpos.cy);

	ret.val = 0;
	ret.size = sizeof pack->winpos;
	ret.buf = &pack->winpos;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void position_changed_callback( PNTPOSCHANGINGPACKEDINFO pack )
{
	NTWINCALLBACKRETINFO ret;

	ok( pack->wininfo != NULL && pack->wininfo->handle != NULL, "*wininfo NULL\n" );
	record_received( pack->wininfo->handle, pack->msg );
	ok( pack->msg == WM_WINDOWPOSCHANGED, "message wrong %08lx\n", pack->msg );
	ok( pack->wndproc == testWndProc, "wndproc wrong %p\n", pack->wndproc );
	ok( pack->func != NULL, "func NULL\n" );
	ok( get_cached_window_handle() == pack->wininfo->handle, "cached handle mismatch\n");
	ok( get_cached_window_pointer() == pack->wininfo, "cached pointer mismatch\n");

	ret.val = 0;
	ret.size = 0;
	ret.buf = 0;

	NtCallbackReturn( &ret, sizeof ret, 0 );
}

void stub_callback( BYTE *data )
{
	dump_bin( data, 0x20 );
	NtCallbackReturn( 0, 0, 0 );
}

void init_callbacks( void )
{
	callback_table[NTWIN32_BASICMSG_CALLBACK] = &basicmsg_callback;
	callback_table[NTWIN32_THREAD_INIT_CALLBACK] = &init_callback;
	callback_table[NTWIN32_MINMAX_CALLBACK] = &getminmax_callback;
	callback_table[NTWIN32_CREATE_CALLBACK] = &create_callback;
	callback_table[NTWIN32_NCCALC_CALLBACK] = &nccalc_callback;
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

#define check_msg( hwnd, msg, n ) __check_msg( hwnd, msg, n, __LINE__ )
static inline void __check_msg( HWND hwnd, ULONG msg, ULONG* n, int line )
{
	ULONG val = received_msg[*n];
	while( received_hwnd[*n] && hwnd != received_hwnd[*n])
		(*n)++;
	if( val != msg)
		dprintf("%d: %ld sequence received %04lx != expected %04lx\n",
			 line, *n, val, msg );
	(*n)++;
}

#define USER_HANDLE_WINDOW 1

struct user_handle_entry_t {
	union {
		void *object;
		ULONG next_free;
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
	ok( user_shared_mem->max_window_handle >= lowpart, "max_window_handle %08lx %08lx\n", user_shared_mem->max_window_handle, lowpart);
	ok( user_handle_table[lowpart].type == type, "type wrong\n");
	ok( user_handle_table[lowpart].highpart == highpart, "highpart wrong\n");
	ok( user_handle_table[lowpart].owner != 0, "owner empty\n");
	ULONG kptr = (ULONG) user_handle_table[lowpart].object;
	ok( (kptr&~0xffff) != 0, "not a pointer\n");
	return (void*) kptr;
}

void dump_handles( void )
{
	struct user_shared_mem_t *user_shared_mem = user_info.Ptr[0];
	struct user_handle_entry_t *user_handle_table = user_info.Ptr[1];
	int i;

	for (i=0; i< user_shared_mem->max_window_handle; i++)
	{
		if (user_handle_table[i].next_free < 0x10000)
			continue;
		dprintf("%d %p %p %04x %04x\n", i,
			user_handle_table[i].object,
			user_handle_table[i].owner,
			user_handle_table[i].type,
			user_handle_table[i].highpart);
	}
}

void* kernel_to_user( void *kptr )
{
	return (void*) ((ULONG)kptr - get_user_pointer_offset());
}

void* user_to_kernel( void *uptr )
{
	return (void*) ((ULONG)uptr + get_user_pointer_offset());
}

void check_classinfo( PCLASSINFO kernel_clsptr )
{
	PCLASSINFO cls = kernel_to_user( kernel_clsptr );
	//dprintf("classinfo = %p (%p)\n", cls, kernel_clsptr );
	ok( cls->pSelf == kernel_clsptr, "self pointer wrong\n");
}

void test_point_from_window( HWND window, INT x, INT y, INT cx, INT cy )
{
	POINT pt;
	HWND hwnd;

	pt.x = x;
	pt.y = y;
	hwnd = NtUserWindowFromPoint( pt );
	ok( hwnd == window, "window wrong %p %p\n", hwnd, window);

	pt.x--;
	hwnd = NtUserWindowFromPoint( pt );
	ok( hwnd != window, "window wrong\n");

	pt.x++;
	pt.y--;
	hwnd = NtUserWindowFromPoint( pt );
	ok( hwnd != window, "window wrong\n");

	pt.y++;
	pt.x += test_cs.cx;
	hwnd = NtUserWindowFromPoint( pt );
	ok( hwnd != window, "window wrong\n");

	pt.x--;
	hwnd = NtUserWindowFromPoint( pt );
	ok( hwnd == window, "window wrong %p %p\n", hwnd, window);
}

// argument selects a set of window styles
void test_create_window( ULONG style, BOOL unicode )
{
	USER32_UNICODE_STRING cls, title;
	WCHAR title_str[] = L"test window";
	CHAR title_strA[] = "test window";
	HANDLE window;
	ULONG n = 0;
	WND *wndptr, *kernel_wndptr, *ptr;
	MSG msg;
	const int quit_magic = 0xdada;
	BOOL r;
	ULONG cx, cy;
	ULONG timer_id;
	PNTUSERINFO thread_user_info;

	clear_msg_sequence();

	// check the desktop window
	thread_user_info = get_thread_user_info();
	kernel_wndptr = thread_user_info->DesktopWindow;
	ok( kernel_wndptr != NULL, "desktop window null\n");
	wndptr = kernel_to_user( kernel_wndptr );
	kernel_wndptr = check_user_handle( wndptr->handle, USER_HANDLE_WINDOW );
	ok( kernel_wndptr == thread_user_info->DesktopWindow, "pointer mismatch %p %p\n",
		 kernel_wndptr, thread_user_info->DesktopWindow );

	cls.Buffer = test_class_name;
	cls.Length = sizeof test_class_name - 2;
	cls.MaximumLength = sizeof test_class_name;

	if (unicode)
	{
		title.Buffer = title_str;
		title.Length = sizeof title_str - 2;
		title.MaximumLength = 0;
	}
	else
	{
		title.Buffer = (WCHAR*) title_strA;
		title.Length = sizeof title_strA - 1;
		title.MaximumLength = 0;
		ok( title.Length & 1, "not odd\n");
	}

	test_cs.lpCreateParams = 0;
	test_cs.hInstance = get_exe_base();
	test_cs.hwndParent = 0;
	test_cs.hMenu = 0;
	test_cs.x = TEST_XPOS;
	test_cs.y = TEST_YPOS;
	test_cs.cx = TEST_WIDTH;
	test_cs.cy = TEST_HEIGHT;
	test_cs.style = style;
	test_cs.dwExStyle = WS_EX_WINDOWEDGE;	// set by NtUserCreateWindowEx

	//dprintf("Creating window\n");
	if (!(style & WS_VISIBLE))
	{
		test_winpos.x = TEST_XPOS;
		test_winpos.y = TEST_YPOS;
		test_winpos.cx = TEST_WIDTH;
		test_winpos.cy = TEST_HEIGHT;
	}
	else
	{
		test_winpos.x = 0;
		test_winpos.y = 0;
		test_winpos.cx = 0;
		test_winpos.cy = 0;
	}
	testCalcSizeWparam = FALSE;

	window = NtUserCreateWindowEx(0x80000000, &cls, &title, test_cs.style,
		test_cs.x, test_cs.y, test_cs.cx, test_cs.cy,
		test_cs.hwndParent, test_cs.hMenu, test_cs.hInstance, test_cs.lpCreateParams, 0x400 );
	ok( window != 0, "window handle zero\n");
	ok( get_cached_window_handle() == 0, "cached handle not clear\n");
	ok( get_cached_window_pointer() == 0, "cached pointer not clear\n");

	style |= WS_CLIPSIBLINGS;

	kernel_wndptr = check_user_handle( window, USER_HANDLE_WINDOW );
	wndptr = kernel_to_user( kernel_wndptr );
	//dprintf("wininfo = %p\n", wndptr );
	ok( wndptr->self == kernel_wndptr, "self pointer wrong\n");
	ok( wndptr->handle == window, "window handle wrong\n");
	ok( wndptr->style == style, "style wrong %08lx %08lx\n", wndptr->style, style);
	ok( wndptr->exstyle == test_cs.dwExStyle, "exstyle wrong %08lx %08lx\n", wndptr->exstyle, test_cs.dwExStyle);
	ok( wndptr->hInstance == test_cs.hInstance, "instance wrong\n");
	thread_user_info = get_thread_user_info();
	//ok( wndptr->parent == thread_user_info->DesktopWindow, "parent not desktop %p %p\n", wndptr->parent, thread_user_info->DesktopWindow);

	// check window rectangle
	ok( wndptr->rcWnd.left == test_cs.x, "x position wrong %ld\n", wndptr->rcWnd.left);
	ok( wndptr->rcWnd.top == test_cs.y, "y position wrong %ld\n", wndptr->rcWnd.top);
	cx = wndptr->rcWnd.right - wndptr->rcWnd.left;
	cy = wndptr->rcWnd.bottom - wndptr->rcWnd.top;
	ok( cx == test_cs.cx, "width wrong %ld\n", cx);
	ok( cy == test_cs.cy, "height wrong %ld\n", cy);

	// check client rectangle
	ok( wndptr->rcClient.left == test_cs.x, "x position wrong %ld\n", wndptr->rcClient.left );
	ok( wndptr->rcClient.top == test_cs.y, "y position wrong %ld\n", wndptr->rcClient.top);
	cx = wndptr->rcClient.right - wndptr->rcClient.left;
	cy = wndptr->rcClient.bottom - wndptr->rcClient.top;
	ok( cx == test_cs.cx, "width wrong %ld\n", cx);
	ok( cy == test_cs.cy, "height wrong %ld\n", cy);

	check_classinfo( wndptr->wndcls );

	check_msg( window, WM_GETMINMAXINFO, &n );
	check_msg( window, WM_NCCREATE, &n );
	check_msg( window, WM_NCCALCSIZE, &n );
	check_msg( window, WM_CREATE, &n );
	if (style & WS_VISIBLE)
	{
		check_msg( window, WM_SHOWWINDOW, &n );
		check_msg( window, WM_WINDOWPOSCHANGING, &n );
		check_msg( window, WM_ACTIVATEAPP, &n );
		check_msg( window, WM_NCACTIVATE, &n );
		check_msg( window, WM_ACTIVATE, &n );
		check_msg( window, WM_SETFOCUS, &n );
		check_msg( window, WM_NCPAINT, &n );
		check_msg( window, WM_ERASEBKGND, &n );
		check_msg( window, WM_WINDOWPOSCHANGED, &n );
		check_msg( window, WM_SIZE, &n );
		check_msg( window, WM_MOVE, &n );
	}
	ok( sequence == n, "got %ld != %ld messages\n", sequence, n);

	r = NtUserInvalidateRect( window, 0, 0 );
	ok( TRUE == r, "NtUserInvalidateRect failed\n");

	if (style & WS_VISIBLE)
	{
		r = NtUserPeekMessage( &msg, 0, 0, 0, 0 );
		ok( TRUE == r, "NtUserPeekMessage indicates message remaining (%04x)\n", msg.message);
		ok( msg.hwnd == window, "window wrong %p\n", msg.hwnd );
		ok( msg.message == WM_PAINT, "message wrong %08x\n", msg.message );
		ok( msg.wParam == 0, "wParam wrong %08x\n", msg.wParam );
		ok( msg.lParam == 0, "lParam wrong %08lx\n", msg.lParam );

		r = NtUserGetMessage( &msg, 0, 0, 0 );
		ok( TRUE == r, "NtUserGetMessage indicates message remaining (%04x)\n", msg.message);
		ok( msg.hwnd == window, "window wrong %p\n", msg.hwnd );
		ok( msg.message == WM_PAINT, "message wrong %08x\n", msg.message );
		ok( msg.wParam == 0, "wParam wrong %08x\n", msg.wParam );
		ok( msg.lParam == 0, "lParam wrong %08lx\n", msg.lParam );

		r = NtUserDispatchMessage( 0 );
		ok( 0 == r, "NtUserDispatchMessage failed %d\n", r);

		r = NtUserDispatchMessage( &msg );
		ok( 0 == r, "NtUserDispatchMessage failed %d\n", r);

		check_msg( window, WM_PAINT, &n );
	}

	// check the window handle -> pointer translation
	ptr = (PWND) NtUserCallOneParam( (ULONG) window, NTUCOP_GETWNDPTR );
	ok( wndptr == ptr, "NTUCOP_GETWNDPTR return wrong\n");

	// clear the message
	msg.hwnd = 0;
	msg.message = 0;
	msg.wParam = 0;
	msg.lParam = 0;

	// check that the null message works
	r = NtUserPostMessage( window, WM_NULL, 1, 1 );
	ok( TRUE == r, "NtPostMessage failed\n");
	ok( get_cached_window_handle() == 0, "cached handle not clear\n");
	ok( get_cached_window_pointer() == 0, "cached pointer not clear\n");

	r = NtUserGetMessage( &msg, 0, 0, 0 );
	ok( r == TRUE, "posted message not received\n");
	ok( msg.hwnd == window, "window wrong %p\n", msg.hwnd );
	ok( msg.message == WM_NULL, "message wrong %08x\n", msg.message );
	ok( msg.wParam == 1, "wParam wrong %08x\n", msg.wParam );
	ok( msg.lParam == 1, "lParam wrong %08lx\n", msg.lParam );
	ok( get_cached_window_handle() == 0, "cached handle not clear\n");
	ok( get_cached_window_pointer() == 0, "cached pointer not clear\n");

	r = NtUserPeekMessage( &msg, 0, 0, 0, 0 );
	ok( FALSE == r, "NtUserPeekMessage indicates message remaining (%04x)\n", msg.message);

	timer_id = NtUserSetTimer( window, 1, 10, 0 );
	ok( timer_id != 0, "timer id wrong\n");

	r = NtUserGetMessage( &msg, 0, 0, 0 );
	ok( r == TRUE, "timer message not received\n");

	ok( msg.hwnd == window, "window wrong %p\n", msg.hwnd );
	ok( msg.message == WM_TIMER, "message wrong %08x\n", msg.message );
	ok( msg.wParam == timer_id, "wParam wrong %08x %08lx\n", msg.wParam, timer_id );
	ok( msg.lParam == 0, "lParam wrong %08lx\n", msg.lParam );

	r = NtUserKillTimer( window, timer_id );
	ok( r == TRUE, "NtUserKillTimer failed\n");

	// move the window
	//dprintf("moving window\n");
	test_winpos.x = TEST_XPOS + 15;
	test_winpos.y = TEST_YPOS + 16;
	test_winpos.cx = TEST_WIDTH + 17;
	test_winpos.cy = TEST_HEIGHT + 18;
	test_cs.x = test_winpos.x;
	test_cs.y = test_winpos.y;
	test_cs.cx = test_winpos.cx;
	test_cs.cy = test_winpos.cy;
	testCalcSizeWparam = TRUE;
	r = NtUserMoveWindow( window, test_winpos.x, test_winpos.y, test_winpos.cx, test_winpos.cy, 0 );
	ok( TRUE == r, "NtPostMessage failed\n");

	if (style & WS_VISIBLE)
		test_point_from_window( window, test_cs.x, test_cs.y, test_cs.cx, test_cs.cy );

	ptr = (PWND) NtUserCallOneParam( (ULONG) window, NTUCOP_GETWNDPTR );
	ok(ptr->rcWnd.left == test_winpos.x, "rcWnd.left wrong\n");
	ok(ptr->rcWnd.top == test_winpos.y, "rcWnd.top wrong\n");
	ok(ptr->rcWnd.right == test_winpos.cx + test_winpos.x, "rcWnd.right wrong %ld %d\n",
		ptr->rcWnd.right, test_winpos.cx + test_winpos.x);
	ok(ptr->rcWnd.bottom == test_winpos.cy + test_winpos.y, "rcWnd.bottom wrong %ld %d\n",
		ptr->rcWnd.bottom, test_winpos.cy + test_winpos.y);

	check_msg( window, WM_WINDOWPOSCHANGING, &n );
	check_msg( window, WM_NCCALCSIZE, &n );
	check_msg( window, WM_WINDOWPOSCHANGED, &n );

	// check that PostQuitMessage will work
	r = NtUserCallOneParam( quit_magic, NTUCOP_POSTQUITMESSAGE );
	ok( TRUE == r, "NtPostMessage failed\n");

	r = NtUserGetMessage( &msg, 0, 0, 0 );
	ok( r == FALSE, "quit message not received\n");
	ok( msg.hwnd == 0, "window wrong %p\n", msg.hwnd );
	ok( msg.message == WM_QUIT, "message wrong %08x\n", msg.message );
	ok( msg.wParam == quit_magic, "wParam wrong %08x\n", msg.wParam );
	ok( msg.lParam == 0, "lParam wrong %08lx\n", msg.lParam );
	ok( get_cached_window_handle() == 0, "cached handle not clear\n");
	ok( get_cached_window_pointer() == 0, "cached pointer not clear\n");

	//dprintf("destroying window\n");
	test_winpos.x = 0;
	test_winpos.y = 0;
	test_winpos.cx = 0;
	test_winpos.cy = 0;

	r = NtUserDestroyWindow( window );
	ok( TRUE == r, "NtUserDestroyWindow failed\n");

	if (style & WS_VISIBLE)
	{
		check_msg( window, WM_WINDOWPOSCHANGING, &n );
		check_msg( window, WM_WINDOWPOSCHANGED, &n );
		check_msg( window, WM_NCACTIVATE, &n );
	}
	check_msg( window, WM_DESTROY, &n );
	check_msg( window, WM_NCDESTROY, &n );
}

void test_window()
{
	register_class();
	//dprintf("non-visible\n");
	test_create_window( WS_CAPTION | WS_SYSMENU | WS_GROUP, FALSE );
	//dprintf("visible\n");
	test_create_window( WS_CAPTION | WS_SYSMENU | WS_GROUP | WS_VISIBLE, FALSE );
	//dprintf("non-visible\n");
	test_create_window( WS_CAPTION | WS_SYSMENU | WS_GROUP, TRUE );
	//dprintf("visible\n");
	test_create_window( WS_CAPTION | WS_SYSMENU | WS_GROUP | WS_VISIBLE, TRUE );
}

void NtProcessStartup( void )
{
	log_init();
	become_gui_thread();
	test_window();
	log_fini();
}
