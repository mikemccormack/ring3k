/*
 * A short smss.exe replacement to get the contents of the initial PPB
 *
 * Copyright 2009 Mike McCormack
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
#include "rtlapi.h"

void dprintf(char *fmt, ...)
{
	static char buffer[0x100];
	static WCHAR wbuffer[0x100];
	UNICODE_STRING str;
	va_list va;
	int n;

	va_start(va, fmt);
	vsprintf(buffer, fmt, va);
	va_end(va);

	for (n=0; buffer[n] && n<sizeof buffer; n++)
		wbuffer[n] = buffer[n];

	/* output the string */
	str.Buffer = wbuffer;
	str.Length = n*2;
	str.MaximumLength = str.Length;
	NtDisplayString(&str);
}

void NtProcessStartup( void **arg )
{
	LARGE_INTEGER timeout;
	NTSTATUS r;
	UNICODE_STRING path;
	OBJECT_ATTRIBUTES oa;
	HANDLE file = 0, event = 0;
	IO_STATUS_BLOCK iosb;
	LARGE_INTEGER offset;
	FILE_END_OF_FILE_INFORMATION eof;

	WCHAR filename[] = L"\\Device\\HarddiskVolume1\\smss.txt";

	path.Buffer = filename;
	path.Length = sizeof filename - 2;
	path.MaximumLength = path.Length;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &path;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreateFile( &file, GENERIC_READ | GENERIC_WRITE, &oa, &iosb,
			0, 0, 0, FILE_OVERWRITE_IF, 0, 0, 0 );
	if (r >= STATUS_SUCCESS)
	{
		dprintf("NtCreateFile succeeded\n", r);
		dprintf("peb = %08lx\n", arg);
		dprintf("ppb = %08lx\n", arg[4]);

		NtCreateEvent(&event, EVENT_ALL_ACCESS, 0, 0, 0);

		/* write the ppb to a file */
		offset.QuadPart = 0L;
		r = NtWriteFile( file, event, 0, 0, &iosb, arg[4], 0x1000, &offset, 0 );
		dprintf("NtWriteFile returned %08lx\n", r);

		NtWaitForSingleObject( event, 0, 0 );

		eof.EndOfFile.QuadPart = 0x1000;
		r = NtSetInformationFile( file, &iosb, &eof, sizeof eof, FileEndOfFileInformation );
		dprintf("NtSetInformationFile returned %08lx\n", r);

		NtClose( file );
	}
	else
		dprintf("NtCreateFile failed %08lx\n", r);


	/* sleep */
	while (1)
	{
		timeout.QuadPart = -20000LL;
		NtDelayExecution( TRUE, &timeout );
	}

	NtTerminateProcess( NtCurrentProcess(), 0 );
}
