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
	IMP( NtQueryInformationPort, 5 ),
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
	IMP( NtQueryInformationPort, 5 ),
	IMP( NtQueryInformationProcess, 5 ),
	IMP( NtQueryInformationThread, 5 ),

	IMP( NtQueryInformationToken, 5 ),
	IMP( NtQueryInstallUILanguage, 1 ),
	DEC( NtQueryIntervalProfile, 2 ),
	DEC( NtQueryIoCompletion, 5 ),

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
	NUL( NtSetDebugFilterState ),
	IMP( NtSetDefaultHardErrorPort, 1 ),

	DEC( NtSetDefaultLocale, 2 ),
	DEC( NtSetDefaultUILanguage, 1 ),
	DEC( NtSetEaFile, 4 ),
	IMP( NtSetEvent, 2 ),

	NUL( NtSetEventBoostPriority ),
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

	IMP( NtSetIoCompletion, 5 ),
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
ntcalldesc win2k_uicalls[] = {
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

ntcalldesc winxp_uicalls[] = {
/*1000*/ NUL(NtGdiAbortDoc),
/*1001*/ NUL(NtGdiAbortPath), // 1
/*1002*/ IMP(NtGdiAddFontResourceW, 6),
/*1003*/ NUL(NtGdiAddRemoteFontToDC), // 4
/*1004*/ NUL(NtGdiAddFontMemResourceEx), // 5
/*1005*/ NUL(NtGdiRemoveMergeFont), // 2
/*1006*/ NUL(NtGdiAddRemoteMMInstanceToDC), // 3
/*1007*/ NUL(NtGdiAlphaBlend), // 12
/*1008*/ NUL(NtGdiAngleArc), // 6
/*1009*/ NUL(NtGdiAnyLinkedFonts), // 0
/*100a*/ NUL(NtGdiFontIsLinked), // 1
/*100b*/ NUL(NtGdiArcInternal), // 10
/*100c*/ NUL(NtGdiBeginPath), // 1
/*100d*/ IMP(NtGdiBitBlt, 11),
/*100e*/ NUL(NtGdiCancelDC), // 1
/*100f*/ NUL(NtGdiCheckBitmapBits), // 8
/*1010*/ NUL(NtGdiCloseFigure), // 1
/*1011*/ NUL(NtGdiClearBitmapAttributes), // 2
/*1012*/ NUL(NtGdiClearBrushAttributes), // 2
/*1013*/ NUL(NtGdiColorCorrectPalette), // 6
/*1014*/ NUL(NtGdiCombineRgn), // 4
/*1015*/ NUL(NtGdiCombineTransform), // 3
/*1016*/ NUL(NtGdiComputeXformCoefficients), // 1
/*1017*/ NUL(NtGdiConsoleTextOut), // 4
/*1018*/ NUL(NtGdiConvertMetafileRect), // 2
/*1019*/ IMP(NtGdiCreateBitmap, 5),
/*101a*/ NUL(NtGdiCreateClientObj), // 1
/*101b*/ NUL(NtGdiCreateColorSpace), // 1
/*101c*/ NUL(NtGdiCreateColorTransform), // 8
/*101d*/ NUL(NtGdiCreateCompatibleBitmap), // 3
/*101c*/ IMP(NtGdiCreateCompatibleDC, 1),
/*101f*/ NUL(NtGdiCreateDIBBrush), // 6
/*1020*/ IMP(NtGdiCreateDIBitmapInternal, 11),
/*1021*/ IMP(NtGdiCreateDIBSection, 9),
/*1022*/ NUL(NtGdiCreateEllipticRgn), // 4
/*1023*/ NUL(NtGdiCreateHalftonePalette), // 1
/*1024*/ NUL(NtGdiCreateHatchBrushInternal), // 3
/*1025*/ NUL(NtGdiCreateMetafileDC), // 1
/*1026*/ NUL(NtGdiCreatePaletteInternal), // 2
/*1027*/ NUL(NtGdiCreatePatternBrushInternal), // 3
/*1028*/ NUL(NtGdiCreatePen), // 4
/*1029*/ NUL(NtGdiCreateRectRgn), // 4
/*102a*/ NUL(NtGdiCreateRoundRectRgn), // 6
/*102b*/ NUL(NtGdiCreateServerMetaFile), // 6
/*102c*/ IMP(NtGdiCreateSolidBrush, 2),
/*102d*/ NUL(NtGdiD3dContextCreate), // 4
/*102e*/ NUL(NtGdiD3dContextDestroy), // 1
/*102f*/ NUL(NtGdiD3dContextDestroyAll), // 1
/*1030*/ NUL(NtGdiD3dValidateTextureStageState), // 1
/*1031*/ NUL(NtGdiD3dDrawPrimitives2), // 7
/*1032*/ NUL(NtGdiDdGetDriverState), // 1
/*1033*/ NUL(NtGdiDdAddAttachedSurface), // 3
/*1034*/ NUL(NtGdiDdAlphaBlt), // 3
/*1035*/ NUL(NtGdiDdAttachSurface), // 2
/*1036*/ NUL(NtGdiDdBeginMoCompFrame), // 2
/*1037*/ NUL(NtGdiDdBlt), // 3
/*1038*/ NUL(NtGdiDdCanCreateSurface), // 2
/*1039*/ NUL(NtGdiDdCanCreateD3DBuffer), // 2
/*103a*/ NUL(NtGdiDdColorControl), // 2
/*103b*/ NUL(NtGdiDdCreateDirectDrawObject), // 1
/*103c*/ NUL(NtGdiDdCreateSurface), // 8
/*103d*/ NUL(NtGdiDdCreateD3DBuffer), // 8
/*103e*/ NUL(NtGdiDdCreateMoComp), // 2
/*103f*/ NUL(NtGdiDdCreateSurfaceObject), // 6
/*1040*/ NUL(NtGdiDdDeleteDirectDrawObject), // 1
/*1041*/ NUL(NtGdiDdDeleteSurfaceObject), // 1
/*1042*/ NUL(NtGdiDdDestroyMoComp), // 2
/*1043*/ NUL(NtGdiDdDestroySurface), // 2
/*1044*/ NUL(NtGdiDdDestroyD3DBuffer), // 1
/*1045*/ NUL(NtGdiDdEndMoCompFrame), // 2
/*1046*/ NUL(NtGdiDdFlip), // 5
/*1047*/ NUL(NtGdiDdFlipToGDISurface), // 2
/*1048*/ NUL(NtGdiDdGetAvailDriverMemory), // 2
/*1049*/ NUL(NtGdiDdGetBltStatus), // 2
/*104a*/ NUL(NtGdiDdGetDC), // 2
/*104b*/ NUL(NtGdiDdGetDriverInfo), // 2
/*104c*/ NUL(NtGdiDdGetDxHandle), // 3
/*104d*/ NUL(NtGdiDdGetFlipStatus), // 2
/*104e*/ NUL(NtGdiDdGetInternalMoCompInfo), // 2
/*104f*/ NUL(NtGdiDdGetMoCompBuffInfo), // 2
/*1050*/ NUL(NtGdiDdGetMoCompGuids), // 2
/*1051*/ NUL(NtGdiDdGetMoCompFormats), // 2
/*1052*/ NUL(NtGdiDdGetScanLine), // 2
/*1053*/ NUL(NtGdiDdLock), // 3
/*1054*/ NUL(NtGdiDdLockD3D), // 2
/*1055*/ NUL(NtGdiDdQueryDirectDrawObject), // 11
/*1056*/ NUL(NtGdiDdQueryMoCompStatus), // 2
/*1057*/ NUL(NtGdiDdReenableDirectDrawObject), // 2
/*1058*/ NUL(NtGdiDdReleaseDC), // 1
/*1059*/ NUL(NtGdiDdRenderMoComp), // 2
/*105a*/ NUL(NtGdiDdResetVisrgn), // 2
/*105b*/ NUL(NtGdiDdSetColorKey), // 2
/*105c*/ NUL(NtGdiDdSetExclusiveMode), // 2
/*105d*/ NUL(NtGdiDdSetGammaRamp), // 3
/*105e*/ NUL(NtGdiDdCreateSurfaceEx), // 3
/*105f*/ NUL(NtGdiDdSetOverlayPosition), // 3
/*1060*/ NUL(NtGdiDdUnattachSurface), // 2
/*1061*/ NUL(NtGdiDdUnlock), // 2
/*1062*/ NUL(NtGdiDdUnlockD3D), // 2
/*1063*/ NUL(NtGdiDdUpdateOverlay), // 3
/*1064*/ NUL(NtGdiDdWaitForVerticalBlank), // 2
/*1065*/ NUL(NtGdiDvpCanCreateVideoPort), // 2
/*1066*/ NUL(NtGdiDvpColorControl), // 2
/*1067*/ NUL(NtGdiDvpCreateVideoPort), // 2
/*1068*/ NUL(NtGdiDvpDestroyVideoPort), // 2
/*1069*/ NUL(NtGdiDvpFlipVideoPort), // 4
/*106a*/ NUL(NtGdiDvpGetVideoPortBandwidth), // 2
/*106b*/ NUL(NtGdiDvpGetVideoPortField), // 2
/*106c*/ NUL(NtGdiDvpGetVideoPortFlipStatus), // 2
/*106d*/ NUL(NtGdiDvpGetVideoPortInputFormats), // 2
/*106e*/ NUL(NtGdiDvpGetVideoPortLine), // 2
/*106f*/ NUL(NtGdiDvpGetVideoPortOutputFormats), // 2
/*1070*/ NUL(NtGdiDvpGetVideoPortConnectInfo), // 2
/*1071*/ NUL(NtGdiDvpGetVideoSignalStatus), // 2
/*1072*/ NUL(NtGdiDvpUpdateVideoPort), // 4
/*1073*/ NUL(NtGdiDvpWaitForVideoPortSync), // 2
/*1074*/ NUL(NtGdiDvpAcquireNotification), // 3
/*1075*/ NUL(NtGdiDvpReleaseNotification), // 2
/*1076*/ NUL(NtGdiDxgGenericThunk), // 6
/*1077*/ NUL(NtGdiDeleteClientObj), // 1
/*1078*/ NUL(NtGdiDeleteColorSpace), // 1
/*1079*/ NUL(NtGdiDeleteColorTransform), // 2
/*107a*/ IMP(NtGdiDeleteObjectApp, 1),
/*107b*/ NUL(NtGdiDescribePixelFormat), // 4
/*107c*/ NUL(NtGdiGetPerBandInfo), // 2
/*107d*/ NUL(NtGdiDoBanding), // 4
/*107e*/ NUL(NtGdiDoPalette), // 6
/*107f*/ NUL(NtGdiDrawEscape), // 4
/*1080*/ NUL(NtGdiEllipse), // 5
/*1081*/ NUL(NtGdiEnableEudc), // 1
/*1082*/ NUL(NtGdiEndDoc), // 1
/*1083*/ NUL(NtGdiEndPage), // 1
/*1084*/ NUL(NtGdiEndPath), // 1
/*1085*/ NUL(NtGdiEnumFontChunk), // 5
/*1086*/ NUL(NtGdiEnumFontClose), // 1
/*1087*/ NUL(NtGdiEnumFontOpen), // 7
/*1088*/ NUL(NtGdiEnumObjects), // 4
/*1089*/ NUL(NtGdiEqualRgn), // 2
/*108a*/ NUL(NtGdiEudcLoadUnloadLink), // 7
/*108b*/ NUL(NtGdiExcludeClipRect), // 5
/*108c*/ NUL(NtGdiExtCreatePen), // 11
/*108d*/ NUL(NtGdiExtCreateRegion), // 3
/*108e*/ NUL(NtGdiExtEscape), // 8
/*108f*/ NUL(NtGdiExtFloodFill), // 5
/*1090*/ IMP(NtGdiExtGetObjectW, 3),
/*1091*/ NUL(NtGdiExtSelectClipRgn), // 3
/*1092*/ NUL(NtGdiExtTextOutW), // 9
/*1093*/ NUL(NtGdiFillPath), // 1
/*1094*/ NUL(NtGdiFillRgn), // 3
/*1095*/ NUL(NtGdiFlattenPath), // 1
/*1096*/ NUL(NtGdiFlushUserBatch), // 0
/*1097*/ IMP(NtGdiFlush, 0), // GreFlush
/*1098*/ NUL(NtGdiForceUFIMapping), // 2
/*1099*/ NUL(NtGdiFrameRgn), // 5
/*109a*/ NUL(NtGdiFullscreenControl), // 5
/*109b*/ NUL(NtGdiGetAndSetDCDword), // 4
/*109c*/ NUL(NtGdiGetAppClipBox), // 2
/*109d*/ NUL(NtGdiGetBitmapBits), // 3
/*109e*/ NUL(NtGdiGetBitmapDimension), // 2
/*109f*/ NUL(NtGdiGetBoundsRect), // 3
/*10a0*/ NUL(NtGdiGetCharABCWidthsW), // 6
/*10a1*/ NUL(NtGdiGetCharacterPlacementW), // 6
/*10a2*/ NUL(NtGdiGetCharSet), // 1
/*10a3*/ NUL(NtGdiGetCharWidthW), // 6
/*10a4*/ NUL(NtGdiGetCharWidthInfo), // 2
/*10a5*/ NUL(NtGdiGetColorAdjustment), // 2
/*10a6*/ NUL(NtGdiGetColorSpaceforBitmap), // 1
/*10a7*/ NUL(NtGdiGetDCDword), // 3
/*10a8*/ IMP(NtGdiGetDCforBitmap, 1),
/*10a9*/ IMP(NtGdiGetDCObject, 2),
/*10aa*/ NUL(NtGdiGetDCPoint), // 3
/*10ab*/ NUL(NtGdiGetDeviceCaps), // 2
/*10ac*/ NUL(NtGdiGetDeviceGammaRamp), // 2
/*10ad*/ NUL(NtGdiGetDeviceCapsAll), // 2
/*10ae*/ NUL(NtGdiGetDIBitsInternal), // 9
/*10af*/ NUL(NtGdiGetETM), // 2
/*10b0*/ NUL(NtGdiGetEudcTimeStampEx), // 3
/*10b1*/ NUL(NtGdiGetFontData), // 5
/*10b2*/ IMP(NtGdiGetFontResourceInfoInternalW, 7),
/*10b3*/ NUL(NtGdiGetGlyphIndicesW), // 5
/*10b4*/ NUL(NtGdiGetGlyphIndicesWInternal), // 6
/*10b5*/ NUL(NtGdiGetGlyphOutline), // 8
/*10b6*/ NUL(NtGdiGetKerningPairs), // 3
/*10b7*/ NUL(NtGdiGetLinkedUFIs), // 3
/*10b8*/ NUL(NtGdiGetMiterLimit), // 2
/*10b9*/ NUL(NtGdiGetMonitorID), // 3
/*10ba*/ NUL(NtGdiGetNearestColor), // 2
/*10bb*/ NUL(NtGdiGetNearestPaletteIndex), // 2
/*10bc*/ NUL(NtGdiGetObjectBitmapHandle), // 2
/*10bd*/ NUL(NtGdiGetOutlineTextMetricsInternalW), // 4
/*10be*/ NUL(NtGdiGetPath), // 4
/*10bf*/ NUL(NtGdiGetPixel), // 3
/*10c0*/ NUL(NtGdiGetRandomRgn), // 3
/*10c1*/ NUL(NtGdiGetRasterizerCaps), // 2
/*10c2*/ NUL(NtGdiGetRealizationInfo), // 3
/*10c3*/ NUL(NtGdiGetRegionData), // 3
/*10c4*/ NUL(NtGdiGetRgnBox), // 2
/*10c5*/ NUL(NtGdiGetServerMetaFileBits), // 7
/*10c6*/ NUL(NtGdiGetSpoolMessage), // 4
/*10c7*/ NUL(NtGdiGetStats), // 5
/*10c8*/ IMP(NtGdiGetStockObject, 1),
/*10c9*/ NUL(NtGdiGetStringBitmapW), // 5
/*10ca*/ NUL(NtGdiGetSystemPaletteUse), // 1
/*10cb*/ NUL(NtGdiGetTextCharsetInfo), // 3
/*10cc*/ NUL(NtGdiGetTextExtent), // 5
/*10cd*/ NUL(NtGdiGetTextExtentExW), // 8
/*10ce*/ NUL(NtGdiGetTextFaceW), // 4
/*10cf*/ NUL(NtGdiGetTextMetricsW), // 3
/*10d0*/ NUL(NtGdiGetTransform), // 3
/*10d1*/ NUL(NtGdiGetUFI), // 6
/*10d2*/ NUL(NtGdiGetEmbUFI), // 7
/*10d3*/ NUL(NtGdiGetUFIPathname), // 10
/*10d4*/ NUL(NtGdiGetEmbedFonts), // 0
/*10d5*/ NUL(NtGdiChangeGhostFont), // 2
/*10d6*/ NUL(NtGdiAddEmbFontToDC), // 2
/*10d7*/ NUL(NtGdiGetFontUnicodeRanges), // 2
/*10d8*/ NUL(NtGdiGetWidthTable), // 7
/*10d9*/ NUL(NtGdiGradientFill), // 6
/*10da*/ NUL(NtGdiHfontCreate), // 5
/*10db*/ NUL(NtGdiIcmBrushInfo), // 8
/*10dc*/ IMP(NtGdiInit, 0),
/*10dd*/ NUL(NtGdiInitSpool), // 0
/*10de*/ NUL(NtGdiIntersectClipRect), // 5
/*10df*/ NUL(NtGdiInvertRgn), // 2
/*10e0*/ NUL(NtGdiLineTo), // 3
/*10e1*/ NUL(NtGdiMakeFontDir), // 5
/*10e2*/ NUL(NtGdiMakeInfoDC), // 2
/*10e3*/ NUL(NtGdiMaskBlt), // 13
/*10e4*/ NUL(NtGdiModifyWorldTransform), // 3
/*10e5*/ NUL(NtGdiMonoBitmap), // 1
/*10e6*/ NUL(NtGdiMoveTo), // 4
/*10e7*/ NUL(NtGdiOffsetClipRgn), // 3
/*10e8*/ NUL(NtGdiOffsetRgn), // 3
/*10e9*/ NUL(NtGdiOpenDCW), // 7
/*10ea*/ NUL(NtGdiPatBlt), // 6
/*10eb*/ NUL(NtGdiPolyPatBlt), // 5
/*10ec*/ NUL(NtGdiPathToRegion), // 1
/*10ed*/ NUL(NtGdiPlgBlt), // 11
/*10ee*/ NUL(NtGdiPolyDraw), // 4
/*10ef*/ NUL(NtGdiPolyPolyDraw), // 5
/*10f0*/ NUL(NtGdiPolyTextOutW), // 4
/*10f1*/ NUL(NtGdiPtInRegion), // 3
/*10f2*/ NUL(NtGdiPtVisible), // 3
/*10f3*/ NUL(NtGdiQueryFonts), // 3
/*10f4*/ IMP(NtGdiQueryFontAssocInfo, 1),
/*10f5*/ NUL(NtGdiRectangle), // 5
/*10f6*/ NUL(NtGdiRectInRegion), // 2
/*10f7*/ NUL(NtGdiRectVisible), // 2
/*10f8*/ NUL(NtGdiRemoveFontResourceW), // 6
/*10f9*/ NUL(NtGdiRemoveFontMemResourceEx), // 1
/*10fa*/ NUL(NtGdiResetDC), // 5
/*10fb*/ NUL(NtGdiResizePalette), // 2
/*10fc*/ IMP(NtGdiRestoreDC, 2),
/*10fd*/ NUL(NtGdiRoundRect), // 7
/*10fe*/ IMP(NtGdiSaveDC, 1),
/*10ff*/ NUL(NtGdiScaleViewportExtEx), // 6
/*1100*/ NUL(NtGdiScaleWindowExtEx), // 6
/*1101*/ IMP(NtGdiSelectBitmap, 2),
/*1102*/ NUL(NtGdiSelectBrush), // 2
/*1103*/ NUL(NtGdiSelectClipPath), // 2
/*1104*/ NUL(NtGdiSelectFont), // 2
/*1105*/ NUL(NtGdiSelectPen), // 2
/*1106*/ NUL(NtGdiSetBitmapAttributes), // 2
/*1107*/ NUL(NtGdiSetBitmapBits), // 3
/*1108*/ NUL(NtGdiSetBitmapDimension), // 4
/*1109*/ NUL(NtGdiSetBoundsRect), // 3
/*110a*/ NUL(NtGdiSetBrushAttributes), // 2
/*110b*/ NUL(NtGdiSetBrushOrg), // 4
/*110c*/ NUL(NtGdiSetColorAdjustment), // 2
/*110d*/ NUL(NtGdiSetColorSpace), // 2
/*110e*/ NUL(NtGdiSetDeviceGammaRamp), // 2
/*110f*/ IMP(NtGdiSetDIBitsToDeviceInternal, 16),
/*1110*/ NUL(NtGdiSetFontEnumeration), // 1
/*1111*/ NUL(NtGdiSetFontXform), // 3
/*1112*/ NUL(NtGdiSetIcmMode), // 3
/*1113*/ NUL(NtGdiSetLinkedUFIs), // 3
/*1114*/ NUL(NtGdiSetMagicColors), // 3
/*1115*/ NUL(NtGdiSetMetaRgn), // 1
/*1116*/ NUL(NtGdiSetMiterLimit), // 3
/*1117*/ NUL(NtGdiGetDeviceWidth), // 1
/*1118*/ NUL(NtGdiMirrorWindowOrg), // 1
/*1119*/ NUL(NtGdiSetLayout), // 3
/*111a*/ NUL(NtGdiSetPixel), // 4
/*111b*/ NUL(NtGdiSetPixelFormat), // 2
/*111c*/ NUL(NtGdiSetRectRgn), // 5
/*111d*/ NUL(NtGdiSetSystemPaletteUse), // 2
/*111e*/ NUL(NtGdiSetTextJustification), // 3
/*111f*/ NUL(NtGdiSetupPublicCFONT), // 3
/*1120*/ NUL(NtGdiSetVirtualResolution), // 5
/*1121*/ NUL(NtGdiSetSizeDevice), // 3
/*1122*/ NUL(NtGdiStartDoc), // 4
/*1123*/ NUL(NtGdiStartPage), // 1
/*1124*/ NUL(NtGdiStretchBlt), // 12
/*1125*/ NUL(NtGdiStretchDIBitsInternal), // 16
/*1126*/ NUL(NtGdiStrokeAndFillPath), // 1
/*1127*/ NUL(NtGdiStrokePath), // 1
/*1128*/ NUL(NtGdiSwapBuffers), // 1
/*1129*/ NUL(NtGdiTransformPoints), // 5
/*112a*/ NUL(NtGdiTransparentBlt), // 11
/*112b*/ NUL(NtGdiUnloadPrinterDriver), // 2
/*112c*/ NUL(NtGdiUnmapMemFont), // 1
/*112d*/ NUL(NtGdiUnrealizeObject), // 1
/*112e*/ NUL(NtGdiUpdateColors), // 1
/*112f*/ NUL(NtGdiWidenPath), // 1
/*1130*/ NUL(NtUserActivateKeyboardLayout), // 2
/*1131*/ NUL(NtUserAlterWindowStyle), // 3
/*1132*/ NUL(NtUserAssociateInputContext), // 3
/*1133*/ NUL(NtUserAttachThreadInput), // 3
/*1134*/ NUL(NtUserBeginPaint), // 2
/*1135*/ NUL(NtUserBitBltSysBmp), // 8
/*1136*/ NUL(NtUserBlockInput), // 1
/*1137*/ NUL(NtUserBuildHimcList), // 4
/*1138*/ NUL(NtUserBuildHwndList), // 7
/*1139*/ NUL(NtUserBuildNameList), // 4
/*113a*/ NUL(NtUserBuildPropList), // 4
/*113b*/ NUL(NtUserCallHwnd), // 2
/*113c*/ NUL(NtUserCallHwndLock), // 2
/*113d*/ NUL(NtUserCallHwndOpt), // 2
/*113e*/ NUL(NtUserCallHwndParam), // 3
/*113f*/ NUL(NtUserCallHwndParamLock), // 3
/*1140*/ NUL(NtUserCallMsgFilter), // 2
/*1141*/ NUL(NtUserCallNextHookEx), // 4
/*1142*/ IMP(NtUserCallNoParam, 1),
/*1143*/ IMP(NtUserCallOneParam, 2),
/*1144*/ IMP(NtUserCallTwoParam, 3),
/*1145*/ NUL(NtUserChangeClipboardChain), // 2
/*1146*/ NUL(NtUserChangeDisplaySettings), // 5
/*1147*/ NUL(NtUserCheckImeHotKey), // 2
/*1148*/ NUL(NtUserCheckMenuItem), // 3
/*1149*/ NUL(NtUserChildWindowFromPointEx), // 4
/*114a*/ NUL(NtUserClipCursor), // 1
/*114b*/ NUL(NtUserCloseClipboard), // 0
/*114c*/ NUL(NtUserCloseDesktop), // 1
/*114d*/ NUL(NtUserCloseWindowStation), // 1
/*114e*/ NUL(NtUserConsoleControl), // 3
/*114f*/ NUL(NtUserConvertMemHandle), // 2
/*1150*/ NUL(NtUserCopyAcceleratorTable), // 3
/*1151*/ NUL(NtUserCountClipboardFormats), // 0
/*1152*/ NUL(NtUserCreateAcceleratorTable), // 2
/*1153*/ NUL(NtUserCreateCaret), // 4
/*1154*/ IMP(NtUserCreateDesktop, 5),
/*1155*/ NUL(NtUserCreateInputContext), // 1
/*1156*/ NUL(NtUserCreateLocalMemHandle), // 4
/*1157*/ IMP(NtUserCreateWindowEx, 13),
/*1158*/ IMP(NtUserCreateWindowStation, 6),
/*1159*/ NUL(NtUserDdeGetQualityOfService), // 3
/*115a*/ NUL(NtUserDdeInitialize), // 5
/*115b*/ NUL(NtUserDdeSetQualityOfService), // 3
/*115c*/ NUL(NtUserDeferWindowPos), // 8
/*115d*/ NUL(NtUserDefSetText), // 2
/*115e*/ NUL(NtUserDeleteMenu), // 3
/*115f*/ NUL(NtUserDestroyAcceleratorTable), // 1
/*1160*/ NUL(NtUserDestroyCursor), // 2
/*1161*/ NUL(NtUserDestroyInputContext), // 1
/*1162*/ NUL(NtUserDestroyMenu), // 1
/*1163*/ NUL(NtUserDestroyWindow), // 1
/*1164*/ NUL(NtUserDisableThreadIme), // 1
/*1165*/ NUL(NtUserDispatchMessage), // 1
/*1166*/ NUL(NtUserDragDetect), // 3
/*1167*/ NUL(NtUserDragObject), // 5
/*1168*/ NUL(NtUserDrawAnimatedRects), // 4
/*1169*/ NUL(NtUserDrawCaption), // 4
/*116a*/ NUL(NtUserDrawCaptionTemp), // 7
/*116b*/ NUL(NtUserDrawIconEx), // 11
/*116c*/ NUL(NtUserDrawMenuBarTemp), // 5
/*116d*/ NUL(NtUserEmptyClipboard), // 0
/*116e*/ NUL(NtUserEnableMenuItem), // 3
/*116f*/ NUL(NtUserEnableScrollBar), // 3
/*1170*/ NUL(NtUserEndDeferWindowPosEx), // 2
/*1171*/ NUL(NtUserEndMenu), // 0
/*1172*/ NUL(NtUserEndPaint), // 2
/*1173*/ NUL(NtUserEnumDisplayDevices), // 4
/*1174*/ NUL(NtUserEnumDisplayMonitors), // 4
/*1175*/ NUL(NtUserEnumDisplaySettings), // 4
/*1176*/ NUL(NtUserEvent), // 1
/*1177*/ NUL(NtUserExcludeUpdateRgn), // 2
/*1178*/ NUL(NtUserFillWindow), // 4
/*1179*/ IMP(NtUserFindExistingCursorIcon, 3),
/*117a*/ NUL(NtUserFindWindowEx), // 5
/*117b*/ NUL(NtUserFlashWindowEx), // 1
/*117c*/ NUL(NtUserGetAltTabInfo), // 6
/*117d*/ NUL(NtUserGetAncestor), // 2
/*117e*/ NUL(NtUserGetAppImeLevel), // 1
/*117f*/ NUL(NtUserGetAsyncKeyState), // 1
/*1180*/ NUL(NtUserGetAtomName), // 2
/*1181*/ IMP(NtUserGetCaretBlinkTime, 0),
/*1182*/ NUL(NtUserGetCaretPos), // 1
/*1183*/ IMP(NtUserGetClassInfo, 5),
/*1184*/ NUL(NtUserGetClassName), // 3
/*1185*/ NUL(NtUserGetClipboardData), // 2
/*1186*/ NUL(NtUserGetClipboardFormatName), // 3
/*1187*/ NUL(NtUserGetClipboardOwner), // 0
/*1188*/ NUL(NtUserGetClipboardSequenceNumber), // 0
/*1189*/ NUL(NtUserGetClipboardViewer), // 0
/*118a*/ NUL(NtUserGetClipCursor), // 1
/*118b*/ NUL(NtUserGetComboBoxInfo), // 2
/*118c*/ NUL(NtUserGetControlBrush), // 3
/*118d*/ NUL(NtUserGetControlColor), // 4
/*118e*/ NUL(NtUserGetCPD), // 3
/*118f*/ NUL(NtUserGetCursorFrameInfo), // 4
/*1190*/ NUL(NtUserGetCursorInfo), // 1
/*1191*/ IMP(NtUserGetDC, 1),
/*1192*/ NUL(NtUserGetDCEx), // 3
/*1193*/ NUL(NtUserGetDoubleClickTime), // 0
/*1194*/ NUL(NtUserGetForegroundWindow), // 0
/*1195*/ NUL(NtUserGetGuiResources), // 2
/*1196*/ NUL(NtUserGetGUIThreadInfo), // 2
/*1197*/ NUL(NtUserGetIconInfo), // 6
/*1198*/ NUL(NtUserGetIconSize), // 4
/*1199*/ NUL(NtUserGetImeHotKey), // 4
/*119a*/ NUL(NtUserGetImeInfoEx), // 2
/*119b*/ NUL(NtUserGetInternalWindowPos), // 3
/*119c*/ IMP(NtUserGetKeyboardLayoutList, 2),
/*119d*/ NUL(NtUserGetKeyboardLayoutName), // 1
/*119e*/ NUL(NtUserGetKeyboardState), // 1
/*119f*/ NUL(NtUserGetKeyNameText), // 3
/*11a0*/ NUL(NtUserGetKeyState), // 1
/*11a1*/ NUL(NtUserGetListBoxInfo), // 1
/*11a2*/ NUL(NtUserGetMenuBarInfo), // 4
/*11a3*/ NUL(NtUserGetMenuIndex), // 2
/*11a4*/ NUL(NtUserGetMenuItemRect), // 4
/*11a5*/ IMP(NtUserGetMessage, 4),
/*11a6*/ NUL(NtUserGetMouseMovePointsEx), // 5
/*11a7*/ NUL(NtUserGetObjectInformation), // 5
/*11a8*/ NUL(NtUserGetOpenClipboardWindow), // 0
/*11a9*/ NUL(NtUserGetPriorityClipboardFormat), // 2
/*11aa*/ IMP(NtUserGetProcessWindowStation, 0),
/*11ab*/ NUL(NtUserGetRawInputBuffer), // 3
/*11ac*/ NUL(NtUserGetRawInputData), // 5
/*11ad*/ NUL(NtUserGetRawInputDeviceInfo), // 4
/*11ae*/ NUL(NtUserGetRawInputDeviceList), // 3
/*11af*/ NUL(NtUserGetRegisteredRawInputDevices), // 3
/*11b0*/ NUL(NtUserGetScrollBarInfo), // 3
/*11b1*/ NUL(NtUserGetSystemMenu), // 2
/*11b2*/ IMP(NtUserGetThreadDesktop, 2),
/*11b3*/ IMP(NtUserGetThreadState, 1),
/*11b4*/ NUL(NtUserGetTitleBarInfo), // 2
/*11b5*/ NUL(NtUserGetUpdateRect), // 3
/*11b6*/ NUL(NtUserGetUpdateRgn), // 3
/*11b7*/ NUL(NtUserGetWindowDC), // 1
/*11b8*/ NUL(NtUserGetWindowPlacement), // 2
/*11b9*/ NUL(NtUserGetWOWClass), // 2
/*11ba*/ NUL(NtUserHardErrorControl), // 3
/*11bb*/ NUL(NtUserHideCaret), // 1
/*11bc*/ NUL(NtUserHiliteMenuItem), // 4
/*11bd*/ NUL(NtUserImpersonateDdeClientWindow), // 2
/*11be*/ IMP(NtUserInitialize, 3),
/*11bf*/ IMP(NtUserInitializeClientPfnArrays, 4),
/*11c0*/ NUL(NtUserInitTask), // 12
/*11c1*/ NUL(NtUserInternalGetWindowText), // 3
/*11c2*/ NUL(NtUserInvalidateRect), // 3
/*11c3*/ NUL(NtUserInvalidateRgn), // 3
/*11c4*/ NUL(NtUserIsClipboardFormatAvailable), // 1
/*11c5*/ NUL(NtUserKillTimer), // 2
/*11c6*/ IMP(NtUserLoadKeyboardLayoutEx, 6),
/*11c7*/ NUL(NtUserLockWindowStation), // 1
/*11c8*/ NUL(NtUserLockWindowUpdate), // 1
/*11c9*/ NUL(NtUserLockWorkStation), // 0
/*11ca*/ NUL(NtUserMapVirtualKeyEx), // 4
/*11cb*/ NUL(NtUserMenuItemFromPoint), // 4
/*11cc*/ NUL(NtUserMessageCall), // 7
/*11cd*/ NUL(NtUserMinMaximize), // 3
/*11ce*/ NUL(NtUserMNDragLeave), // 0
/*11cf*/ NUL(NtUserMNDragOver), // 2
/*11d0*/ NUL(NtUserModifyUserStartupInfoFlags), // 2
/*11d1*/ NUL(NtUserMoveWindow), // 6
/*11d2*/ NUL(NtUserNotifyIMEStatus), // 3
/*11d3*/ IMP(NtUserNotifyProcessCreate, 4),
/*11d4*/ NUL(NtUserNotifyWinEvent), // 4
/*11d5*/ NUL(NtUserOpenClipboard), // 2
/*11d6*/ NUL(NtUserOpenDesktop), // 3
/*11d7*/ NUL(NtUserOpenInputDesktop), // 3
/*11d8*/ NUL(NtUserOpenWindowStation), // 2
/*11d9*/ NUL(NtUserPaintDesktop), // 1
/*11da*/ NUL(NtUserPeekMessage), // 5
/*11db*/ NUL(NtUserPostMessage), // 4
/*11dc*/ NUL(NtUserPostThreadMessage), // 4
/*11dd*/ NUL(NtUserPrintWindow), // 3
/*11de*/ IMP(NtUserProcessConnect, 3),
/*11df*/ NUL(NtUserQueryInformationThread), // 5
/*11e0*/ NUL(NtUserQueryInputContext), // 2
/*11e1*/ NUL(NtUserQuerySendMessage), // 1
/*11e2*/ NUL(NtUserQueryUserCounters), // 5
/*11e3*/ NUL(NtUserQueryWindow), // 2
/*11e4*/ NUL(NtUserRealChildWindowFromPoint), // 3
/*11e5*/ NUL(NtUserRealInternalGetMessage), // 6
/*11e6*/ NUL(NtUserRealWaitMessageEx), // 2
/*11e7*/ NUL(NtUserRedrawWindow), // 4
/*11e8*/ IMP(NtUserRegisterClassExWOW, 6),
/*11e9*/ NUL(NtUserRegisterUserApiHook), // 2
/*11ea*/ NUL(NtUserRegisterHotKey), // 4
/*11eb*/ NUL(NtUserRegisterRawInputDevices), // 3
/*11ec*/ NUL(NtUserRegisterTasklist), // 1
/*11ed*/ IMP(NtUserRegisterWindowMessage, 1),
/*11ee*/ NUL(NtUserRemoveMenu), // 3
/*11ef*/ NUL(NtUserRemoveProp), // 2
/*11f0*/ NUL(NtUserResolveDesktop), // 4
/*11f1*/ NUL(NtUserResolveDesktopForWOW), // 1
/*11f2*/ NUL(NtUserSBGetParms), // 4
/*11f3*/ NUL(NtUserScrollDC), // 7
/*11f4*/ NUL(NtUserScrollWindowEx),
/*11f5*/ IMP(NtUserSelectPalette, 3),
/*11f6*/ NUL(NtUserSendInput), // 3
/*11f7*/ NUL(NtUserSetActiveWindow), // 1
/*11f8*/ NUL(NtUserSetAppImeLevel), // 2
/*11f9*/ NUL(NtUserSetCapture), // 1
/*11fa*/ NUL(NtUserSetClassLong), // 4
/*11fb*/ NUL(NtUserSetClassWord), // 3
/*11fc*/ NUL(NtUserSetClipboardData), // 3
/*11fd*/ NUL(NtUserSetClipboardViewer), // 1
/*11fe*/ NUL(NtUserSetConsoleReserveKeys), // 2
/*11ff*/ NUL(NtUserSetCursor), // 1
/*1200*/ NUL(NtUserSetCursorContents), // 2
/*1201*/ IMP(NtUserSetCursorIconData, 4),
/*1202*/ NUL(NtUserSetDbgTag), // 2
/*1203*/ NUL(NtUserSetFocus), // 1
/*1204*/ IMP(NtUserSetImeHotKey, 5),
/*1205*/ NUL(NtUserSetImeInfoEx), // 1
/*1206*/ NUL(NtUserSetImeOwnerWindow), // 2
/*1207*/ NUL(NtUserSetInformationProcess), // 4
/*1208*/ IMP(NtUserSetInformationThread, 4),
/*1209*/ NUL(NtUserSetInternalWindowPos), // 4
/*120a*/ NUL(NtUserSetKeyboardState), // 1
/*120b*/ IMP(NtUserSetLogonNotifyWindow, 1),
/*120c*/ NUL(NtUserSetMenu), // 3
/*120d*/ NUL(NtUserSetMenuContextHelpId), // 2
/*120e*/ NUL(NtUserSetMenuDefaultItem), // 3
/*120f*/ NUL(NtUserSetMenuFlagRtoL), // 1
/*1210*/ NUL(NtUserSetObjectInformation), // 4
/*1211*/ NUL(NtUserSetParent), // 2
/*1212*/ IMP(NtUserSetProcessWindowStation, 1),
/*1213*/ NUL(NtUserSetProp), // 3
/*1214*/ NUL(NtUserSetRipFlags), // 2
/*1215*/ NUL(NtUserSetScrollInfo), // 4
/*1216*/ NUL(NtUserSetShellWindowEx), // 2
/*1217*/ NUL(NtUserSetSysColors), // 4
/*1218*/ NUL(NtUserSetSystemCursor), // 2
/*1219*/ NUL(NtUserSetSystemMenu), // 2
/*121a*/ NUL(NtUserSetSystemTimer), // 4
/*121b*/ IMP(NtUserSetThreadDesktop, 1),
/*121c*/ NUL(NtUserSetThreadLayoutHandles), // 2
/*121d*/ NUL(NtUserSetThreadState), // 2
/*121e*/ NUL(NtUserSetTimer), // 4
/*121f*/ NUL(NtUserSetWindowFNID), // 2
/*1220*/ NUL(NtUserSetWindowLong), // 4
/*1221*/ NUL(NtUserSetWindowPlacement), // 2
/*1222*/ NUL(NtUserSetWindowPos), // 7
/*1223*/ NUL(NtUserSetWindowRgn), // 3
/*1224*/ NUL(NtUserSetWindowsHookAW), // 3
/*1225*/ NUL(NtUserSetWindowsHookEx), // 6
/*1226*/ IMP(NtUserSetWindowStationUser, 4),
/*1227*/ NUL(NtUserSetWindowWord), // 3
/*1228*/ NUL(NtUserSetWinEventHook), // 8
/*1229*/ NUL(NtUserShowCaret), // 1
/*122a*/ NUL(NtUserShowScrollBar), // 3
/*122b*/ NUL(NtUserShowWindow), // 2
/*122c*/ NUL(NtUserShowWindowAsync), // 2
/*122d*/ NUL(NtUserSoundSentry), // 0
/*122e*/ NUL(NtUserSwitchDesktop), // 1
/*122f*/ IMP(NtUserSystemParametersInfo, 4),
/*1230*/ NUL(NtUserTestForInteractiveUser), // 1
/*1231*/ NUL(NtUserThunkedMenuInfo), // 2
/*1232*/ NUL(NtUserThunkedMenuItemInfo), // 6
/*1233*/ NUL(NtUserToUnicodeEx), // 7
/*1234*/ NUL(NtUserTrackMouseEvent), // 1
/*1235*/ NUL(NtUserTrackPopupMenuEx), // 6
/*1236*/ NUL(NtUserCalcMenuBar), // 5
/*1237*/ NUL(NtUserPaintMenuBar), // 6
/*1238*/ NUL(NtUserTranslateAccelerator), // 3
/*1239*/ NUL(NtUserTranslateMessage), // 2
/*123a*/ NUL(NtUserUnhookWindowsHookEx), // 1
/*123b*/ NUL(NtUserUnhookWinEvent), // 1
/*123c*/ NUL(NtUserUnloadKeyboardLayout), // 1
/*123d*/ NUL(NtUserUnlockWindowStation), // 1
/*123e*/ NUL(NtUserUnregisterClass), // 3
/*123f*/ NUL(NtUserUnregisterUserApiHook), // 0
/*1240*/ NUL(NtUserUnregisterHotKey), // 2
/*1241*/ NUL(NtUserUpdateInputContext), // 3
/*1242*/ NUL(NtUserUpdateInstance), // 3
/*1243*/ NUL(NtUserUpdateLayeredWindow), // 9
/*1244*/ NUL(NtUserGetLayeredWindowAttributes), // 4
/*1245*/ NUL(NtUserSetLayeredWindowAttributes), // 4
/*1246*/ IMP(NtUserUpdatePerUserSystemParameters, 2),
/*1247*/ NUL(NtUserUserHandleGrantAccess), // 3
/*1248*/ NUL(NtUserValidateHandleSecure), // 2
/*1249*/ NUL(NtUserValidateRect), // 2
/*124a*/ NUL(NtUserValidateTimerCallback), // 3
/*124b*/ NUL(NtUserVkKeyScanEx), // 3
/*124c*/ NUL(NtUserWaitForInputIdle), // 3
/*124d*/ NUL(NtUserWaitForMsgAndEvent), // 1
/*124e*/ NUL(NtUserWaitMessage), // 0
/*124f*/ NUL(NtUserWin32PoolAllocationStats), // 6
/*1250*/ NUL(NtUserWindowFromPoint), // 2
/*1251*/ NUL(NtUserYieldTask), // 0
/*1252*/ NUL(NtUserRemoteConnect), // 3
/*1253*/ NUL(NtUserRemoteRedrawRectangle), // 4
/*1254*/ NUL(NtUserRemoteRedrawScreen), // 0
/*1255*/ NUL(NtUserRemoteStopScreenUpdates), // 0
/*1256*/ NUL(NtUserCtxDisplayIOCtl), // 3
/*1257*/ NUL(NtGdiEngAssociateSurface), // 3
/*1258*/ NUL(NtGdiEngCreateBitmap), // 6
/*1259*/ NUL(NtGdiEngCreateDeviceSurface), // 4
/*125a*/ NUL(NtGdiEngCreateDeviceBitmap), // 4
/*125b*/ NUL(NtGdiEngCreatePalette), // 6
/*125c*/ NUL(NtGdiEngComputeGlyphSet), // 3
/*125d*/ NUL(NtGdiEngCopyBits), // 6
/*125e*/ NUL(NtGdiEngDeletePalette), // 1
/*125f*/ NUL(NtGdiEngDeleteSurface), // 1
/*1260*/ NUL(NtGdiEngEraseSurface), // 3
/*1261*/ NUL(NtGdiEngUnlockSurface), // 1
/*1262*/ NUL(NtGdiEngLockSurface), // 1
/*1263*/ NUL(NtGdiEngBitBlt), // 11
/*1264*/ NUL(NtGdiEngStretchBlt), // 11
/*1265*/ NUL(NtGdiEngPlgBlt), // 11
/*1266*/ NUL(NtGdiEngMarkBandingSurface), // 1
/*1267*/ NUL(NtGdiEngStrokePath), // 8
/*1268*/ NUL(NtGdiEngFillPath), // 7
/*1269*/ NUL(NtGdiEngStrokeAndFillPath), // 10
/*126a*/ NUL(NtGdiEngPaint), // 5
/*126b*/ NUL(NtGdiEngLineTo), // 9
/*126c*/ NUL(NtGdiEngAlphaBlend), // 7
/*126d*/ NUL(NtGdiEngGradientFill), // 10
/*126e*/ NUL(NtGdiEngTransparentBlt), // 8
/*126f*/ NUL(NtGdiEngTextOut), // 10
/*1270*/ NUL(NtGdiEngStretchBltROP), // 13
/*1271*/ NUL(NtGdiXLATEOBJ_cGetPalette), // 4
/*1272*/ NUL(NtGdiXLATEOBJ_iXlate), // 2
/*1273*/ NUL(NtGdiXLATEOBJ_hGetColorTransform), // 1
/*1274*/ NUL(NtGdiCLIPOBJ_bEnum), // 3
/*1275*/ NUL(NtGdiCLIPOBJ_cEnumStart), // 5
/*1276*/ NUL(NtGdiCLIPOBJ_ppoGetPath), // 1
/*1277*/ NUL(NtGdiEngDeletePath), // 1
/*1278*/ NUL(NtGdiEngCreateClip), // 0
/*1279*/ NUL(NtGdiEngDeleteClip), // 1
/*127a*/ NUL(NtGdiBRUSHOBJ_ulGetBrushColor), // 1
/*127b*/ NUL(NtGdiBRUSHOBJ_pvAllocRbrush), // 2
/*127c*/ NUL(NtGdiBRUSHOBJ_pvGetRbrush), // 1
/*127d*/ NUL(NtGdiBRUSHOBJ_hGetColorTransform), // 1
/*127e*/ NUL(NtGdiXFORMOBJ_bApplyXform), // 5
/*127f*/ NUL(NtGdiXFORMOBJ_iGetXform), // 2
/*1280*/ NUL(NtGdiFONTOBJ_vGetInfo), // 3
/*1281*/ NUL(NtGdiFONTOBJ_pxoGetXform), // 1
/*1282*/ NUL(NtGdiFONTOBJ_cGetGlyphs), // 5
/*1283*/ NUL(NtGdiFONTOBJ_pifi), // 1
/*1284*/ NUL(NtGdiFONTOBJ_pfdg), // 1
/*1285*/ NUL(NtGdiFONTOBJ_pQueryGlyphAttrs), // 2
/*1286*/ NUL(NtGdiFONTOBJ_pvTrueTypeFontFile), // 2
/*1287*/ NUL(NtGdiFONTOBJ_cGetAllGlyphHandles), // 2
/*1288*/ NUL(NtGdiSTROBJ_bEnum), // 3
/*1289*/ NUL(NtGdiSTROBJ_bEnumPositionsOnly), // 3
/*128a*/ NUL(NtGdiSTROBJ_bGetAdvanceWidths), // 4
/*128b*/ NUL(NtGdiSTROBJ_vEnumStart), // 1
/*128c*/ NUL(NtGdiSTROBJ_dwGetCodePage), // 1
/*128d*/ NUL(NtGdiPATHOBJ_vGetBounds), // 2
/*128e*/ NUL(NtGdiPATHOBJ_bEnum), // 2
/*128f*/ NUL(NtGdiPATHOBJ_vEnumStart), // 1
/*1290*/ NUL(NtGdiPATHOBJ_vEnumStartClipLines), // 4
/*1291*/ NUL(NtGdiPATHOBJ_bEnumClipLines), // 3
/*1292*/ NUL(NtGdiGetDhpdev), // 1
/*1293*/ NUL(NtGdiEngCheckAbort), // 1
/*1294*/ NUL(NtGdiHT_Get8BPPFormatPalette), // 4
/*1295*/ NUL(NtGdiHT_Get8BPPMaskPalette), // 6
/*1296*/ NUL(NtGdiUpdateTransform), // 1
/*1297*/ NUL(NtGdiSetPUMPDOBJ), // 4
/*1298*/ NUL(NtGdiBRUSHOBJ_DeleteRbrush), // 2
/*1299*/ NUL(NtGdiUnmapMemFont), // 1
/*129a*/ NUL(NtGdiDrawStream), // 3
};

int option_trace;

ntcalldesc *ntcalls;
ULONG number_of_ntcalls;

ntcalldesc *ntuicalls;
ULONG number_of_uicalls;

static ULONG uicall_offset = 0x1000;

void init_syscalls(bool xp)
{
	if (xp)
	{
		number_of_ntcalls = sizeof winxp_calls/sizeof winxp_calls[0];
		ntcalls = winxp_calls;
		number_of_uicalls = sizeof winxp_uicalls/sizeof winxp_uicalls[0];
		ntuicalls = winxp_uicalls;
	}
	else
	{
		number_of_ntcalls = sizeof win2k_calls/sizeof win2k_calls[0];
		ntcalls = win2k_calls;
		number_of_uicalls = sizeof win2k_uicalls/sizeof win2k_uicalls[0];
		ntuicalls = win2k_uicalls;
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
	const int magic_val = 0x1248;	// random unlikely value
	int magic = magic_val;
	BOOLEAN win32k_func = FALSE;

	/* check the call number is in range */
	if (func >= 0 && func < number_of_ntcalls)
		ntcall = &ntcalls[func];
	else if (func >= uicall_offset && func <= (uicall_offset + number_of_uicalls))
	{
		win32k_func = TRUE;
		ntcall = &ntuicalls[func - uicall_offset];
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

	assert( magic == magic_val );

end:
	trace_syscall_exit(id, ntcall, r, retaddr);

	return r;
}
