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

#include <stdio.h>
#include "ntapi.h"
#include <windows.h>

void* get_process_peb( HANDLE process )
{
	PROCESS_BASIC_INFORMATION info;
	NTSTATUS r;
	ULONG sz;

	memset( &info, 0, sizeof info );
	r = NtQueryInformationProcess( process, ProcessBasicInformation, &info, sizeof info, &sz );
	if (r == STATUS_SUCCESS)
		return info.PebBaseAddress;
	return NULL;
}

ULONG get_process_exit_code( HANDLE process )
{
	PROCESS_BASIC_INFORMATION info;
	NTSTATUS r;
	ULONG sz;

	memset( &info, 0, sizeof info );
	r = NtQueryInformationProcess( process, ProcessBasicInformation, &info, sizeof info, &sz );
	if (r == STATUS_SUCCESS)
		return info.ExitStatus;
	return ~0;
}

ULONG get_thread_exit_code( HANDLE thread )
{
	THREAD_BASIC_INFORMATION info;
	NTSTATUS r;
	ULONG sz;

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	if( r == STATUS_SUCCESS )
		return info.ExitStatus;
	return ~0;
}

void copy_ustring_to_block( void* addr, ULONG *ofs, UNICODE_STRING *ustr, LPCWSTR str )
{
	UINT len = lstrlenW( str ) * sizeof (WCHAR);
	memcpy( addr + *ofs, str, len + 2 );
	ustr->Buffer = (void*) *ofs;
	ustr->Length = len;
	ustr->MaximumLength = len;
	*ofs += (len + 2);
}

NTSTATUS create_process_parameters( HANDLE process, void *peb, LPCWSTR ImageFile, LPCWSTR DllPath,
	LPCWSTR CurrentDirectory, LPCWSTR CommandLine, LPCWSTR WindowTitle, LPCWSTR Desktop )
{
	RTL_USER_PROCESS_PARAMETERS *p;
	LPWSTR penv = 0;
	NTSTATUS r;
	ULONG sz = 0x1000;
	void *addr;

	p = malloc( sz );

	p->AllocationSize = sz;
	p->Size = sizeof (*p);
	p->Flags = 0;		/* no PROCESS_PARAMS_FLAG_NORMALIZED */

	copy_ustring_to_block( p, &p->Size, &p->ImagePathName, ImageFile );
	copy_ustring_to_block( p, &p->Size, &p->DllPath, DllPath );
	copy_ustring_to_block( p, &p->Size, &p->CurrentDirectory.DosPath, CurrentDirectory );
	copy_ustring_to_block( p, &p->Size, &p->CommandLine, CommandLine );
	copy_ustring_to_block( p, &p->Size, &p->WindowTitle, WindowTitle );
	copy_ustring_to_block( p, &p->Size, &p->Desktop, Desktop );
	copy_ustring_to_block( p, &p->Size, &p->RuntimeInfo, L"" );

#if 0
	lstrcpyW( penv, L"=::=::\\" );
#endif
	p->Environment = penv;

	addr = NULL;
	r = NtAllocateVirtualMemory( process, &addr, 0, &sz, MEM_COMMIT, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	sz = 0x1000;
	r = NtWriteVirtualMemory( process, addr, p, sz, NULL );
	if (r != STATUS_SUCCESS)
		return r;

	r = NtWriteVirtualMemory( process, (void*) peb + 0x10, &addr, sizeof addr, &sz );
	return r;
}

NTSTATUS map_file( HANDLE process, LPCWSTR filename, PVOID *addr )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING name;
	NTSTATUS r;
	HANDLE file, section;
	IO_STATUS_BLOCK iosb;
	LARGE_INTEGER ofs;
	SIZE_T sz;

	name.Buffer = (void*) filename;
	name.Length = lstrlenW(filename) * sizeof (WCHAR);
	name.MaximumLength = name.Length;

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenFile(&file, FILE_READ_DATA | SYNCHRONIZE, &oa, &iosb,
				   FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
	if (r != STATUS_SUCCESS)
	{
		return r;
	}

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = NULL;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	section = NULL;
	r = NtCreateSection(&section, SECTION_ALL_ACCESS, &oa, 0, PAGE_READONLY, SEC_COMMIT, file);
	if (r != STATUS_SUCCESS)
		return r;

	sz = 0;
	ofs.QuadPart = 0;
	r = NtMapViewOfSection(section, process, addr, 0, 0, &ofs, &sz, 1, 0, PAGE_READONLY);
	if (r != STATUS_SUCCESS)
		return r;

	NtClose( file );

	return r;
}

NTSTATUS map_locale_file( HANDLE process, void *peb, LPCWSTR sysdir, LPCWSTR file, UINT ofs )
{
	WCHAR nls_file[MAX_PATH+4];
	VOID *p = NULL;
	NTSTATUS r;

	lstrcpyW( nls_file, sysdir );
	lstrcatW( nls_file, file );

	r = map_file( process, nls_file, &p );
	if (r != STATUS_SUCCESS)
		return r;

	/* setup the codepage data */
	r = NtWriteVirtualMemory( process, (void*) peb + ofs, &p, sizeof p, NULL );
	return r;
}

NTSTATUS setup_locale_info( HANDLE process, void *peb, LPCWSTR sysdir )
{
	NTSTATUS r;

	r = map_locale_file( process, peb, sysdir, L"\\c_1252.nls", 0x58 );
	if (r != STATUS_SUCCESS)
		return r;

	r = map_locale_file( process, peb, sysdir, L"\\c_850.nls", 0x5c );
	if (r != STATUS_SUCCESS)
		return r;

	r = map_locale_file( process, peb, sysdir, L"\\l_intl.nls", 0x60 );

	return r;
}

NTSTATUS create_nt_process( LPCWSTR native_exe, HANDLE *pprocess )
{
	HANDLE process = NULL, file = NULL, section = NULL, thread = NULL;
	NTSTATUS r;
	OBJECT_ATTRIBUTES oa;
	WCHAR procname[] = L"\\Naative";
	UNICODE_STRING name;
	IO_STATUS_BLOCK iosb;
	WCHAR filename[MAX_PATH+4];
	SECTION_IMAGE_INFORMATION si;
	INITIAL_TEB teb;
	ULONG old_prot, sz;
	VOID *p;
	CONTEXT context = {CONTEXT_FULL};
	CLIENT_ID cid;
	VOID *peb;
	WCHAR cwd[MAX_PATH+4], sysdir[MAX_PATH+4];
	WCHAR dllpath[MAX_PATH];

	/* open the executable */
	lstrcpyW( cwd, L"\\??\\");
	GetCurrentDirectoryW( MAX_PATH, cwd+4 );

	lstrcpyW( sysdir, L"\\??\\");
	GetSystemDirectoryW( sysdir+4, MAX_PATH );

	lstrcpyW( dllpath, sysdir+4 );
	lstrcatW( dllpath, L";");
	lstrcatW( dllpath, cwd+4);

	if (native_exe[0] && native_exe[1] != ':')
	{
		lstrcpyW( filename, cwd );
		lstrcatW( filename, L"\\" );
	}
	else
		filename[0] = 0;
	lstrcatW( filename, native_exe );

	name.Buffer = filename;
	name.Length = lstrlenW(filename) * sizeof (WCHAR);
	name.MaximumLength = name.Length;

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	r = NtOpenFile( &file, FILE_EXECUTE | SYNCHRONIZE, &oa, &iosb,
					FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT );
	if (r != STATUS_SUCCESS)
		goto end;

	/* create a section for the executable */
	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = NULL;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	section = NULL;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, &oa, 0, PAGE_EXECUTE, SEC_IMAGE, file );
	if (r != STATUS_SUCCESS)
		goto end;

	/* create a new process */
	name.Buffer = procname;
	name.Length = sizeof procname - 2;
	name.MaximumLength = name.Length;

	memset( &oa, 0, sizeof oa );
	oa.Length = sizeof oa;
	oa.RootDirectory = NULL;
	oa.ObjectName = &name;
	oa.Attributes = OBJ_CASE_INSENSITIVE;

	process = NULL;
	r = NtCreateProcess( &process, PROCESS_ALL_ACCESS, &oa,
						 NtCurrentProcess(), FALSE, section, NULL, NULL );
	if (r != STATUS_SUCCESS)
		goto end;

	peb = get_process_peb( process );

	/* create the process parameters block */
	r = create_process_parameters( process, peb, filename+4, dllpath, cwd+4,
					   native_exe, native_exe, L"WinSta0\\Default");
	if (r != STATUS_SUCCESS)
		goto end;

	r = setup_locale_info( process, peb, sysdir );
	if (r != STATUS_SUCCESS)
		goto end;

	/* query the entry point and stack information */
	memset( &si, 0, sizeof si );
	r = NtQuerySection( section, SectionImageInformation, &si, sizeof si, NULL );
	if (r != STATUS_SUCCESS)
		goto end;

	/* allocate the stack */
	memset( &teb, 0, sizeof teb );
	sz = si.StackReserved;
	r = NtAllocateVirtualMemory( process, &teb.StackReserved,
								 0, &sz, MEM_RESERVE, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	teb.StackCommit = (BYTE*)teb.StackReserved + si.StackReserved;
	teb.StackCommitMax = (BYTE*)teb.StackCommit - si.StackCommit;

	sz = si.StackCommit + PAGE_SIZE;
	p = (BYTE*)teb.StackCommit - sz;
	r = NtAllocateVirtualMemory( process, &p, 0, &sz, MEM_COMMIT, PAGE_READWRITE );
	if (r != STATUS_SUCCESS)
		return r;

	old_prot = 0;
	sz = PAGE_SIZE;
	r = NtProtectVirtualMemory( process, &p, &sz, PAGE_READWRITE | PAGE_GUARD, &old_prot );
	if (r != STATUS_SUCCESS)
		return r;

	/* setup the thread context */
	r = NtGetContextThread( NtCurrentThread(), &context );
	if (r != STATUS_SUCCESS)
		return r;

	context.Esp = (ULONG)teb.StackCommitMax;
	context.Eip = (ULONG)si.EntryPoint;

	/* create a thread to run in the process */
	r = NtCreateThread( &thread, THREAD_ALL_ACCESS, NULL, process, &cid, &context, &teb, FALSE );
	if (r != STATUS_SUCCESS)
		goto end;

	*pprocess = process;

end:
	NtClose( thread );
	NtClose( section );
	NtClose( file );

	return r;
}

#define MAILSLOT_BUFFER_SIZE 0x100

void get_mailslot_message( HANDLE slot )
{
	char buffer[MAILSLOT_BUFFER_SIZE + 1];
	DWORD count, msg_count, next;
	BOOL r;

	while (1)
	{
		next = 0;
		msg_count = 0;
		r = GetMailslotInfo( slot, NULL, &next, &msg_count, NULL );
		if (!r)
			return;
		if (msg_count == 0)
			return;

		r = ReadFile( slot, buffer, next, &count, NULL );
		if (!r)
		{
			fprintf(stderr,"ReadFile failed (%ld)\n", GetLastError());
			return;
		}

		buffer[count] = 0;
		printf("%s", buffer );
	}
}

HANDLE hBreakEvent;

BOOL WINAPI break_handler( DWORD ctrl )
{
	SetEvent(hBreakEvent);
	return TRUE;
}

int main(int argc, char **argv)
{
	WCHAR exe[MAX_PATH];
	HANDLE handles[3], slot;
	NTSTATUS r;

	hBreakEvent = CreateEvent( 0, 0, 0, 0 );
	if (!hBreakEvent)
	{
		fprintf(stderr, "Failed to create event\n");
		return 1;
	}

	if (!SetConsoleCtrlHandler( break_handler, TRUE ))
	{
		fprintf(stderr, "Failed to set ^C handler\n");
		return 1;
	}

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s native.exe\n", argv[0]);
		return 1;
	}

	MultiByteToWideChar( CP_ACP, 0, argv[1], -1, exe, MAX_PATH );

	slot = CreateMailslot( "\\\\.\\mailslot\\nthost", MAILSLOT_BUFFER_SIZE, 0, NULL );
	if (slot == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "CreateMailslot failed with error %ld\n", GetLastError());
		return 1;
	}

	handles[1] = CreateEvent( NULL, FALSE, FALSE, "nthost" );
	if (handles[1] == NULL)
	{
		fprintf(stderr, "CreateEvent failed with error %ld\n", GetLastError());
		return 1;
	}

	r = create_nt_process( exe, &handles[0] );
	if (r != STATUS_SUCCESS)
	{
		fprintf(stderr, "create_nt_process failed with error %08lx\n", r);
		CloseHandle( handles[1] );
		return 1;
	}

	handles[2] = hBreakEvent;

	while (1)
	{
		r = WaitForMultipleObjects( 3, handles, FALSE, INFINITE );

		get_mailslot_message( slot );

		if (r == (WAIT_OBJECT_0))
		{
			r = get_process_exit_code( handles[0] );
			fprintf(stderr,"process exitted (%08lx)\n", r);
			break;
		}

		if (r == (WAIT_OBJECT_0+2))
		{
			fprintf(stderr,"User abort!\n");
			NtTerminateProcess( handles[0], STATUS_UNSUCCESSFUL );
			break;
		}
	}

	CloseHandle( handles[2] );
	CloseHandle( handles[1] );
	CloseHandle( handles[0] );
	CloseHandle( slot );

	return r;
}
