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

	IMP( NtAcceptConnectPort, 6 ),
	IMP( NtAccessCheck, 8 ),
	DEC( NtAccessCheckAndAuditAlarm, 11),
	DEC( NtAccessCheckByType, 11 ),
	NUL( NtAccessCheckByTypeAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultList ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarm ),
	NUL( NtAccessCheckByTypeResultListAndAuditAlarmByHandle ),
	IMP( NtAddAtom, 3 ),
#ifdef SYSCALL_WINXP
	NUL( NtAddBootEntry ),
#endif
	DEC( NtAdjustGroupsToken, 6 ),
	IMP( NtAdjustPrivilegesToken, 6 ),
	IMP( NtAlertResumeThread, 2 ),
	IMP( NtAlertThread, 1 ),
	IMP( NtAllocateLocallyUniqueId, 1 ),
	DEC( NtAllocateUserPhysicalPages, 3 ),
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
	IMP( NtCloseObjectAuditAlarm, 3 ),
#ifdef SYSCALL_WINXP
	NUL( NtCompactKeys ),
	NUL( NtCompareTokens ),
#endif
	IMP( NtCompleteConnectPort, 1 ),
#ifdef SYSCALL_WINXP
	NUL( NtCompressKey ),
#endif
	IMP( NtConnectPort, 8 ),
	IMP( NtContinue, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtCreateDebugObject ),
#endif
	IMP( NtCreateDirectoryObject, 3 ),
	IMP( NtCreateEvent, 5 ),
	IMP( NtCreateEventPair, 3 ),
	IMP( NtCreateFile, 11 ),
	IMP( NtCreateIoCompletion, 4 ),
	IMP( NtCreateJobObject, 3 ),
#ifdef SYSCALL_WINXP
	NUL( NtCreateJobSet ),
#endif
	IMP( NtCreateKey, 7 ),
	IMP( NtCreateMailslotFile, 8 ),
	IMP( NtCreateMutant, 4 ),
	IMP( NtCreateNamedPipeFile, 14 ),
	IMP( NtCreatePagingFile, 4 ),
	IMP( NtCreatePort, 5 ),
	IMP( NtCreateProcess, 8 ),
#ifdef SYSCALL_WINXP
	NUL( NtCreateProcessEx ),
#endif
	DEC( NtCreateProfile, 9 ),
	IMP( NtCreateSection, 7 ),
	IMP( NtCreateSemaphore, 5 ),
	IMP( NtCreateSymbolicLinkObject, 4 ),
	IMP( NtCreateThread, 8 ),
	IMP( NtCreateTimer, 4 ),
	DEC( NtCreateToken, 13 ),
	DEC( NtCreateWaitablePort, 5 ),
#ifdef SYSCALL_WINXP
	NUL( NtDebugActiveProcess ),
	NUL( NtDebugContinue ),
#endif
	IMP( NtDelayExecution, 2 ),
	IMP( NtDeleteAtom, 1 ),
#ifdef SYSCALL_WINXP
	NUL( NtDeleteBootEntry ),
#endif
	IMP( NtDeleteFile, 1 ),
	IMP( NtDeleteKey, 1 ),
	NUL( NtDeleteObjectAuditAlarm ),
	IMP( NtDeleteValueKey, 2 ),
	IMP( NtDeviceIoControlFile, 10 ),
	IMP( NtDisplayString, 1 ),
	IMP( NtDuplicateObject, 7 ),
	IMP( NtDuplicateToken, 6 ),
#ifdef SYSCALL_WINXP
	NUL( NtEnumerateBootEntries ),
#endif
	IMP( NtEnumerateKey, 6 ),
#ifdef SYSCALL_WINXP
	NUL( NtEnumerateSystemEnvironmentValuesEx ),
#endif
	IMP( NtEnumerateValueKey, 6 ),
	DEC( NtExtendSection, 2 ),
	IMP( NtFilterToken, 6 ),
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
#ifdef SYSCALL_WIN2K
	DEC( NtGetTickCount, 0 ),
#endif
	DEC( NtGetWriteWatch, 7 ),
	DEC( NtImpersonateAnonymousToken, 1 ),
	DEC( NtImpersonateClientOfPort, 2 ),
	IMP( NtImpersonateThread, 3 ),
	IMP( NtInitializeRegistry, 1 ),
	IMP( NtInitiatePowerAction, 4 ),
#ifdef SYSCALL_WINXP
	NUL( NtIsProcessInJob ),
#endif
	IMP( NtIsSystemResumeAutomatic, 0 ),
	IMP( NtListenPort, 2 ),
	IMP( NtLoadDriver, 1 ),
	IMP( NtLoadKey, 2 ),
	NUL( NtLoadKey2 ),
	IMP( NtLockFile, 10 ),
#ifdef SYSCALL_WINXP
	NUL( NtLockProductActivationKeys ),
	NUL( NtLockRegistryKey ),
#endif
	IMP( NtLockVirtualMemory, 4 ),
#ifdef SYSCALL_WINXP
	NUL( NtMakePermanentObject ),
#endif
	DEC( NtMakeTemporaryObject, 1 ),
	DEC( NtMapUserPhysicalPages, 3 ),
	NUL( NtMapUserPhysicalPagesScatter ),
	IMP( NtMapViewOfSection, 10 ),
#ifdef SYSCALL_WINXP
	NUL( NtModifyBootEntry ),
#endif
	DEC( NtNotifyChangeDirectoryFile, 9 ),
	IMP( NtNotifyChangeKey, 10 ),
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
#ifdef SYSCALL_WINXP
	NUL( NtOpenProcessTokenEx ),
#endif
	IMP( NtOpenSection, 3 ),
	IMP( NtOpenSemaphore, 3 ),
	IMP( NtOpenSymbolicLinkObject, 3 ),
	DEC( NtOpenThread, 4 ),
	IMP( NtOpenThreadToken, 4 ),
#ifdef SYSCALL_WINXP
	NUL( NtOpenThreadTokenEx ),
#endif
	IMP( NtOpenTimer, 3 ),
	NUL( NtPlugPlayControl ),
	IMP( NtPowerInformation, 5 ),
	IMP( NtPrivilegeCheck, 3 ),
	IMP( NtPrivilegedServiceAuditAlarm, 5 ),
	IMP( NtPrivilegeObjectAuditAlarm, 6 ),
	IMP( NtProtectVirtualMemory, 5 ),
	IMP( NtPulseEvent, 2 ),
#ifdef SYSCALL_WIN2K
	IMP( NtQueryInformationAtom, 5 ),
#endif
	IMP( NtQueryAttributesFile, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtQueryBootOrder ),
	NUL( NtQueryBootOptions ),
	IMP( NtQueryDebugFilterState, 2 ),
#endif
	IMP( NtQueryDefaultLocale, 2 ),
	IMP( NtQueryDefaultUILanguage, 1 ),
	IMP( NtQueryDirectoryFile, 11 ),
	IMP( NtQueryDirectoryObject, 7 ),
	DEC( NtQueryEaFile, 9 ),
	IMP( NtQueryEvent, 5 ),
	IMP( NtQueryFullAttributesFile, 2 ),
#ifdef SYSCALL_WINXP
	IMP( NtQueryInformationAtom, 5 ),
#endif
	IMP( NtQueryInformationFile, 5 ),
	IMP( NtQueryInformationJobObject, 5 ),
#ifdef SYSCALL_WIN2K
	DEC( NtQueryIoCompletion, 5 ),
#endif
	IMP( NtQueryInformationPort, 5 ),
	IMP( NtQueryInformationProcess, 5 ),
	IMP( NtQueryInformationThread, 5 ),
	IMP( NtQueryInformationToken, 5 ),
	IMP( NtQueryInstallUILanguage, 1 ),
	DEC( NtQueryIntervalProfile, 2 ),
#ifdef SYSCALL_WINXP
	DEC( NtQueryIoCompletion, 5 ),
#endif
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
#ifdef SYSCALL_WINXP
	NUL( NtQuerySystemEnvironmentValueEx ),
#endif
	IMP( NtQuerySystemInformation, 4 ),
	IMP( NtQuerySystemTime, 1 ),
	IMP( NtQueryTimer, 5 ),
	IMP( NtQueryTimerResolution, 3 ),
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
#ifdef SYSCALL_WINXP
	NUL( NtRemoveProcessDebug ),
	NUL( NtRenameKey ),
#endif
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
#ifdef SYSCALL_WINXP
	NUL( NtResumeProcess ),
#endif
	IMP( NtResumeThread, 2 ),
	IMP( NtSaveKey, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtSaveKeyEx ),
#endif
	IMP( NtSaveMergedKeys, 3 ),
	IMP( NtSecureConnectPort, 9 ),
#ifdef SYSCALL_WINXP
	NUL( NtSetBootEntryOrder ),
	NUL( NtSetBootOptions ),
#endif
#ifdef SYSCALL_WIN2K
	IMP( NtSetIoCompletion, 5 ),
#endif
	IMP( NtSetContextThread, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtSetDebugFilterState ),
#endif
	IMP( NtSetDefaultHardErrorPort, 1 ),
	DEC( NtSetDefaultLocale, 2 ),
	DEC( NtSetDefaultUILanguage, 1 ),
	DEC( NtSetEaFile, 4 ),
	IMP( NtSetEvent, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtSetEventBoostPriority ),
#endif
	IMP( NtSetHighEventPair, 1 ),
	IMP( NtSetHighWaitLowEventPair, 1 ),
#ifdef SYSCALL_WINXP
	NUL( NtSetInformationDebugObject ),
#endif
	IMP( NtSetInformationFile, 5 ),
	IMP( NtSetInformationJobObject, 4 ),
	DEC( NtSetInformationKey, 4 ),
	IMP( NtSetInformationObject, 4 ),
	IMP( NtSetInformationProcess, 4 ),
	IMP( NtSetInformationThread, 4 ),
	DEC( NtSetInformationToken, 4 ),
	DEC( NtSetIntervalProfile, 2 ),
#ifdef SYSCALL_WINXP
	IMP( NtSetIoCompletion, 5 ),
#endif
	DEC( NtSetLdtEntries, 4 ),
	IMP( NtSetLowEventPair, 1 ),
	IMP( NtSetLowWaitHighEventPair, 1 ),
	IMP( NtSetQuotaInformationFile, 4 ),
	IMP( NtSetSecurityObject, 3 ),
	DEC( NtSetSystemEnvironmentValue, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtSetSystemEnvironmentValueEx ),
#endif
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
#ifdef SYSCALL_WINXP
	NUL( NtSuspendProcess ),
#endif
	IMP( NtSuspendThread, 2 ),
	DEC( NtSystemDebugControl, 6 ),
	IMP( NtTerminateJobObject, 2 ),
	IMP( NtTerminateProcess, 2 ),
	IMP( NtTerminateThread, 2 ),
	IMP( NtTestAlert, 0 ),
#ifdef SYSCALL_WINXP
	NUL( NtTraceEvent ),
	NUL( NtTranslateFilePath ),
#endif
	IMP( NtUnloadDriver, 1 ),
	IMP( NtUnloadKey, 1 ),
#ifdef SYSCALL_WINXP
	NUL( NtUnloadKeyEx ),
#endif
	IMP( NtUnlockFile, 5 ),
	DEC( NtUnlockVirtualMemory, 4 ),
	IMP( NtUnmapViewOfSection, 2 ),
	DEC( NtVdmControl, 2 ),
#ifdef SYSCALL_WINXP
	NUL( NtWaitForDebugEvent ),
#endif
	IMP( NtWaitForMultipleObjects, 5 ),
	IMP( NtWaitForSingleObject, 3 ),
	IMP( NtWaitHighEventPair, 1 ),
	IMP( NtWaitLowEventPair, 1 ),
	IMP( NtWriteFile, 9 ),
	DEC( NtWriteFileGather, 9 ),
	DEC( NtWriteRequestData, 6 ),
	IMP( NtWriteVirtualMemory, 5 ),
#ifdef SYSCALL_WIN2K
	NUL( NtCreateChannel ),
	NUL( NtListenChannel ),
	NUL( NtOpenChannel ),
	NUL( NtReplyWaitSendChannel ),
	NUL( NtSendWaitReplyChannel ),
	NUL( NtSendContextChannel ),
#endif
	IMP( NtYieldExecution, 0 ),
#ifdef SYSCALL_WINXP
	NUL( NtCreateKeyedEvent ),
	IMP( NtOpenKeyedEvent, 3 ),
	NUL( NtReleaseKeyedEvent ),
	NUL( NtWaitForKeyedEvent ),
	NUL( NtQueryPortInformationProcess )
#endif
