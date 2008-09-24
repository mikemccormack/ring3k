#ifndef __RTLAPI_H__
#define __RTLAPI_H__

#include <stdarg.h>

int __cdecl swprintf(WCHAR *, const WCHAR *, ...);
int __cdecl vsprintf(char *, const char *, va_list);
int __cdecl sprintf(char *, const char *, ...);
PVOID NTAPI RtlAllocateHeap(HANDLE,ULONG,SIZE_T);
BOOLEAN NTAPI RtlFreeHeap(HANDLE,ULONG,PVOID);
void NTAPI RtlInitUnicodeString(PUNICODE_STRING, PCWSTR);
NTSTATUS NTAPI RtlCreateUserThread(HANDLE, const SECURITY_DESCRIPTOR *, BOOLEAN, PVOID, SIZE_T, SIZE_T, void *, void *, HANDLE *, CLIENT_ID *);
BOOLEAN NTAPI RtlDosPathNameToNtPathName_U(PWSTR, PUNICODE_STRING, PWSTR *, CURDIR *);
PVOID NTAPI RtlReAllocateHeap(HANDLE,ULONG,PVOID,SIZE_T);

#endif
