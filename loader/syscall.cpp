/*
 * nt loader
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
#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "debug.h"
#include "ntcall.h"
#include "ntgdi.h"
#include "ntuser.h"

typedef struct _ntcalldesc {
	const char *name;
	void *func;
	unsigned int numargs;
} ntcalldesc;

#define NUL(x) { #x, NULL, 0 }	/* not even declared */
#define DEC(x,n) { #x, NULL, n }  /* no stub implemented */
#define IMP(x,n) { #x, (void*)x, n }	 /* entry point implemented */

ntcalldesc win2k_calls[] = {
	/* 0x00 */
	IMP( NtAcceptConnectPort, 6 ),
	IMP( NtAccessCheck, 8 ),
	DEC( NtAccessCheckAndAuditAlarm, 11),
	DEC( NtAccessCheckByType, 11 ),

	NUL( NtAccessCheckByTypeAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultList ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarmByHandle ),

	IMP( NtAddAtom, 3 ),
	DEC( NtAdjustGroupsToken, 6 ),
	IMP( NtAdjustPrivilegesToken, 6 ),
	IMP( NtAlertResumeThread, 2 ),

	IMP( NtAlertThread, 1 ),
	IMP( NtAllocateLocallyUniqueId, 1 ),
	DEC( NtAllocateUserPhysicalPages, 3 ),
	DEC( NtAllocateUuids, 3 ),
	//"NtAlertResumeThread", // check
	/* 0x10 */
	IMP( NtAllocateVirtualMemory, 6 ),
	IMP( NtAreMappedFilesTheSame, 2 ),
	IMP( NtAssignProcessToJobObject, 2 ),
	IMP( NtCallbackReturn, 3 ),

	IMP( NtCancelIoFile, 2 ),
	IMP( NtCancelTimer, 2 ),
	IMP( NtCancelDeviceWakeupRequest, 1 ),
	IMP( NtClearEvent, 1 ),

	IMP( NtClose, 1 ),
	IMP( NtCloseObjectAuditAlarm, 3 ),
	IMP( NtCompleteConnectPort, 1 ),
	IMP( NtConnectPort, 8 ),

	IMP( NtContinue, 2 ),
	IMP( NtCreateDirectoryObject, 3 ),
	IMP( NtCreateEvent, 5 ),
	IMP( NtCreateEventPair, 3 ),
	/* 0x20 */
	IMP( NtCreateFile, 11 ),
	IMP( NtCreateIoCompletion, 4 ),
	IMP( NtCreateJobObject, 3 ),
	IMP( NtCreateKey, 7 ),

	IMP( NtCreateMailslotFile, 8 ),
	IMP( NtCreateMutant, 4 ),
	IMP( NtCreateNamedPipeFile, 14 ),
	IMP( NtCreatePagingFile, 4 ),

	IMP( NtCreatePort, 5 ),
	IMP( NtCreateProcess, 8 ),
	DEC( NtCreateProfile, 9 ),
	IMP( NtCreateSection, 7 ),

	IMP( NtCreateSemaphore, 5 ),
	IMP( NtCreateSymbolicLinkObject, 4 ),
	IMP( NtCreateThread, 8 ),
	IMP( NtCreateTimer, 4 ),
	/* 0x30 */
	DEC( NtCreateToken, 13 ),
	DEC( NtCreateWaitablePort, 5 ),
	IMP( NtDelayExecution, 2 ),
	IMP( NtDeleteAtom, 1 ),

	IMP( NtDeleteFile, 1 ),
	IMP( NtDeleteKey, 1 ),
	NUL( NtDeleteObjectAuditAlarm ),
	IMP( NtDeleteValueKey, 2 ),

	IMP( NtDeviceIoControlFile, 10 ),
	IMP( NtDisplayString, 1 ),
	IMP( NtDuplicateObject, 7 ),
	IMP( NtDuplicateToken, 6 ),

	IMP( NtEnumerateKey, 6 ),
	IMP( NtEnumerateValueKey, 6 ),
	DEC( NtExtendSection, 2 ),
	IMP( NtFilterToken, 6 ),
	/* 0x40 */
	IMP( NtFindAtom, 3 ),
	IMP( NtFlushBuffersFile, 2 ),
	IMP( NtFlushInstructionCache, 3 ),
	IMP( NtFlushKey, 1 ),

	IMP( NtFlushVirtualMemory, 4 ),
	DEC( NtFlushWriteBuffer, 1 ),
	DEC( NtFreeUserPhysicalPages, 3 ),
	IMP( NtFreeVirtualMemory, 4 ),

	IMP( NtFsControlFile, 10 ),
	IMP( NtGetContextThread, 2 ),
	IMP( NtGetDevicePowerState, 2 ),
	DEC( NtGetPlugPlayEvent, 4 ),

	DEC( NtGetTickCount, 0 ),
	DEC( NtGetWriteWatch, 7 ),
	DEC( NtImpersonateAnonymousToken, 1 ),
	DEC( NtImpersonateClientOfPort, 2 ),
	/* 0x50 */
	IMP( NtImpersonateThread, 3 ),
	IMP( NtInitializeRegistry, 1 ),
	IMP( NtInitiatePowerAction, 4 ),
	IMP( NtIsSystemResumeAutomatic, 0 ),

	IMP( NtListenPort, 2 ),
	IMP( NtLoadDriver, 1 ),
	IMP( NtLoadKey, 2 ),
	NUL( NtLoadKey2 ),

	IMP( NtLockFile, 10 ),
	IMP( NtLockVirtualMemory, 4 ),
	DEC( NtMakeTemporaryObject, 1 ),
	DEC( NtMapUserPhysicalPages, 3 ),

	NUL( NtMapUserPhysicalPagesScatter ),
	IMP( NtMapViewOfSection, 10 ),
	DEC( NtNotifyChangeDirectoryFile, 9 ),
	IMP( NtNotifyChangeKey, 10 ),
	/* 0x60 */
	IMP( NtNotifyChangeMultipleKeys, 12 ),
	IMP( NtOpenDirectoryObject, 3 ),
	IMP( NtOpenEvent, 3 ),
	IMP( NtOpenEventPair, 3 ),

	IMP( NtOpenFile, 6 ),
	IMP( NtOpenIoCompletion, 3 ),
	IMP( NtOpenJobObject, 3 ),
	IMP( NtOpenKey, 3 ),

	IMP( NtOpenMutant, 3),
	DEC( NtOpenObjectAuditAlarm, 12 ),
	IMP( NtOpenProcess, 4 ),
	IMP( NtOpenProcessToken, 3 ),

	IMP( NtOpenSection, 3 ),
	IMP( NtOpenSemaphore, 3 ),
	IMP( NtOpenSymbolicLinkObject, 3 ),
	DEC( NtOpenThread, 4 ),
	/* 0x70 */
	IMP( NtOpenThreadToken, 4 ),
	IMP( NtOpenTimer, 3 ),
	NUL( NtPlugPlayControl ),
	IMP( NtPowerInformation, 5 ),

	IMP( NtPrivilegeCheck, 3 ),
	IMP( NtPrivilegedServiceAuditAlarm, 5 ),
	IMP( NtPrivilegeObjectAuditAlarm, 6 ),
	IMP( NtProtectVirtualMemory, 5 ),

	IMP( NtPulseEvent, 2 ),
	IMP( NtQueryInformationAtom, 5 ),
	IMP( NtQueryAttributesFile, 2 ),
	IMP( NtQueryDefaultLocale, 2 ),

	IMP( NtQueryDefaultUILanguage, 1 ),
	IMP( NtQueryDirectoryFile, 11 ),
	IMP( NtQueryDirectoryObject, 7 ),
	DEC( NtQueryEaFile, 9 ),
	/* 0x80 */
	IMP( NtQueryEvent, 5 ),
	IMP( NtQueryFullAttributesFile, 2 ),
	IMP( NtQueryInformationFile, 5 ),
	IMP( NtQueryInformationJobObject, 5 ),

	DEC( NtQueryIoCompletion, 5 ),
	DEC( NtQueryInformationPort, 5 ),
	IMP( NtQueryInformationProcess, 5 ),
	IMP( NtQueryInformationThread, 5 ),

	IMP( NtQueryInformationToken, 5 ),
	IMP( NtQueryInstallUILanguage, 1 ),
	DEC( NtQueryIntervalProfile, 2 ),
	IMP( NtQueryKey, 5 ),

	IMP( NtQueryMutant, 5 ),
	DEC( NtQueryMultipleValueKey, 6 ),
	IMP( NtQueryObject, 5 ),
	IMP( NtQueryOpenSubKeys, 2 ),
	/* 0x90 */
	IMP( NtQueryPerformanceCounter, 2 ),
	DEC( NtQueryQuotaInformationFile, 9 ),
	IMP( NtQuerySection, 5 ),
	IMP( NtQuerySecurityObject, 5 ),

	IMP( NtQuerySemaphore, 5 ),
	IMP( NtQuerySymbolicLinkObject, 3 ),
	DEC( NtQuerySystemEnvironmentValue, 4 ),
	IMP( NtQuerySystemInformation, 4 ),

	IMP( NtQuerySystemTime, 1 ),
	IMP( NtQueryTimer, 5 ),
	DEC( NtQueryTimerResolution, 3 ),
	IMP( NtQueryValueKey, 6 ),

	IMP( NtQueryVirtualMemory, 6 ),
	IMP( NtQueryVolumeInformationFile, 5 ),
	IMP( NtQueueApcThread, 5 ),
	IMP( NtRaiseException, 3 ),
	/* 0xa0 */
	IMP( NtRaiseHardError, 6 ),
	IMP( NtReadFile, 9 ),
	DEC( NtReadFileScatter, 9 ),
	DEC( NtReadRequestData, 6 ),

	DEC( NtReadVirtualMemory, 5 ),
	IMP( NtRegisterThreadTerminatePort, 1 ),
	IMP( NtReleaseMutant, 2 ),
	IMP( NtReleaseSemaphore, 3 ),

	IMP( NtRemoveIoCompletion, 5 ),
	IMP( NtReplaceKey, 3 ),
	IMP( NtReplyPort, 2 ),
	IMP( NtReplyWaitReceivePort, 4 ),

	DEC( NtReplyWaitReceivePortEx, 5 ),
	DEC( NtReplyWaitReplyPort, 2 ),
	NUL( NtRequestDeviceWakeup ),
	IMP( NtRequestPort, 2 ),
	/* 0xb0 */
	IMP( NtRequestWaitReplyPort, 3 ),
	NUL( NtRequestWakeupLatency ),
	IMP( NtResetEvent, 2 ),
	DEC( NtResetWriteWatch, 3 ),

	IMP( NtRestoreKey, 3 ),
	IMP( NtResumeThread, 2 ),
	IMP( NtSaveKey, 2 ),
	IMP( NtSaveMergedKeys, 3 ),

	IMP( NtSecureConnectPort, 9 ),
	IMP( NtSetIoCompletion, 5 ),
	IMP( NtSetContextThread, 2 ),
	IMP( NtSetDefaultHardErrorPort, 1 ),

	DEC( NtSetDefaultLocale, 2 ),
	DEC( NtSetDefaultUILanguage, 1 ),
	DEC( NtSetEaFile, 4 ),
	IMP( NtSetEvent, 2 ),
	/* 0xc0 */
	IMP( NtSetHighEventPair, 1 ),
	IMP( NtSetHighWaitLowEventPair, 1 ),
	IMP( NtSetInformationFile, 5 ),
	IMP( NtSetInformationJobObject, 4 ),

	DEC( NtSetInformationKey, 4 ),
	IMP( NtSetInformationObject, 4 ),
	IMP( NtSetInformationProcess, 4 ),
	IMP( NtSetInformationThread, 4 ),

	DEC( NtSetInformationToken, 4 ),
	DEC( NtSetIntervalProfile, 2 ),
	DEC( NtSetLdtEntries, 4 ),
	IMP( NtSetLowEventPair, 1 ),

	IMP( NtSetLowWaitHighEventPair, 1 ),
	IMP( NtSetQuotaInformationFile, 4 ),
	IMP( NtSetSecurityObject, 3 ),
	DEC( NtSetSystemEnvironmentValue, 2 ),
	/* 0xd0 */
	IMP( NtSetSystemInformation, 3 ),
	IMP( NtSetSystemPowerState, 3 ),
	DEC( NtSetSystemTime, 2 ),
	IMP( NtSetThreadExecutionState, 2 ),

	IMP( NtSetTimer, 7 ),
	DEC( NtSetTimerResolution, 3 ),
	DEC( NtSetUuidSeed, 1 ),
	IMP( NtSetValueKey, 6 ),

	DEC( NtSetVolumeInformationFile, 5 ),
	IMP( NtShutdownSystem, 1 ),
	DEC( NtSignalAndWaitForSingleObject, 4 ),
	DEC( NtStartProfile, 1 ),

	DEC( NtStopProfile, 1 ),
	IMP( NtSuspendThread, 2 ),
	DEC( NtSystemDebugControl, 6 ),
	IMP( NtTerminateJobObject, 2 ),
	/* 0xe0 */
	IMP( NtTerminateProcess, 2 ),
	IMP( NtTerminateThread, 2 ),
	IMP( NtTestAlert, 0 ),
	IMP( NtUnloadDriver, 1 ),

	IMP( NtUnloadKey, 1 ),
	IMP( NtUnlockFile, 5 ),
	DEC( NtUnlockVirtualMemory, 4 ),
	IMP( NtUnmapViewOfSection, 2 ),

	DEC( NtVdmControl, 2 ),
	IMP( NtWaitForMultipleObjects, 5 ),
	IMP( NtWaitForSingleObject, 3 ),
	IMP( NtWaitHighEventPair, 1 ),

	IMP( NtWaitLowEventPair, 1 ),
	IMP( NtWriteFile, 9 ),
	DEC( NtWriteFileGather, 9 ),
	DEC( NtWriteRequestData, 6 ),
	/* 0xf0 */
	IMP( NtWriteVirtualMemory, 5 ),
	NUL( NtCreateChannel ),
	NUL( NtListenChannel ),
	NUL( NtOpenChannel ),

	NUL( NtReplyWaitSendChannel ),
	NUL( NtSendWaitReplyChannel ),
	NUL( NtSendContextChannel ),
	IMP( NtYieldExecution, 0 ),
};

ntcalldesc winxp_calls[] = {
	/* 0x00 */
	IMP( NtAcceptConnectPort, 6 ),
	IMP( NtAccessCheck, 8 ),
	DEC( NtAccessCheckAndAuditAlarm, 11),
	DEC( NtAccessCheckByType, 11 ),

	NUL( NtAccessCheckByTypeAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultList ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarmByHandle ),

	IMP( NtAddAtom, 3 ),
	NUL( NtAddBootEntry ),
	DEC( NtAdjustGroupsToken, 6 ),
	IMP( NtAdjustPrivilegesToken, 6 ),

	IMP( NtAlertResumeThread, 2 ),
	IMP( NtAlertThread, 1 ),
	IMP( NtAllocateLocallyUniqueId, 1 ),
	DEC( NtAllocateUserPhysicalPages, 3 ),

	/* 0x10 */
	DEC( NtAllocateUuids, 3 ),
	IMP( NtAllocateVirtualMemory, 6 ),
	IMP( NtAreMappedFilesTheSame, 2 ),
	IMP( NtAssignProcessToJobObject, 2 ),

	IMP( NtCallbackReturn, 3 ),
	IMP( NtCancelIoFile, 2 ),
	IMP( NtCancelTimer, 2 ),
	IMP( NtCancelDeviceWakeupRequest, 1 ),

	IMP( NtClearEvent, 1 ),
	IMP( NtClose, 1 ),
	NUL( NtCompactKeys ),
	NUL( NtCompareTokens ),

	IMP( NtCloseObjectAuditAlarm, 3 ),
	IMP( NtCompleteConnectPort, 1 ),
	NUL( NtCompressKey ),
	IMP( NtConnectPort, 8 ),

	/* 0x20 */
	IMP( NtContinue, 2 ),
	NUL( NtCreateDebugObject ),
	IMP( NtCreateDirectoryObject, 3 ),
	IMP( NtCreateEvent, 5 ),

	IMP( NtCreateEventPair, 3 ),
	IMP( NtCreateFile, 11 ),
	IMP( NtCreateIoCompletion, 4 ),
	IMP( NtCreateJobObject, 3 ),

	NUL( NtCreateJobSet ),
	IMP( NtCreateKey, 7 ),
	IMP( NtCreateMailslotFile, 8 ),
	IMP( NtCreateMutant, 4 ),

	IMP( NtCreateNamedPipeFile, 14 ),
	IMP( NtCreatePagingFile, 4 ),
	IMP( NtCreatePort, 5 ),
	IMP( NtCreateProcess, 8 ),

	/* 0x30 */
	NUL( NtCreateProcessEx ),
	DEC( NtCreateProfile, 9 ),
	IMP( NtCreateSection, 7 ),
	IMP( NtCreateSemaphore, 5 ),

	IMP( NtCreateSymbolicLinkObject, 4 ),
	IMP( NtCreateThread, 8 ),
	IMP( NtCreateTimer, 4 ),
	DEC( NtCreateToken, 13 ),

	DEC( NtCreateWaitablePort, 5 ),
	NUL( NtDebugActiveProcess ),
	NUL( NtDebugContinue ),
	IMP( NtDelayExecution, 2 ),

	IMP( NtDeleteAtom, 1 ),
	NUL( NtDeleteBootEntry ),
	IMP( NtDeleteFile, 1 ),
	IMP( NtDeleteKey, 1 ),

	/* 0x40 */
	NUL( NtDeleteObjectAuditAlarm ),
	IMP( NtDeleteValueKey, 2 ),
	IMP( NtDeviceIoControlFile, 10 ),
	IMP( NtDisplayString, 1 ),

	IMP( NtDuplicateObject, 7 ),
	IMP( NtDuplicateToken, 6 ),
	NUL( NtEnumerateBootEntries ),
	IMP( NtEnumerateKey, 6 ),

	NUL( NtEnumerateSystemEnvironmentValuesEx ),
	IMP( NtEnumerateValueKey, 6 ),
	DEC( NtExtendSection, 2 ),
	IMP( NtFilterToken, 6 ),

	IMP( NtFindAtom, 3 ),
	IMP( NtFlushBuffersFile, 2 ),
	IMP( NtFlushInstructionCache, 3 ),
	IMP( NtFlushKey, 1 ),

	/* 0x50 */
	IMP( NtFlushVirtualMemory, 4 ),
	DEC( NtFlushWriteBuffer, 1 ),
	DEC( NtFreeUserPhysicalPages, 3 ),
	IMP( NtFreeVirtualMemory, 4 ),

	IMP( NtFsControlFile, 10 ),
	IMP( NtGetContextThread, 2 ),
	IMP( NtGetDevicePowerState, 2 ),
	DEC( NtGetPlugPlayEvent, 4 ),

	DEC( NtGetWriteWatch, 7 ),
	DEC( NtImpersonateAnonymousToken, 1 ),
	DEC( NtImpersonateClientOfPort, 2 ),
	IMP( NtImpersonateThread, 3 ),

	IMP( NtInitializeRegistry, 1 ),
	IMP( NtInitiatePowerAction, 4 ),
	NUL( NtIsProcessInJob ),
	IMP( NtIsSystemResumeAutomatic, 0 ),

	/* 0x60 */
	IMP( NtListenPort, 2 ),
	IMP( NtLoadDriver, 1 ),
	IMP( NtLoadKey, 2 ),
	NUL( NtLoadKey2 ),

	IMP( NtLockFile, 10 ),
	NUL( NtLockProductActivationKeys ),
	NUL( NtLockRegistryKey ),
	IMP( NtLockVirtualMemory, 4 ),

	NUL( NtMakePermanentObject ),
	DEC( NtMakeTemporaryObject, 1 ),
	DEC( NtMapUserPhysicalPages, 3 ),
	NUL( NtMapUserPhysicalPagesScatter ),

	IMP( NtMapViewOfSection, 10 ),
	NUL( NtModifyBootEntry ),
	DEC( NtNotifyChangeDirectoryFile, 9 ),
	IMP( NtNotifyChangeKey, 10 ),

	/* 0x70 */
	IMP( NtNotifyChangeMultipleKeys, 12 ),
	IMP( NtOpenDirectoryObject, 3 ),
	IMP( NtOpenEvent, 3 ),
	IMP( NtOpenEventPair, 3 ),

	IMP( NtOpenFile, 6 ),
	IMP( NtOpenIoCompletion, 3 ),
	IMP( NtOpenJobObject, 3 ),
	IMP( NtOpenKey, 3 ),

	IMP( NtOpenMutant, 3),
	DEC( NtOpenObjectAuditAlarm, 12 ),
	IMP( NtOpenProcess, 4 ),
	IMP( NtOpenProcessToken, 3 ),

	NUL( NtOpenProcessTokenEx ),
	IMP( NtOpenSection, 3 ),
	IMP( NtOpenSemaphore, 3 ),
	IMP( NtOpenSymbolicLinkObject, 3 ),

	/* 0x80 */
	DEC( NtOpenThread, 4 ),
	IMP( NtOpenThreadToken, 4 ),
	NUL( NtOpenThreadTokenEx ),
	IMP( NtOpenTimer, 3 ),

	NUL( NtPlugPlayControl ),
	IMP( NtPowerInformation, 5 ),
	IMP( NtPrivilegeCheck, 3 ),
	IMP( NtPrivilegedServiceAuditAlarm, 5 ),

	IMP( NtPrivilegeObjectAuditAlarm, 6 ),
	IMP( NtProtectVirtualMemory, 5 ),
	IMP( NtPulseEvent, 2 ),
	IMP( NtQueryAttributesFile, 2 ),

	NUL( NtQueryBootOrder ),
	NUL( NtQueryBootOptions ),
	IMP( NtQueryDebugFilterState, 2 ),
	IMP( NtQueryDefaultLocale, 2 ),

	/* 0x90 */
	IMP( NtQueryDefaultUILanguage, 1 ),
	IMP( NtQueryDirectoryFile, 11 ),
	IMP( NtQueryDirectoryObject, 7 ),
	DEC( NtQueryEaFile, 9 ),

	IMP( NtQueryEvent, 5 ),
	IMP( NtQueryFullAttributesFile, 2 ),
	IMP( NtQueryInformationAtom, 5 ),
	IMP( NtQueryInformationFile, 5 ),

	IMP( NtQueryInformationJobObject, 5 ),
	DEC( NtQueryIoCompletion, 5 ),
	DEC( NtQueryInformationPort, 5 ),
	IMP( NtQueryInformationProcess, 5 ),

	IMP( NtQueryInformationThread, 5 ),
	IMP( NtQueryInformationToken, 5 ),
	IMP( NtQueryInstallUILanguage, 1 ),
	DEC( NtQueryIntervalProfile, 2 ),

	/* 0xa0 */
	IMP( NtQueryKey, 5 ),
	IMP( NtQueryMutant, 5 ),
	DEC( NtQueryMultipleValueKey, 6 ),
	IMP( NtQueryObject, 5 ),

	IMP( NtQueryOpenSubKeys, 2 ),
	IMP( NtQueryPerformanceCounter, 2 ),
	DEC( NtQueryQuotaInformationFile, 9 ),
	IMP( NtQuerySection, 5 ),

	IMP( NtQuerySecurityObject, 5 ),
	IMP( NtQuerySemaphore, 5 ),
	IMP( NtQuerySymbolicLinkObject, 3 ),
	DEC( NtQuerySystemEnvironmentValue, 4 ),

	NUL( NtQuerySystemEnvironmentValueEx ),
	IMP( NtQuerySystemInformation, 4 ),
	IMP( NtQuerySystemTime, 1 ),
	IMP( NtQueryTimer, 5 ),

	/* 0xb0 */
	DEC( NtQueryTimerResolution, 3 ),
	IMP( NtQueryValueKey, 6 ),
	IMP( NtQueryVirtualMemory, 6 ),
	IMP( NtQueryVolumeInformationFile, 5 ),

	IMP( NtQueueApcThread, 5 ),
	IMP( NtRaiseException, 3 ),
	IMP( NtRaiseHardError, 6 ),
	IMP( NtReadFile, 9 ),

	DEC( NtReadFileScatter, 9 ),
	DEC( NtReadRequestData, 6 ),
	DEC( NtReadVirtualMemory, 5 ),
	IMP( NtRegisterThreadTerminatePort, 1 ),

	IMP( NtReleaseMutant, 2 ),
	IMP( NtReleaseSemaphore, 3 ),
	IMP( NtRemoveIoCompletion, 5 ),
	NUL( NtRemoveProcessDebug ),

	/* 0xc0 */
	NUL( NtRenameKey ),
	IMP( NtReplaceKey, 3 ),
	IMP( NtReplyPort, 2 ),
	IMP( NtReplyWaitReceivePort, 4 ),

	DEC( NtReplyWaitReceivePortEx, 5 ),
	DEC( NtReplyWaitReplyPort, 2 ),
	NUL( NtRequestDeviceWakeup ),
	IMP( NtRequestPort, 2 ),

	IMP( NtRequestWaitReplyPort, 3 ),
	NUL( NtRequestWakeupLatency ),
	IMP( NtResetEvent, 2 ),
	DEC( NtResetWriteWatch, 3 ),

	IMP( NtRestoreKey, 3 ),
	NUL( NtResumeProcess ),
	IMP( NtResumeThread, 2 ),
	IMP( NtSaveKey, 2 ),

	/* 0xd0 */
	NUL( NtSaveKeyEx ),
	IMP( NtSaveMergedKeys, 3 ),
	IMP( NtSecureConnectPort, 9 ),
	NUL( NtSetBootEntryOrder ),

	NUL( NtSetBootOptions ),
	IMP( NtSetContextThread, 2 ),
	IMP( NtSetDefaultHardErrorPort, 1 ),
	DEC( NtSetDefaultLocale, 2 ),

	DEC( NtSetDefaultUILanguage, 1 ),
	DEC( NtSetEaFile, 4 ),
	IMP( NtSetEvent, 2 ),
	NUL( NtSetBoostPriority ),

	NUL( NtSetBootOptions ),
	IMP( NtSetHighEventPair, 1 ),
	IMP( NtSetHighWaitLowEventPair, 1 ),
	NUL( NtSetInformationDebugObject ),

	/* 0xe0 */
	IMP( NtSetInformationFile, 5 ),
	IMP( NtSetInformationJobObject, 4 ),
	DEC( NtSetInformationKey, 4 ),
	IMP( NtSetInformationObject, 4 ),

	IMP( NtSetInformationProcess, 4 ),
	IMP( NtSetInformationThread, 4 ),
	DEC( NtSetInformationToken, 4 ),
	DEC( NtSetIntervalProfile, 2 ),

	DEC( NtSetIoCompletion, 4 ),
	DEC( NtSetLdtEntries, 4 ),
	IMP( NtSetLowEventPair, 1 ),
	IMP( NtSetLowWaitHighEventPair, 1 ),

	IMP( NtSetQuotaInformationFile, 4 ),
	IMP( NtSetSecurityObject, 3 ),
	DEC( NtSetSystemEnvironmentValue, 2 ),
	NUL( NtSetSystemEnvironmentValueEx ),

	/* 0xf0 */
	IMP( NtSetSystemInformation, 3 ),
	IMP( NtSetSystemPowerState, 3 ),
	DEC( NtSetSystemTime, 2 ),
	IMP( NtSetThreadExecutionState, 2 ),

	IMP( NtSetTimer, 7 ),
	DEC( NtSetTimerResolution, 3 ),
	DEC( NtSetUuidSeed, 1 ),
	IMP( NtSetValueKey, 6 ),

	DEC( NtSetVolumeInformationFile, 5 ),
	IMP( NtShutdownSystem, 1 ),
	DEC( NtSignalAndWaitForSingleObject, 4 ),
	DEC( NtStartProfile, 1 ),

	DEC( NtStopProfile, 1 ),
	NUL( NtSuspendProcess ),
	IMP( NtSuspendThread, 2 ),
	DEC( NtSystemDebugControl, 6 ),

	/* 0x100 */
	IMP( NtTerminateJobObject, 2 ),
	IMP( NtTerminateProcess, 2 ),
	IMP( NtTerminateThread, 2 ),
	IMP( NtTestAlert, 0 ),

	NUL( NtTraceEvent ),
	NUL( NtTranslateFilePath ),
	IMP( NtUnloadDriver, 1 ),
	IMP( NtUnloadKey, 1 ),

	NUL( NtUnloadKeyEx ),
	IMP( NtUnlockFile, 5 ),
	DEC( NtUnlockVirtualMemory, 4 ),
	IMP( NtUnmapViewOfSection, 2 ),

	DEC( NtVdmControl, 2 ),
	NUL( NtWaitForDebugEvent ),
	IMP( NtWaitForMultipleObjects, 5 ),
	IMP( NtWaitForSingleObject, 3 ),

	/* 0x110 */
	IMP( NtWaitHighEventPair, 1 ),
	IMP( NtWaitLowEventPair, 1 ),
	IMP( NtWriteFile, 9 ),
	DEC( NtWriteFileGather, 9 ),

	DEC( NtWriteRequestData, 6 ),
	IMP( NtWriteVirtualMemory, 5 ),
	IMP( NtYieldExecution, 0 ),
	NUL( NtCreateKeyedEvent ),

	IMP( NtOpenKeyedEvent, 3 ),
	NUL( NtReleaseKeyedEvent ),
	NUL( NtWaitForKeyedEvent ),
	NUL( NtQueryPortInformationProcess )
};

// see http://www.fengyuan.com/article/win32ksyscall.html
ntcalldesc ntgdicalls[] = {
/*1000*/ NUL(NtGdiAbortDoc),
/*1001*/ NUL(NtGdiAbortPath),
/*1002*/ IMP(NtGdiAddFontResourceW, 6),
/*1003*/ NUL(NtGdiAddRemoteFontToDC),
/*1004*/ NUL(NtGdiAddFontMemResourceEx),
/*1005*/ NUL(NtGdiRemoveMergeFont),
/*1006*/ NUL(NtGdiAddRemoteMMInstanceToDC),
/*1007*/ NUL(NtGdiAlphaBlend),
/*1008*/ NUL(NtGdiAngleArc),
/*1009*/ NUL(NtGdiAnyLinkedFonts),
/*100a*/ NUL(NtGdiFontIsLinked),
/*100b*/ NUL(NtGdiArcInternal),
/*100c*/ NUL(NtGdiBeginPath),
/*100d*/ IMP(NtGdiBitBlt, 11),
/*100e*/ NUL(NtGdiCancelDC),
/*100f*/ NUL(NtGdiCheckBitmapBits),
/*1010*/ NUL(NtGdiCloseFigure),
/*1011*/ NUL(NtGdiColorCorrectPalette),
/*1012*/ NUL(NtGdiCombineRgn),
/*1013*/ NUL(NtGdiCombineTransform),
/*1014*/ NUL(NtGdiComputeXformCoefficients),
/*1015*/ NUL(NtGdiConsoleTextOut),
/*1016*/ NUL(NtGdiConvertMetafileRect),
/*1017*/ IMP(NtGdiCreateBitmap, 5),
/*1018*/ NUL(NtGdiCreateClientObj),
/*1019*/ NUL(NtGdiCreateColorSpace),
/*101a*/ NUL(NtGdiCreateColorTransform),
/*101b*/ NUL(NtGdiCreateCompatibleBitmap),
/*101c*/ IMP(NtGdiCreateCompatibleDC, 1),
/*101d*/ NUL(NtGdiCreateDIBBrush),
/*101e*/ IMP(NtGdiCreateDIBitmapInternal, 11),
/*101f*/ IMP(NtGdiCreateDIBSection, 9),
/*1020*/ NUL(NtGdiCreateEllipticRgn),
/*1021*/ NUL(NtGdiCreateHalftonePalette),
/*1022*/ NUL(NtGdiCreateHatchBrushInternal),
/*1023*/ NUL(NtGdiCreateMetafileDC),
/*1024*/ NUL(NtGdiCreatePaletteInternal),
/*1025*/ NUL(NtGdiCreatePatternBrushInternal),
/*1026*/ NUL(NtGdiCreatePen),
/*1027*/ NUL(NtGdiCreateRectRgn),
/*1028*/ NUL(NtGdiCreateRoundRectRgn),
/*1029*/ NUL(NtGdiCreateServerMetaFile),
/*102a*/ IMP(NtGdiCreateSolidBrush, 2),
/*102b*/ NUL(NtGdiD3dContextCreate),
/*102c*/ NUL(NtGdiD3dContextDestroy),
/*102d*/ NUL(NtGdiD3dContextDestroyAll),
/*102e*/ NUL(NtGdiD3dValidateTextureStageState),
/*102f*/ NUL(NtGdiD3dDrawPrimitives2),
/*1030*/ NUL(NtGdiDdGetDriverState),
/*1031*/ NUL(NtGdiDdAddAttachedSurface),
/*1032*/ NUL(NtGdiDdAlphaBlt),
/*1033*/ NUL(NtGdiDdAttachSurface),
/*1034*/ NUL(NtGdiDdBeginMoCompFrame),
/*1035*/ NUL(NtGdiDdBlt),
/*1036*/ NUL(NtGdiDdCanCreateSurface),
/*1037*/ NUL(NtGdiDdCanCreateD3DBuffer),
/*1038*/ NUL(NtGdiDdColorControl),
/*1039*/ NUL(NtGdiDdCreateDirectDrawObject),
/*103a*/ NUL(NtGdiDdCreateSurface),
/*103b*/ NUL(NtGdiDdCreateSurface),
/*103c*/ NUL(NtGdiDdCreateMoComp),
/*103d*/ NUL(NtGdiDdCreateSurfaceObject),
/*103e*/ NUL(NtGdiDdDeleteDirectDrawObject),
/*103f*/ NUL(NtGdiDdDeleteSurfaceObject),
/*1040*/ NUL(NtGdiDdDestroyMoComp),
/*1041*/ NUL(NtGdiDdDestroySurface),
/*1042*/ NUL(NtGdiDdDestroyD3DBuffer),
/*1043*/ NUL(NtGdiDdEndMoCompFrame),
/*1044*/ NUL(NtGdiDdFlip),
/*1045*/ NUL(NtGdiDdFlipToGDISurface),
/*1046*/ NUL(NtGdiDdGetAvailDriverMemory),
/*1047*/ NUL(NtGdiDdGetBltStatus),
/*1048*/ NUL(NtGdiDdGetDC),
/*1049*/ NUL(NtGdiDdGetDriverInfo),
/*104a*/ NUL(NtGdiDdGetDxHandle),
/*104b*/ NUL(NtGdiDdGetFlipStatus),
/*104c*/ NUL(NtGdiDdGetInternalMoCompInfo),
/*104d*/ NUL(NtGdiDdGetMoCompBuffInfo),
/*104e*/ NUL(NtGdiDdGetMoCompGuids),
/*104f*/ NUL(NtGdiDdGetMoCompFormats),
/*1050*/ NUL(NtGdiDdGetScanLine),
/*1051*/ NUL(NtGdiDdLock),
/*1052*/ NUL(NtGdiDdLockD3D),
/*1053*/ NUL(NtGdiDdQueryDirectDrawObject),
/*1054*/ NUL(NtGdiDdQueryMoCompStatus),
/*1055*/ NUL(NtGdiDdReenableDirectDrawObject),
/*1056*/ NUL(NtGdiDdReleaseDC),
/*1057*/ NUL(NtGdiDdRenderMoComp),
/*1058*/ NUL(NtGdiDdResetVisrgn),
/*1059*/ NUL(NtGdiDdSetColorKey),
/*105a*/ NUL(NtGdiDdSetExclusiveMode),
/*105b*/ NUL(NtGdiDdSetGammaRamp),
/*105c*/ NUL(NtGdiDdCreateSurfaceEx),
/*105d*/ NUL(NtGdiDdSetOverlayPosition),
/*105e*/ NUL(NtGdiDdUnattachSurface),
/*105f*/ NUL(NtGdiDdUnlock),
/*1060*/ NUL(NtGdiDdUnlockD3D),
/*1061*/ NUL(NtGdiDdUpdateOverlay),
/*1062*/ NUL(NtGdiDdWaitForVerticalBlank),
/*1063*/ NUL(NtGdiDvpCanCreateVideoPort),
/*1064*/ NUL(NtGdiDvpColorControl),
/*1065*/ NUL(NtGdiDvpCreateVideoPort),
/*1066*/ NUL(NtGdiDvpDestroyVideoPort),
/*1067*/ NUL(NtGdiDvpFlipVideoPort),
/*1068*/ NUL(NtGdiDvpGetVideoPortBandwidth),
/*1069*/ NUL(NtGdiDvpGetVideoPortField),
/*106a*/ NUL(NtGdiDvpGetVideoPortFlipStatus),
/*106b*/ NUL(NtGdiDvpGetVideoPortInputFormats),
/*106c*/ NUL(NtGdiDvpGetVideoPortLine),
/*106d*/ NUL(NtGdiDvpGetVideoPortOutputFormats),
/*106e*/ NUL(NtGdiDvpGetVideoPortConnectInfo),
/*106f*/ NUL(NtGdiDvpGetVideoSignalStatus),
/*1070*/ NUL(NtGdiDvpUpdateVideoPort),
/*1071*/ NUL(NtGdiDvpWaitForVideoPortSync),
/*1072*/ NUL(NtGdiDeleteClientObj),
/*1073*/ NUL(NtGdiDeleteColorSpace),
/*1074*/ NUL(NtGdiDeleteColorTransform),
/*1075*/ IMP(NtGdiDeleteObjectApp, 1),
/*1076*/ NUL(NtGdiDescribePixelFormat),
/*1077*/ NUL(NtGdiGetPerBandInfo),
/*1078*/ NUL(NtGdiDoBanding),
/*1079*/ NUL(NtGdiDoPalette),
/*107a*/ NUL(NtGdiDrawEscape),
/*107b*/ NUL(NtGdiEllipse),
/*107c*/ NUL(NtGdiEnableEudc),
/*107d*/ NUL(NtGdiEndDoc),
/*107e*/ NUL(NtGdiEndPage),
/*107f*/ NUL(NtGdiEndPath),
/*1080*/ NUL(NtGdiEnumFontChunk),
/*1081*/ NUL(NtGdiEnumFontClose),
/*1082*/ NUL(NtGdiEnumFontOpen),
/*1083*/ NUL(NtGdiEnumObjects),
/*1084*/ NUL(NtGdiEqualRgn),
/*1085*/ NUL(NtGdiEudcEnumFaceNameLinkW),
/*1086*/ NUL(NtGdiEudcLoadUnloadLink),
/*1087*/ NUL(NtGdiExcludeClipRect),
/*1088*/ NUL(NtGdiExtCreatePen),
/*1089*/ NUL(NtGdiExtCreateRegion),
/*108a*/ NUL(NtGdiExtEscape),
/*108b*/ NUL(NtGdiExtFloodFill),
/*108c*/ IMP(NtGdiExtGetObjectW, 3),
/*108d*/ NUL(NtGdiExtSelectClipRgn),
/*108e*/ NUL(NtGdiExtTextOutW),
/*108f*/ NUL(NtGdiFillPath),
/*1090*/ NUL(NtGdiFillRgn),
/*1091*/ NUL(NtGdiFlattenPath),
/*1092*/ NUL(NtGdiFlushUserBatch),
/*1093*/ IMP(NtGdiFlush, 0), // GreFlush
/*1094*/ NUL(NtGdiForceUFIMapping),
/*1095*/ NUL(NtGdiFrameRgn),
/*1096*/ NUL(NtGdiFullscreenControl),
/*1097*/ NUL(NtGdiGetAndSetDCDword),
/*1098*/ NUL(NtGdiGetAppClipBox),
/*1099*/ NUL(NtGdiGetBitmapBits),
/*109a*/ NUL(NtGdiGetBitmapDimension),
/*109b*/ NUL(NtGdiGetBoundsRect),
/*109c*/ NUL(NtGdiGetCharABCWidthsW),
/*109d*/ NUL(NtGdiGetCharacterPlacementW),
/*109e*/ NUL(NtGdiGetCharSet),
/*109f*/ NUL(NtGdiGetCharWidthW),
/*10a0*/ NUL(NtGdiGetCharWidthInfo),
/*10a1*/ NUL(NtGdiGetColorAdjustment),
/*10a2*/ NUL(NtGdiGetColorSpaceforBitmap),
/*10a3*/ NUL(NtGdiGetDCDword),
/*10a4*/ IMP(NtGdiGetDCforBitmap, 1),
/*10a5*/ IMP(NtGdiGetDCObject, 2),
/*10a6*/ NUL(NtGdiGetDCPoint),
/*10a7*/ NUL(NtGdiGetDeviceCaps),
/*10a8*/ NUL(NtGdiGetDeviceGammaRamp),
/*10a9*/ NUL(NtGdiGetDeviceCapsAll),
/*10aa*/ NUL(NtGdiGetDIBitsInternal),
/*10ab*/ NUL(NtGdiGetETM),
/*10ac*/ NUL(NtGdiGetEudcTimeStampEx),
/*10ad*/ NUL(NtGdiGetFontData),
/*10ae*/ IMP(NtGdiGetFontResourceInfoInternalW, 7),
/*10af*/ NUL(NtGdiGetGlyphIndicesW),
/*10b0*/ NUL(NtGdiGetGlyphIndicesWInternal),
/*10b1*/ NUL(NtGdiGetGlyphOutline),
/*10b2*/ NUL(NtGdiGetKerningPairs),
/*10b3*/ NUL(NtGdiGetLinkedUFIs),
/*10b4*/ NUL(NtGdiGetMiterLimit),
/*10b5*/ NUL(NtGdiGetMonitorID),
/*10b6*/ NUL(NtGdiGetNearestColor),
/*10b7*/ NUL(NtGdiGetNearestPaletteIndex),
/*10b8*/ NUL(NtGdiGetObjectBitmapHandle),
/*10b9*/ NUL(NtGdiGetOutlineTextMetricsInternalW),
/*10ba*/ NUL(NtGdiGetPath),
/*10bb*/ NUL(NtGdiGetPixel),
/*10bc*/ NUL(NtGdiGetRandomRgn),
/*10bd*/ NUL(NtGdiGetRasterizerCaps),
/*10be*/ NUL(NtGdiGetRealizationInfo),
/*10bf*/ NUL(NtGdiGetRegionData),
/*10c0*/ NUL(NtGdiGetRgnBox),
/*10c1*/ NUL(NtGdiGetServerMetaFileBits),
/*10c2*/ NUL(NtGdiGetSpoolMessage),
/*10c3*/ NUL(NtGdiGetStats),
/*10c4*/ IMP(NtGdiGetStockObject, 1),
/*10c5*/ NUL(NtGdiGetStringBitmapW),
/*10c6*/ NUL(NtGdiGetSystemPaletteUse),
/*10c7*/ NUL(NtGdiGetTextCharsetInfo),
/*10c8*/ NUL(NtGdiGetTextExtent),
/*10c9*/ NUL(NtGdiGetTextExtentExW),
/*10ca*/ NUL(NtGdiGetTextFaceW),
/*10cb*/ NUL(NtGdiGetTextMetricsW),
/*10cc*/ NUL(NtGdiGetTransform),
/*10cd*/ NUL(NtGdiGetUFI),
/*10ce*/ NUL(NtGdiGetUFIPathname),
/*10cf*/ NUL(NtGdiGetFontUnicodeRanges),
/*10d0*/ NUL(NtGdiGetWidthTable),
/*10d1*/ NUL(NtGdiGradientFill),
/*10d2*/ NUL(NtGdiHfontCreate),
/*10d3*/ NUL(NtGdiIcmBrushInfo),
/*10d4*/ IMP(NtGdiInit, 0),
/*10d5*/ NUL(NtGdiInitSpool),
/*10d6*/ NUL(NtGdiIntersectClipRect),
/*10d7*/ NUL(NtGdiInvertRgn),
/*10d8*/ NUL(NtGdiLineTo),
/*10d9*/ NUL(NtGdiMakeFontDir),
/*10da*/ NUL(NtGdiMakeInfoDC),
/*10db*/ NUL(NtGdiMaskBlt),
/*10dc*/ NUL(NtGdiModifyWorldTransform),
/*10dd*/ NUL(NtGdiMonoBitmap),
/*10de*/ NUL(NtGdiMoveTo),
/*10df*/ NUL(NtGdiOffsetClipRgn),
/*10e0*/ NUL(NtGdiOffsetRgn),
/*10e1*/ NUL(NtGdiOpenDCW),
/*10e2*/ NUL(NtGdiPatBlt),
/*10e3*/ NUL(NtGdiPolyPatBlt),
/*10e4*/ NUL(NtGdiPathToRegion),
/*10e5*/ NUL(NtGdiPlgBlt),
/*10e6*/ NUL(NtGdiPolyDraw),
/*10e7*/ NUL(NtGdiPolyPolyDraw),
/*10e8*/ NUL(NtGdiPolyTextOutW),
/*10e9*/ NUL(NtGdiPtInRegion),
/*10ea*/ NUL(NtGdiPtVisible),
/*10eb*/ NUL(NtGdiQueryFonts),
/*10ec*/ IMP(NtGdiQueryFontAssocInfo, 1),
/*10ed*/ NUL(NtGdiRectangle),
/*10ee*/ NUL(NtGdiRectInRegion),
/*10ef*/ NUL(NtGdiRectVisible),
/*10f0*/ NUL(NtGdiRemoveFontResourceW),
/*10f1*/ NUL(NtGdiRemoveFontMemResourceEx),
/*10f2*/ NUL(NtGdiResetDC),
/*10f3*/ NUL(NtGdiResizePalette),
/*10f4*/ IMP(NtGdiRestoreDC, 2),
/*10f5*/ NUL(NtGdiRoundRect),
/*10f6*/ IMP(NtGdiSaveDC, 1),
/*10f7*/ NUL(NtGdiScaleViewportExtEx),
/*10f8*/ NUL(NtGdiScaleWindowExtEx),
/*10f9*/ IMP(NtGdiSelectBitmap, 2),
/*10fa*/ NUL(NtGdiSelectBrush),
/*10fb*/ NUL(NtGdiSelectClipPath),
/*10fc*/ NUL(NtGdiSelectFont),
/*10fd*/ NUL(NtGdiSelectPen),
/*10fe*/ NUL(NtGdiSetBitmapBits),
/*10ff*/ NUL(NtGdiSetBitmapDimension),
/*1100*/ NUL(NtGdiSetBoundsRect),
/*1101*/ NUL(NtGdiSetBrushOrg),
/*1102*/ NUL(NtGdiSetColorAdjustment),
/*1103*/ NUL(NtGdiSetColorSpace),
/*1104*/ NUL(NtGdiSetDeviceGammaRamp),
/*1105*/ IMP(NtGdiSetDIBitsToDeviceInternal, 16),
/*1106*/ NUL(NtGdiSetFontEnumeration),
/*1107*/ NUL(NtGdiSetFontXform),
/*1108*/ NUL(NtGdiSetIcmMode),
/*1109*/ NUL(NtGdiSetLinkedUFIs),
/*110a*/ NUL(NtGdiSetMagicColors),
/*110b*/ NUL(NtGdiSetMetaRgn),
/*110c*/ NUL(NtGdiSetMiterLimit),
/*110d*/ NUL(NtGdiGetDeviceWidth),
/*110e*/ NUL(NtGdiMirrorWindowOrg),
/*110f*/ NUL(NtGdiSetLayout),
/*1110*/ NUL(NtGdiSetPixel),
/*1111*/ NUL(NtGdiSetPixelFormat),
/*1112*/ NUL(NtGdiSetRectRgn),
/*1113*/ NUL(NtGdiSetSystemPaletteUse),
/*1114*/ NUL(NtGdiSetTextJustification),
/*1115*/ NUL(NtGdiSetupPublicCFONT),
/*1116*/ NUL(NtGdiSetVirtualResolution),
/*1117*/ NUL(NtGdiSetSizeDevice),
/*1118*/ NUL(NtGdiStartDoc),
/*1119*/ NUL(NtGdiStartPage),
/*111a*/ NUL(NtGdiStretchBlt),
/*111b*/ NUL(NtGdiStretchDIBitsInternal),
/*111c*/ NUL(NtGdiStrokeAndFillPath),
/*111d*/ NUL(NtGdiStrokePath),
/*111e*/ NUL(NtGdiSwapBuffers),
/*111f*/ NUL(NtGdiTransformPoints),
/*1120*/ NUL(NtGdiTransparentBlt),
/*1121*/ NUL(NtGdiUnloadPrinterDriver),
/*1122*/ NUL(NtGdiUnmapMemFont),
/*1123*/ NUL(NtGdiUnrealizeObject),
/*1124*/ NUL(NtGdiUpdateColors),
/*1125*/ NUL(NtGdiWidenPath),
/*1126*/ NUL(NtUserActivateKeyboardLayout),
/*1127*/ NUL(NtUserAlterWindowStyle),
/*1128*/ NUL(NtUserAssociateInputContext),
/*1129*/ NUL(NtUserAttachThreadInput),
/*112a*/ NUL(NtUserBeginPaint),
/*112b*/ NUL(NtUserBitBltSysBmp),
/*112c*/ NUL(NtUserBlockInput),
/*112d*/ NUL(NtUserBuildHimcList),
/*112e*/ NUL(NtUserBuildHwndList),
/*112f*/ NUL(NtUserBuildNameList),
/*1130*/ NUL(NtUserBuildPropList),
/*1131*/ NUL(NtUserCallHwnd),
/*1132*/ NUL(NtUserCallHwndLock),
/*1133*/ NUL(NtUserCallHwndOpt),
/*1134*/ NUL(NtUserCallHwndParam),
/*1135*/ NUL(NtUserCallHwndParamLock),
/*1136*/ NUL(NtUserCallMsgFilter),
/*1137*/ NUL(NtUserCallNextHookEx),
/*1138*/ IMP(NtUserCallNoParam, 1),
/*1139*/ IMP(NtUserCallOneParam, 2),
/*113a*/ IMP(NtUserCallTwoParam, 3),
/*113b*/ NUL(NtUserChangeClipboardChain),
/*113c*/ NUL(NtUserChangeDisplaySettings),
/*113d*/ NUL(NtUserCheckImeHotKey),
/*113e*/ NUL(NtUserCheckMenuItem),
/*113f*/ NUL(NtUserChildWindowFromPointEx),
/*1140*/ NUL(NtUserClipCursor),
/*1141*/ NUL(NtUserCloseClipboard),
/*1142*/ NUL(NtUserCloseDesktop),
/*1143*/ NUL(NtUserCloseWindowStation),
/*1144*/ NUL(NtUserConsoleControl),
/*1145*/ NUL(NtUserConvertMemHandle),
/*1146*/ NUL(NtUserCopyAcceleratorTable),
/*1147*/ NUL(NtUserCountClipboardFormats),
/*1148*/ NUL(NtUserCreateAcceleratorTable),
/*1149*/ NUL(NtUserCreateCaret),
/*114a*/ IMP(NtUserCreateDesktop, 5),
/*114b*/ NUL(NtUserCreateInputContext),
/*114c*/ NUL(NtUserCreateLocalMemHandle),
/*114d*/ IMP(NtUserCreateWindowEx, 13),
/*114e*/ IMP(NtUserCreateWindowStation, 6),
/*114f*/ NUL(NtUserDdeGetQualityOfService),
/*1150*/ NUL(NtUserDdeInitialize),
/*1151*/ NUL(NtUserDdeSetQualityOfService),
/*1152*/ NUL(NtUserDeferWindowPos),
/*1153*/ NUL(NtUserDefSetText),
/*1154*/ NUL(NtUserDeleteMenu),
/*1155*/ NUL(NtUserDestroyAcceleratorTable),
/*1156*/ NUL(NtUserDestroyCursor),
/*1157*/ NUL(NtUserDestroyInputContext),
/*1158*/ NUL(NtUserDestroyMenu),
/*1159*/ NUL(NtUserDestroyWindow),
/*115a*/ NUL(NtUserDisableThreadIme),
/*115b*/ NUL(NtUserDispatchMessage),
/*115c*/ NUL(NtUserDragDetect),
/*115d*/ NUL(NtUserDragObject),
/*115e*/ NUL(NtUserDrawAnimatedRects),
/*115f*/ NUL(NtUserDrawCaption),
/*1160*/ NUL(NtUserDrawCaptionTemp),
/*1161*/ NUL(NtUserDrawIconEx),
/*1162*/ NUL(NtUserDrawMenuBarTemp),
/*1163*/ NUL(NtUserEmptyClipboard),
/*1164*/ NUL(NtUserEnableMenuItem),
/*1165*/ NUL(NtUserEnableScrollBar),
/*1166*/ NUL(NtUserEndDeferWindowPosEx),
/*1167*/ NUL(NtUserEndMenu),
/*1168*/ NUL(NtUserEndPaint),
/*1169*/ NUL(NtUserEnumDisplayDevices),
/*116a*/ NUL(NtUserEnumDisplayMonitors),
/*116b*/ NUL(NtUserEnumDisplaySettings),
/*116c*/ NUL(NtUserEvent),
/*116d*/ NUL(NtUserExcludeUpdateRgn),
/*116e*/ NUL(NtUserFillWindow),
/*116f*/ IMP(NtUserFindExistingCursorIcon, 3),
/*1170*/ NUL(NtUserFindWindowEx),
/*1171*/ NUL(NtUserFlashWindowEx),
/*1172*/ NUL(NtUserGetAltTabInfo),
/*1173*/ NUL(NtUserGetAncestor),
/*1174*/ NUL(NtUserGetAppImeLevel),
/*1175*/ NUL(NtUserGetAsyncKeyState),
/*1176*/ IMP(NtUserGetCaretBlinkTime, 0),
/*1177*/ NUL(NtUserGetCaretPos),
/*1178*/ IMP(NtUserGetClassInfo, 5),
/*1179*/ NUL(NtUserGetClassName),
/*117a*/ NUL(NtUserGetClipboardData),
/*117b*/ NUL(NtUserGetClipboardFormatName),
/*117c*/ NUL(NtUserGetClipboardOwner),
/*117d*/ NUL(NtUserGetClipboardSequenceNumber),
/*117e*/ NUL(NtUserGetClipboardViewer),
/*117f*/ NUL(NtUserGetClipCursor),
/*1180*/ NUL(NtUserGetComboBoxInfo),
/*1181*/ NUL(NtUserGetControlBrush),
/*1182*/ NUL(NtUserGetControlColor),
/*1183*/ NUL(NtUserGetCPD),
/*1184*/ NUL(NtUserGetCursorFrameInfo),
/*1185*/ NUL(NtUserGetCursorInfo),
/*1186*/ IMP(NtUserGetDC, 1),
/*1187*/ NUL(NtUserGetDCEx),
/*1188*/ NUL(NtUserGetDoubleClickTime),
/*1189*/ NUL(NtUserGetForegroundWindow),
/*118a*/ NUL(NtUserGetGuiResources),
/*118b*/ NUL(NtUserGetGUIThreadInfo),
/*118c*/ NUL(NtUserGetIconInfo),
/*118d*/ NUL(NtUserGetIconSize),
/*118e*/ NUL(NtUserGetImeHotKey),
/*118f*/ NUL(NtUserGetImeInfoEx),
/*1190*/ NUL(NtUserGetInternalWindowPos),
/*1191*/ IMP(NtUserGetKeyboardLayoutList, 2),
/*1192*/ NUL(NtUserGetKeyboardLayoutName),
/*1193*/ NUL(NtUserGetKeyboardState),
/*1194*/ NUL(NtUserGetKeyNameText),
/*1195*/ NUL(NtUserGetKeyState),
/*1196*/ NUL(NtUserGetListBoxInfo),
/*1197*/ NUL(NtUserGetMenuBarInfo),
/*1198*/ NUL(NtUserGetMenuIndex),
/*1199*/ NUL(NtUserGetMenuItemRect),
/*119a*/ IMP(NtUserGetMessage, 4),
/*119b*/ NUL(NtUserGetMouseMovePointsEx),
/*119c*/ NUL(NtUserGetObjectInformation),
/*119d*/ NUL(NtUserGetOpenClipboardWindow),
/*119e*/ NUL(NtUserGetPriorityClipboardFormat),
/*119f*/ IMP(NtUserGetProcessWindowStation, 0),
/*11a0*/ NUL(NtUserGetScrollBarInfo),
/*11a1*/ NUL(NtUserGetSystemMenu),
/*11a2*/ IMP(NtUserGetThreadDesktop, 2),
/*11a3*/ IMP(NtUserGetThreadState, 1),
/*11a4*/ NUL(NtUserGetTitleBarInfo),
/*11a5*/ NUL(NtUserGetUpdateRect),
/*11a6*/ NUL(NtUserGetUpdateRgn),
/*11a7*/ NUL(NtUserGetWindowDC),
/*11a8*/ NUL(NtUserGetWindowPlacement),
/*11a9*/ NUL(NtUserGetWOWClass),
/*11aa*/ NUL(NtUserHardErrorControl),
/*11ab*/ NUL(NtUserHideCaret),
/*11ac*/ NUL(NtUserHiliteMenuItem),
/*11ad*/ NUL(NtUserImpersonateDdeClientWindow),
/*11ae*/ IMP(NtUserInitialize, 3),
/*11af*/ IMP(NtUserInitializeClientPfnArrays, 4),
/*11b0*/ NUL(NtUserInitTask),
/*11b1*/ NUL(NtUserInternalGetWindowText),
/*11b2*/ NUL(NtUserInvalidateRect),
/*11b3*/ NUL(NtUserInvalidateRgn),
/*11b4*/ NUL(NtUserIsClipboardFormatAvailable),
/*11b5*/ NUL(NtUserKillTimer),
/*11b6*/ IMP(NtUserLoadKeyboardLayoutEx, 6),
/*11b7*/ NUL(NtUserLockWindowStation),
/*11b8*/ NUL(NtUserLockWindowUpdate),
/*11b9*/ NUL(NtUserLockWorkStation),
/*11ba*/ NUL(NtUserMapVirtualKeyEx),
/*11bb*/ NUL(NtUserMenuItemFromPoint),
/*11bc*/ NUL(NtUserMessageCall),
/*11bd*/ NUL(NtUserMinMaximize),
/*11be*/ NUL(NtUserMNDragLeave),
/*11bf*/ NUL(NtUserMNDragOver),
/*11c0*/ NUL(NtUserModifyUserStartupInfoFlags),
/*11c1*/ NUL(NtUserMoveWindow),
/*11c2*/ NUL(NtUserNotifyIMEStatus),
/*11c3*/ IMP(NtUserNotifyProcessCreate, 4),
/*11c4*/ NUL(NtUserNotifyWinEvent),
/*11c5*/ NUL(NtUserOpenClipboard),
/*11c6*/ NUL(NtUserOpenDesktop),
/*11c7*/ NUL(NtUserOpenInputDesktop),
/*11c8*/ NUL(NtUserOpenWindowStation),
/*11c9*/ NUL(NtUserPaintDesktop),
/*11ca*/ NUL(NtUserPeekMessage),
/*11cb*/ NUL(NtUserPostMessage),
/*11cc*/ NUL(NtUserPostThreadMessage),
/*11cd*/ IMP(NtUserProcessConnect, 3),
/*11ce*/ NUL(NtUserQueryInformationThread),
/*11cf*/ NUL(NtUserQueryInputContext),
/*11d0*/ NUL(NtUserQuerySendMessage),
/*11d1*/ NUL(NtUserQueryUserCounters),
/*11d2*/ NUL(NtUserQueryWindow),
/*11d3*/ NUL(NtUserRealChildWindowFromPoint),
/*11d4*/ NUL(NtUserRedrawWindow),
/*11d5*/ IMP(NtUserRegisterClassExWOW, 6),
/*11d6*/ NUL(NtUserRegisterHotKey),
/*11d7*/ NUL(NtUserRegisterTasklist),
/*11d8*/ IMP(NtUserRegisterWindowMessage, 1),
/*11d9*/ NUL(NtUserRemoveMenu),
/*11da*/ NUL(NtUserRemoveProp),
/*11db*/ NUL(NtUserResolveDesktop),
/*11dc*/ NUL(NtUserResolveDesktopForWOW),
/*11dd*/ NUL(NtUserSBGetParms),
/*11de*/ NUL(NtUserScrollDC),
/*11df*/ NUL(NtUserScrollWindowEx),
/*11e0*/ IMP(NtUserSelectPalette, 3),
/*11e1*/ NUL(NtUserSendInput),
/*11e2*/ NUL(NtUserSendMessageCallback),
/*11e3*/ NUL(NtUserSendNotifyMessage),
/*11e4*/ NUL(NtUserSetActiveWindow),
/*11e5*/ NUL(NtUserSetAppImeLevel),
/*11e6*/ NUL(NtUserSetCapture),
/*11e7*/ NUL(NtUserSetClassLong),
/*11e8*/ NUL(NtUserSetClassWord),
/*11e9*/ NUL(NtUserSetClipboardData),
/*11ea*/ NUL(NtUserSetClipboardViewer),
/*11eb*/ NUL(NtUserSetConsoleReserveKeys),
/*11ec*/ NUL(NtUserSetCursor),
/*11ed*/ NUL(NtUserSetCursorContents),
/*11ee*/ IMP(NtUserSetCursorIconData, 4),
/*11ef*/ NUL(NtUserSetDbgTag),
/*11f0*/ NUL(NtUserSetFocus),
/*11f1*/ IMP(NtUserSetImeHotKey, 5),
/*11f2*/ NUL(NtUserSetImeInfoEx),
/*11f3*/ NUL(NtUserSetImeOwnerWindow),
/*11f4*/ NUL(NtUserSetInformationProcess),
/*11f5*/ IMP(NtUserSetInformationThread, 4),
/*11f6*/ NUL(NtUserSetInternalWindowPos),
/*11f7*/ NUL(NtUserSetKeyboardState),
/*11f8*/ IMP(NtUserSetLogonNotifyWindow, 1),
/*11f9*/ NUL(NtUserSetMenu),
/*11fa*/ NUL(NtUserSetMenuContextHelpId),
/*11fb*/ NUL(NtUserSetMenuDefaultItem),
/*11fc*/ NUL(NtUserSetMenuFlagRtoL),
/*11fd*/ NUL(NtUserSetObjectInformation),
/*11fe*/ NUL(NtUserSetParent),
/*11ff*/ IMP(NtUserSetProcessWindowStation, 1),
/*1200*/ NUL(NtUserSetProp),
/*1201*/ NUL(NtUserSetRipFlags),
/*1202*/ NUL(NtUserSetScrollInfo),
/*1203*/ NUL(NtUserSetShellWindowEx),
/*1204*/ NUL(NtUserSetSysColors),
/*1205*/ NUL(NtUserSetSystemCursor),
/*1206*/ NUL(NtUserSetSystemMenu),
/*1207*/ NUL(NtUserSetSystemTimer),
/*1208*/ IMP(NtUserSetThreadDesktop, 1),
/*1209*/ NUL(NtUserSetThreadLayoutHandles),
/*120a*/ NUL(NtUserSetThreadState),
/*120b*/ NUL(NtUserSetTimer),
/*120c*/ NUL(NtUserSetWindowFNID),
/*120d*/ NUL(NtUserSetWindowLong),
/*120e*/ NUL(NtUserSetWindowPlacement),
/*120f*/ NUL(NtUserSetWindowPos),
/*1210*/ NUL(NtUserSetWindowRgn),
/*1211*/ NUL(NtUserSetWindowsHookAW),
/*1212*/ NUL(NtUserSetWindowsHookEx),
/*1213*/ IMP(NtUserSetWindowStationUser, 4),
/*1214*/ NUL(NtUserSetWindowWord),
/*1215*/ NUL(NtUserSetWinEventHook),
/*1216*/ NUL(NtUserShowCaret),
/*1217*/ NUL(NtUserShowScrollBar),
/*1218*/ NUL(NtUserShowWindow),
/*1219*/ NUL(NtUserShowWindowAsync),
/*121a*/ NUL(NtUserSoundSentry),
/*121b*/ NUL(NtUserSwitchDesktop),
/*121c*/ IMP(NtUserSystemParametersInfo, 4),
/*121d*/ NUL(NtUserTestForInteractiveUser),
/*121e*/ NUL(NtUserThunkedMenuInfo),
/*121f*/ NUL(NtUserThunkedMenuItemInfo),
/*1220*/ NUL(NtUserToUnicodeEx),
/*1221*/ NUL(NtUserTrackMouseEvent),
/*1222*/ NUL(NtUserTrackPopupMenuEx),
/*1223*/ NUL(NtUserTranslateAccelerator),
/*1224*/ NUL(NtUserTranslateMessage),
/*1225*/ NUL(NtUserUnhookWindowsHookEx),
/*1226*/ NUL(NtUserUnhookWinEvent),
/*1227*/ NUL(NtUserUnloadKeyboardLayout),
/*1228*/ NUL(NtUserUnlockWindowStation),
/*1229*/ NUL(NtUserUnregisterClass),
/*122a*/ NUL(NtUserUnregisterHotKey),
/*122b*/ NUL(NtUserUpdateInputContext),
/*122c*/ NUL(NtUserUpdateInstance),
/*122d*/ NUL(NtUserUpdateLayeredWindow),
/*122e*/ NUL(NtUserSetLayeredWindowAttributes),
/*122f*/ IMP(NtUserUpdatePerUserSystemParameters, 2),
/*1230*/ NUL(NtUserUserHandleGrantAccess),
/*1231*/ NUL(NtUserValidateHandleSecure),
/*1232*/ NUL(NtUserValidateRect),
/*1233*/ NUL(NtUserVkKeyScanEx),
/*1234*/ NUL(NtUserWaitForInputIdle),
/*1235*/ NUL(NtUserWaitForMsgAndEvent),
/*1236*/ NUL(NtUserWaitMessage),
/*1237*/ NUL(NtUserWin32PoolAllocationStats),
/*1238*/ NUL(NtUserWindowFromPoint),
/*1239*/ NUL(NtUserYieldTask),
/*123a*/ NUL(NtUserRemoteConnect),
/*123b*/ NUL(NtUserRemoteRedrawRectangle),
/*123c*/ NUL(NtUserRemoteRedrawScreen),
/*123d*/ NUL(NtUserRemoteStopScreenUpdates),
/*123e*/ NUL(NtUserCtxDisplayIOCtl),
/*123f*/ NUL(NtGdiEngAssociateSurface),
/*1240*/ NUL(NtGdiEngCreateBitmap),
/*1241*/ NUL(NtGdiEngCreateDeviceSurface),
/*1242*/ NUL(NtGdiEngCreateDeviceBitmap),
/*1243*/ NUL(NtGdiEngCreatePalette),
/*1244*/ NUL(NtGdiEngComputeGlyphSet),
/*1245*/ NUL(NtGdiEngCopyBits),
/*1246*/ NUL(NtGdiEngDeletePalette),
/*1247*/ NUL(NtGdiEngDeleteSurface),
/*1248*/ NUL(NtGdiEngEraseSurface),
/*1249*/ NUL(NtGdiEngUnlockSurface),
/*124a*/ NUL(NtGdiEngLockSurface),
/*124b*/ NUL(NtGdiEngBitBlt),
/*124c*/ NUL(NtGdiEngStretchBlt),
/*124d*/ NUL(NtGdiEngPlgBlt),
/*124e*/ NUL(NtGdiEngMarkBandingSurface),
/*124f*/ NUL(NtGdiEngStrokePath),
/*1250*/ NUL(NtGdiEngFillPath),
/*1251*/ NUL(NtGdiEngStrokeAndFillPath),
/*1252*/ NUL(NtGdiEngPaint),
/*1253*/ NUL(NtGdiEngLineTo),
/*1254*/ NUL(NtGdiEngAlphaBlend),
/*1255*/ NUL(NtGdiEngGradientFill),
/*1256*/ NUL(NtGdiEngTransparentBlt),
/*1257*/ NUL(NtGdiEngTextOut),
/*1258*/ NUL(NtGdiEngStretchBltROP),
/*1259*/ NUL(NtGdiXLATEOBJ_cGetPalette),
/*125a*/ NUL(NtGdiXLATEOBJ_iXlate),
/*125b*/ NUL(NtGdiXLATEOBJ_hGetColorTransform),
/*125c*/ NUL(NtGdiCLIPOBJ_bEnum),
/*125d*/ NUL(NtGdiCLIPOBJ_cEnumStart),
/*125e*/ NUL(NtGdiCLIPOBJ_ppoGetPath),
/*125f*/ NUL(NtGdiEngDeletePath),
/*1260*/ NUL(NtGdiEngCreateClip),
/*1261*/ NUL(NtGdiEngDeleteClip),
/*1262*/ NUL(NtGdiBRUSHOBJ_ulGetBrushColor),
/*1263*/ NUL(NtGdiBRUSHOBJ_pvAllocRbrush),
/*1264*/ NUL(NtGdiBRUSHOBJ_pvGetRbrush),
/*1265*/ NUL(NtGdiBRUSHOBJ_hGetColorTransform),
/*1266*/ NUL(NtGdiXFORMOBJ_bApplyXform),
/*1267*/ NUL(NtGdiXFORMOBJ_iGetXform),
/*1268*/ NUL(NtGdiFONTOBJ_vGetInfo),
/*1269*/ NUL(NtGdiFONTOBJ_pxoGetXform),
/*126a*/ NUL(NtGdiFONTOBJ_cGetGlyphs),
/*126b*/ NUL(NtGdiFONTOBJ_pifi),
/*126c*/ NUL(NtGdiFONTOBJ_pfdg),
/*126d*/ NUL(NtGdiFONTOBJ_pQueryGlyphAttrs),
/*126e*/ NUL(NtGdiFONTOBJ_pvTrueTypeFontFile),
/*126f*/ NUL(NtGdiFONTOBJ_cGetAllGlyphHandles),
/*1270*/ NUL(NtGdiSTROBJ_bEnum),
/*1271*/ NUL(NtGdiSTROBJ_bEnumPositionsOnly),
/*1272*/ NUL(NtGdiSTROBJ_bGetAdvanceWidths),
/*1273*/ NUL(NtGdiSTROBJ_vEnumStart),
/*1274*/ NUL(NtGdiSTROBJ_dwGetCodePage),
/*1275*/ NUL(NtGdiPATHOBJ_vGetBounds),
/*1276*/ NUL(NtGdiPATHOBJ_bEnum),
/*1277*/ NUL(NtGdiPATHOBJ_vEnumStart),
/*1278*/ NUL(NtGdiPATHOBJ_vEnumStartClipLines),
/*1279*/ NUL(NtGdiPATHOBJ_bEnumClipLines),
/*127a*/ NUL(NtGdiGetDhpdev),
/*127b*/ NUL(NtGdiEngCheckAbort),
/*127c*/ NUL(NtGdiHT_Get8BPPFormatPalette),
/*127d*/ NUL(NtGdiHT_Get8BPPMaskPalette),
/*127e*/ NUL(NtGdiUpdateTransform),
};

int option_trace;

ntcalldesc *ntcalls;
ULONG number_of_ntcalls = sizeof ntcalls/sizeof ntcalls[0];

void init_syscalls(bool xp)
{
	if (xp)
	{
		number_of_ntcalls = sizeof winxp_calls/sizeof winxp_calls[0];
		ntcalls = winxp_calls;
	}
	else
	{
		number_of_ntcalls = sizeof win2k_calls/sizeof win2k_calls[0];
		ntcalls = win2k_calls;
	}
}

void trace_syscall_enter(ULONG id, ntcalldesc *ntcall, ULONG *args, ULONG retaddr)
{
	/* print a relay style trace line */
	if (!option_trace)
		return;

	fprintf(stderr,"%04lx: %s(", id, ntcall->name);
	if (ntcall->numargs)
	{
		unsigned int i;
		fprintf(stderr,"%08lx", args[0]);
		for (i=1; i<ntcall->numargs; i++)
			fprintf(stderr,",%08lx", args[i]);
	}
	fprintf(stderr,") ret=%08lx\n", retaddr);
}

void trace_syscall_exit(ULONG id, ntcalldesc *ntcall, ULONG r, ULONG retaddr)
{
	if (!option_trace)
		return;

	fprintf(stderr, "%04lx: %s retval=%08lx ret=%08lx\n",
			id, ntcall->name, r, retaddr);
}

NTSTATUS do_nt_syscall(ULONG id, ULONG func, ULONG *uargs, ULONG retaddr)
{
	NTSTATUS r = STATUS_INVALID_SYSTEM_SERVICE;
	ntcalldesc *ntcall = 0;
	ULONG args[16];
	int magic = 0x1248;
	BOOLEAN win32k_func = FALSE;

	/* check the call number is in range */
	if (func >= 0 && func < number_of_ntcalls)
		ntcall = &ntcalls[func];
	else if (func >= 0x1000 && func <= 0x127e)
	{
		win32k_func = TRUE;
		ntcall = &ntgdicalls[func - 0x1000];
	}
	else
	{
		dprintf("invalid syscall %ld ret=%08lx\n", func, retaddr);
		return r;
	}

	BYTE inst[4];
	r = copy_from_user( inst, (const void*)retaddr, sizeof inst );
	if (r == STATUS_SUCCESS && inst[0] == 0xc2)
	{
		// detect the number of args
		if (!ntcall->func && !ntcall->numargs && inst[2] == 0)
		{
			ntcall->numargs = inst[1]/4;
			fprintf(stderr, "%s: %d args\n", ntcall->name, inst[1]/4);
		}

		// many syscalls are a short asm function
		// find the caller of that function
		if (option_trace)
		{
			ULONG r2 = 0;
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_CONTROL;
			current->get_context( ctx );
			r = copy_from_user( &r2, (const void*) ctx.Esp, sizeof r2);
			if (r == STATUS_SUCCESS)
				retaddr = r2;
		}
	}

	if (sizeof args/sizeof args[0] < ntcall->numargs)
		die("not enough room for %d args\n", ntcall->numargs);

	/* call it */
	r = copy_from_user( args, uargs, ntcall->numargs*sizeof (args[0]) );
	if (r != STATUS_SUCCESS)
		goto end;

	trace_syscall_enter(id, ntcall, args, retaddr );

	// initialize the windows subsystem if necessary
	if (win32k_func)
		win32k_thread_init(current);

	/* debug info for this call */
	if (!ntcall->func)
	{
		fprintf(stderr, "syscall %s (%02lx) not implemented\n", ntcall->name, func);
		r = STATUS_NOT_IMPLEMENTED;
		goto end;
	}

	__asm__(
		"pushl %%edx\n\t"
		"subl %%ecx, %%esp\n\t"
		"movl %%esp, %%edi\n\t"
		"cld\n\t"
		"repe\n\t"
		"movsb\n\t"
		"call *%%eax\n\t"
		"popl %%edx\n\t"
		: "=a"(r), "=d"(magic)   // output
		: "a"(ntcall->func), "c"(ntcall->numargs*4), "d"(magic), "S"(args) // input
		: "%edi"	// clobber
	);

	assert( magic == 0x1248 );

end:
	trace_syscall_exit(id, ntcall, r, retaddr);

	return r;
}
