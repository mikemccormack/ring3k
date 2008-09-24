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


#include <stdarg.h>
#include "ntapi.h"
#include "log.h"

BYTE buffer[0x10000];

void list_processes(void)
{
	PSYSTEM_PROCESS_INFORMATION pspi;
	ULONG ofs = 0, sz, i, j;
	NTSTATUS r;

	sz = 0;
	r = NtQuerySystemInformation( SystemProcessInformation, buffer, sizeof buffer, &sz );
	ok( r == STATUS_SUCCESS, "NtQuerySystemInformation failed\n" );
	if (r != STATUS_SUCCESS)
		return;

	for (i=0, ofs=0; ofs<sz; i++)
	{
		pspi = (PSYSTEM_PROCESS_INFORMATION) (buffer + ofs);
		dprintf( "%ld %ld %ld %S\n", pspi->ThreadCount, pspi->ProcessId,
				 pspi->InheritedFromProcessId, pspi->ProcessName.Buffer);
		for (j=0; j<pspi->ThreadCount; j++)
		{
			 dprintf("%p %p %p %08lx %08lx\n",
					 pspi->Threads[j].StartAddress,
					 pspi->Threads[j].ClientId.UniqueProcess,
					 pspi->Threads[j].ClientId.UniqueThread,
					 pspi->Threads[j].State,
					 pspi->Threads[j].WaitReason);
		}
		if (!pspi->NextEntryDelta)
			break;
		ofs += pspi->NextEntryDelta;
	}
}

void NtProcessStartup( void )
{
	log_init();
	list_processes();
	log_fini();
}
