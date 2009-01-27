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
#include "rtlapi.h"
#include "ntwin32.h"
#include "log.h"

void* get_teb( void )
{
	void *p;
	__asm__ ( "movl %%fs:0x18, %%eax\n\t" : "=a" (p) );
	return p;
}

const int ofs_peb_in_teb = 0x30;

void* get_peb( void )
{
	void **p = get_teb();
	return p[ofs_peb_in_teb/sizeof (*p)];
}

HANDLE get_process_heap( void )
{
	void **p = get_peb();
	return p[0x18/sizeof (*p)];
}

typedef struct _font_enum_entry {
	ULONG size;
	ULONG elf_size;
	ULONG fonttype;
	ENUMLOGFONTEXW elfew;
	ULONG pad1[2];
	ULONG pad2;
	ULONG flags;
	NEWTEXTMETRICEXW ntme;
	ULONG pad3[2];
} font_enum_entry;

BOOL strequal( PCWSTR left, PCWSTR right )
{
	while (*left == *right)
	{
		if (!*left)
			return TRUE;
		left++, right++;
	}
	return FALSE;
}

void test_font_enum( void )
{
	HANDLE heap = get_process_heap();
	HANDLE henum, hdc;
	BOOL r;
	ULONG buf[8];
	ULONG size = 0, retlen = 0;
	PVOID buffer;
	ULONG ofs;
	BOOL system_font_exists = FALSE;
	BOOL terminal_font_exists = FALSE;
	ULONG oldmode;

	oldmode = NtGdiSetFontEnumeration( 0 );

	hdc = NtGdiOpenDCW(0,0,0,0,0,0,&buf);
	ok( hdc != 0, "NtGdiOpenDCW failed\n");

	henum = NtGdiEnumFontOpen(hdc,3,0,0,0,1,&size);
	ok( henum != 0, "NtGdiEnumFontOpen failed\n");
	ok( size != 0, "size not set\n");

	buffer = RtlAllocateHeap( heap, 0, size );
	ok( buffer != 0, "RtlAllocateHeap failed\n");

	r = NtGdiEnumFontChunk(hdc, henum, size, &retlen, buffer);
	ok(r, "NtGdiEnumFontChunk failed\n");

	ofs = 0;
	while (ofs < size)
	{
		font_enum_entry *fe = (font_enum_entry*)(buffer+ofs);

		/*dprintf("offset %04lx type=%04lx flags = %08lx name=%S\n",
			 ofs, fe->fonttype, fe->flags, fe->elfew.elfFullName);*/

		//ok( fe->size == sizeof *fe, "Size wrong %04lx %04x\n", ofs, sizeof *fe);

		// System font should exist (usually the first font)
		if (strequal( L"System", fe->elfew.elfFullName ))
		{
			ok(strequal( L"System", fe->elfew.elfFullName ), "wrong font\n");
			ok(strequal( L"System", fe->elfew.elfLogFont.lfFaceName ), "wrong font\n");
			ok(fe->elfew.elfLogFont.lfHeight == 16, "System font height wrong\n");
			ok(fe->elfew.elfLogFont.lfWidth == 7, "System font width wrong\n");
			ok(fe->elfew.elfLogFont.lfWeight == FW_BOLD, "System font weight wrong %ld\n", fe->elfew.elfLogFont.lfWeight);
			ok(fe->elfew.elfLogFont.lfPitchAndFamily == (FF_SWISS | VARIABLE_PITCH), "System font pitch wrong\n");
			ok(fe->elf_size == FIELD_OFFSET( font_enum_entry, pad2 ), "field offset wrong %04lx %04lx\n", fe->elf_size,  FIELD_OFFSET( font_enum_entry, pad2 ));
			ok(fe->flags == 0x2080ff20, "flags wrong %08lx\n", fe->flags );
			ok(fe->ntme.ntmTm.tmCharSet == 0, "charset wrong %02x\n", fe->ntme.ntmTm.tmCharSet);
			system_font_exists = TRUE;
		}

		// Terminal font should exist (usually the second font)
		if (strequal( L"Terminal", fe->elfew.elfFullName ))
		{
			ok(strequal( L"Terminal", fe->elfew.elfFullName ), "wrong font\n");
			ok(strequal( L"Terminal", fe->elfew.elfLogFont.lfFaceName ), "wrong font\n");
			ok(fe->elfew.elfLogFont.lfHeight == 12, "Terminal font height wrong\n");
			ok(fe->elfew.elfLogFont.lfWidth == 8, "Terminal font width wrong\n");
			ok(fe->elfew.elfLogFont.lfWeight == FW_REGULAR, "Terminal font weight wrong %ld\n", fe->elfew.elfLogFont.lfWeight);
			ok(fe->elfew.elfLogFont.lfPitchAndFamily == (FF_MODERN | FIXED_PITCH), "System font pitch wrong\n");
			ok(fe->elf_size == FIELD_OFFSET( font_enum_entry, pad2 ), "field offset wrong %04lx %04lx\n", fe->elf_size,  FIELD_OFFSET( font_enum_entry, pad2 ));
			ok(fe->flags == 0x2020fe01, "flags wrong %08lx\n", fe->flags );
			terminal_font_exists = TRUE;
			ok(fe->ntme.ntmTm.tmCharSet == OEM_CHARSET, "charset wrong %02x\n", fe->ntme.ntmTm.tmCharSet);
		}

		ok(fe->ntme.ntmTm.tmHeight == fe->elfew.elfLogFont.lfHeight, "height mismatch\n");

		if (!fe->size)
			break;

		// next
		ofs += fe->size;
	}
	ok( ofs == size, "length mismatch\n");

	ok( system_font_exists, "no system font\n");
	ok( terminal_font_exists, "no terminal font\n");

	//dump_bin( buffer, retlen );

	r = NtGdiEnumFontClose(henum);
	ok(r, "NtGdiEnumFontClose failed\n");

	NtGdiSetFontEnumeration( oldmode );
}

const int ofs_shared_handle_table_in_peb = 0x94;

void* get_gdi_shared_handle_table( void )
{
	void **p = get_peb();
	return p[ofs_shared_handle_table_in_peb/sizeof (*p)];
}

const int ofs_process_id_table_in_peb = 0x20;

ULONG get_process_id( void )
{
	ULONG *p = get_teb();
	return p[ofs_process_id_table_in_peb/sizeof (*p)];
}

ULONG get_handle_index(HANDLE handle)
{
	return (((ULONG)handle)&0x3fff);
}

ULONG get_handle_upper(HANDLE handle)
{
	return (((ULONG)handle)>>16);
}

void test_text_metric( void )
{
	ULONG buf[8];
	HANDLE hdc;
	gdi_handle_table_entry *table;
	ULONG index;
	ULONG status;
	PVOID ptr;
	BOOL r;
	BYTE tmbuf[0x44];
	TEXTMETRICW *tm;

	hdc = NtGdiOpenDCW(0,0,0,0,0,0,&buf);
	ok( hdc != 0, "NtGdiOpenDCW failed\n");

	r = NtGdiComputeXformCoefficients( 0 );
	ok( r == FALSE, "NtGdiComputeXformCoefficients returned %d\n", r);

	r = NtGdiComputeXformCoefficients( hdc );
	ok( r == TRUE, "NtGdiComputeXformCoefficients returned %d\n", r);

	status = NtGdiQueryFontAssocInfo( hdc );

	table = get_gdi_shared_handle_table();

	//dprintf("table = %p\n", table);
	index = get_handle_index( hdc );
	ok( index < 0x4000, "index too large\n");

	ok( table[index].Type == GDI_OBJECT_DC,
		"not a device context %02x\n", table[index].Type);
	ok( table[index].Upper == get_handle_upper( hdc ),
		"handle mismatch\n");
	ok( (table[index].ProcessId&~1) == (get_process_id()&~3),
		"process id mismatch %08x %08lx\n",
		table[index].ProcessId, get_process_id());

	ok( sizeof *table == 0x10, "sizeof table wrong\n");

	ptr = table[index].user_info;

	ok( ptr != 0, "user pointer was empty\n");

	r = NtGdiGetTextMetricsW( 0, 0, 0 );
	ok( r == FALSE, "NtGdiGetTextMetricsW returned %d\n", r);

	r = NtGdiGetTextMetricsW( hdc, 0, 0 );
	ok( r == FALSE, "NtGdiGetTextMetricsW returned %d\n", r);

	r = NtGdiGetTextMetricsW( hdc, tmbuf, 0 );
	ok( r == FALSE, "NtGdiGetTextMetricsW returned %d\n", r);

	memset( &tmbuf, 0, sizeof tmbuf );
	tm = (void*) tmbuf;

	r = NtGdiGetTextMetricsW( hdc, tm, sizeof *tm );
	ok( r == TRUE, "NtGdiGetTextMetricsW returned %d\n", r);

	r = NtGdiGetTextMetricsW( hdc, tmbuf, sizeof tmbuf );
	ok( r == TRUE, "NtGdiGetTextMetricsW returned %d\n", r);

/*
#define d(x) dprintf(#x " = %ld\n", tm->tm##x);
	d(Height)
	d(CharSet)
#undef d

	dump_bin(tmbuf, sizeof tmbuf);
*/

	// DeleteDC(hdc)
}

// magic numbers for everybody
void NTAPI init_callback(void *arg)
{
	NtCallbackReturn( 0, 0, 0 );
}

void *callback_table[NUM_USER32_CALLBACKS];

void init_callbacks( void )
{
	callback_table[NTWIN32_THREAD_INIT_CALLBACK] = &init_callback;
	__asm__ (
		"movl %%fs:0x18, %%eax\n\t"
		"movl 0x30(%%eax), %%eax\n\t"
		"movl %%ebx, 0x2c(%%eax)\n\t"  // set PEB's KernelCallbackTable
		: : "b" (&callback_table) : "eax" );
}

void become_gui_thread( void )
{
	init_callbacks();
	NtGdiInit();
}

void NtProcessStartup( void )
{
	log_init();
	become_gui_thread();
	test_text_metric();
	test_font_enum();
	log_fini();
}
