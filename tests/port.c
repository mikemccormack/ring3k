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
#include "rtlapi.h"
#include "log.h"

void test_create_port(void)
{
	NTSTATUS r;
	HANDLE port = NULL;
	OBJECT_ATTRIBUTES oa;
	ULONG out;

	r = NtCreatePort( NULL, NULL, 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, NULL, 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	oa.Length = 0;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreatePort( NULL, &oa, 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = 0;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtCreatePort( NULL, &oa, 0, 0, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0, 0, NULL );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	out = 0xf00;
	r = NtCreatePort( &port, &oa, 0, 0, &out );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
	ok( out == 0xf00, "return value wrong\n");

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x100000, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x10000, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x1000, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x200, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x180, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x160, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x150, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x14c, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x14a, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x149, NULL );
	ok( r == STATUS_INVALID_PARAMETER_4, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x148, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x140, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x100000, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x149, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x140, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x120, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x110, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x108, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x106, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x105, 0x100, NULL );
	ok( r == STATUS_INVALID_PARAMETER_3, "wrong return %08lx\n", r );

	r = NtCreatePort( &port, &oa, 0x104, 0x100, NULL );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtCreatePort( NULL, NULL, 0x100000, 0x100000, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	r = NtCreatePort( NULL, &oa, 0x100000, 0x100000, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );

	r = NtCreatePort( NULL, &oa, 0x100000, 0x100000, (void*) -1 );
	ok( r == STATUS_ACCESS_VIOLATION, "wrong return %08lx\n", r );
}

WCHAR portname[] = L"\\RPC Control\\testport";

void set_portname( UNICODE_STRING *us, OBJECT_ATTRIBUTES *oa )
{
	us->Buffer = portname;
	us->Length = sizeof portname - 2;
	us->MaximumLength = 0;

	memset( oa, 0, sizeof *oa );
	oa->Length = sizeof *oa;
	oa->ObjectName = us;
}

void thread_proc( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	WCHAR badname[] = L"\\foobar_nonexistant";
	SECURITY_QUALITY_OF_SERVICE qos;

	//dprintf("enter thread_proc\n");

	r = NtConnectPort( NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtConnectPort %08lx\n", r );

	r = NtConnectPort( &port, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtConnectPort %08lx\n", r );

	us.Buffer = badname;
	us.Length = sizeof badname - 2;
	us.MaximumLength = 0;

	r = NtConnectPort( &port, &us, NULL, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtConnectPort %08lx\n", r );

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_OBJECT_NAME_NOT_FOUND, "NtConnectPort %08lx\n", r );

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	//dprintf("connecting to port...\n");
	NtYieldExecution();

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );

	//dprintf("ending thread\n");
	NtYieldExecution();

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server(void)
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	ULONG sz;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	NtYieldExecution();
	//dprintf("port create finished\n");

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	r = NtListenPort( port, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtListenPort wrong return %08lx\n", r );

	r = NtListenPort( port, (void*)1 );
	ok( r == STATUS_DATATYPE_MISALIGNMENT, "NtListenPort wrong return %08lx\n", r );

	r = NtListenPort( port, (void*)4 );
	ok( r == STATUS_ACCESS_VIOLATION, "NtListenPort wrong return %08lx\n", r );

	memset( buffer, 0xff, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );
	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");

	NtYieldExecution();
	//dprintf("listen finished\n");

	con_port = 0;
	r = NtAcceptConnectPort( &con_port, 0, req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");
	ok( con_port != NULL, "connect port was zero\n");

	NtYieldExecution();
	//dprintf("accept finished\n");

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	NtYieldExecution();
	//dprintf("complete finished\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");

	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");
}

void thread_proc_reject( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_PORT_CONNECTION_REFUSED, "NtConnectPort failed %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server_reject(void)
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	ULONG sz;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_reject, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	memset( buffer, 0xff, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );

	con_port = 0;
	r = NtAcceptConnectPort( &con_port, 0, req, FALSE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( con_port == 0, "con_port should be zero\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");

	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");
}

void thread_proc_ping( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;
	BYTE req_buffer[0x100], reply_buffer[0x100];
	LPC_MESSAGE *req, *reply;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;
	//dprintf("c: before NtConnectPort\n");

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort failed %08lx\n", r);

	memset( req_buffer, 0, sizeof req_buffer );
	memset( reply_buffer, 0, sizeof reply_buffer );
	req = (void*) req_buffer;
	reply = (void*) reply_buffer;

	req->MessageType = LPC_NEW_MESSAGE;
	req->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + 1;
	req->DataSize = 1;
	memset( req->Data, '>', sizeof req_buffer - FIELD_OFFSET(LPC_MESSAGE, Data) );
	req->SectionSize = 0xf0f00000;

	r = NtRequestWaitReplyPort( port, req, reply );
	ok( r == STATUS_SUCCESS, "NtRequestWaitReplyPort failed %08lx\n", r);
	//dprintf("c: after NtRequestWaitReplyPort\n");

	ok( reply->DataSize == 1, "data size wrong\n");
	ok( reply->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + 1, "message size wrong\n");
	ok( reply->MessageType == LPC_REPLY, "message type wrong (%d)\n", reply->MessageType);
	ok( reply->VirtualRangesOffset == 0, "offset wrong\n");
	ok( reply->MessageId != 0, "message id was zero\n");
	ok( reply->SectionSize == 0, "section size wrong\n");
	ok( reply->Data[0] == '<', "data wrong\n");
	ok( reply->Data[1] == '<', "data wrong\n");

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
	//dprintf("c: ending\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server_ping(void)
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	ULONG sz;
	HANDLE client_handle;
	LPC_MESSAGE *req, bad_req;
	BYTE buffer[0x100];

	set_portname( &us, &oa );

	//dprintf("s: before NtCreatePort\n");

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_INVALID_HANDLE, "NtCompleteConnectPort failed %08lx\n", r );

	r = NtCompleteConnectPort( port );
	ok( r == STATUS_INVALID_PORT_HANDLE, "NtCompleteConnectPort failed %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_ping, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	//dprintf("s: before NtListenPort\n");
	memset( buffer, 0xff, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );
	//dprintf("s: before NtAcceptConnectPort\n");

	// what's needed to connect the port?
	memset( &bad_req, 0, sizeof bad_req );
	r = NtAcceptConnectPort( &con_port, 0, &bad_req, TRUE, NULL, NULL );
	ok( r == STATUS_INVALID_CID, "NtAcceptConnectPort failed %08lx\n", r );

	r = NtAcceptConnectPort( &con_port, 0, &bad_req, FALSE, NULL, NULL );
	ok( r == STATUS_INVALID_CID, "NtAcceptConnectPort failed %08lx\n", r );

	bad_req.ClientId.UniqueProcess = id.UniqueProcess;
	bad_req.ClientId.UniqueThread = id.UniqueThread;
	r = NtAcceptConnectPort( &con_port, 0, &bad_req, TRUE, NULL, NULL );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "NtAcceptConnectPort failed %08lx\n", r );

	bad_req.MessageId = req->MessageId;
	r = NtAcceptConnectPort( &con_port, 0, &bad_req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );

	r = NtAcceptConnectPort( &con_port, 0, req, TRUE, NULL, NULL );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "NtAcceptConnectPort failed %08lx\n", r );

	//dprintf("s: before NtCompleteConnectPort\n");

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_INVALID_PARAMETER, "NtCompleteConnectPort failed %08lx\n", r );

	//dprintf("s: before NtWaitReceivePort\n");
	client_handle = 0;
	r = NtReplyWaitReceivePort( con_port, &client_handle, 0, req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == con_port, "port wrong %p %p\n", client_handle, con_port);

	ok( req->DataSize == 1, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + 1, "message size wrong (%d)\n", req->MessageSize);
	ok( req->MessageType == LPC_REQUEST, "message type wrong (%d)\n", req->MessageType);
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");
	ok( req->Data[0] == '>', "data wrong\n");
	ok( req->Data[1] == '>', "data wrong\n");
	ok( req->Data[2] == '>', "data wrong\n");
	ok( req->Data[3] == '>', "data wrong\n");
	ok( req->Data[4] == 0xff, "data wrong\n");
	ok( req->ClientId.UniqueThread == id.UniqueThread, "thread id wrong\n");
	ok( req->ClientId.UniqueProcess == id.UniqueProcess, "process id wrong\n");

	req->Data[0] = '<';
	req->Data[1] = '<';
	//dprintf("s: before NtReplyPort\n");

	r = NtReplyPort( con_port, req );
	ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );
	//dprintf("s: after NtReplyPort\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );
	//dprintf("s: done\n");

	r = NtClose( con_port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");

	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");
}

void test_wrong_object( void )
{
	HANDLE handle = NULL;
	NTSTATUS r;
	LPC_MESSAGE req;
	HANDLE client_handle;

	r = NtCreateEvent( &handle, EVENT_ALL_ACCESS, NULL, NotificationEvent, 2 );
	ok(r == STATUS_SUCCESS, "return wrong (%08lx)\n", r);

	r = NtListenPort( handle, &req );
	ok(r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong (%08lx)\n", r);

	r = NtCompleteConnectPort( handle );
	ok(r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong (%08lx)\n", r);

	r = NtReplyWaitReceivePort( handle, &client_handle, 0, &req );
	ok(r == STATUS_OBJECT_TYPE_MISMATCH, "return wrong (%08lx)\n", r);

	r = NtClose( handle );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

void thread_proc_simple( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;
	OBJECT_ATTRIBUTES oa;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_OBJECT_NAME_COLLISION, "wrong return %08lx\n", r );

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server_wait_receive( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	HANDLE client_handle;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtCreatePort( &con_port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_OBJECT_NAME_COLLISION, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_simple, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	req = (void*) buffer;

	client_handle = 0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle set for unconnect port (%p)\n", client_handle);

	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong (%d)\n", req->MessageType);
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");
	ok( req->ClientId.UniqueThread == id.UniqueThread, "thread id wrong\n");
	ok( req->ClientId.UniqueProcess == id.UniqueProcess, "process id wrong\n");

	r = NtAcceptConnectPort( &con_port, 0, req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	client_handle = 0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == con_port, "port wrong %p %p\n", client_handle, con_port);

	ok( req->DataSize == 8, "data size wrong (%d)\n", req->DataSize);
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + 8, "message size wrong (%d)\n", req->MessageSize);
	ok( req->MessageType == LPC_PORT_CLOSED, "message type wrong (%d)\n", req->MessageType);
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");
	ok( req->ClientId.UniqueThread == id.UniqueThread, "thread id wrong\n");
	ok( req->ClientId.UniqueProcess == id.UniqueProcess, "process id wrong\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( con_port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

void thread_proc_no_connect( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;
	OBJECT_ATTRIBUTES oa;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = 0;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	r = NtConnectPort( &port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_PORT_CONNECTION_REFUSED, "NtConnectPort failed %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_no_connect( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	HANDLE client_handle;
	LPC_MESSAGE req;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, sizeof req, sizeof req, (void*) 0 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_no_connect, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	client_handle = 0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, &req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle set\n");

	ok( req.DataSize == 0, "data size wrong\n");
	ok( req.MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req.MessageType == LPC_CONNECTION_REQUEST, "message type wrong (%d)\n", req.MessageType);
	ok( req.VirtualRangesOffset == 0, "offset wrong\n");
	ok( req.MessageId != 0, "message id was zero\n");
	ok( req.SectionSize == 0, "section size wrong\n");
	ok( req.ClientId.UniqueThread == id.UniqueThread, "thread id wrong\n");
	ok( req.ClientId.UniqueProcess == id.UniqueProcess, "process id wrong\n");

	// Is there any way to unblock the other thread other than
	//  NtAcceptConnectPort or NtCompleteConnectPort?
	req.DataSize = sizeof req - FIELD_OFFSET( LPC_MESSAGE, Data );
	req.MessageSize = sizeof req;
	req.Data[0] = 'x';
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_REPLY;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_NEW_MESSAGE;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	// this hangs
	if (0)
	{
		req.MessageType = LPC_REQUEST;
		r = NtReplyPort( port, &req );
		ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );
	}

	req.MessageType = LPC_PORT_CLOSED;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_CLIENT_DIED;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_EXCEPTION;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_ERROR_EVENT;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_DEBUG_EVENT;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	req.MessageType = LPC_DATAGRAM;
	r = NtReplyPort( port, &req );
	ok( r == STATUS_REPLY_MESSAGE_MISMATCH, "reply failed (%08lx)\n", r );

	r = NtAcceptConnectPort( NULL, 0, &req, FALSE, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtAcceptConnectPort failed %08lx\n", r );

	con_port = (HANDLE) -1;
	r = NtAcceptConnectPort( &con_port, 0, &req, FALSE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( con_port == (HANDLE) -1, "returned a non-zero handle (%p)\n", con_port);

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

// Can we pass a port handle between threads?
// What happens if we do?
void thread_proc_share_handle( void *param )
{
	LPC_MESSAGE req, reply;
	HANDLE port = param;
	NTSTATUS r;

	memset( &req, 0, sizeof req );
	memset( &reply, 0, sizeof reply );

	req.MessageType = LPC_NEW_MESSAGE;
	req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + 1;
	req.DataSize = 1;
	req.Data[0] = '>';

	r = NtRequestWaitReplyPort( port, &req, &reply );
	ok( r == STATUS_SUCCESS, "NtRequestWaitReplyPort failed %08lx\n", r);

	ok( reply.DataSize == 1, "data size wrong\n");
	ok( reply.MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + 1, "message size wrong\n");
	ok( reply.MessageType == LPC_REPLY, "message type wrong (%d)\n", reply.MessageType);
	ok( reply.VirtualRangesOffset == 0, "offset wrong\n");
	ok( reply.MessageId != 0, "message id was zero\n");
	ok( reply.SectionSize == 0, "section size wrong\n");
	ok( reply.Data[0] == '<', "data wrong\n");

	req.Data[0] = '1';
	r = NtRequestWaitReplyPort( port, &req, &reply );
	ok( r == STATUS_SUCCESS, "NtRequestWaitReplyPort failed %08lx\n", r);
	ok( reply.Data[0] == '2', "data wrong\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server_share_handle( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0;
	NTSTATUS r;
	ULONG prev;
	HANDLE client_handle;
	LPC_MESSAGE req;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, sizeof req, sizeof req, (void*) 0 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_share_handle, port, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	client_handle = 0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, &req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle set (%p)\n", client_handle);

	ok( req.DataSize == 1, "data size wrong\n");
	ok( req.MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + 1, "message size wrong\n");
	ok( req.MessageType == LPC_REQUEST, "message type wrong (%d)\n", req.MessageType);
	ok( req.VirtualRangesOffset == 0, "offset wrong\n");
	ok( req.MessageId != 0, "message id was zero\n");
	ok( req.SectionSize == 0, "section size wrong\n");
	ok( req.ClientId.UniqueThread == id.UniqueThread, "thread id wrong\n");
	ok( req.ClientId.UniqueProcess == id.UniqueProcess, "process id wrong\n");
	ok( req.Data[0] == '>', "data wrong\n");

	req.Data[0] = '<';

	prev = req.MessageId;

	r = NtReplyPort( port, &req );
	ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

	r = NtReplyWaitReceivePort( port, &client_handle, 0, &req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle not zero\n");

	ok( req.Data[0] == '1', "data wrong\n");
	ok( (prev+1) == req.MessageId, "message didn't change (%ld == %ld)\n", prev, req.MessageId);
	req.Data[0] = '2';

	r = NtReplyPort( port, &req );
	ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

LPC_MESSAGE copy_req;

void thread_proc_test_copy( void *param )
{
	LPC_MESSAGE reply;
	HANDLE port = param;
	NTSTATUS r;

	memset( &reply, 0, sizeof reply );

	r = NtRequestWaitReplyPort( port, &copy_req, &reply );
	ok( r == STATUS_SUCCESS, "NtRequestWaitReplyPort failed %08lx\n", r);

	copy_req.DataSize = 0x1000;
	r = NtRequestWaitReplyPort( port, &copy_req, &reply );
	ok( r == STATUS_INVALID_PARAMETER, "NtRequestWaitReplyPort failed %08lx\n", r);

	copy_req.DataSize = 1;
	copy_req.MessageSize = 0x1000;
	r = NtRequestWaitReplyPort( port, &copy_req, &reply );
	ok( r == STATUS_PORT_MESSAGE_TOO_LONG, "NtRequestWaitReplyPort failed %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

// The object of this test is to show when the contents of a message are copied.
//  Is copying done when the message is sent, or when the receiving thread receives the message?
//  iow. if we change the buffer a NtRequestWaitReply call is made,
//   but before NtReplyWaitReceive returns, what is returned?
void test_port_message_copy( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0;
	NTSTATUS r;
	ULONG i;
	HANDLE client_handle;
	LPC_MESSAGE req;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, sizeof req, sizeof req, (void*) 0 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	memset( &copy_req, 0, sizeof copy_req );
	copy_req.MessageType = LPC_NEW_MESSAGE;
	copy_req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + 1;
	copy_req.DataSize = 1;
	copy_req.Data[0] = '>';

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_test_copy, port, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	for (i=0; i<0x100; i++)
		NtYieldExecution();

	copy_req.Data[0] = ']';

	client_handle = 0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, &req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle not zero\n");

	ok( req.Data[0] == '>', "data wrong\n");

	req.Data[0] = '<';

	r = NtReplyPort( port, &req );
	ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

	r = NtReplyPort( port, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtReplyPort wrong return %08lx\n", r );

	r = NtReplyWaitReceivePort( port, &client_handle, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtReplyWaitReceivePort wrong return %08lx\n", r );

	r = NtRequestWaitReplyPort( port, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtRequestWaitReply wrong return %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

void *msg_address;

void thread_proc_bad_address( void *param )
{
	LPC_MESSAGE req, reply;
	HANDLE port = param;
	NTSTATUS r;

	memset( &req, 0, sizeof req );
	memset( &reply, 0, sizeof reply );

	req.MessageType = LPC_NEW_MESSAGE;
	req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data);

	r = NtRequestWaitReplyPort( port, &req, msg_address );
	ok( r == STATUS_ACCESS_VIOLATION, "NtRequestWaitReplyPort failed %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

// What happens when virtual memory specified for writing
//   a received message is freed after the call waits?
void test_port_bad_address( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0;
	NTSTATUS r;
	ULONG size;
	LPC_MESSAGE req;
	HANDLE client_handle;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, sizeof req, sizeof req, (void*) 0 );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	memset( &req, 0, sizeof req );
	req.MessageType = LPC_NEW_MESSAGE;
	req.MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data);

	// allocate a pages
	msg_address = NULL;
	size = 0x1000;
	r = NtAllocateVirtualMemory( NtCurrentProcess(), &msg_address, 0, &size, MEM_COMMIT, PAGE_READWRITE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_bad_address, port, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	client_handle = (HANDLE) ~0;
	r = NtReplyWaitReceivePort( port, &client_handle, 0, &req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == 0, "client handle not zero\n");

	// free the page we allocated then reply
	r = NtFreeVirtualMemory( NtCurrentProcess(), &msg_address, &size, MEM_RELEASE );
	ok(r == STATUS_SUCCESS, "wrong return code\n");

	r = NtReplyPort( port, &req );
	ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );
}

HANDLE connect_port;

// figure out when the port's handle is written to the output address of NtConnectPort
void thread_proc_check_connect_output( void *param )
{
	NTSTATUS r;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	r = NtConnectPort( &connect_port, &us, &qos, NULL, NULL, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort failed %08lx\n", r);

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_connect_handle_output( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	LPC_MESSAGE req;

	set_portname( &us, &oa );

	ok( connect_port == NULL, "connect_port set\n");
	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_check_connect_output, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	r = NtListenPort( port, &req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );
	ok( connect_port == NULL, "connect_port set\n");

	r = NtAcceptConnectPort( &con_port, 0, &req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );

	NtYieldExecution();
	ok( connect_port == NULL, "connect_port set\n");

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

#if 0
	// FIXME: this test relies on "connect_port" being written when the thread
	// first runs again, but we write it when the thread is set running....

	ok( connect_port == NULL, "connect_port set\n");

	// the thread is runnable here, but connect_port won't be set until it runs
	// the test above and below are subject to races, but we'll usually win them...
#endif

	NtYieldExecution();
	NtYieldExecution();

	ok( connect_port != NULL, "connect_port not set\n");

	r = NtClose( con_port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );
}

#define TEST_SECTION_SIZE (0x1000)

struct section_info_t {
	LPC_SECTION_READ rd;
	LPC_SECTION_WRITE wr;
};

void thread_proc_section( void *param )
{
	struct section_info_t *info = param;
	NTSTATUS r;
	HANDLE port = 0, section = 0;
	UNICODE_STRING us;
	LARGE_INTEGER sz;
	LPC_SECTION_READ rd;
	LPC_SECTION_WRITE wr;
	SECURITY_QUALITY_OF_SERVICE qos;
	BYTE *sec;

	// try without setting the lengths
	memset( &us, 0, sizeof us );
	memset( &wr, 0, sizeof wr );
	memset( &rd, 0, sizeof rd );
	r = NtConnectPort( &port, &us, NULL, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "NtConnectPort %08lx\n", r );

	wr.Length = sizeof wr;
	r = NtConnectPort( &port, &us, NULL, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_INVALID_PARAMETER, "NtConnectPort %08lx\n", r );

	rd.Length = sizeof rd;
	r = NtConnectPort( &port, &us, NULL, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_ACCESS_VIOLATION, "NtConnectPort %08lx\n", r );

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	r = NtConnectPort( &port, &us, &qos, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_OBJECT_NAME_INVALID, "NtConnectPort %08lx\n", r );

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	r = NtConnectPort( &port, &us, &qos, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_INVALID_HANDLE, "NtConnectPort %08lx\n", r );

	sz.QuadPart = TEST_SECTION_SIZE;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, NULL, &sz, PAGE_EXECUTE_READWRITE, SEC_COMMIT, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
	ok( section != 0, "section zero\n");

	memset( &wr, 0, sizeof wr );
	wr.Length = sizeof wr;
	wr.SectionHandle = section;
	wr.SectionOffset = 0;
	wr.ViewSize = sz.QuadPart;
	wr.ViewBase = 0;
	wr.TargetViewBase = 0;

	memset( &rd, 0, sizeof rd );
	rd.Length = sizeof rd;
	rd.ViewSize = ~0;
	rd.ViewBase = (void*) ~0;

	r = NtConnectPort( &port, &us, &qos, &wr, &rd, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );
	ok( port != 0, "port zero\n");

	ok( rd.Length == 0, "length wrong %ld\n", rd.Length );
	ok( rd.ViewSize == TEST_SECTION_SIZE, "Viewsize wrong\n");
	ok( rd.ViewBase != 0, "Viewbase wrong\n");

	ok( wr.Length == sizeof wr, "Length wrong\n");
	ok( wr.SectionHandle == section, "SectionHandle wrong\n");
	ok( wr.SectionOffset == 0, "SectionOffset wrong\n");
	ok( wr.ViewSize == sz.QuadPart, "ViewSize wrong\n");

	// we got mapped
	ok( wr.ViewBase != 0, "ViewBase wrong\n");
	ok( wr.TargetViewBase != 0, "TargetViewBase wrong\n");
	ok( wr.ViewBase != wr.TargetViewBase, "bases the same\n");

	sec = wr.TargetViewBase;
	ok( sec[0] == 1, "byte not set\n");

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	memcpy( &info->wr, &wr, sizeof wr );
	memcpy( &info->rd, &rd, sizeof rd );

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_server_section(void)
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0, section = 0;
	NTSTATUS r;
	LARGE_INTEGER section_size;
	ULONG sz;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];
	LPC_SECTION_READ rd;
	LPC_SECTION_WRITE wr;
	BYTE *sec;
	struct section_info_t client_info;

	memset( &client_info, 0, sizeof client_info );
	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_proc_section, &client_info, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	memset( buffer, 0xff, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );
	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == TEST_SECTION_SIZE, "section size wrong %ld\n", req->SectionSize);

	NtYieldExecution();

	// create a section
	section_size.QuadPart = TEST_SECTION_SIZE;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, NULL, &section_size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
	ok( section != 0, "section zero\n");

	memset( &rd, 0, sizeof rd );
	rd.Length = sizeof rd;

	memset( &wr, 0, sizeof wr );
	wr.Length = sizeof wr;
	wr.SectionHandle = section;
	wr.SectionOffset = 0;
	wr.ViewSize = section_size.QuadPart;

	con_port = 0;
	r = NtAcceptConnectPort( &con_port, 0, req, TRUE, &wr, &rd );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == TEST_SECTION_SIZE, "section size wrong\n");
	ok( con_port != NULL, "connect port was zero\n");

	ok( rd.Length == sizeof rd, "rd.Length wrong %ld\n", rd.Length);
	ok( rd.ViewSize == TEST_SECTION_SIZE, "rd.ViewSize wrong %ld\n", rd.ViewSize);
	ok( rd.ViewBase != 0, "rd.ViewBase was zero\n");

	sec = rd.ViewBase;

	sec[0] = 1;

	// we got mapped
	ok( wr.ViewBase != 0, "ViewBase wrong\n");
	ok( wr.TargetViewBase != 0, "TargetViewBase wrong\n");
	ok( wr.ViewBase != wr.TargetViewBase, "bases the same\n");

	NtYieldExecution();

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	NtYieldExecution();

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");
	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");

	ok( client_info.wr.TargetViewBase == rd.ViewBase,
		"base mismatch %p %p\n", client_info.wr.TargetViewBase, rd.ViewBase );
	ok( client_info.rd.ViewBase == wr.TargetViewBase,
		"base mismatch %p %p\n", client_info.rd.ViewBase, wr.TargetViewBase );
}

void thread_secure_proc_section( void *param )
{
	NTSTATUS r;
	HANDLE port = 0, section = 0;
	UNICODE_STRING us;
	LARGE_INTEGER sz;
	LPC_SECTION_READ rd;
	LPC_SECTION_WRITE wr;
	SECURITY_QUALITY_OF_SERVICE qos;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	sz.QuadPart = TEST_SECTION_SIZE;
	r = NtCreateSection( &section, SECTION_ALL_ACCESS, NULL, &sz, PAGE_EXECUTE_READWRITE, SEC_COMMIT, 0 );
	ok( r == STATUS_SUCCESS, "return code wrong %08lx\n", r);
	ok( section != 0, "section zero\n");

	memset( &wr, 0, sizeof wr );
	wr.Length = sizeof wr;
	wr.SectionHandle = section;
	wr.SectionOffset = 0;
	wr.ViewSize = sz.QuadPart;

	memset( &rd, 0, sizeof rd );
	rd.Length = sizeof rd;

	r = NtSecureConnectPort( &port, &us, &qos, &wr, NULL, &rd, NULL, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );
	ok( port != 0, "port zero\n");

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_secure_connect( void )
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0;
	NTSTATUS r;
	ULONG sz;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_secure_proc_section, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	memset( buffer, 0, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );

	con_port = 0;
	r = NtAcceptConnectPort( &con_port, 0, req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( req->DataSize == 0, "data size wrong\n");
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data), "message size wrong\n");
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == TEST_SECTION_SIZE, "section size wrong\n");
	ok( con_port != NULL, "connect port was zero\n");

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");
	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");
}

//
// The purpose of this test is to show how the ConnectData parameter works
//
char client_connect_data[] = "Hello";
char server_connect_data[] = "Welcome";

void thread_connect_param( void *param )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;
	ULONG sz, max_msg_size;
	BYTE connect_data[0x10];

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	sz = sizeof client_connect_data;
	memcpy( connect_data, client_connect_data, sz );
	max_msg_size = 0;
	r = NtConnectPort( &port, &us, &qos, NULL, NULL, &max_msg_size, connect_data, &sz );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );
	ok( port != 0, "port zero\n");
	ok( sz == sizeof client_connect_data, "returned connect data not zero %ld\n", sz);
	ok( !memcmp(connect_data, server_connect_data, sz), "returned connect data wrong\n");
	ok( max_msg_size == 0x100, "maximum message size %ld\n", max_msg_size);

	r = NtRegisterThreadTerminatePort( 0 );
	ok( r == STATUS_INVALID_HANDLE, "NtRegisterThreadTerminatePort failed %08lx\n", r);

	r = NtRegisterThreadTerminatePort( port );
	ok( r == STATUS_SUCCESS, "NtRegisterThreadTerminatePort failed %08lx\n", r);

	r = NtRegisterThreadTerminatePort( port );
	ok( r == STATUS_SUCCESS, "NtRegisterThreadTerminatePort failed %08lx\n", r);

	//r = NtClose( port );
	//ok( r == STATUS_SUCCESS, "NtClose failed\n");

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_connect_param( void )
{
	THREAD_BASIC_INFORMATION info;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0, client_handle;
	NTSTATUS r;
	ULONG sz;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];
	void *magic = (void*) 0xf00baa;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_connect_param, NULL, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	memset( buffer, 0, sizeof buffer );
	req = (void*) buffer;
	r = NtListenPort( port, req );
	ok( r == STATUS_SUCCESS, "NtListenPort failed %08lx\n", r );

	ok( req->DataSize == sizeof client_connect_data, "data size wrong %d\n", req->DataSize);
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + sizeof client_connect_data, "message size wrong %d\n", req->MessageSize);
	ok( req->MessageType == LPC_CONNECTION_REQUEST, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");

	req->DataSize = sizeof server_connect_data;
	req->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + req->DataSize;
	memcpy( req->Data, server_connect_data, req->DataSize );
	con_port = 0;
	r = NtAcceptConnectPort( &con_port, magic, req, TRUE, NULL, NULL );
	ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
	ok( con_port != NULL, "connect port was zero\n");

	r = NtCompleteConnectPort( con_port );
	ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );

	// wait for the client thread to terminate
	memset( buffer, 0, sizeof buffer );
	client_handle = 0;
	r = NtReplyWaitReceivePort( con_port, &client_handle, 0, req );
	ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );
	ok( client_handle == magic, "port wrong %p %p\n", client_handle, magic);

	ok( req->DataSize == sizeof (LARGE_INTEGER), "data size wrong %d\n", req->DataSize);
	ok( req->MessageSize == FIELD_OFFSET(LPC_MESSAGE, Data) + sizeof (LARGE_INTEGER), "message size wrong\n");
	ok( req->MessageType == LPC_CLIENT_DIED, "message type wrong\n");
	ok( req->VirtualRangesOffset == 0, "offset wrong\n");
	ok( req->MessageId != 0, "message id was zero\n");
	ok( req->SectionSize == 0, "section size wrong\n");

	r = NtWaitForSingleObject( thread, FALSE, NULL );
	ok( r == STATUS_SUCCESS, "wait failed\n" );

	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed\n");

	r = NtQueryInformationThread( thread, ThreadBasicInformation, &info, sizeof info, &sz );
	ok( r == STATUS_SUCCESS, "NtQueryInformationThread failed\n");
	ok( info.ExitStatus == STATUS_SUCCESS, "exit code wrong\n");
}

void thread_loop_client( PVOID arg )
{
	NTSTATUS r;
	HANDLE port = 0;
	UNICODE_STRING us;
	SECURITY_QUALITY_OF_SERVICE qos;
	int i;

	qos.Length = sizeof(qos);
	qos.ImpersonationLevel = SecurityAnonymous;
	qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
	qos.EffectiveOnly = TRUE;

	us.Buffer = portname;
	us.Length = sizeof portname - 2;
	us.MaximumLength = 0;

	r = NtConnectPort( &port, &us, &qos, 0, 0, 0, 0, 0 );
	ok( r == STATUS_SUCCESS, "NtConnectPort %08lx\n", r );

	r = NtRegisterThreadTerminatePort( port );
	ok( r == STATUS_SUCCESS, "NtRegisterThreadTerminatePort failed %08lx\n", r);

	for (i=0; i<10; i++)
	{
		BYTE req_buffer[0x100], reply_buffer[0x100];
		LPC_MESSAGE *req, *reply;

		memset( req_buffer, 0, sizeof req_buffer );
		memset( reply_buffer, 0, sizeof reply_buffer );
		req = (void*) req_buffer;
		reply = (void*) reply_buffer;

		req->MessageSize = FIELD_OFFSET(LPC_MESSAGE, Data) + 1;
		req->DataSize = 1;
		req->Data[0] = i+1;

		r = NtRequestWaitReplyPort( port, req, reply );
		ok( r == STATUS_SUCCESS, "NtRequestWaitReplyPort failed %08lx\n", r);
	}

	if (arg)
	{
		/* crash */
		__asm__ ( "\n\tmovl $0, (%%eax)\n" : : "a"(0) );
	}

	NtTerminateThread( NtCurrentThread(), STATUS_SUCCESS );
}

void test_port_loop(void)
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	CLIENT_ID id;
	HANDLE thread = 0, port = 0, con_port = 0, client_handle;
	NTSTATUS r;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];
	int i = 0;
	int finished = 0;
	int caught_exception = 0;
	int connection_requests = 0;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE,
							 NULL, 0, 0, thread_loop_client, (PVOID)1, &thread, &id );
	ok( r == STATUS_SUCCESS, "failed to create thread\n" );

	r = NtSetInformationProcess( NtCurrentProcess(), ProcessExceptionPort, &port, sizeof port );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtSetInformationProcess( NtCurrentProcess(), ProcessExceptionPort, &port, sizeof port );
	ok( r == STATUS_PORT_ALREADY_SET, "wrong return %08lx\n", r );

	while (!finished)
	{
		req = (LPC_MESSAGE*) buffer;
		r = NtReplyWaitReceivePort( port, &client_handle, 0, req );
		ok( r == STATUS_SUCCESS, "NtReplyWaitReceivePort failed %08lx\n", r );

		switch (req->MessageType)
		{
		case LPC_CONNECTION_REQUEST:
			connection_requests++;
			ok( client_handle == 0, "port wrong %p\n", client_handle );
			ok(req->Data[0] == i, "sequence wrong %d\n", i);
			i++;

			r = NtAcceptConnectPort( &con_port, (void*)1234, req, TRUE, NULL, NULL );
			ok( r == STATUS_SUCCESS, "NtAcceptConnectPort failed %08lx\n", r );
			r = NtCompleteConnectPort( con_port );
			ok( r == STATUS_SUCCESS, "NtCompleteConnectPort failed %08lx\n", r );
			break;

		case LPC_CLIENT_DIED:
			i++;
			finished = 1;
			break;

		case LPC_REQUEST:
			ok( client_handle == (void*)1234, "port wrong %p\n", client_handle );

			// count the number of messages received
			ok(req->Data[0] == i, "sequence wrong %d\n", i);
			i++;

			// wake the other side
			r = NtReplyPort( port, req );
			ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

			break;

		case LPC_EXCEPTION:
			{
				CONTEXT ctx;
				static ULONG dummy;
				struct {
					ULONG EventCode;
					ULONG Status;
					EXCEPTION_RECORD ExceptionRecord;
				} *x = (void*) req->Data;

				ok( client_handle == 0, "port wrong %p\n", client_handle );
				ok( req->DataSize == 0x5c, "size wrong %d\n", req->DataSize);
				ok( req->MessageSize == 0x78, "size wrong %d\n", req->MessageSize);

				ok( x->EventCode == 0, "Event code wrong\n");
				ok( x->Status == STATUS_PENDING, "Status code wrong\n");

				memset( &ctx, 0, sizeof ctx );
				ctx.ContextFlags = CONTEXT_INTEGER;
				r = NtGetContextThread( thread, &ctx );
				ok( r == STATUS_SUCCESS, "NtGetContextThread failed %08lx\n", r );

				ctx.Eax = (LONG) &dummy;

				ctx.ContextFlags = CONTEXT_INTEGER;
				r = NtSetContextThread( thread, &ctx );
				ok( r == STATUS_SUCCESS, "NtSetContextThread failed %08lx\n", r );

				x->Status = DBG_CONTINUE;

				r = NtReplyPort( port, req );
				ok( r == STATUS_SUCCESS, "NtReplyPort failed %08lx\n", r );

				caught_exception = 1;
			}
			break;

		default:
			ok(0, "unknown message type %d\n", req->MessageType);
			finished = 1;
		}
	}

	// connect + 10 messages + thread died message
	ok( i == 12, "message count wrong %d\n", i);
	ok( connection_requests == 1, "message count wrong %d\n", i);
	ok( caught_exception == 1, "exception not handled\n");

	r = NtClose( con_port );
	ok( r == STATUS_SUCCESS, "NtClose failed %08lx\n", r );
	r = NtClose( port );
	ok( r == STATUS_SUCCESS, "NtClose failed %08lx\n", r );
	r = NtClose( thread );
	ok( r == STATUS_SUCCESS, "NtClose failed %08lx\n", r );
}

#define NUM_THREADS 2

void test_port_multi_thread( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	HANDLE client_thread[NUM_THREADS];
	CLIENT_ID client_id[NUM_THREADS];
	HANDLE port = 0;
	NTSTATUS r;
	LPC_MESSAGE *req;
	BYTE buffer[0x100];
	int i = 0;
	int terminated_threads = 0;
	void *multi_thread_client = thread_loop_client;
	BOOL accept_connect_ok = TRUE;
	BOOL complete_connect_ok = TRUE;
	BOOL reply_port_ok = TRUE;
	BOOL unknown_messages = FALSE;

	set_portname( &us, &oa );

	r = NtCreatePort( &port, &oa, 0x100, 0x100, (void*) 0x4d4d );
	ok( r == STATUS_SUCCESS, "wrong return %08lx\n", r );

	r = NtRegisterThreadTerminatePort( port );
	ok( r == STATUS_SUCCESS, "NtRegisterThreadTerminatePort failed %08lx\n", r);

	// kick off clients
	for (i=0; i<NUM_THREADS; i++)
	{
		r = RtlCreateUserThread( NtCurrentProcess(), NULL, FALSE, NULL, 0, 0,
						 multi_thread_client, NULL, client_thread+i, client_id+i );
		ok( r == STATUS_SUCCESS, "failed to create thread\n" );
	}

	// listen for connecting clients and start server threads
	while (terminated_threads < NUM_THREADS)
	{
		HANDLE client_handle = 0;
		HANDLE conn_port = 0;

		req = (LPC_MESSAGE*) buffer;
		r = NtReplyWaitReceivePort( port, &client_handle, 0, req );
		if (r != STATUS_SUCCESS)
		{
			ok( 0, "NtReplyWaitReceivePort failed %08lx\n", r );
			break;
		}

		switch (req->MessageType)
		{
		case LPC_CONNECTION_REQUEST:
			// accept the connection
			r = NtAcceptConnectPort( &conn_port, 0, req, TRUE, NULL, NULL );
			if (accept_connect_ok && r != STATUS_SUCCESS)
			{
				ok( 0, "NtAcceptConnectPort failed %08lx\n", r );
				accept_connect_ok = FALSE;
			}

			r = NtCompleteConnectPort( conn_port );
			if (complete_connect_ok && r != STATUS_SUCCESS)
			{
				ok( 0, "NtCompleteConnectPort failed %08lx\n", r );
				complete_connect_ok = FALSE;
			}
			break;

		case LPC_REQUEST:
			// wake the other side
			r = NtReplyPort( port, req );
			if (reply_port_ok && r != STATUS_SUCCESS)
			{
				ok( 0, "NtReplyPort failed %08lx\n", r );
				reply_port_ok = FALSE;
			}
			break;

		case LPC_CLIENT_DIED:
			// count terminated threads
			terminated_threads++;
			break;

		default:
			if (!unknown_messages)
			{
				ok( 0, "unknown message %08x\n", req->MessageType );
			}
			unknown_messages |= TRUE;
		}
	}

	// bump up the test counts
	ok( accept_connect_ok, "NtAcceptConnectPort failed\n" );
	ok( complete_connect_ok, "NtCompleteConnectPort failed\n" );
	ok( reply_port_ok, "NtReplyPort failed\n" );
	ok( !unknown_messages, "unknown messages!\n");

	r = NtWaitForMultipleObjects( NUM_THREADS, client_thread, WaitAll, 0, 0 );
	ok( r == STATUS_SUCCESS, "wait failed %08lx\n", r );

	// close all the thread handles
	for (i=0; i<NUM_THREADS; i++)
		NtClose( client_thread );

	NtClose( port );
}

// creation of \RPC Control is done by smss.exe
// create it for tests
void create_rpc_control_link( void )
{
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING us;
	WCHAR dirname[] = L"\\RPC Control";
	HANDLE dir;

	us.Buffer = dirname;
	us.Length = sizeof dirname - 2;
	us.MaximumLength = 0;

	oa.Length = sizeof oa;
	oa.RootDirectory = 0;
	oa.ObjectName = &us;
	oa.Attributes = OBJ_CASE_INSENSITIVE;
	oa.SecurityDescriptor = 0;
	oa.SecurityQualityOfService = 0;

	NtCreateDirectoryObject( &dir, DIRECTORY_ALL_ACCESS, &oa );
}

void NtProcessStartup( void )
{
	log_init();

	create_rpc_control_link();
	test_create_port();
	test_port_server();
	test_port_server_reject();
	test_port_server_ping();
	test_wrong_object();
	test_port_server_wait_receive();
	test_port_no_connect();
	test_port_server_share_handle();
	test_port_message_copy();
	test_port_bad_address();
	test_port_connect_handle_output();
	test_port_server_section();
	test_port_secure_connect();
	test_port_connect_param();
	test_port_loop();
	test_port_multi_thread();

	log_fini();
}
