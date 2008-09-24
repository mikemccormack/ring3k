/* Unit test suite for Ntdll Port API functions
 *
 * Copyright 2006 James Hawkins
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
#include <stdarg.h>

#include "ntapi.h"
#include "rtlapi.h"
#include "log.h"

static const WCHAR PORTNAME[] = {'\\','M','y','P','o','r','t',0};

#define REQUEST1    "Request1"
#define REQUEST2    "Request2"
#define REPLY       "Reply"

#define MAX_MESSAGE_LEN    30

UNICODE_STRING  port;

int lstrcmp(const char *a, const char *b)
{
	while (*a || *b)
	{
		if (*a < *b)
			return -1;
		if (*a > *b)
			return 1;
		a++;
		b++;
	}
	return 0;
}

int lstrlen(const char *a)
{
	int n = 0;
	while (a[n])
		n++;
	return n;
}

void lstrcpy(char *dest, const char *src)
{
	while ((*dest++ = *src++))
		;
}

static void ProcessConnectionRequest(PLPC_MESSAGE LpcMessage, PHANDLE pAcceptPortHandle)
{
    NTSTATUS status;

    ok(LpcMessage->MessageType == LPC_CONNECTION_REQUEST,
       "Expected LPC_CONNECTION_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!*LpcMessage->Data, "Expected empty string!\n");

    status = NtAcceptConnectPort(pAcceptPortHandle, 0, LpcMessage, 1, 0, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    status = NtCompleteConnectPort(*pAcceptPortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
}

static void ProcessLpcRequest(HANDLE PortHandle, PLPC_MESSAGE LpcMessage)
{
    NTSTATUS status;

    ok(LpcMessage->MessageType == LPC_REQUEST,
       "Expected LPC_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST2),
       "Expected %s, got %s\n", REQUEST2, LpcMessage->Data);

    lstrcpy((LPSTR)LpcMessage->Data, REPLY);

    status = NtReplyPort(PortHandle, LpcMessage);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(LpcMessage->MessageType == LPC_REQUEST,
       "Expected LPC_REQUEST, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REPLY),
       "Expected %s, got %s\n", REPLY, LpcMessage->Data);
}

static DWORD WINAPI test_ports_client(LPVOID arg)
{
    SECURITY_QUALITY_OF_SERVICE sqos;
    LPC_MESSAGE *LpcMessage, *out;
    HANDLE PortHandle;
    ULONG len, size;
    NTSTATUS status;
	BYTE in_buf[0x100];
	BYTE out_buf[0x100];

    sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    sqos.EffectiveOnly = TRUE;

    status = NtConnectPort(&PortHandle, &port, &sqos, 0, 0, &len, NULL, NULL);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    if (status != STATUS_SUCCESS) return 1;

    status = NtRegisterThreadTerminatePort(PortHandle);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);

    size = FIELD_OFFSET(LPC_MESSAGE, Data) + MAX_MESSAGE_LEN;
	memset( in_buf, 0, sizeof in_buf );
    LpcMessage = (void*) in_buf;
	memset( out_buf, 0, sizeof out_buf );
    out = (void*) out_buf;

    LpcMessage->DataSize = lstrlen(REQUEST1) + 1;
    LpcMessage->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + LpcMessage->DataSize;
    lstrcpy((LPSTR)LpcMessage->Data, REQUEST1);

    status = NtRequestPort(PortHandle, LpcMessage);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(LpcMessage->MessageType == 0, "Expected 0, got %d\n", LpcMessage->MessageType);
    ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST1),
       "Expected %s, got %s\n", REQUEST1, LpcMessage->Data);

    /* Fill in the message */
    memset(LpcMessage, 0, size);
    LpcMessage->DataSize = lstrlen(REQUEST2) + 1;
    LpcMessage->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + LpcMessage->DataSize;
    lstrcpy((LPSTR)LpcMessage->Data, REQUEST2);

    /* Send the message and wait for the reply */
    status = NtRequestWaitReplyPort(PortHandle, LpcMessage, out);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    ok(!lstrcmp((LPSTR)out->Data, REPLY), "Expected %s, got %s\n", REPLY, out->Data);
    ok(out->MessageType == LPC_REPLY, "Expected LPC_REPLY, got %d\n", out->MessageType);

	// returning from here crashes and terminates this thread...
    return 0;
}

static void test_ports_server(void)
{
    OBJECT_ATTRIBUTES obj;
    HANDLE PortHandle;
    HANDLE AcceptPortHandle;
    PLPC_MESSAGE LpcMessage;
    ULONG size;
    NTSTATUS status;
    BOOL done = FALSE;
	BYTE buffer[0x100];

    RtlInitUnicodeString(&port, PORTNAME);

    memset(&obj, 0, sizeof(OBJECT_ATTRIBUTES));
    obj.Length = sizeof(OBJECT_ATTRIBUTES);
    obj.ObjectName = &port;

    status = NtCreatePort(&PortHandle, &obj, 100, 100, 0);
    ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %08lx\n", status);
    if (status != STATUS_SUCCESS) return;

	memset( buffer, 0, sizeof buffer );
    size = FIELD_OFFSET(LPC_MESSAGE, Data) + MAX_MESSAGE_LEN;
    LpcMessage = (void*) buffer;

    while (TRUE)
    {
        status = NtReplyWaitReceivePort(PortHandle, NULL, NULL, LpcMessage);
        ok(status == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got (%08lx)\n", status);
        /* STATUS_INVALID_HANDLE: win2k without admin rights will perform an
         *                        endless loop here
         */
        if ((status == STATUS_NOT_IMPLEMENTED) ||
            (status == STATUS_INVALID_HANDLE)) return;

        switch (LpcMessage->MessageType)
        {
            case LPC_CONNECTION_REQUEST:
                ProcessConnectionRequest(LpcMessage, &AcceptPortHandle);
                break;

            case LPC_REQUEST:
                ProcessLpcRequest(PortHandle, LpcMessage);
                done = TRUE;
                break;

            case LPC_DATAGRAM:
                ok(!lstrcmp((LPSTR)LpcMessage->Data, REQUEST1),
                   "Expected %s, got %s\n", REQUEST1, LpcMessage->Data);
                break;

            case LPC_CLIENT_DIED:
                ok(done, "Expected LPC request to be completed!\n");
                return;

            default:
                ok(FALSE, "Unexpected message: %d\n", LpcMessage->MessageType);
                break;
        }
    }
}

void NtProcessStartup( void )
{
	NTSTATUS r;
	CLIENT_ID id;
	HANDLE thread = 0;
	THREAD_BASIC_INFORMATION info;
	ULONG sz = 0;

	log_init();

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, test_ports_client, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );
	test_ports_server();

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");

	ok( info.ClientId.UniqueThread == id.UniqueThread, "thread id mismatch\n");
	ok( info.ClientId.UniqueProcess == id.UniqueProcess, "thread id mismatch\n");
	ok( info.ExitStatus == STATUS_ACCESS_VIOLATION, "exit code wrong\n");

	NtClose(thread);

	log_fini();
}
