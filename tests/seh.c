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
#include "log.h"

#define MAGIC 0x1234

ULONG value = MAGIC;
void *pointer;

EXCEPTION_DISPOSITION NTAPI test_handler(struct _EXCEPTION_RECORD *rec, void *frame, struct _CONTEXT *ctx, void* dispatch)
{
	const ULONG all = \
		CONTEXT_FLOATING_POINT |
		CONTEXT_DEBUG_REGISTERS |
		CONTEXT_EXTENDED_REGISTERS |
		CONTEXT_FULL;
	ok(ctx->ContextFlags == all, "context flags %08lx\n", ctx->ContextFlags);
	ctx->Eax = (DWORD) &value;
	return ExceptionContinueExecution;
}

void test_exception_handler(void)
{
	ULONG ret = 0;

	pointer = NULL;

	__asm__ __volatile__ (
		"movl %%fs:0, %%eax\n\t"
		"pushl %2\n\t"
		"pushl %%eax\n\t"
		"movl %%esp, %%fs:0\n\t"
		"movl (%0), %%eax\n\t"
		"movl (%%eax), %1\n\t"  // exception frame executed here
		"popl %%eax\n\t"
		"movl %%eax, %%fs:0\n\t"
		"addl $4, %%esp\n\t"
		 : "=m"(pointer),"=r"(ret) : "r"(test_handler) : "eax");

	ok( ret == MAGIC, "exception handler didn't trigger\n");
}

void test_continue(void)
{
	CONTEXT ctx;
	ULONG ret;

	ret = NtContinue( NULL, 0 );
	ok( STATUS_ACCESS_VIOLATION, "ret wrong (%08lx)\n", ret);

	// setup all control registers
	memset(&ctx, 0, sizeof ctx);
	ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	ctx.Eax = 456;

	__asm__ __volatile__ (
		"movl %%ebp, 0(%1)\n"	// ctx.Ebp
		"\tcall app1\n"
		"\tjmp app2\n"		   // execution starts here 2nd time round
		"app1:\n"
		"\tpopl %%ebx\n"
		"\tmovl %%ebx, 4(%1)\n"  // ctx.Eip
		"\tmov %%cs, 8(%1)\n"	// ctx.SegCs
		"\tpushf\n"
		"\tpopl %%ebx\n"
		"\tmovl %%ebx, 12(%1)\n" // ctx.EFlags
		"\tmovl %%esp, 16(%1)\n" // ctx.Esp
		"\tmov %%ss, 20(%1)\n"   // ctx.SegSs
		"\tmovl $123, %%eax\n"
		"app2:\n\t"
		: "=a"(ret) : "a"(&ctx.Ebp) : "ebx" );

	switch (ret)
	{
	case 123:
		ok(1, "should get here\n");
		NtContinue( &ctx, 0 );
		ok(0, "shouldn't get here\n");
		break;
	case 456:
		ok(1, "should get here\n");
		break;
	default:
		ok(0, "eax wrong\n");
	}
}

EXCEPTION_DISPOSITION NTAPI raise_handler(
	PEXCEPTION_RECORD rec,
	void *frame,
	PCONTEXT ctx,
	void* dispatch)
{
	ctx->Eax = 456;
	return ExceptionContinueExecution;
}

void test_raise_exception(void)
{
	ULONG ret;
	struct _info {
		CONTEXT ctx;
		EXCEPTION_RECORD rec;
	} info;

	memset( &info, 0, sizeof info );
	info.ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	info.ctx.Eax = 123;

	// simulate an access violation
	info.rec.ExceptionCode = STATUS_ACCESS_VIOLATION;

	ret = NtRaiseException( NULL, NULL, 0 );
	ok( STATUS_ACCESS_VIOLATION, "ret wrong (%ld)\n", ret);

	ret = NtRaiseException( &info.rec, NULL, 0 );
	ok( STATUS_ACCESS_VIOLATION, "ret wrong (%ld)\n", ret);

	ret = NtRaiseException( NULL, &info.ctx, 0 );
	ok( STATUS_ACCESS_VIOLATION, "ret wrong (%ld)\n", ret);

#if 0
	// This call cause the process to exit immediately without
	//  entering the exception frame.

	ret = NtRaiseException( &info.rec, &info.ctx, 0 );
	ok( STATUS_SUCCESS, "ret wrong (%d)\n", ret);
#endif

	// pushing the frame onto the stack with a
	// couple of push instructions didn't work so well...
	void *exframe[2];
	exframe[1] = raise_handler;
	__asm__ __volatile__ (
		"movl %%fs:0, %%eax\n"
		"\tmov %%eax, (%0)\n"
		"\tmovl %0, %%fs:0\n"
		 : : "r"(&exframe) : "eax");

	// write registers into CONTEXT
	__asm__ __volatile__ (
		"movl %%ebp, 0(%%ebx)\n"		// ctx.Ebp
		"\tcall app3\n"				 // push eip
		"\tjmp app4\n"
		"app3:\n"
		"\tpopl %%eax\n"
		"\tmovl %%eax, 4(%%ebx)\n"	  // ctx.Eip
		"\tmov %%cs, 8(%%ebx)\n"		// ctx.SegCs
		"\tpushf\n"
		"\tpopl %%eax\n"
		"\tmovl %%eax, 12(%%ebx)\n"	 // ctx.EFlags
		"\tmovl %%esp, 16(%%ebx)\n"	 // ctx.Esp
		"\tmov %%ss, 20(%%ebx)\n"	   // ctx.SegSs
		 : : "b"(&info.ctx.Ebp) : "eax" );

	ret = NtRaiseException( &info.rec, &info.ctx, 1 );

	// pop the exception frame
	__asm__ __volatile__ (
		"movl $0, %%eax\n"
		"app4:\n"
		"\tmovl %0, %%fs:0\n"
		 : "=a"(ret) : "r"(exframe[0]) );

	ok( ret == 456, "return was (%08lx)\n", ret);
}

ULONG apc_value;

#define APC_PARAM1_MAGIC ((void*)0xfeed0001)
#define APC_PARAM2_MAGIC ((void*)0xfeed0002)
#define APC_PARAM3_MAGIC ((void*)0xfeed0003)

VOID NTAPI apc_routine(PVOID arg1, PVOID arg2, PVOID arg3)
{
	//dprintf("apc called %p %p %p\n", arg1, arg2, arg3);

	ok(arg1 == APC_PARAM1_MAGIC, "arg1 wrong\n");
	ok(arg2 == APC_PARAM2_MAGIC, "arg2 wrong\n");
	ok(arg3 == APC_PARAM3_MAGIC, "arg3 wrong\n");

	apc_value++;
}

void test_apc(void)
{
	NTSTATUS r;
	HANDLE event = 0;

	r = NtQueueApcThread(0, 0, 0, 0, 0);
	ok( r == STATUS_INVALID_HANDLE, "return was (%08lx)\n", r);

	r = NtQueueApcThread(NtCurrentThread(), apc_routine,
						 APC_PARAM1_MAGIC, APC_PARAM2_MAGIC, APC_PARAM3_MAGIC);
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	// let the APC run
	apc_value = 0;
	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( apc_value == 1, "apc_value not changed\n" );

	r = NtQueueApcThread(NtCurrentThread(), 0, 0, 0, 0);
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtQueueApcThread(NtCurrentThread(), apc_routine,
						 APC_PARAM1_MAGIC, APC_PARAM2_MAGIC, APC_PARAM3_MAGIC);
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtCreateEvent( &event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, 1 );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	apc_value = 0;
	r = NtWaitForSingleObject( event, 1, NULL );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
	ok( apc_value == 0, "apc_value changed\n" );

	apc_value = 0;
	r = NtWaitForSingleObject( event, 1, NULL );
	ok( r == STATUS_USER_APC, "return was (%08lx)\n", r);
	ok( apc_value == 1, "apc_value not changed\n" );

	// show that NULL procedure APCs are ignored
	r = NtQueueApcThread(NtCurrentThread(), 0, 0, 0, 0);
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	apc_value = 0;
	LARGE_INTEGER timeout;
	timeout.QuadPart = 0LL;
	r = NtWaitForSingleObject( event, 1, &timeout );
	ok( r == STATUS_TIMEOUT, "return was (%08lx)\n", r);
	ok( apc_value == 0, "apc_value changed\n" );

	r = NtAlertThread( NtCurrentThread() );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	// show NtTestAlert clears the alerted flag
	r = NtTestAlert();
	ok( r == STATUS_ALERTED, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	// show the number of times a thread is alerted is not counted
	r = NtAlertThread( NtCurrentThread() );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtAlertThread( NtCurrentThread() );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_ALERTED, "return was (%08lx)\n", r);

	r = NtTestAlert();
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	r = NtClose( event );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);
}

#define DBG_PRINTEXCEPTION_C 0x40010006

void test_outputdebugstring( void )
{
	char string[] = "hello";
	CONTEXT ctx;
	EXCEPTION_RECORD rec;
	NTSTATUS r;

	rec.ExceptionCode = DBG_PRINTEXCEPTION_C;
	rec.ExceptionFlags = 0;
	rec.ExceptionRecord = 0;
	rec.ExceptionAddress = 0;
	rec.NumberParameters = 2;
	rec.ExceptionInformation[0] = sizeof string - 1;
	rec.ExceptionInformation[1] = (ULONG) string;

	ctx.ContextFlags = CONTEXT_CONTROL;
	r = NtGetContextThread( NtCurrentThread(), &ctx );
	ok( r == STATUS_SUCCESS, "return was (%08lx)\n", r);

	__asm__ __volatile(
		"\tpush $1\n"
		"\tpush %1\n"
		"\tpush %0\n"
	// set the instuction pointer in the context to be after NtRaiseException
		"\tcall 1f\n"
	"1:\n"
		"\tpop %%eax\n"
		"\tadd $(2f - 1b), %%eax\n"
		"\tmov %%eax, (%2)\n"
		"\tcall _NtRaiseException@12\n"
	"2:\n"
		: : "a"( &rec), "b"(&ctx), "c"(&ctx.Eip) );
}

EXCEPTION_DISPOSITION NTAPI int80_handler(struct _EXCEPTION_RECORD *rec, void *frame, struct _CONTEXT *ctx, void* dispatch)
{
	// check eip is correct
	BYTE *ins = (BYTE*) ctx->Eip;
	if (ins[0] != 0xcd || ins[1] != 0x80)
		return ExceptionContinueSearch;

	// skip int80 below
	ctx->Eip += 2;

	// return the exception code
	ctx->Ebx = rec->ExceptionCode;

	return ExceptionContinueExecution;
}

void test_syscall( void )
{
	ULONG ret;

	__asm__ __volatile__ (
		"movl %%fs:0, %%eax\n\t"
		"pushl %1\n\t"
		"pushl %%eax\n\t"
		"movl %%esp, %%fs:0\n\t"

		// try make a linux system call
		"movl $60, %%eax\n\t"
		"movl $1, %%ebx\n\t"
		"int $0x80\n\t"

		"popl %%eax\n\t"
		"movl %%eax, %%fs:0\n\t"
		"popl %%eax\n\t"
		 : "=b"(ret) : "r"(int80_handler) : "eax");

	// check the exception code is correct
	ok( ret == STATUS_ACCESS_VIOLATION, "wrong exception type %08lx\n", ret);
}

EXCEPTION_DISPOSITION NTAPI int3_handler(struct _EXCEPTION_RECORD *rec, void *frame, struct _CONTEXT *ctx, void* dispatch)
{
	// check Eip is correct
	BYTE *ins = (BYTE*) ctx->Eip;
	if (*ins != 0xcc)
		return ExceptionContinueSearch;

	// skip int3 below
	ctx->Eip += 1;

	// return the exception code
	ctx->Ebx = rec->ExceptionCode;

	return ExceptionContinueExecution;
}

void test_breakpoint( void )
{
	ULONG ret;

	__asm__ __volatile__ (
		"movl %%fs:0, %%eax\n\t"
		"pushl %1\n\t"
		"pushl %%eax\n\t"
		"movl %%esp, %%fs:0\n\t"

		// try make a break point
		"movl $0, %%ebx\n\t"
		"int $3\n\t"

		"popl %%eax\n\t"
		"movl %%eax, %%fs:0\n\t"
		"popl %%eax\n\t"
		 : "=b"(ret) : "r"(int3_handler) : "eax");
	ok( ret == STATUS_BREAKPOINT, "wrong handling of int3 %08lx\n", ret);
}

void NtProcessStartup( void )
{
	NTSTATUS r = 0;

	r = log_init();

	test_exception_handler();
	test_exception_handler();
	test_continue();
	test_raise_exception();
	test_apc();
	test_syscall();
	test_breakpoint();

	// doesn't match windows behaviour yet
	//test_outputdebugstring();

	log_fini();
}
