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
#include <windef.h>
#include <ntdef.h>
#include <winnt.h>
#include <ddk/ntifs.h>
#include <windows.h>

//  <    matches 0 or more characters (excluding .)
//  *    matches 0 or more characters
//  x    matches x
//  "    matches . including current directory
//  .    matches start of an extension, invalid at start of a mask
//  >    matches any single character except .
//  ?    matches any single character including .

//  :    always give object name invalid
//  \    always give object name invalid
//  /    always give object name invalid
//  |    always give object name invalid

// directory foo contains files:
//  bar boo tmp1.txt tmp2.txt

// mask      matches
// ----      -------
// tmp1.txt  yes
// tmp1"txt  yes
// tmp"txt   no
// tmp1."txt no
// tmp1."xt  no
// tmp1".txt no
// tmp1".txt no
// tmp".txt  no
// tmp*.txt  yes
// tmp?.txt  yes
// tmp1.>>>  yes
// tmp1.>>   no
// tmp1.>    no
// tmp1.     no
// <tmp1.*   yes
// *tmp1.*   yes
// ba"       no
// bar       yes
// bar.      yes
// bar*      yes
// bar?      no
// bar>      yes
// bar"      yes
// bar<      yes
// bar<<     yes
// bar<<<    yes
// <bar      yes
// <<bar     yes
// <<<bar    yes
// b<ar      yes
// b<<ar     yes
// b<<<ar    yes
// <ar       yes
// <r        yes
// <         yes (.)
// bar"      yes
// bar"d     no
// bar>"     yes
// bar<"     yes
// <"txt     yes
// >"txt     no
// >>"txt    no
// >>>"txt   no
// >>>>"txt  yes
// >>>>>"txt yes
// >>>1.>>>  yes (tmp1.txt)
// >>>1.>>>  yes (tmp1.txt)
// >>>>.>>>  yes (.)
// <1.>>>    yes (tmp1.txt)
// .         STATUS_INVALID_OBJECT_NAME
// "         yes (.)
// ?         yes (.)
// >         yes (.)
// *         yes (.)
// ".        no

/*typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
	ULONG		NextEntryOffset;
	ULONG		FileIndex;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	ChangeTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG		FileAttributes;
	ULONG		FileNameLength;
	ULONG		EaSize;
	WCHAR		FileName[ANYSIZE_ARRAY];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION,
  FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;*/

NTSYSAPI NTSTATUS NTAPI NtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);

void dump_string(PWSTR str, ULONG len)
{
	int i;
	for (i=0; i<len; i++)
	{
		if (str[i] >= 0x20 && str[i] < 0x80)
			fprintf(stderr, "%c", str[i]);
		else
			fprintf(stderr, "\\%04x", str[i]);
	}
}

NTSTATUS query_one(HANDLE handle, HANDLE event, PUNICODE_STRING mask)
{
	IO_STATUS_BLOCK iosb;
	BYTE info[0x400];
	NTSTATUS r;
	PFILE_FULL_DIR_INFORMATION fdi;
	//PFILE_DIRECTORY_INFORMATION fdi;

	memset(&info, 0, sizeof info);
	r = NtQueryDirectoryFile (
		handle,
		event,
		0,
		0,
		&iosb,
		info,
		sizeof info,
		2, //FileInformation...
		TRUE,
		mask,
		FALSE);

	if (r == STATUS_PENDING)
	{
		r = WaitForSingleObject(event, INFINITE);
		if (r != STATUS_SUCCESS)
		{
			fprintf(stderr, "wait failed %08lx\n", r);
			return 1;
		}
		r = iosb.Status;
	}
	//fprintf(stderr, "completed %08lx\n", r);

	if (r != STATUS_SUCCESS)
	{
		if (r == STATUS_OBJECT_NAME_INVALID)
			fprintf(stderr, "query returned STATUS_OBJECT_NAME_INVALID\n");
		else if (r == STATUS_NO_MORE_FILES)
			/* fprintf(stderr, "query returned STATUS_NO_MORE_FILES\n")*/ ;
		else if (r == STATUS_NO_SUCH_FILE)
			fprintf(stderr, "query returned STATUS_NO_SUCH_FILE\n");
		else if (r == STATUS_INVALID_PARAMETER)
			fprintf(stderr, "query returned STATUS_INVALID_PARAMETER\n");
		else
			fprintf(stderr, "query failed %08lx\n", r);
		return r;
	}

	int i;
	if (0)
	{
		fprintf(stderr, "size = %08lx\n", iosb.Information);
		for (i=0; i<iosb.Information; i++)
			fprintf(stderr, "%02x%c", info[i], (i+1)%16?' ':'\n');
		fprintf(stderr, "\n");
	}

#if 0
	// fprintf("%S\n") doesn't seem to like a string with a single .
	fprintf(stderr, "name = ");
	for (i=0; i<iosb.Information - 0x40; i++)
			fprintf(stderr, "%c", ((PWCHAR)(info+0x40))[i]);
#endif
	fdi = (void*)info;
	fprintf(stderr, "%08lx ", fdi->FileAttributes);
	fprintf(stderr, "%08lx ", fdi->EaSize);
	dump_string(fdi->FileName, fdi->FileNameLength/2);
	fprintf(stderr, "\n");

	return STATUS_SUCCESS;
}

int main(int argc, char **argv)
{
	HANDLE handle = 0, event = 0;
	NTSTATUS r;
	UNICODE_STRING path, mask, *pmask;
	OBJECT_ATTRIBUTES oa;
	WCHAR dirname[0x100];
	union {
		PCWSTR cp;
		PWSTR p;
	} u;
	WCHAR pathbuf[0x100];
	BYTE unknown[0x100];
	ULONG len;
	IO_STATUS_BLOCK iosb;

	if (argc != 2)
	{
		fprintf(stderr, "%s <dir>\\<mask>\n", argv[0]);
		return 1;
	}

	len = MultiByteToWideChar( CP_ACP, 0, argv[1], -1, dirname, sizeof dirname);

	path.Buffer = pathbuf;
	path.Length = 0;
	path.MaximumLength = sizeof pathbuf;

	if (!RtlDosPathNameToNtPathName_U( dirname, &path, &u.cp, &unknown ))
	{
		fprintf(stderr, "path conversion failed\n");
		fprintf(stderr, "path %S\n", dirname);
		return 1;
	}

	if (u.p != &pathbuf[0])
	{
		len = lstrlenW(u.p) + 1;
		path.Length -= len*2;
		path.Buffer[path.Length/2] = 0;
	}

	path.Buffer[path.Length/2] = 0;
	//fprintf(stderr, "path = ");
	//dump_string(path.Buffer, path.Length/2);
	//fprintf(stderr, "\n");

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &path;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtOpenFile(&handle, GENERIC_READ, &oa, &iosb, FILE_SHARE_READ, 0 /*FILE_SYNCHRONOUS_IO_NONALERT*/);
	if (r != STATUS_SUCCESS)
	{
		fprintf(stderr, "openfile failed\n");
		fprintf(stderr, "path %S\n", pathbuf);
		return 1;
	}

	len = lstrlenW(u.p);
	if (len)
	{
		mask.Buffer = u.p;
		mask.Length = len*2;
		mask.MaximumLength = 0;
		pmask = &mask;

		//fprintf(stderr, "mask = ");
		//dump_string(u.p, len);
		//fprintf(stderr, "\n");
	}
	else
		pmask = 0;

	event = CreateEvent(0,0,0,0);
	if (!event)
	{
		fprintf(stderr, "createevent failed\n");
		return 1;
	}

	do {
		r = query_one(handle, event, pmask);
	} while (r == STATUS_SUCCESS);

	NtClose(handle);
	return 0;
}
