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

#define UNICODE

#include <windows.h>
#include <stdio.h>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
// NT definitions

#define NUMBER_OF_USER_SHARED_SECTIONS 33

typedef struct USER_SHARED_MEMORY_INFO {
	ULONG Flags;
	PVOID Address;
} USER_SHARED_MEMORY_INFO, *PUSER_SHARED_MEMORY_INFO;

typedef struct _USER_PROCESS_CONNECT_INFO {
	ULONG Version;  // set by caller
	ULONG Unknown;
	ULONG MinorVersion;
	PVOID Ptr[4];	// pointers from here on stored
	USER_SHARED_MEMORY_INFO SharedSection[NUMBER_OF_USER_SHARED_SECTIONS];
} USER_PROCESS_CONNECT_INFO, *PUSER_PROCESS_CONNECT_INFO;

ULONG WINAPI NtUserProcessConnect(HANDLE,PVOID,ULONG);
#define NtCurrentProcess() ((HANDLE)-1)

typedef struct _user_handle_info {
	PVOID ptr;
	PVOID kernel_ptr;
	BYTE type;
	BYTE flags;
	USHORT generation;
} user_handle_info;

void dump_handles( int max, user_handle_info *hi )
{
	int i;
	int total = 0;

	for (i=0; i<max; i++)
	{
		if (0xffff0000 & (ULONG)hi[i].ptr)
		{
			HANDLE handle = (HANDLE) ((hi[i].generation << 16) | i);
			const char *type_name = "unknown";
			switch (hi[i].type)
			{
			// HKL looks like an integer with an LCID in it, not a user handle
			case 1:
				type_name = "window";
				break;
			case 2:
				type_name = "menu";
				break;
			case 3:
				type_name = "icon";
				break;
			case 4:
				type_name = "defer window pos";
				break;
			case 8:
				type_name = "accelerator";
				break;
			case 12:
				type_name = "monitor";
				break;
			// seen these, but don't know what they are...
			case 5:
			case 6:
			case 7:
			case 9:
			case 13:
			case 14:
			case 16:
			case 17:
			case 19:
			// hook
			// callproc
				SetLastError(0);
				SetCursor(handle);
				if (GetLastError() == 0)
				{
					type_name = "cursor";
				}
				break;
			default:
				type_name = "unseen";
			}

			fprintf(stdout, "handle=%p ptr=%p kptr=%p flags=%02x type=%02x (%s)\n",
				handle, hi[i].ptr, hi[i].kernel_ptr, hi[i].flags, hi[i].type, type_name);
			total++;
		}
	}
	fprintf(stdout,"%d/%d handles\n", total, max);
}

USER_PROCESS_CONNECT_INFO user32_info;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int nShow)
{
	ULONG r;

	// force user32 to load
	// otherwise NtUserProcessConnect will crash because the callback table is missing
	LoadLibrary(TEXT("user32"));
	fprintf(stdout, "dumping user handles\n");

	// connect to user32
	user32_info.Version = 0x00050000;
	user32_info.MinorVersion = 0x0000029b;
	r = NtUserProcessConnect( NtCurrentProcess(), &user32_info, sizeof user32_info );
	if (r)
	{
		fprintf(stderr, "Failed to connect to user32 (r=%08lx)\n", r);
		return TRUE;
	}
	fprintf(stdout,"user handles   @ %p\n", user32_info.Ptr[0]);

	dump_handles( ((ULONG*)(user32_info.Ptr[0]))[2], user32_info.Ptr[1] );

	return 0;
}
