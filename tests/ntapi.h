#ifndef __NTAPI_H__
#define __NTAPI_H__

#include <windef.h>
#include <winnt.h>
#include <ntdll.h>
#include <ntdef.h>

#define STATUS_USER_APC                  ((NTSTATUS) 0x000000C0)
#define STATUS_ALERTED                   ((NTSTATUS) 0x00000101)
#define STATUS_TIMEOUT                   ((NTSTATUS) 0x00000102)
#define STATUS_PENDING                   ((NTSTATUS) 0x00000103)

#define STATUS_OBJECT_NAME_EXISTS        ((NTSTATUS) 0x40000000)

#define STATUS_DATATYPE_MISALIGNMENT     ((NTSTATUS) 0x80000002)
#define STATUS_BREAKPOINT                ((NTSTATUS) 0x80000003)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS) 0x80000005)
#define STATUS_NO_MORE_ENTRIES           ((NTSTATUS) 0x8000001A)

#define STATUS_NO_MORE_FILES             ((NTSTATUS) 0x80000006)
#define STATUS_PARTIAL_COPY              ((NTSTATUS) 0x8000000D)

#define STATUS_UNSUCCESSFUL              ((NTSTATUS) 0xC0000001)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS) 0xC0000002)
#define STATUS_INVALID_INFO_CLASS        ((NTSTATUS) 0xC0000003)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS) 0xC0000004)
#define STATUS_ACCESS_VIOLATION          ((NTSTATUS) 0xC0000005)
#define STATUS_INVALID_HANDLE            ((NTSTATUS) 0xC0000008)
#define STATUS_INVALID_CID               ((NTSTATUS) 0xC000000B)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS) 0xC000000D)
#define STATUS_NO_SUCH_FILE              ((NTSTATUS) 0xC000000F)
#define STATUS_NO_MEMORY                 ((NTSTATUS) 0xC0000017)
#define STATUS_CONFLICTING_ADDRESSES     ((NTSTATUS) 0xC0000018)
#define STATUS_UNABLE_TO_FREE_VM         ((NTSTATUS) 0xC000001A)
#define STATUS_INVALID_SYSTEM_SERVICE    ((NTSTATUS) 0xC000001C)
#define STATUS_ILLEGAL_INSTRUCTION       ((NTSTATUS) 0xC000001D)
#define STATUS_INVALID_FILE_FOR_SECTION  ((NTSTATUS) 0xC0000020)
#define STATUS_ACCESS_DENIED             ((NTSTATUS) 0xC0000022)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS) 0xC0000023)
#define STATUS_OBJECT_TYPE_MISMATCH      ((NTSTATUS) 0xC0000024)
#define STATUS_NOT_COMMITTED             ((NTSTATUS) 0xC000002D)
#define STATUS_INVALID_DISPOSITION       ((NTSTATUS) 0xC0000026)
#define STATUS_PORT_MESSAGE_TOO_LONG     ((NTSTATUS) 0xC000002F)
#define STATUS_INVALID_PARAMETER_MIX     ((NTSTATUS) 0xC0000030)
#define STATUS_OBJECT_NAME_INVALID       ((NTSTATUS) 0xC0000033)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS) 0xC0000034)
#define STATUS_OBJECT_NAME_COLLISION     ((NTSTATUS) 0xC0000035)
#define STATUS_OBJECT_PATH_NOT_FOUND     ((NTSTATUS) 0xC000003A)
#define STATUS_OBJECT_PATH_SYNTAX_BAD    ((NTSTATUS) 0xC000003B)
#define STATUS_PORT_CONNECTION_REFUSED   ((NTSTATUS) 0xC0000041)
#define STATUS_INVALID_PORT_HANDLE       ((NTSTATUS) 0xC0000042)
#define STATUS_INVALID_PAGE_PROTECTION   ((NTSTATUS) 0xC0000045)
#define STATUS_MUTANT_NOT_OWNED          ((NTSTATUS) 0xC0000046)
#define STATUS_SEMAPHORE_LIMIT_EXCEEDED  ((NTSTATUS) 0xC0000047)
#define STATUS_PORT_ALREADY_SET          ((NTSTATUS) 0xC0000048)
#define STATUS_EA_CORRUPT_ERROR          ((NTSTATUS) 0xC0000053)
#define STATUS_NO_TOKEN                  ((NTSTATUS) 0xC000007C)
#define STATUS_MEMORY_NOT_ALLOCATED      ((NTSTATUS) 0xC00000A0)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS) 0xC00000BB)
#define STATUS_INVALID_PARAMETER_1       ((NTSTATUS) 0xC00000EF)
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS) 0xC00000F0)
#define STATUS_INVALID_PARAMETER_3       ((NTSTATUS) 0xC00000F1)
#define STATUS_INVALID_PARAMETER_4       ((NTSTATUS) 0xC00000F2)
#define STATUS_INVALID_PARAMETER_5       ((NTSTATUS) 0xC00000F3)
#define STATUS_INVALID_PARAMETER_6       ((NTSTATUS) 0xC00000F4)
#define STATUS_REPLY_MESSAGE_MISMATCH    ((NTSTATUS) 0xC000021F)

#define FSCTL_IS_VOLUME_MOUNTED 0x00090028

#define NtCurrentProcess() ((HANDLE)-1)
#define NtCurrentThread() ((HANDLE)-2)

#define EVENT_QUERY_STATE          0x0001
#define EVENT_MODIFY_STATE         0x0002
#define EVENT_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

#define SEMAPHORE_MODIFY_STATE     0x0002
#define SEMAPHORE_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

#define SEC_BASED    0x00200000
#define SEC_NOCHANGE 0x00400000
#define SEC_FILE     0x00800000
#define SEC_IMAGE    0x01000000
#define SEC_VLM      0x02000000
#define SEC_RESERVE  0x04000000
#define SEC_COMMIT   0x08000000
#define SEC_NOCACHE  0x10000000

#define PAGE_SIZE 0x1000

#define SECURITY_LOCAL_SERVICE_RID              0x00000013L

#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_LINK 6
#define REG_MULTI_SZ 7

#define DIRECTORY_QUERY (0x0001)
#define DIRECTORY_TRAVERSE (0x0002)
#define DIRECTORY_CREATE_OBJECT (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

typedef enum _WAIT_TYPE {
	WaitAll,
	WaitAny,
} WAIT_TYPE;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataLength;
	UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataOffset;
	ULONG DataLength;
	ULONG NameLength;
	WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef enum _EVENT_TYPE {
	NotificationEvent,
	SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation
} MEMORY_INFORMATION_CLASS, *PMEMORY_INFORMATION_CLASS;

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation = 0,
	SystemCpuInformation = 1,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemProcessInformation = 5,
	SystemProcessorPerformanceInformation = 8,
	SystemModuleInformation = 11,
	SystemHandleInformation = 16,
	SystemPageFileInformation = 18,
	SystemCacheInformation = 21,
	SystemInterruptInformation = 23,
	SystemDpcBehaviourInformation = 24,
	SystemFullMemoryInformation = 25,
	SystemNotImplemented6 = 25,
	SystemLoadImage = 26,
	SystemUnloadImage = 27,
	SystemTimeAdjustmentInformation = 28,
	SystemTimeAdjustment = 28,
	SystemSummaryMemoryInformation = 29,
	SystemNotImplemented7 = 29,
	SystemNextEventIdInformation = 30,
	SystemNotImplemented8 = 30,
	SystemEventIdsInformation = 31,
	SystemCrashDumpInformation = 32,
	SystemExceptionInformation = 33,
	SystemCrashDumpStateInformation = 34,
	SystemKernelDebuggerInformation = 35,
	SystemContextSwitchInformation = 36,
	SystemRegistryQuotaInformation = 37,
	SystemCurrentTimeZoneInformation = 44,
	SystemTimeZoneInformation = 44,
	SystemLookasideInformation = 45,
	SystemSetTimeSlipEvent = 46,
	SystemCreateSession = 47,
	SystemDeleteSession = 48,
	SystemInvalidInfoClass4 = 49,
	SystemRangeStartInformation = 50,
	SystemVerifierInformation = 51,
	SystemAddVerifier = 52,
	SystemSessionProcessesInformation   = 53,
	SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
	ProcessBasicInformation = 0,
	ProcessQuotaLimits = 1,
	ProcessIoCounters = 2,
	ProcessVmCounters = 3,
	ProcessTimes = 4,
	ProcessBasePriority = 5,
	ProcessRaisePriority = 6,
	ProcessDebugPort = 7,
	ProcessExceptionPort = 8,
	ProcessAccessToken = 9,
	ProcessLdtInformation = 10,
	ProcessLdtSize = 11,
	ProcessDefaultHardErrorMode = 12,
	ProcessIoPortHandlers = 13,
	ProcessPooledUsageAndLimits = 14,
	ProcessWorkingSetWatch = 15,
	ProcessUserModeIOPL = 16,
	ProcessEnableAlignmentFaultFixup = 17,
	ProcessPriorityClass = 18,
	ProcessWx86Information = 19,
	ProcessHandleCount = 20,
	ProcessAffinityMask = 21,
	ProcessPriorityBoost = 22,
	ProcessDeviceMap = 23,
	ProcessSessionInformation = 24,
	ProcessForegroundInformation = 25,
	ProcessWow64Information = 26,
	ProcessImageFileName = 27,
	ProcessLUIDDeviceMapsEnabled = 28,
	ProcessBreakOnTermination = 29,
	ProcessDebugObjectHandle = 30,
	ProcessDebugFlags = 31,
	ProcessHandleTracing = 32,
	ProcessIoPriority = 33,
	ProcessExecuteFlags = 34,
	ProcessTlsInformation = 35,
	ProcessCookie = 36,
	ProcessImageInformation = 37,
	MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_PERMANENT 0x08

typedef enum _THREADINFOCLASS {
	ThreadBasicInformation,
	ThreadTimes,
	ThreadPriority,
	ThreadBasePriority,
	ThreadAffinityMask,
	ThreadImpersonationToken,
	ThreadDescriptorTableEntry,
	ThreadEnableAlignmentFaultFixup,
	ThreadEventPair_Reusable,
	ThreadQuerySetWin32StartAddress,
	ThreadZeroTlsCell,
	ThreadPerformanceCount,
	ThreadAmILastThread,
	ThreadIdealProcessor,
	ThreadPriorityBoost,
	ThreadSetTlsArrayAddress,
	ThreadIsIoPending,
	MaxThreadInfoClass
} THREADINFOCLASS;

typedef struct _KERNEL_USER_TIMES {
	LARGE_INTEGER CreateTime;
	LARGE_INTEGER ExitTime;
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES;

typedef enum _SECTION_INFORMATION_CLASS {
	SectionBasicInformation,
	SectionImageInformation,
} SECTION_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION {
	DWORD dwUnknown1;
	ULONG uKeMaximumIncrement;
	ULONG uPageSize;
	ULONG uMmNumberOfPhysicalPages;
	ULONG uMmLowestPhysicalPage;
	ULONG uMmHighestPhysicalPage;
	ULONG uAllocationGranularity;
	PVOID pLowestUserAddress;
	PVOID pMmHighestUserAddress;
	ULONG uKeActiveProcessors;
	ULONG uKeNumberProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_CPU_INFORMATION {
	WORD Architecture;
	WORD Level;
	WORD Revision;
	WORD Reserved;
	DWORD FeatureSet;
} SYSTEM_CPU_INFORMATION, *PSYSTEM_CPU_INFORMATION;

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION {
	LARGE_INTEGER BootTime;
	LARGE_INTEGER CurrentTime;
	LARGE_INTEGER TimeZoneBias;
	ULONG CurrentTimeZoneId;
	ULONG unknown[5];
} SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;

typedef struct SYSTEM_RANGE_START_INFORMATION {
	PVOID SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, *PSYSTEM_RANGE_START_INFORMATION;

typedef void *PPEB;
typedef LONG KPRIORITY;
typedef ULONG_PTR KAFFINITY, *PKAFFINITY;

typedef struct _PROCESS_BASIC_INFORMATION {
	NTSTATUS ExitStatus;
	PPEB PebBaseAddress;
	KAFFINITY AffinityMask;
	KPRIORITY BasePriority;
	ULONG UniqueProcessId;
	ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _THREAD_BASIC_INFORMATION {
	NTSTATUS  ExitStatus;
	PVOID	 TebBaseAddress;
	CLIENT_ID ClientId;
	ULONG	 AffinityMask;
	LONG	  Priority;
	LONG	  BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct RTL_DRIVE_LETTER_CURDIR {
	USHORT Flags;
	USHORT Length;
	ULONG TimeStamp;
	UNICODE_STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _CURDIR {
	UNICODE_STRING DosPath;
	PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG AllocationSize;
	ULONG Size;
	ULONG Flags;
	ULONG DebugFlags;
	HANDLE ConsoleHandle;
	ULONG ConsoleFlags;
	HANDLE hStdInput;
	HANDLE hStdOutput;
	HANDLE hStdError;
	CURDIR CurrentDirectory;
	UNICODE_STRING DllPath;
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
	PWSTR Environment;
	ULONG dwX;
	ULONG dwY;
	ULONG dwXSize;
	ULONG dwYSize;
	ULONG dwXCountChars;
	ULONG dwYCountChars;
	ULONG dwFillAttribute;
	ULONG dwFlags;
	ULONG wShowWindow;
	UNICODE_STRING WindowTitle;
	UNICODE_STRING Desktop;
	UNICODE_STRING ShellInfo;
	UNICODE_STRING RuntimeInfo;
	RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _SECTION_BASIC_INFORMATION {
	PVOID BaseAddress;
	ULONG Attributes;
	LARGE_INTEGER Size;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

typedef struct _SECTION_IMAGE_INFORMATION {
	PVOID EntryPoint;
	ULONG StackZeroBits;
	ULONG StackReserved;
	ULONG StackCommit;
	ULONG ImageSubsystem;
	WORD SubsystemVersionLow;
	WORD SubsystemVersionHigh;
	ULONG Unknown1;
	ULONG ImageCharacteristics;
	ULONG ImageMachineType;
	ULONG Unknown2[3];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

typedef struct _VM_COUNTERS {
	SIZE_T		PeakVirtualSize;
	SIZE_T		VirtualSize;
	ULONG		 PageFaultCount;
	SIZE_T		PeakWorkingSetSize;
	SIZE_T		WorkingSetSize;
	SIZE_T		QuotaPeakPagedPoolUsage;
	SIZE_T		QuotaPagedPoolUsage;
	SIZE_T		QuotaPeakNonPagedPoolUsage;
	SIZE_T		QuotaNonPagedPoolUsage;
	SIZE_T		PagefileUsage;
	SIZE_T		PeakPagefileUsage;
} VM_COUNTERS;

typedef enum _THREAD_STATE {
	StateInitialized,
	StateReady,
	StateRunning,
	StateStandby,
	StateTerminated,
	StateWait,
	StateTransition,
	StateUnknown
} THREAD_STATE;

typedef enum _KWAIT_REASON {
	Executive,
	FreePage,
	PageIn,
	PoolAllocation,
	DelayExecution,
	Suspended,
	UserRequest,
	WrExecutive,
	WrFreePage,
	WrPageIn,
	WrPoolAllocation,
	WrDelayExecution,
	WrSuspended,
	WrUserRequest,
	WrEventPair,
	WrQueue,
	WrLpcReceive,
	WrLpcReply,
	WrVertualMemory,
	WrPageOut,
	WrRendezvous,
	Spare2,
	Spare3,
	Spare4,
	Spare5,
	Spare6,
	WrKernel
} KWAIT_REASON;

typedef struct _SYSTEM_THREAD_INFORMATION {
	LARGE_INTEGER   KernelTime;
	LARGE_INTEGER   UserTime;
	LARGE_INTEGER   CreateTime;
	ULONG		   WaitTime;
	PVOID		   StartAddress;
	CLIENT_ID	   ClientId;
	KPRIORITY	   Priority;
	KPRIORITY	   BasePriority;
	ULONG		   ContextSwitchCount;
	LONG			State;
	LONG			WaitReason;
} SYSTEM_THREAD_INFORMATION, * PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG		   NextEntryDelta;
	ULONG		   ThreadCount;
	ULONG		   Reserved1[6];
	LARGE_INTEGER   CreateTime;
	LARGE_INTEGER   UserTime;
	LARGE_INTEGER   KernelTime;
	UNICODE_STRING  ProcessName;
	KPRIORITY	   BasePriority;
	ULONG		   ProcessId;
	ULONG		   InheritedFromProcessId;
	ULONG		   HandleCount;
	ULONG		   Reserved2[2];
	VM_COUNTERS	 VmCounters;
//#if _WIN32_WINNT >= 0x500
	IO_COUNTERS	 IoCounters;
//#endif
	SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _KSYSTEM_TIME {
	ULONG LowPart;
	LONG High1Time;
	LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE {
	StandardDesign,
	NEC98x86,
	EndAlternatives,
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _INITIAL_TEB {
	PVOID StackBase;
	PVOID StackLimit;
	PVOID StackCommit;
	PVOID StackCommitMax;
	PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _KUSER_SHARED_DATA {
	UINT  TickCountLow;
	UINT  TickCountMultiplier;
	KSYSTEM_TIME  InterruptTime;
	KSYSTEM_TIME  SystemTime;
	KSYSTEM_TIME  TimeZoneBias;
	USHORT ImageNumberLow;
	USHORT ImageNumberHigh;
	WCHAR WindowsDirectory[MAX_PATH];
	UINT MaxStackTraceDepth;
	UINT CryptoExponent;
	UINT TimeZoneId;
	UINT Reserved2[8];
	UINT NtProductType;
	BOOLEAN ProductIsValid;
	UINT NtMajorVersion;
	UINT NtMinorVersion;
	BOOLEAN ProcessorFeatures[64];
	UINT Reserved1;
	UINT Reserved3;
	UINT TimeSlip;
	ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;
	LARGE_INTEGER SystemExpirationDate;
	UINT SuiteMask;
	BOOLEAN KdDebuggerEnabled;
	BOOLEAN NXSupportPolicy;
	UINT ActiveConsoleId;
	UINT DismountCount;
	UINT ComPlusPackage;
	UINT LastSystemRITEventTickCount;
	UINT NumberOfPhysicalPages;
	BOOLEAN SafeBootMode;
	UINT TraceLogging;
	ULONGLONG TestRetInstruction;
	ULONGLONG SystemCall[4];
} KUSER_SHARED_DATA;

typedef struct _LPC_MESSAGE {
	USHORT DataSize;
	USHORT MessageSize;
	USHORT MessageType;
	USHORT VirtualRangesOffset;
	CLIENT_ID ClientId;
	ULONG MessageId;
	ULONG SectionSize;
	UCHAR Data[ANYSIZE_ARRAY];
} LPC_MESSAGE, *PLPC_MESSAGE;

typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;

typedef struct _LPC_SECTION_WRITE {
	ULONG Length;
	HANDLE SectionHandle;
	ULONG SectionOffset;
	ULONG ViewSize;
	PVOID ViewBase;
	PVOID TargetViewBase;
} LPC_SECTION_WRITE, *PLPC_SECTION_WRITE;

typedef struct _LPC_SECTION_READ {
	ULONG Length;
	ULONG ViewSize;
	PVOID ViewBase;
} LPC_SECTION_READ, *PLPC_SECTION_READ;

typedef enum _LPC_TYPE {
	LPC_NEW_MESSAGE,
	LPC_REQUEST,
	LPC_REPLY,
	LPC_DATAGRAM,
	LPC_LOST_REPLY,
	LPC_PORT_CLOSED,
	LPC_CLIENT_DIED,
	LPC_EXCEPTION,
	LPC_DEBUG_EVENT,
	LPC_ERROR_EVENT,
	LPC_CONNECTION_REQUEST
} LPC_TYPE;

#define EH_NONCONTINUABLE       0x01
#define EH_UNWINDING            0x02
#define EH_EXIT_UNWIND          0x04
#define EH_STACK_INVALID        0x08
#define EH_NESTED_CALL          0x10

typedef enum {
	ExceptionContinueExecution,
	ExceptionContinueSearch,
	ExceptionNestedException,
	ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;

typedef EXCEPTION_DISPOSITION (*PEXCEPTION_HANDLER)(struct _EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);
typedef struct _EXCEPTION_REGISTRATION {
	struct _EXCEPTION_REGISTRATION* prev;
	PEXCEPTION_HANDLER handler;
} EXCEPTION_REGISTRATION, *PEXCEPTION_REGISTRATION;

typedef EXCEPTION_REGISTRATION EXCEPTION_REGISTRATION_RECORD;
typedef PEXCEPTION_REGISTRATION PEXCEPTION_REGISTRATION_RECORD;

typedef void (NTAPI *PIO_APC_ROUTINE)(PVOID,PIO_STATUS_BLOCK,ULONG);
typedef void (NTAPI *PTIMER_APC_ROUTINE)(PVOID,ULONG,ULONG);
typedef void (NTAPI *PKNORMAL_ROUTINE)(PVOID,PVOID,PVOID);

typedef enum _ATOM_INFORMATION_CLASS {
	AtomBasicInformation,
	AtomListInformation,
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION {
	USHORT ReferenceCount;
	USHORT Pinned;
	USHORT NameLength;
	WCHAR  Name[1];
} ATOM_BASIC_INFORMATION;

typedef struct _ATOM_LIST_INFORMATION {
	ULONG NumberOfAtoms;
	USHORT Atoms[1];
} ATOM_LIST_INFORMATION;

typedef enum _TIMER_TYPE {
	NotificationTimer,
	SynchronizationTimer,
} TIMER_TYPE;

typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,
	FileFullDirectoryInformation,
	FileBothDirectoryInformation,
	FileBasicInformation,
	FileStandardInformation,
	FileInternalInformation,
	FileEaInformation,
	FileAccessInformation,
	FileNameInformation,
	FileRenameInformation,
	FileLinkInformation,
	FileNamesInformation,
	FileDispositionInformation,
	FilePositionInformation,
	FileFullEaInformation,
	FileModeInformation,
	FileAlignmentInformation,
	FileAllInformation,
	FileAllocationInformation,
	FileEndOfFileInformation,
	FileAlternateNameInformation,
	FileStreamInformation,
	FilePipeInformation,
	FilePipeLocalInformation,
	FilePipeRemoteInformation,
	FileMailslotQueryInformation,
	FileMailslotSetInformation,
	FileCompressionInformation,
	FileObjectIdInformation,
	FileCompletionInformation,
	FileMoveClusterInformation,
	FileQuotaInformation,
	FileReparsePointInformation,
	FileNetworkOpenInformation,
	FileAttributeTagInformation,
	FileTrackingInformation,
	FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

typedef struct _FILE_BOTH_DIRECTORY_INFORMATION {
	ULONG               NextEntryOffset;
	ULONG               FileIndex;
	LARGE_INTEGER       CreationTime;
	LARGE_INTEGER       LastAccessTime;
	LARGE_INTEGER       LastWriteTime;
	LARGE_INTEGER       ChangeTime;
	LARGE_INTEGER       EndOfFile;
	LARGE_INTEGER       AllocationSize;
	ULONG               FileAttributes;
	ULONG               FileNameLength;
	ULONG               EaSize;
	CHAR                ShortNameLength;
	WCHAR               ShortName[12];
	WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

#define FILE_SUPERSEDED     0
#define FILE_OPENED         1
#define FILE_CREATED        2
#define FILE_OVERWRITTEN    3
#define FILE_EXISTS         4
#define FILE_DOES_NOT_EXIST 5

#define FILE_DEVICE_IS_MOUNTED          0x00000020

typedef enum _TIMER_INFORMATION_CLASS {
	TimerBasicInformation,
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION {
	LARGE_INTEGER TimeRemaining;
	BOOLEAN SignalState;
} TIMER_BASIC_INFORMATION;

#define	SECURITY_DESCRIPTOR_REVISION	1
#define	SECURITY_DESCRIPTOR_REVISION1	1

typedef struct {
	BYTE Revision;
	BYTE Sbz1;
	SECURITY_DESCRIPTOR_CONTROL Control;
	DWORD Owner;
	DWORD Group;
	DWORD Sacl;
	DWORD Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyNameInformation,
} KEY_INFORMATION_CLASS;

typedef struct _KEY_FULL_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG TitleIndex;
	ULONG ClassOffset;
	ULONG ClassLength;
	ULONG SubKeys;
	ULONG MaxNameLen;
	ULONG MaxClassLen;
	ULONG Values;
	ULONG MaxValueNameLen;
	ULONG MaxValueDataLen;
	WCHAR Class[ANYSIZE_ARRAY];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

#define REG_CREATED_NEW_KEY 1
#define REG_OPENED_EXISTING_KEY 2

typedef struct _FILE_PIPE_WAIT_FOR_BUFFER {
    LARGE_INTEGER   Timeout;
    ULONG           NameLength;
    BOOLEAN         TimeoutSpecified;
    WCHAR           Name[1];
} FILE_PIPE_WAIT_FOR_BUFFER, *PFILE_PIPE_WAIT_FOR_BUFFER;

#define CTL_CODE(DeviceType, Function, Method, Access)( \
  ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))

#define FILE_DEVICE_NAMED_PIPE            0x00000011

#define FILE_ANY_ACCESS                   0x00000000

#define METHOD_BUFFERED                   0

#define FSCTL_PIPE_DISCONNECT	CTL_CODE(FILE_DEVICE_NAMED_PIPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_LISTEN	CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_WAIT		CTL_CODE(FILE_DEVICE_NAMED_PIPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

NTSTATUS NTAPI NtAddAtom(PWSTR,ULONG,PUSHORT);
NTSTATUS NTAPI NtAcceptConnectPort(PHANDLE,HANDLE,PLPC_MESSAGE,BOOLEAN,PLPC_SECTION_WRITE,PLPC_SECTION_READ);
NTSTATUS NTAPI NtAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,ULONG,PTOKEN_PRIVILEGES,PULONG);
NTSTATUS NTAPI NtAlertThread(HANDLE);
NTSTATUS NTAPI NtAllocateVirtualMemory(HANDLE,PVOID*,ULONG,PULONG,ULONG,ULONG);
NTSTATUS NTAPI NtCallbackReturn(PVOID,ULONG,NTSTATUS);
NTSTATUS NTAPI NtClearEvent(HANDLE);
NTSTATUS NTAPI NtClose(HANDLE);
NTSTATUS NTAPI NtCreateMailslotFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSTATUS NTAPI NtCreateTimer(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,TIMER_TYPE);
NTSTATUS NTAPI NtCreateNamedPipeFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,BOOLEAN,BOOLEAN,BOOLEAN,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSTATUS NTAPI NtCompleteConnectPort(HANDLE);
NTSTATUS NTAPI NtConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS NTAPI NtContinue(PCONTEXT,BOOLEAN);
NTSTATUS NTAPI NtCreateDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtCreateEvent(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,EVENT_TYPE,BOOLEAN);
NTSTATUS NTAPI NtCreateEventPair(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS NTAPI NtCreateIoCompletion(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG);
NTSTATUS NTAPI NtCreateKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG,PUNICODE_STRING,ULONG,PULONG);
NTSTATUS NTAPI NtCreateMutant(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN);
NTSTATUS NTAPI NtCreatePort(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,PULONG);
NTSTATUS NTAPI NtCreateProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,BOOLEAN,HANDLE,HANDLE,HANDLE);
NTSTATUS NTAPI NtCreateSection(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PLARGE_INTEGER,ULONG,ULONG,HANDLE);
NTSTATUS NTAPI NtCreateSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PUNICODE_STRING);
NTSTATUS NTAPI NtCreateSemaphore(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,LONG,LONG);
NTSTATUS NTAPI NtCreateThread(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,PCLIENT_ID,PCONTEXT,PINITIAL_TEB,BOOLEAN);
NTSTATUS NTAPI NtDelayExecution(BOOLEAN,PLARGE_INTEGER);
NTSTATUS NTAPI NtDeleteAtom(USHORT);
NTSTATUS NTAPI NtDeleteFile(POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtDeleteKey(HANDLE);
NTSTATUS NTAPI NtDeleteValueKey(HANDLE,PUNICODE_STRING);
NTSTATUS NTAPI NtDisplayString(PUNICODE_STRING);
NTSTATUS NTAPI NtEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS NTAPI NtFindAtom(PWSTR,ULONG,PUSHORT);
NTSTATUS NTAPI NtFreeVirtualMemory(HANDLE,PVOID*,PULONG,ULONG);
NTSTATUS NTAPI NtGetContextThread(HANDLE,PCONTEXT);
NTSTATUS NTAPI NtListenPort(HANDLE,PLPC_MESSAGE);
NTSTATUS NTAPI NtMapViewOfSection(HANDLE,HANDLE,PVOID*,ULONG,SIZE_T,LARGE_INTEGER*,SIZE_T*,SECTION_INHERIT,ULONG,ULONG);
NTSTATUS NTAPI NtOpenDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtOpenEvent(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSTATUS NTAPI NtOpenKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtOpenProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);
NTSTATUS NTAPI NtOpenProcessToken(HANDLE,ACCESS_MASK,PHANDLE);
NTSTATUS NTAPI NtOpenSection(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS NTAPI NtOpenThreadToken(HANDLE,ACCESS_MASK,BOOLEAN,PHANDLE);
NTSTATUS NTAPI NtPulseEvent(HANDLE,PULONG);
NTSTATUS NTAPI NtProtectVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG,ULONG*);
NTSTATUS NTAPI NtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
NTSTATUS NTAPI NtQueryDirectoryObject(HANDLE,PVOID,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG);
NTSTATUS NTAPI NtQueryInformationAtom(USHORT,ATOM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryInformationJobObject(HANDLE,JOBOBJECTINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryInformationProcess(HANDLE,PROCESS_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryTimer(HANDLE,TIMER_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryInformationToken(HANDLE,TOKEN_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryKey(HANDLE,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryValueKey(HANDLE,PUNICODE_STRING,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQuerySecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,ULONG,PULONG);
NTSTATUS NTAPI NtQuerySection(HANDLE,SECTION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSTATUS NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueryVirtualMemory(HANDLE,PVOID,MEMORY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtQueueApcThread(HANDLE,PKNORMAL_ROUTINE,PVOID,PVOID,PVOID);
NTSTATUS NTAPI NtQueryDefaultLocale(BOOLEAN,PLCID);
NTSTATUS NTAPI NtRaiseException(PEXCEPTION_RECORD,PCONTEXT,BOOLEAN);
NTSTATUS NTAPI NtReadVirtualMemory(HANDLE,LPCVOID,LPVOID,SIZE_T,SIZE_T*);
NTSTATUS NTAPI NtRegisterThreadTerminatePort(HANDLE);
NTSTATUS NTAPI NtRemoveIoCompletion(HANDLE,PULONG,PULONG,PIO_STATUS_BLOCK,PLARGE_INTEGER);
NTSTATUS NTAPI NtReplyPort(HANDLE,PLPC_MESSAGE);
NTSTATUS NTAPI NtReplyWaitReceivePort(HANDLE,PHANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS NTAPI NtRequestPort(HANDLE,PLPC_MESSAGE);
NTSTATUS NTAPI NtRequestWaitReplyPort(HANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS NTAPI NtReleaseMutant(HANDLE,PULONG);
NTSTATUS NTAPI NtReleaseSemaphore(HANDLE,ULONG,PULONG);
NTSTATUS NTAPI NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS NTAPI NtResetEvent(HANDLE,PULONG);
NTSTATUS NTAPI NtResumeThread(HANDLE,PULONG);
NTSTATUS NTAPI NtSecureConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PSID,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS NTAPI NtSetEvent(HANDLE,PULONG);
NTSTATUS NTAPI NtSetInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSTATUS NTAPI NtSetInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG);
NTSTATUS NTAPI NtSetInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG);
NTSTATUS NTAPI NtSetLowEventPair(HANDLE);
NTSTATUS NTAPI NtSetLowWaitHighEventPair(HANDLE);
NTSTATUS NTAPI NtSetHighEventPair(HANDLE);
NTSTATUS NTAPI NtSetHighWaitLowEventPair(HANDLE);
NTSTATUS NTAPI NtSetIoCompletion(HANDLE,ULONG,ULONG,NTSTATUS,ULONG);
NTSTATUS NTAPI NtSetTimer(HANDLE,PLARGE_INTEGER,PTIMER_APC_ROUTINE,PVOID,BOOLEAN,ULONG,PBOOLEAN);
NTSTATUS NTAPI NtSetContextThread(HANDLE,PCONTEXT);
NTSTATUS NTAPI NtSetValueKey(HANDLE,PUNICODE_STRING,ULONG,ULONG,PVOID,ULONG);
NTSTATUS NTAPI NtTerminateProcess(HANDLE,NTSTATUS);
NTSTATUS NTAPI NtTerminateThread(HANDLE,NTSTATUS);
NTSTATUS NTAPI NtTestAlert();
NTSTATUS NTAPI NtUnmapViewOfSection(HANDLE,PVOID);
NTSTATUS NTAPI NtWaitForSingleObject(HANDLE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG,PHANDLE,WAIT_TYPE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS NTAPI NtWaitLowEventPair(HANDLE);
NTSTATUS NTAPI NtWaitHighEventPair(HANDLE);
NTSTATUS NTAPI NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS NTAPI NtWriteVirtualMemory(HANDLE,PVOID,PVOID,ULONG,PULONG);
NTSTATUS NTAPI NtYieldExecution(void);

#endif
