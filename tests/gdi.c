/*
 * native test suite
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

#include "ntapi.h"
#include "ntwin32.h"
#include "log.h"

////// gdi32 callbacks

#define NUM_CALLBACKS 90

int cb_called[NUM_CALLBACKS];

void end_cb( ULONG status )
{
	NtCallbackReturn( 0, 0, 0 );
}

#define cb(x)						   \
void NTAPI callback##x(void *arg)	   \
{									   \
	if (0) dprintf(#x " %p %p\n", arg, &arg);  \
	cb_called[x]++;					 \
	end_cb(0);						  \
}

cb(0)
cb(1)
cb(2)
cb(3)
cb(4)
cb(5)
cb(6)
cb(7)
cb(8)
cb(9)
cb(10)
cb(11)
cb(12)
cb(13)
cb(14)
cb(15)
cb(16)
cb(17)
cb(18)
cb(19)
cb(20)
cb(21)
cb(22)
cb(23)
cb(24)
cb(25)
cb(26)
cb(27)
cb(28)
cb(29)
cb(30)
cb(31)
cb(32)
cb(33)
cb(34)
cb(35)
cb(36)
cb(37)
cb(38)
cb(39)
cb(40)
cb(41)
cb(42)
cb(43)
cb(44)
cb(45)
cb(46)
cb(47)
cb(48)
cb(49)
cb(50)
cb(51)
cb(52)
cb(53)
cb(54)
cb(55)
cb(56)
cb(57)
cb(58)
cb(59)
cb(60)
cb(61)
cb(62)
cb(63)
cb(64)
cb(65)
cb(66)
cb(67)
cb(68)
cb(69)
cb(70)
cb(71)
cb(72)
cb(73)
cb(74)
cb(75)
cb(76)
cb(77)
cb(78)
cb(79)
cb(80)
cb(81)
cb(82)
cb(83)
cb(84)
cb(85)
cb(86)
cb(87)
cb(88)
cb(89)

#undef cb

#define cbt(x) callback##x

void *callback_table[NUM_CALLBACKS] = {
	cbt(0), cbt(1), cbt(2), cbt(3), cbt(4), cbt(5), cbt(6), cbt(7), cbt(8), cbt(9),
	cbt(10), cbt(11), cbt(12), cbt(13), cbt(14), cbt(15), cbt(16), cbt(17), cbt(18), cbt(19),
	cbt(20), cbt(21), cbt(22), cbt(23), cbt(24), cbt(25), cbt(26), cbt(27), cbt(28), cbt(29),
	cbt(30), cbt(31), cbt(32), cbt(33), cbt(34), cbt(35), cbt(36), cbt(37), cbt(38), cbt(39),
	cbt(40), cbt(41), cbt(42), cbt(43), cbt(44), cbt(45), cbt(46), cbt(47), cbt(48), cbt(49),
	cbt(50), cbt(51), cbt(52), cbt(53), cbt(54), cbt(55), cbt(56), cbt(57), cbt(58), cbt(59),
	cbt(60), cbt(61), cbt(62), cbt(63), cbt(64), cbt(65), cbt(66), cbt(67), cbt(68), cbt(69),
	cbt(70), cbt(71), cbt(72), cbt(73), cbt(74), cbt(75), cbt(76), cbt(77), cbt(78), cbt(79),
	cbt(80), cbt(81), cbt(82), cbt(83), cbt(84), cbt(85), cbt(86), cbt(87), cbt(88), cbt(89),
};

#undef cbt

////// user32 callbacks

#define ucb(type,x)						   \
void NTAPI ucallback##type##x(void *arg)	  \
{											 \
	if (1) dprintf(#x " %p %p\n", arg, &arg); \
	ucb##type##_called[x]++;				  \
	end_cb(0);								\
}

// neutral callbacks

ULONG ucbN_called[9];

ucb(N,0)
ucb(N,1)
ucb(N,2)
ucb(N,3)
ucb(N,4)
ucb(N,5)
ucb(N,6)
ucb(N,7)
ucb(N,8)

void *ucbN_funcs[9] = {
	ucallbackN0,
	ucallbackN1,
	ucallbackN2,
	ucallbackN3,
	ucallbackN4,
	ucallbackN5,
	ucallbackN6,
	ucallbackN7,
	ucallbackN8,
};

// unicode callbacks

ULONG ucbW_called[20];

ucb(W,0)
ucb(W,1)
ucb(W,2)
ucb(W,3)
ucb(W,4)
ucb(W,5)
ucb(W,6)
ucb(W,7)
ucb(W,8)
ucb(W,9)
ucb(W,10)
ucb(W,11)
ucb(W,12)
ucb(W,13)
ucb(W,14)
ucb(W,15)
ucb(W,16)
ucb(W,17)
ucb(W,18)
ucb(W,19)

void *ucbW_funcs[20] = {
	ucallbackW0,
	ucallbackW1,
	ucallbackW2,
	ucallbackW3,
	ucallbackW4,
	ucallbackW5,
	ucallbackW6,
	ucallbackW7,
	ucallbackW8,
	ucallbackW9,
	ucallbackW10,
	ucallbackW11,
	ucallbackW12,
	ucallbackW13,
	ucallbackW14,
	ucallbackW15,
	ucallbackW16,
	ucallbackW17,
	ucallbackW18,
	ucallbackW19,
};

// ascii callbacks

ULONG ucbA_called[20];

ucb(A,0)
ucb(A,1)
ucb(A,2)
ucb(A,3)
ucb(A,4)
ucb(A,5)
ucb(A,6)
ucb(A,7)
ucb(A,8)
ucb(A,9)
ucb(A,10)
ucb(A,11)
ucb(A,12)
ucb(A,13)
ucb(A,14)
ucb(A,15)
ucb(A,16)
ucb(A,17)
ucb(A,18)
ucb(A,19)

void *ucbA_funcs[20] = {
	ucallbackA0,
	ucallbackA1,
	ucallbackA2,
	ucallbackA3,
	ucallbackA4,
	ucallbackA5,
	ucallbackA6,
	ucallbackA7,
	ucallbackA8,
	ucallbackA9,
	ucallbackA10,
	ucallbackA11,
	ucallbackA12,
	ucallbackA13,
	ucallbackA14,
	ucallbackA15,
	ucallbackA16,
	ucallbackA17,
	ucallbackA18,
	ucallbackA19,
};

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

ULONG get_thread_id()
{
	ULONG *teb = get_teb();
	return teb[0x24/4];
}

ULONG get_process_id()
{
	ULONG *teb = get_teb();
	return teb[0x20/4];
}

void *get_win32threadinfo( void )
{
	void **p = get_teb();
	return p[0x40/sizeof (*p)];
}

const int ofs_peb_in_teb = 0x30;

void* get_peb( void )
{
	void **p = get_teb();
	return p[ofs_peb_in_teb/sizeof (*p)];
}

const int ofs_shared_handle_table_in_peb = 0x94;

void* get_gdi_shared_handle_table( void )
{
	void **p = get_peb();
	return p[ofs_shared_handle_table_in_peb/sizeof (*p)];
}

void set_gdi_shared_handle_table( void *table )
{
	void **p = get_peb();
	p[ofs_shared_handle_table_in_peb/sizeof (*p)] = table;
}

const int ofs_post_process_routine_in_peb = 0x14c;

void set_post_process_init_routine(ULONG val)
{
	ULONG *p = get_peb();
	p[ofs_post_process_routine_in_peb/4] = val;
}

const int ofs_callback_table_in_peb = 0x2c;

void set_kernel_callback_table( void *table )
{
	void **p = get_peb();
	p[ofs_callback_table_in_peb/4] = table;
}

void *get_kernel_callback_table( void )
{
	void **p = get_peb();
	return p[ofs_callback_table_in_peb/4];
}

const int ofs_session_id_in_peb = 0x1d4;

unsigned int get_session_id( void )
{
	unsigned int *p = get_peb();
	return p[ofs_session_id_in_peb/4];
}

void *get_exe_base( void )
{
	void** peb = get_peb();
	return peb[8/4];
}

void *get_readonly_shared_server_data( void )
{
	void** peb = get_peb();
	return peb[0x54/4];
}

void *get_user_info( HANDLE handle )
{
	gdi_handle_table_entry *table = get_gdi_shared_handle_table();
	ULONG Index = get_handle_index(handle);
	return table[Index].user_info;
}

BOOLEAN check_gdi_handle_any(HANDLE handle)
{
	ULONG Index = get_handle_index(handle);
	ULONG Top = (((ULONG)handle)>>16);

	gdi_handle_table_entry *table = get_gdi_shared_handle_table();

	if (sizeof table[0] != 0x10)
	{
		dprintf("%d\n", __LINE__);
		return FALSE;
	}
	// stock objects have ProcessId = 0
	/*if (table[Index].ProcessId == 0)
	{
		dprintf("ProcessId is zero\n");
		return FALSE;
	}*/
	if (table[Index].Count != 0)
	{
		dprintf("%d\n", __LINE__);
		return FALSE;
	}
	if (table[Index].Upper != Top)
	{
		dprintf("Top = %04lx\n", Top);
		dprintf("table[Index].Upper = %04x\n", table[Index].Upper);
		return FALSE;
	}
	return TRUE;
}

BOOLEAN check_gdi_handle(HANDLE handle)
{
	return check_gdi_handle_any(handle);
}

BOOLEAN check_pen_handle(HANDLE handle)
{
	ULONG ObjectType = get_handle_type(handle);
	ULONG Index = get_handle_index(handle);
	gdi_handle_table_entry *table = get_gdi_shared_handle_table();

	if (!check_gdi_handle_any(handle))
		return FALSE;
	if ((table[Index].Type&0x7f) != GDI_OBJECT_BRUSH)
	{
		dprintf("table[Index].Type = %04x\n", table[Index].Type);
		return FALSE;
	}
	if (GDI_OBJECT_PEN != ObjectType)
	{
		dprintf("ObjectType = %04lx\n", ObjectType);
		return FALSE;
	}
	return TRUE;
}

BOOLEAN verify_gdi_handle_deleted(HANDLE handle)
{
	ULONG Index = get_handle_index(handle);

	gdi_handle_table_entry *table = get_gdi_shared_handle_table();

	if (table[Index].Type&0x7f)
		return FALSE;
	if (table[Index].Count)
		return FALSE;
	if (table[Index].ProcessId)
		return FALSE;
	return TRUE;
}

void test_gdi_init( void )
{
	NTSTATUS r;
	void *p;
	ULONG id;

	// set the kernel callback table
	p = get_kernel_callback_table();
	ok( p == 0, "returned %p\n", p);

	p = get_gdi_shared_handle_table();
	ok( p == 0, "returned %p\n", p);

	id = get_session_id();
	ok( id == 0, "id wrong %08lx\n", id);

	set_kernel_callback_table( &callback_table );

	p = get_kernel_callback_table();
	ok( p == &callback_table, "returned %p\n", p);

	r = NtGdiInit();
	ok( r == 1, "return %08lx\n", r );

	// check the ClientThreadSetup callback is called
	ok( cb_called[NTWIN32_THREAD_INIT_CALLBACK] == 1, "init callback not called\n");
	{
		int i;
		for (i=0; i<NUM_CALLBACKS; i++)
			if (i != NTWIN32_THREAD_INIT_CALLBACK)
				ok( cb_called[i] == 0, "cb%d called\n", i);
	}

	p = get_gdi_shared_handle_table();
	ok( p != 0, "returned %p\n", p);
}

void test_user_init( void )
{
	NTSTATUS r;
	void *p;
	USER_PROCESS_CONNECT_INFO info;
	ULONG id;

	r = NtUserProcessConnect( NtCurrentProcess(), 0, 0 );
	ok( r == STATUS_UNSUCCESSFUL, "return %08lx\n", r );

	r = NtUserProcessConnect( NtCurrentProcess(), 0, sizeof info );
	ok( r == STATUS_UNSUCCESSFUL, "return %08lx\n", r );

	memset( &info, 0, sizeof info );
	r = NtUserProcessConnect( NtCurrentProcess(), &info, sizeof info );
	ok( r == STATUS_UNSUCCESSFUL, "return %08lx\n", r );

	info.Version = 0x00050000;
	r = NtUserProcessConnect( NtCurrentProcess(), &info, sizeof info );
	ok( r == STATUS_SUCCESS, "return %08lx\n", r );

	ok( info.Version == 0x50000, "version changed\n");
	ok( info.Unknown == 0, "empty 0 set\n");
	ok( info.MinorVersion == 0, "empty 1 set\n");
	ok( info.Ptr[0] != 0, "ptr 0 not set\n");  // <- this appears to be important
	ok( info.Ptr[1] != 0, "ptr 1 not set\n");
	ok( info.Ptr[2] != 0, "ptr 2 not set\n");
	ok( info.Ptr[3] != 0, "ptr 3 not set\n");

	//r = NtUserInitializeClientPfnArrays( 0, 0, 0, 0 );
	//ok( r == 0, "return %08lx\n", r );

	p = get_exe_base();
	r = NtUserInitializeClientPfnArrays( ucbN_funcs, ucbW_funcs, ucbA_funcs, p );
	ok( r == 0, "return %08lx\n", r );

	p = get_readonly_shared_server_data();
	ok( p == 0, "readonly shared server data set\n");

	id = get_session_id();
	ok( id == 0, "id wrong %08lx\n", id);
}

void test_gdi_font_assoc( void )
{
	NTSTATUS r;

	r = NtGdiQueryFontAssocInfo(0);
	ok( r == 0, "return %08lx\n", r );
}

void test_not_connected( void )
{
	void *p = get_win32threadinfo();
	ok( p == NULL, "already a gdi thread\n");
}

void test_create_compat_dc( void )
{
	ULONG r;
	HANDLE hdc, hdc2;

	hdc = NtGdiCreateCompatibleDC(0);
	ok (check_gdi_handle(hdc), "invalid gdi handle %p\n", hdc );

	hdc2 = NtGdiCreateCompatibleDC(0);
	ok (check_gdi_handle(hdc2), "invalid gdi handle %p\n", hdc2 );

	r = NtGdiDeleteObjectApp(hdc2);
	ok( r == TRUE, "delete failed\n");
	ok (verify_gdi_handle_deleted(hdc2), "handle %p still valid\n", hdc2 );

	r = NtGdiDeleteObjectApp(hdc);
	ok( r == TRUE, "delete failed\n");
	ok (verify_gdi_handle_deleted(hdc), "handle %p still valid\n", hdc );
}

void test_get_dc( void )
{
	ULONG type;
	HANDLE dc;
	GDI_DEVICE_CONTEXT_SHARED *dcshm;
	BOOL r;
	//HANDLE white_brush = NtGdiGetStockObject( WHITE_BRUSH );
	//HANDLE black_pen = NtGdiGetStockObject( BLACK_PEN );

	dc = NtUserGetDC(0);
	ok( check_gdi_handle(dc), "invalid gdi handle %p\n", dc );

	type = get_handle_type(dc);
	ok( type == GDI_OBJECT_DC, "wrong handle type %ld\n", type );

	dcshm = get_user_info( dc );
	if (dcshm)
	{
		//ok( dcshm->Brush == white_brush, "default brush wrong\n" );
		//ok( dcshm->Pen == black_pen, "default pen wrong\n" );
	}
	else
		ok( 0, "dcshm null\n" );

	ok( dcshm->BackgroundColor == RGB( 255, 255, 255 ), "default bk color wrong\n");
	ok( dcshm->TextColor == RGB( 0, 0, 0 ), "default text color wrong\n");

	r = NtGdiDeleteObjectApp( dc );
	ok( r == TRUE, "delete failed\n");
}

void check_stock_brush( ULONG id )
{
	HANDLE brush;
	ULONG type;

	brush = NtGdiGetStockObject( id );
	type = get_handle_type( brush );
	ok( type == GDI_OBJECT_BRUSH, "brush wrong handle type %ld\n", type );
	ok( get_user_info( brush ) == NULL, "user_info not null\n");
}

void test_stock_brush( void )
{
	check_stock_brush( WHITE_BRUSH );
	check_stock_brush( LTGRAY_BRUSH );
	check_stock_brush( GRAY_BRUSH );
	check_stock_brush( DKGRAY_BRUSH );
	check_stock_brush( BLACK_BRUSH );
	check_stock_brush( NULL_BRUSH );
	check_stock_brush( HOLLOW_BRUSH );
}

void test_solid_brush( void )
{
	HANDLE brush;
	ULONG type;

	brush = NtGdiCreateSolidBrush( RGB(1, 2, 3), 0 );
	type = get_handle_type( brush );
	ok( type == GDI_OBJECT_BRUSH, "brush wrong handle type %ld\n", type );
	//ok( get_user_info( brush ) == NULL, "user_info not null\n");
}

BOOL rect_equal( PRECT rect, INT left, INT top, INT right, INT bottom )
{
	return rect->left == left && rect->top == top &&
		rect->right == right && rect->bottom == bottom;
}

void set_rect( PRECT rect, INT left, INT top, INT right, INT bottom )
{
	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;
}

void test_region( void )
{
	GDI_REGION_SHARED* info;
	char buffer[0x100];
	RGNDATA *data;
	HRGN region;
	ULONG type;
	RECT rect;
	int r;

	region = NtGdiCreateRectRgn( 0, 0, 0, 0 );
	ok( region != 0, "region was null");
	info = get_user_info( region );
	info->flags = 0x30;
	info->type = 0;

	type = get_handle_type( region );
	ok( type == GDI_OBJECT_REGION, "region wrong handle type %ld\n", type );

	r = NtGdiSetRectRgn( 0, 0, 0, 0, 0 );
	ok( r == FALSE, "NtGdiSetRectRgn failed\n");

	r = NtGdiSetRectRgn( region, 9, 10, 19, 20 );
	ok( r == TRUE, "NtGdiSetRectRgn failed\n");

	r = NtGdiGetRgnBox( region, &rect );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );

	ok( rect_equal( &rect, 9, 10, 19, 20 ), "rect wrong\n");

	r = NtGdiEqualRgn( 0, 0 );
	ok( r == ERROR, "NtGdiEqualRgn failed %d\n", r);

	r = NtGdiEqualRgn( 0, region );
	ok( r == FALSE, "NtGdiEqualRgn failed %d\n", r);

	r = NtGdiEqualRgn( region, 0 );
	ok( r == FALSE, "NtGdiEqualRgn failed %d\n", r);

	r = NtGdiEqualRgn( region, region );
	ok( r == TRUE, "NtGdiEqualRgn failed %d\n", r);

	r = NtGdiOffsetRgn( region, 1, 2 );
	ok( r == SIMPLEREGION, "NtGdiOffsetRgn failed %d\n", r);

	r = NtGdiOffsetRgn( 0, 1, 2 );
	ok( r == ERROR, "NtGdiOffsetRgn failed %d\n", r);

	r = NtGdiGetRgnBox( region, &rect );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );

	ok( rect_equal( &rect, 10, 12, 20, 22 ), "rect wrong\n");

	r = NtGdiGetRgnBox( 0, 0 );
	ok( r == ERROR, "Region type wrong %d\n", r );

	r = NtGdiGetRgnBox( region, 0 );
	ok( r == ERROR, "Region type wrong %d\n", r );

	r = NtGdiGetRgnBox( 0, &rect );
	ok( r == ERROR, "Region type wrong %d\n", r );

	data = (RGNDATA*) buffer;
	r = NtGdiGetRegionData( region, sizeof buffer, data );
	ok( r == 0x30, "size wrong %d\n", r );

	ok( data->rdh.dwSize == 0x20, "dwSize wrong %ld\n", data->rdh.dwSize );
	ok( data->rdh.iType == NULLREGION, "iType wrong %ld\n", data->rdh.iType );
	ok( data->rdh.nCount == 1, "nCount wrong %ld\n", data->rdh.nCount );
	ok( rect_equal( &data->rdh.rcBound, 10, 12, 20, 22 ), "rect wrong\n");

	r = NtGdiPtInRegion( 0, 0, 0 );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiPtInRegion( region, 0, 0 );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiPtInRegion( region, 10, 12 );
	ok( r == TRUE, "return wrong\n");

	r = NtGdiPtInRegion( region, 19, 21 );
	ok( r == TRUE, "return wrong\n");

	r = NtGdiPtInRegion( region, 19, 22 );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiPtInRegion( region, 20, 22 );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiPtInRegion( region, 15, 15 );
	ok( r == TRUE, "return wrong\n");

	set_rect( &rect, 0, 0, 0, 0 );
	r = NtGdiRectInRegion( 0, 0 );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiRectInRegion( 0, &rect );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiRectInRegion( region, 0 );
	ok( r == FALSE, "return wrong\n");

	set_rect( &rect, 0, 0, 0, 0 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == FALSE, "return wrong\n");

	set_rect( &rect, 0, 0, 15, 15 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == TRUE, "return wrong\n");

	set_rect( &rect, 15, 15, 0, 0 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == TRUE, "return wrong\n");

	set_rect( &rect, 15, 15, 15, 15 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == TRUE, "return wrong\n");

	set_rect( &rect, 9, 11, 10, 12 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == FALSE, "return wrong\n");

	set_rect( &rect, 19, 21, 20, 22 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == TRUE, "return wrong\n");

	set_rect( &rect, 19, 22, 20, 22 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == FALSE, "return wrong\n");

	set_rect( &rect, 40, 40, 40, 40 );
	r = NtGdiRectInRegion( region, &rect );
	ok( r == FALSE, "return wrong\n");

	r = NtGdiDeleteObjectApp( region );
	ok( r == TRUE, "delete failed\n");
}

void test_region_shared( void )
{
	HRGN region;
	ULONG type;
	GDI_REGION_SHARED* info;
	RECT rect;
	int r;

	region = NtGdiCreateRectRgn( 0, 0, 1, 1 );
	ok( region != 0, "region was null");

	type = get_handle_type( region );
	ok( type == GDI_OBJECT_REGION, "region wrong handle type %ld\n", type );
	ok( check_gdi_handle( region ), "invalid gdi handle %p\n", region );
	info = get_user_info( region );
	ok( info != NULL, "user_info was null\n");
	ok( info->type == 0, "type wrong %ld\n", info->type);
	ok( rect_equal( &info->rect, 0, 0, 0, 0 ), "first rect wrong\n");

	// flags      return of NtGdiSetRectRgn
	// 0xffffffff ERROR
	// 0xffff     ERROR
	// 0xff       ERROR
	// 0xf        ERROR
	// 0          ERROR
	// 0x10       NULLREGION
	// 0x20       ERROR
	// 0x50       NULLREGION
	// 0x1f       ERROR
	// 0x17       ERROR
	// 0x13       ERROR
	// 0x11       ERROR
	// 0x18       NULLREGION
	// 0x1c       NULLREGION
	// 0x1e       NULLREGION
	info->flags = 0;
	info->type = 0;
	r = NtGdiSetRectRgn( region, 0, 0, 0, 0 );
	ok( info == get_user_info( region ), "region changed\n");
	ok( r == ERROR, "NtGdiSetRectRgn return wrong %08x\n", r);

	// initialize shared data
	info->flags = 0x30;
	info->type = NULLREGION;
	set_rect( &info->rect, 0, 0, 0, 0 );

	r = NtGdiGetRgnBox( region, &rect );
	ok( r == NULLREGION, "Region type wrong %d\n", r );
	ok( rect_equal( &rect, 0, 0, 0, 0 ), "rect wrong\n");
	ok( info == get_user_info( region ), "region changed\n");
	ok( info->flags == 0x10, "flags not changed %08lx\n", info->flags);

	// set the correct number of rectangles
	set_rect( &info->rect, 9, 10, 19, 20 );
	info->type = SIMPLEREGION;
	info->flags |= 0x20;
	r = NtGdiGetRgnBox( region, &rect );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );
	ok( info == get_user_info( region ), "region changed\n");
	ok( rect_equal( &rect, 9, 10, 19, 20 ), "rect wrong\n");
	ok( info->flags == 0x10, "flags not changed %08lx\n", info->flags);

	//dprintf("rect = %ld,%ld-%ld,%ld\n", rect.left, rect.top, rect.right, rect.bottom );

	r = NtGdiSetRectRgn( region, 1, 2, 3, 4 );
	ok( r == TRUE, "NtGdiSetRectRgn failed\n");
	ok( info == get_user_info( region ), "region changed\n");
	ok( rect_equal( &info->rect, 1, 2, 3, 4 ), "rect wrong\n");

	r = NtGdiGetRgnBox( region, &rect );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );
	ok( rect_equal( &rect, 1, 2, 3, 4 ), "rect wrong\n");

	r = NtGdiDeleteObjectApp( region );
	ok( r == TRUE, "delete failed\n");
}

void test_multiregion( void )
{
	HRGN region1, region2, region3;
	GDI_REGION_SHARED* info;
	int r;

	region1 = NtGdiCreateRectRgn( 0, 0, 1, 1 );
	ok( region1 != 0, "region was null");

	info = get_user_info( region1 );
	info->flags = 0x30;
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 5, 5, 20, 20 );

	region2 = NtGdiCreateRectRgn( 0, 0, 1, 1 );
	ok( region2 != 0, "region was null");

	info = get_user_info( region2 );
	info->flags = 0x30;
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 0, 0, 15, 15 );

	region3 = NtGdiCreateRectRgn( 0, 0, 1, 1 );
	ok( region3 != 0, "region was null");

	info = get_user_info( region3 );
	info->flags = 0x30;
	info->type = NULLREGION;
	set_rect( &info->rect, 0, 0, 0, 0 );

	r = NtGdiCombineRgn( region3, region1, region2, RGN_AND );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );
	info = get_user_info( region3 );
	ok( info->type == r, "type wrong %ld\n", info->type);
	ok( rect_equal( &info->rect, 5, 5, 15, 15 ), "rect wrong\n");
	ok( info->flags == 0x10, "flags not changed %08lx\n", info->flags);

	// set the rectangle region
	info = get_user_info( region1 );
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 5, 6, 10, 11 );
	info->flags = 0x30;

	// set the rectangle region
	info = get_user_info( region2 );
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 10, 11, 15, 16 );
	info->flags = 0x30;

	// region1 does not overlap region2
	r = NtGdiCombineRgn( region3, region1, region2, RGN_AND );
	ok( r == NULLREGION, "Region type wrong %d\n", r );
	info = get_user_info( region3 );
	ok( info->type == r, "type wrong %ld\n", info->type);

	// set the rectangle region
	info = get_user_info( region1 );
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 5, 6, 20, 21 );
	info->flags = 0x30;

	// set the rectangle region
	info = get_user_info( region2 );
	info->type = SIMPLEREGION;
	set_rect( &info->rect, 10, 11, 15, 16 );
	info->flags = 0x30;

	// region1 full contains region2
	r = NtGdiCombineRgn( region3, region1, region2, RGN_AND );
	ok( r == SIMPLEREGION, "Region type wrong %d\n", r );
	info = get_user_info( region3 );
	ok( rect_equal( &info->rect, 10, 11, 15, 16 ), "rect wrong\n");
	ok( info->flags == 0x10, "flags not changed %08lx\n", info->flags);
	ok( info->type == r, "type wrong %ld\n", info->type);

	r = NtGdiDeleteObjectApp( region1 );
	ok( r == TRUE, "delete failed\n");

	r = NtGdiDeleteObjectApp( region2 );
	ok( r == TRUE, "delete failed\n");

	r = NtGdiDeleteObjectApp( region3 );
	ok( r == TRUE, "delete failed\n");
}

void test_savedc(void)
{
	ULONG type;
	HDC dc;
	int r;

	r = NtGdiSaveDC( 0 );
	ok( r == FALSE, "savedc failed\n");

	r = NtGdiRestoreDC( 0, 0 );
	ok( r == FALSE, "restoredc failed\n");

	dc = NtUserGetDC( 0 );
	ok( check_gdi_handle( dc ), "invalid gdi handle %p\n", dc );

	type = get_handle_type( dc );
	ok( type == GDI_OBJECT_DC, "wrong handle type %ld\n", type );

	r = NtGdiRestoreDC( dc, -1 );
	ok( r == FALSE, "restoredc failed\n");

	r = NtGdiSaveDC( dc );
	ok( r == 1, "savedc failed %d\n", r);

	r = NtGdiRestoreDC( dc, r );
	ok( r == TRUE, "restoredc failed\n");

	r = NtGdiRestoreDC( dc, r );
	ok( r == FALSE, "restoredc failed\n");

	r = NtGdiSaveDC( dc );
	ok( r == 1, "savedc failed %d\n", r);

	r = NtGdiRestoreDC( dc, -1 );
	ok( r == TRUE, "restoredc failed\n");

	r = NtGdiRestoreDC( dc, -1 );
	ok( r == FALSE, "restoredc failed\n");

	r = NtGdiSaveDC( dc );
	ok( r == 1, "savedc failed %d\n", r);

	r = NtGdiSaveDC( dc );
	ok( r == 2, "savedc failed %d\n", r);

	r = NtGdiSaveDC( dc );
	ok( r == 3, "savedc failed %d\n", r);

	r = NtGdiRestoreDC( dc, 0 );
	ok( r == FALSE, "restoredc failed\n");

	r = NtGdiRestoreDC( dc, -3 );
	ok( r == TRUE, "restoredc failed\n");

	r = NtGdiRestoreDC( dc, 1 );
	ok( r == FALSE, "restoredc failed\n");

	r = NtGdiDeleteObjectApp( dc );
	ok( r == TRUE, "delete failed\n");

	// NtGdiFlush appears to return 0x93...
	r = NtGdiFlush();
	ok( r == 0x93, "flush failed %d\n", r);

	r = NtGdiFlush();
	ok( r == 0x93, "flush failed %d\n", r);
}

static inline BOOLEAN NtUserReleaseDC( HDC dc )
{
	return NtUserCallOneParam( (ULONG) dc, NTUCOP_RELEASEDC );
}

BOOLEAN dc_info_changed(
	GDI_DEVICE_CONTEXT_SHARED *info,
	GDI_DEVICE_CONTEXT_SHARED *backup )
{
	BYTE *p1, *p2;
	ULONG i;
	p1 = (BYTE*) info;
	p2 = (BYTE*) backup;
	for (i=0; i<sizeof *info; i++)
	{
		if (p1[i] != p2[i])
		{
			dprintf("changed at offset %04lx\n", i);
			return TRUE;
		}
	}
	return FALSE;
}

void test_bitmap(void)
{
	HDC hdc;
	HBITMAP bitmap, old;
	ULONG type;
	GDI_DEVICE_CONTEXT_SHARED *info;
	GDI_DEVICE_CONTEXT_SHARED backup;

	hdc = NtUserGetDC( 0 );
	info = get_user_info( hdc );
	//dprintf("after NtUserGetDC\n");
	memcpy( &backup, info, sizeof *info );

	bitmap = NtGdiCreateCompatibleBitmap( hdc, 16, 16 );

	type = get_handle_type( bitmap );
	ok( type == GDI_OBJECT_BITMAP, "wrong handle type %ld\n", type );
	ok( check_gdi_handle( bitmap ), "invalid gdi handle %p\n", bitmap );
	ok( NULL == get_user_info( bitmap ), "info was not NULL\n");

	old = NtGdiSelectBitmap( hdc, bitmap );
	ok( NULL == old, "NtGdiSelectBitmap should return NULL\n");

	info = get_user_info( hdc );
	ok( info != NULL, "info was null\n");
	ok( !dc_info_changed( info, &backup ), "dc info changed\n");

	old = NtGdiSelectBitmap( hdc, old );
	ok( NULL == old, "NtGdiSelectBitmap should return NULL\n");
	//dprintf("after NtGdiSelectBitmap 2\n");
	ok( !dc_info_changed( info, &backup ), "dc info changed\n");

	NtUserReleaseDC( hdc );
	NtGdiDeleteObjectApp( bitmap );
}

void test_pen(void)
{
	ULONG r;
	HPEN pen;

	pen = NtGdiCreatePen( 0, 0, 0, 0 );
	ok( check_pen_handle( pen ), "invalid gdi pen %p\n", pen );

	pen = NtGdiCreatePen( 0, 2, 0, 0 );
	ok( check_pen_handle( pen ), "invalid gdi pen %p\n", pen );

	r = NtGdiDeleteObjectApp( pen );
	ok( r == TRUE, "delete failed\n");
}

void test_dc_position(void)
{
	GDI_DEVICE_CONTEXT_SHARED *info;
	HDC hdc;
	ULONG r;

	hdc = NtUserGetDC( 0 );
	info = get_user_info( hdc );

	r = NtGdiMoveTo(0, 1, 2, 0);
	ok( r == FALSE, "NtGdiMoveTo failed\n");

	ok( info->WindowOriginOffset.x == 0, "initial x offset wrong\n");
	ok( info->WindowOriginOffset.y == 0, "initial y offset wrong\n");

	ok( info->CurrentPenPos.x == 0, "initial x offset wrong\n");
	ok( info->CurrentPenPos.y == 0, "initial y offset wrong\n");

	r = NtGdiMoveTo(hdc, 3, 4, (void*)1);
	ok( r == FALSE, "NtGdiMoveTo failed\n");

	/* returns FALSE, but the position has changed */
	ok( info->CurrentPenPos.x == 3, "x offset wrong %ld\n", info->CurrentPenPos.x);
	ok( info->CurrentPenPos.y == 4, "y offset wrong %ld\n", info->CurrentPenPos.y);

	r = NtGdiMoveTo(hdc, 1, 2, 0);
	ok( r == TRUE, "NtGdiMoveTo failed\n");

	ok( info->CurrentPenPos.x == 1, "x offset wrong %ld\n", info->CurrentPenPos.x);
	ok( info->CurrentPenPos.y == 2, "y offset wrong %ld\n", info->CurrentPenPos.y);

	r = NtGdiDeleteObjectApp( hdc );
	ok( r == TRUE, "delete failed\n");
}

void NtProcessStartup( void )
{
	log_init();
	test_not_connected();
	test_gdi_init();
	//test_gdi_font_assoc();
	test_user_init();
	test_create_compat_dc();
	test_create_compat_dc();
	test_get_dc();
	test_stock_brush();
	test_solid_brush();
	test_region();
	test_region_shared();
	test_multiregion();
	test_savedc();
	test_bitmap();
	test_pen();
	test_dc_position();
	log_fini();
}
