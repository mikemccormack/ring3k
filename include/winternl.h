/*
 * Internal NT APIs and data structures
 *
 * Copyright (C) the Wine project
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

#ifndef __WINE_WINTERNL_H
#define __WINE_WINTERNL_H

#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


/**********************************************************************
 * Fundamental types and data structures
 */

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif

typedef CONST char *PCSZ;

typedef short CSHORT;
typedef CSHORT *PCSHORT;

#ifndef __STRING_DEFINED__
#define __STRING_DEFINED__
typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING, *PSTRING;
#endif

typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;
typedef const STRING *PCANSI_STRING;

typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;
typedef const STRING *PCOEM_STRING;

#ifndef __UNICODE_STRING_DEFINED__
#define __UNICODE_STRING_DEFINED__
typedef struct _UNICODE_STRING {
  USHORT Length;        /* bytes */
  USHORT MaximumLength; /* bytes */
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

typedef const UNICODE_STRING *PCUNICODE_STRING;

#ifndef _FILETIME_
#define _FILETIME_
/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct _FILETIME
{
#ifdef WORDS_BIGENDIAN
  DWORD  dwHighDateTime;
  DWORD  dwLowDateTime;
#else
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
#endif
} FILETIME, *PFILETIME, *LPFILETIME;
#endif /* _FILETIME_ */

/*
 * RTL_SYSTEM_TIME and RTL_TIME_ZONE_INFORMATION are the same as
 * the SYSTEMTIME and TIME_ZONE_INFORMATION structures defined
 * in winbase.h, however we need to define them separately so
 * winternl.h doesn't depend on winbase.h.  They are used by
 * RtlQueryTimeZoneInformation and RtlSetTimeZoneInformation.
 * The names are guessed; if anybody knows the real names, let me know.
 */
typedef struct _RTL_SYSTEM_TIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} RTL_SYSTEM_TIME, *PRTL_SYSTEM_TIME;

typedef struct _RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[32];
    RTL_SYSTEM_TIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    RTL_SYSTEM_TIME DaylightDate;
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION, *PRTL_TIME_ZONE_INFORMATION;

typedef struct _CLIENT_ID
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct RTL_DRIVE_LETTER_CURDIR
{
    USHORT              Flags;
    USHORT              Length;
    ULONG               TimeStamp;
    UNICODE_STRING      DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct tagRTL_BITMAP {
    ULONG  SizeOfBitMap; /* Number of bits in the bitmap */
    PULONG Buffer; /* Bitmap data, assumed sized to a DWORD boundary */
} RTL_BITMAP, *PRTL_BITMAP;

typedef const RTL_BITMAP *PCRTL_BITMAP;

typedef struct tagRTL_BITMAP_RUN {
    ULONG StartingIndex; /* Bit position at which run starts */
    ULONG NumberOfBits;  /* Size of the run in bits */
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

typedef const RTL_BITMAP_RUN *PCRTL_BITMAP_RUN;

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG               AllocationSize;
    ULONG               Size;
    ULONG               Flags;
    ULONG               DebugFlags;
    HANDLE              ConsoleHandle;
    ULONG               ConsoleFlags;
    HANDLE              hStdInput;
    HANDLE              hStdOutput;
    HANDLE              hStdError;
    CURDIR              CurrentDirectory;
    UNICODE_STRING      DllPath;
    UNICODE_STRING      ImagePathName;
    UNICODE_STRING      CommandLine;
    PWSTR               Environment;
    ULONG               dwX;
    ULONG               dwY;
    ULONG               dwXSize;
    ULONG               dwYSize;
    ULONG               dwXCountChars;
    ULONG               dwYCountChars;
    ULONG               dwFillAttribute;
    ULONG               dwFlags;
    ULONG               wShowWindow;
    UNICODE_STRING      WindowTitle;
    UNICODE_STRING      Desktop;
    UNICODE_STRING      ShellInfo;
    UNICODE_STRING      RuntimeInfo;
    RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/* value for Flags field (FIXME: not the correct name) */
#define PROCESS_PARAMS_FLAG_NORMALIZED 1

typedef struct _PEB_LDR_DATA
{
    ULONG               Length;
    BOOLEAN             Initialized;
    PVOID               SsHandle;
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _GDI_TEB_BATCH
{
    ULONG  Offset;
    HANDLE HDC;
    ULONG  Buffer[0x136];
} GDI_TEB_BATCH;

typedef struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME
{
    struct _RTL_ACTIVATION_CONTEXT_STACK_FRAME *Previous;
    struct _ACTIVATION_CONTEXT                 *ActivationContext;
    ULONG                                       Flags;
} RTL_ACTIVATION_CONTEXT_STACK_FRAME, *PRTL_ACTIVATION_CONTEXT_STACK_FRAME;

typedef struct _ACTIVATION_CONTEXT_STACK
{
    ULONG                               Flags;
    ULONG                               NextCookieSequenceNumber;
    RTL_ACTIVATION_CONTEXT_STACK_FRAME *ActiveFrame;
    LIST_ENTRY                          FrameListCache;
} ACTIVATION_CONTEXT_STACK, *PACTIVATION_CONTEXT_STACK;

// NtGlobalFlag flags
#define FLG_STOP_ON_EXCEPTION           0x00000001
#define FLG_SHOW_LDR_SNAPS              0x00000002
#define FLG_DEBUG_INITIAL_COMMAND       0x00000004
#define FLG_STOP_ON_HUNG_GUI            0x00000008
#define FLG_HEAP_ENABLE_TAIL_CHECK      0x00000010
#define FLG_HEAP_ENABLE_FREE_CHECK      0x00000020
#define FLG_HEAP_VALIDATE_PARAMETERS    0x00000040
#define FLG_HEAP_VALIDATE_ALL           0x00000080
#define FLG_POOL_ENABLE_TAIL_CHECK      0x00000100
#define FLG_POOL_ENABLE_FREE_CHECK      0x00000200
#define FLG_POOL_ENABLE_TAGGING         0x00000400
#define FLG_HEAP_ENABLE_TAGGING         0x00000800
#define FLG_USER_STACK_TRACE_DB         0x00001000
#define FLG_KERNEL_STACK_TRACE_DB       0x00002000
#define FLG_MAINTAIN_OBJECT_TYPELIST    0x00004000
#define FLG_HEAP_ENABLE_TAG_BY_DLL      0x00008000
#define FLG_IGNORE_DEBUG_PRIV           0x00010000
#define FLG_ENABLE_CSRDEBUG             0x00020000
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD   0x00040000
#define FLG_DISABLE_PAGE_KERNEL_STACKS  0x00080000
#define FLG_HEAP_ENABLE_CALL_TRACING    0x00100000
#define FLG_HEAP_DISABLE_COALESCING     0x00200000
#define FLG_VALID_BITS                  0x003FFFFF
#define FLG_ENABLE_CLOSE_EXCEPTION      0x00400000
#define FLG_ENABLE_EXCEPTION_LOGGING    0x00800000
#define FLG_ENABLE_HANDLE_TYPE_TAGGING  0x01000000
#define FLG_HEAP_PAGE_ALLOCS            0x02000000
#define FLG_DEBUG_WINLOGON              0x04000000
#define FLG_ENABLE_DBGPRINT_BUFFERING   0x08000000
#define FLG_EARLY_CRITICAL_SECTION_EVT  0x10000000
#define FLG_DISABLE_DLL_VERIFICATION    0x80000000

/***********************************************************************
 * PEB data structure
 */
typedef struct _PEB
{
    BOOLEAN                      InheritedAddressSpace;             /*  00 */
    BOOLEAN                      ReadImageFileExecOptions;          /*  01 */
    BOOLEAN                      BeingDebugged;                     /*  02 */
    BOOLEAN                      SpareBool;                         /*  03 */
    HANDLE                       Mutant;                            /*  04 */
    HMODULE                      ImageBaseAddress;                  /*  08 */
    PPEB_LDR_DATA                LdrData;                           /*  0c */
    RTL_USER_PROCESS_PARAMETERS *ProcessParameters;                 /*  10 */
    PVOID                        SubSystemData;                     /*  14 */
    HANDLE                       ProcessHeap;                       /*  18 */
    PRTL_CRITICAL_SECTION        FastPebLock;                       /*  1c */
    PVOID /*PPEBLOCKROUTINE*/    FastPebLockRoutine;                /*  20 */
    PVOID /*PPEBLOCKROUTINE*/    FastPebUnlockRoutine;              /*  24 */
    ULONG                        EnvironmentUpdateCount;            /*  28 */
    PVOID                        KernelCallbackTable;               /*  2c */
    PVOID                        EventLogSection;                   /*  30 */
    PVOID                        EventLog;                          /*  34 */
    PVOID /*PPEB_FREE_BLOCK*/    FreeList;                          /*  38 */
    ULONG                        TlsExpansionCounter;               /*  3c */
    PRTL_BITMAP                  TlsBitmap;                         /*  40 */
    ULONG                        TlsBitmapBits[2];                  /*  44 */
    PVOID                        ReadOnlySharedMemoryBase;          /*  4c */
    PVOID                        ReadOnlySharedMemoryHeap;          /*  50 */
    PVOID                       *ReadOnlyStaticServerData;          /*  54 */
    PVOID                        AnsiCodePageData;                  /*  58 */
    PVOID                        OemCodePageData;                   /*  5c */
    PVOID                        UnicodeCaseTableData;              /*  60 */
    ULONG                        NumberOfProcessors;                /*  64 */
    ULONG                        NtGlobalFlag;                      /*  68 */
    BYTE                         Spare2[4];                         /*  6c */
    LARGE_INTEGER                CriticalSectionTimeout;            /*  70 */
    ULONG                        HeapSegmentReserve;                /*  78 */
    ULONG                        HeapSegmentCommit;                 /*  7c */
    ULONG                        HeapDeCommitTotalFreeThreshold;    /*  80 */
    ULONG                        HeapDeCommitFreeBlockThreshold;    /*  84 */
    ULONG                        NumberOfHeaps;                     /*  88 */
    ULONG                        MaximumNumberOfHeaps;              /*  8c */
    PVOID                       *ProcessHeaps;                      /*  90 */
    PVOID                        GdiSharedHandleTable;              /*  94 */
    PVOID                        ProcessStarterHelper;              /*  98 */
    PVOID                        GdiDCAttributeList;                /*  9c */
    PVOID                        LoaderLock;                        /*  a0 */
    ULONG                        OSMajorVersion;                    /*  a4 */
    ULONG                        OSMinorVersion;                    /*  a8 */
    ULONG                        OSBuildNumber;                     /*  ac */
    ULONG                        OSPlatformId;                      /*  b0 */
    ULONG                        ImageSubSystem;                    /*  b4 */
    ULONG                        ImageSubSystemMajorVersion;        /*  b8 */
    ULONG                        ImageSubSystemMinorVersion;        /*  bc */
    ULONG                        ImageProcessAffinityMask;          /*  c0 */
    ULONG                        GdiHandleBuffer[34];               /*  c4 */
    ULONG                        PostProcessInitRoutine;            /* 14c */
    PRTL_BITMAP                  TlsExpansionBitmap;                /* 150 */
    ULONG                        TlsExpansionBitmapBits[32];        /* 154 */
    ULONG                        SessionId;                         /* 1d4 */
} PEB, *PPEB;


/***********************************************************************
 * TEB data structure
 */
#ifndef WINE_NO_TEB  /* don't define TEB if included from thread.h */
# ifndef WINE_TEB_DEFINED
# define WINE_TEB_DEFINED
typedef struct _TEB
{
    NT_TIB          Tib;                        /* 000 */
    PVOID           EnvironmentPointer;         /* 01c */
    CLIENT_ID       ClientId;                   /* 020 */
    PVOID           ActiveRpcHandle;            /* 028 */
    PVOID           ThreadLocalStoragePointer;  /* 02c */
    PPEB            Peb;                        /* 030 */
    ULONG           LastErrorValue;             /* 034 */
    ULONG           CountOfOwnedCriticalSections;/* 038 */
    PVOID           CsrClientThread;            /* 03c */
    PVOID           Win32ThreadInfo;            /* 040 */
    ULONG           Win32ClientInfo[31];        /* 044 */
    PVOID           WOW32Reserved;              /* 0c0 */
    ULONG           CurrentLocale;              /* 0c4 */
    ULONG           FpSoftwareStatusRegister;   /* 0c8 */
    PVOID           SystemReserved1[54];        /* 0cc */
    LONG            ExceptionCode;              /* 1a4 */
    ACTIVATION_CONTEXT_STACK ActivationContextStack; /* 1a8 */
    BYTE            SpareBytes1[24];            /* 1bc */
    PVOID           SystemReserved2[10];        /* 1d4 */
    GDI_TEB_BATCH   GdiTebBatch;                /* 1fc */
    ULONG           gdiRgn;                     /* 6dc */
    ULONG           gdiPen;                     /* 6e0 */
    ULONG           gdiBrush;                   /* 6e4 */
    union {
    ULONG           KernelUserPointerOffset;	/* 6e8 */
    CLIENT_ID       RealClientId;               /* 6e8 */
    };
    HANDLE          GdiCachedProcessHandle;     /* 6f0 */
    union {
    HANDLE          CachedWindowHandle;         /* 6f4 */
    ULONG           GdiClientPID;               /* 6f4 */
    };
    ULONG           GdiClientTID;               /* 6f8 */
    PVOID           GdiThreadLocaleInfo;        /* 6fc */
    PVOID           UserReserved[5];            /* 700 */
    PVOID           glDispachTable[280];        /* 714 */
    ULONG           glReserved1[26];            /* b74 */
    PVOID           glReserved2;                /* bdc */
    PVOID           glSectionInfo;              /* be0 */
    PVOID           glSection;                  /* be4 */
    PVOID           glTable;                    /* be8 */
    PVOID           glCurrentRC;                /* bec */
    PVOID           glContext;                  /* bf0 */
    ULONG           LastStatusValue;            /* bf4 */
    UNICODE_STRING  StaticUnicodeString;        /* bf8 used by advapi32 */
    WCHAR           StaticUnicodeBuffer[261];   /* c00 used by advapi32 */
    PVOID           DeallocationStack;          /* e0c */
    PVOID           TlsSlots[64];               /* e10 */
    LIST_ENTRY      TlsLinks;                   /* f10 */
    PVOID           Vdm;                        /* f18 */
    PVOID           ReservedForNtRpc;           /* f1c */
    PVOID           DbgSsReserved[2];           /* f20 */
    ULONG           HardErrorDisabled;          /* f28 */
    PVOID           Instrumentation[16];        /* f2c */
    PVOID           WinSockData;                /* f6c */
    ULONG           GdiBatchCount;              /* f70 */
    ULONG           Spare2;                     /* f74 */
    ULONG           Spare3;                     /* f78 */
    ULONG           Spare4;                     /* f7c */
    PVOID           ReservedForOle;             /* f80 */
    ULONG           WaitingOnLoaderLock;        /* f84 */
    PVOID           Reserved5[3];               /* f88 */
    PVOID          *TlsExpansionSlots;          /* f94 */
} TEB, *PTEB;
# endif /* WINE_TEB_DEFINED */
#endif  /* WINE_NO_TEB */

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
	ULONG SystemCall;
	ULONG SystemCallRet;
} KUSER_SHARED_DATA;

/***********************************************************************
 * Enums
 */

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

typedef struct _FILE_PIPE_INFORMATION {
	ULONG ReadModeMessage;
	ULONG WaitModeBlocking;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION {
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
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
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
    WCHAR               FileName[ANYSIZE_ARRAY];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION,
  FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

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
} FILE_BOTH_DIRECTORY_INFORMATION, *PFILE_BOTH_DIRECTORY_INFORMATION,
  FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN Replace;
    HANDLE RootDir;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION {
    LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
    ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION
{
    ULONG FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
    ULONG MaximumMessageSize;
    ULONG MailslotQuota;
    ULONG NextMessageSize;
    ULONG MessagesAvailable;
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
    LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
    ULONG NamedPipeType;
    ULONG NamedPipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG ReadDataAvailable;
    ULONG OutboundQuota;
    ULONG WriteQuotaAvailable;
    ULONG NamedPipeState;
    ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION     BasicInformation;
    FILE_STANDARD_INFORMATION  StandardInformation;
    FILE_INTERNAL_INFORMATION  InternalInformation;
    FILE_EA_INFORMATION        EaInformation;
    FILE_ACCESS_INFORMATION    AccessInformation;
    FILE_POSITION_INFORMATION  PositionInformation;
    FILE_MODE_INFORMATION      ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION      NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation = 1,
    FileFsLabelInformation,
    FileFsSizeInformation,
    FileFsDeviceInformation,
    FileFsAttributeInformation,
    FileFsControlInformation,
    FileFsFullSizeInformation,
    FileFsObjectIdInformation,
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;

typedef enum _OBJECT_INFORMATION_CLASS {
    ObjectBasicInformation,
    ObjectNameInformation,
    ObjectTypeInformation,
    ObjectAllTypesInformation,
    ObjectHandleInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

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
    ProcessExecuteFlags = 34,
    MaxProcessInfoClass
} PROCESSINFOCLASS, PROCESS_INFORMATION_CLASS;

#define MEM_EXECUTE_OPTION_DISABLE   0x01
#define MEM_EXECUTE_OPTION_ENABLE    0x02
#define MEM_EXECUTE_OPTION_PERMANENT 0x08

typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

typedef struct _PROCESS_SESSION_INFORMATION {
	ULONG SessionId;
} PROCESS_SESSION_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemCpuInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3, /* was SystemTimeInformation */
    Unknown4,
    SystemProcessInformation = 5,
    Unknown6,
    Unknown7,
    SystemProcessorPerformanceInformation = 8,
    SystemGlobalFlag = 9,
    Unknown10,
    SystemModuleInformation = 11,
    Unknown12,
    Unknown13,
    Unknown14,
    Unknown15,
    SystemHandleInformation = 16,
    Unknown17,
    SystemPageFileInformation = 18,
    Unknown19,
    Unknown20,
    SystemCacheInformation = 21,
    Unknown22,
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
    SystemSessionProcessesInformation	= 53,
    SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_CRASH_DUMP_STATE_INFORMATION {
	ULONG CrashDumpSectionExists;
	ULONG Unknown;
} SYSTEM_CRASH_DUMP_STATE_INFORMATION, PSYSTEM_CRASH_DUMP_STATE_INFORMATION;

typedef enum _TIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} TIMER_TYPE;

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

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    LONG      Priority;
    LONG      BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef struct _THREAD_DESCRIPTOR_INFORMATION
{
    DWORD       Selector;
    LDT_ENTRY   Entry;
} THREAD_DESCRIPTOR_INFORMATION, *PTHREAD_DESCRIPTOR_INFORMATION;

typedef struct _KERNEL_USER_TIMES {
    LARGE_INTEGER  CreateTime;
    LARGE_INTEGER  ExitTime;
    LARGE_INTEGER  KernelTime;
    LARGE_INTEGER  UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef enum _WINSTATIONINFOCLASS {
    WinStationInformation = 8
} WINSTATIONINFOCLASS;

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation,
    MemoryWorkingSetList,
    MemorySectionName,
    MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

typedef enum _MUTANT_INFORMATION_CLASS
{
    MutantBasicInformation
} MUTANT_INFORMATION_CLASS, *PMUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION {
    LONG        CurrentCount;
    BOOLEAN     OwnedByCaller;
    BOOLEAN     AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

typedef enum _TIMER_INFORMATION_CLASS
{
    TimerBasicInformation = 0
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION {
    LARGE_INTEGER TimeRemaining;
    BOOLEAN       SignalState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

/* return type of RtlDetermineDosPathNameType_U (FIXME: not the correct names) */
typedef enum
{
    INVALID_PATH = 0,
    UNC_PATH,              /* "//foo" */
    ABSOLUTE_DRIVE_PATH,   /* "c:/foo" */
    RELATIVE_DRIVE_PATH,   /* "c:foo" */
    ABSOLUTE_PATH,         /* "/foo" */
    RELATIVE_PATH,         /* "foo" */
    DEVICE_PATH,           /* "//./foo" */
    UNC_DOT_PATH           /* "//." */
} DOS_PATHNAME_TYPE;

/***********************************************************************
 * IA64 specific types and data structures
 */

#ifdef __ia64__

typedef struct _FRAME_POINTERS {
  ULONGLONG MemoryStackFp;
  ULONGLONG BackingStoreFp;
} FRAME_POINTERS, *PFRAME_POINTERS;

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _RUNTIME_FUNCTION {
  ULONG BeginAddress;
  ULONG EndAddress;
  ULONG UnwindInfoAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
  ULONG64 ImageBase;
  ULONG64 Gp;
  PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE {
  ULONG Count;
  UCHAR Search;
  ULONG64 LowAddress;
  ULONG64 HighAddress;
  UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

#endif /* defined(__ia64__) */

/***********************************************************************
 * Types and data structures
 */

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

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny,
} WAIT_TYPE;

/* This is used by NtQuerySystemInformation */
typedef struct _SYSTEM_THREAD_INFORMATION{
    FILETIME    ftKernelTime;
    FILETIME    ftUserTime;
    FILETIME    ftCreateTime;
    DWORD       dwTickCount;
    DWORD       dwStartAddress;
    DWORD       dwOwningPID;
    DWORD       dwThreadID;
    DWORD       dwCurrentPriority;
    DWORD       dwBasePriority;
    DWORD       dwContextSwitches;
    DWORD       dwThreadState;
    DWORD       dwWaitReason;
    DWORD       dwUnknown;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_GLOBAL_FLAG {
	ULONG GlobalFlag;
} SYSTEM_GLOBAL_FLAG, *PSYSTEM_GLOBAL_FLAG;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID Pointer;
  } DUMMYUNIONNAME;

  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef void (NTAPI * PIO_APC_ROUTINE)(PVOID,PIO_STATUS_BLOCK,ULONG);

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         NameLength;
    WCHAR         Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         ClassOffset;
    ULONG         ClassLength;
    ULONG         NameLength;
    WCHAR         Name[1];
   /* Class[1]; */
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
    ULONG         TitleIndex;
    ULONG         ClassOffset;
    ULONG         ClassLength;
    ULONG         SubKeys;
    ULONG         MaxNameLen;
    ULONG         MaxClassLen;
    ULONG         Values;
    ULONG         MaxValueNameLen;
    ULONG         MaxValueDataLen;
    WCHAR         Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_VALUE_ENTRY
{
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataOffset;
    ULONG DataLength;
    ULONG NameLength;
    WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG TitleIndex;
    ULONG Type;
    ULONG DataLength;
    UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

#ifndef __OBJECT_ATTRIBUTES_DEFINED__
#define __OBJECT_ATTRIBUTES_DEFINED__
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       /* type SECURITY_DESCRIPTOR */
  PVOID SecurityQualityOfService; /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif

typedef struct _OBJECT_DATA_INFORMATION {
    BOOLEAN InheritHandle;
    BOOLEAN ProtectFromClose;
} OBJECT_DATA_INFORMATION, *POBJECT_DATA_INFORMATION;

typedef struct _OBJECT_BASIC_INFORMATION {
    ULONG  Attributes;
    ACCESS_MASK  GrantedAccess;
    ULONG  HandleCount;
    ULONG  PointerCount;
    ULONG  PagedPoolUsage;
    ULONG  NonPagedPoolUsage;
    ULONG  Reserved[3];
    ULONG  NameInformationLength;
    ULONG  TypeInformationLength;
    ULONG  SecurityDescriptorLength;
    LARGE_INTEGER  CreateTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION {
	BOOLEAN Inherit;
	BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

typedef LONG KPRIORITY;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

#define PROCESS_PRIOCLASS_IDLE          1
#define PROCESS_PRIOCLASS_NORMAL        2
#define PROCESS_PRIOCLASS_HIGH          3
#define PROCESS_PRIOCLASS_REALTIME      4
#define PROCESS_PRIOCLASS_BELOW_NORMAL  5
#define PROCESS_PRIOCLASS_ABOVE_NORMAL  6

typedef struct _PROCESS_PRIORITY_CLASS {
    BOOLEAN     Foreground;
    UCHAR       PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

typedef struct _PROCESS_DEVICEMAP_INFORMATION {
    union {
        struct {
            HANDLE DirectoryHandle;
        } Set;
        struct {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

#define DRIVE_UNKNOWN 0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_FIXED 3
#define DRIVE_REMOTE 4
#define DRIVE_CDROM 5
#define DRIVE_RAMDISK 6

typedef struct _RTL_HEAP_DEFINITION {
    ULONG Length; /* = sizeof(RTL_HEAP_DEFINITION) */

    ULONG Unknown[11];
} RTL_HEAP_DEFINITION, *PRTL_HEAP_DEFINITION;

typedef struct _RTL_RWLOCK {
    RTL_CRITICAL_SECTION rtlCS;

    HANDLE hSharedReleaseSemaphore;
    UINT   uSharedWaiters;

    HANDLE hExclusiveReleaseSemaphore;
    UINT   uExclusiveWaiters;

    INT    iNumberActive;
    HANDLE hOwningThreadId;
    DWORD  dwTimeoutBoost;
    PVOID  pDebugInfo;
} RTL_RWLOCK, *LPRTL_RWLOCK;

/* System Information Class 0x00 */

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

/* System Information Class 0x01 */

typedef struct _SYSTEM_CPU_INFORMATION {
    WORD Architecture;
    WORD Level;
    WORD Revision;       /* combination of CPU model and stepping */
    WORD Reserved;       /* always zero */
    DWORD FeatureSet;    /* see bit flags below */
} SYSTEM_CPU_INFORMATION, *PSYSTEM_CPU_INFORMATION;

/* definitions of bits in the Feature set for the x86 processors */
#define CPU_FEATURE_VME    0x00000005   /* Virtual 86 Mode Extensions */
#define CPU_FEATURE_TSC    0x00000002   /* Time Stamp Counter available */
#define CPU_FEATURE_CMOV   0x00000008   /* Conditional Move instruction*/
#define CPU_FEATURE_PGE    0x00000014   /* Page table Entry Global bit */
#define CPU_FEATURE_PSE    0x00000024   /* Page Size Extension */
#define CPU_FEATURE_MTRR   0x00000040   /* Memory Type Range Registers */
#define CPU_FEATURE_CX8    0x00000080   /* Compare and eXchange 8 byte instr. */
#define CPU_FEATURE_MMX    0x00000100   /* Multi Media eXtensions */
#define CPU_FEATURE_X86    0x00000200   /* seems to be alway ON, on the '86 */
#define CPU_FEATURE_PAT    0x00000400   /* Page Attribute Table */
#define CPU_FEATURE_FXSR   0x00000800   /* FXSAVE and FXSTORE instructions */
#define CPU_FEATURE_SEP    0x00001000   /* SYSENTER and SYSEXIT instructions */
#define CPU_FEATURE_SSE    0x00002000   /* SSE extenstions (ext. MMX) */
#define CPU_FEATURE_3DNOW  0x00008000   /* 3DNOW instructions available
                                           (FIXME: needs to be confirmed) */
#define CPU_FEATURE_SSE2   0x00010000   /* SSE2 extensions (XMMI64) */
#define CPU_FEATURE_DS     0x00020000   /* Debug Store */
#define CPU_FEATURE_HTT    0x00040000   /* Hyper Threading Technology */

/* System Information Class 0x02 */

typedef struct _SYSTEM_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    ULONG ReadOperationCount;
    ULONG WriteOperationCount;
    ULONG OtherOperationCount;
    ULONG AvailablePages;
    ULONG TotalCommittedPages;
    ULONG TotalCommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaults;
    ULONG WriteCopyFaults;
    ULONG TransitionFaults;
    ULONG CacheTransitionCount;
    ULONG DemandZeroFaults;
    ULONG PagesRead;
    ULONG PageReadIos;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG PageFilePagesWritten;
    ULONG PageFilePageWriteIos;
    ULONG MappedFilePagesWritten;
    ULONG MappedFilePageWriteIos;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG TotalFreeSystemPtes;
    ULONG SystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG SmallNonPagedLookasideListAllocateHits;
    ULONG SmallPagedLookasideListAllocateHits;
    ULONG Spare3Count;
    ULONG MmSystemCachePage;
    ULONG PagedPoolPage;
    ULONG SystemDriverPage;
    ULONG FastReadNoWait;
    ULONG FastReadWait;
    ULONG FastReadResourceMiss;
    ULONG FastReadNotPossible;
    ULONG FastMdlReadNoWait;
    ULONG FastMdlReadWait;
    ULONG FastMdlReadResourceMiss;
    ULONG FastMdlReadNotPossible;
    ULONG MapDataNoWait;
    ULONG MapDataWait;
    ULONG MapDataNoWaitMiss;
    ULONG MapDataWaitMiss;
    ULONG PinMappedDataCount;
    ULONG PinReadNoWait;
    ULONG PinReadWait;
    ULONG PinReadNoWaitMiss;
    ULONG PinReadWaitMiss;
    ULONG CopyReadNoWait;
    ULONG CopyReadWait;
    ULONG CopyReadNoWaitMiss;
    ULONG CopyReadWaitMiss;
    ULONG MdlReadNoWait;
    ULONG MdlReadWait;
    ULONG MdlReadNoWaitMiss;
    ULONG MdlReadWaitMiss;
    ULONG ReadAheadIos;
    ULONG LazyWriteIos;
    ULONG LazyWritePages;
    ULONG DataFlushes;
    ULONG DataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

/* System Information Class 0x03 */

#if 0
typedef struct _SYSTEM_TIMEOFDAY_INFORMATION {
#ifdef __WINESRC__
    LARGE_INTEGER liKeBootTime;
    LARGE_INTEGER liKeSystemTime;
    LARGE_INTEGER liExpTimeZoneBias;
    ULONG uCurrentTimeZoneId;
    DWORD dwUnknown1[5];
#else
    BYTE Reserved1[48];
#endif
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION; /* was SYSTEM_TIME_INFORMATION */
#endif

typedef struct _SYSTEM_TIME_OF_DAY_INFORMATION {
    LARGE_INTEGER BootTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG CurrentTimeZoneId;
    ULONG Unknown[5];
} SYSTEM_TIME_OF_DAY_INFORMATION, *PSYSTEM_TIME_OF_DAY_INFORMATION;

/* System Information Class 0x08 */

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
#ifdef __WINESRC__
    LARGE_INTEGER liIdleTime;
    LARGE_INTEGER liKernelTime;
    LARGE_INTEGER liUserTime;
    DWORD dwSpare[5];
#else
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
#endif
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

/* System Information Class 0x0b */

typedef struct _SYSTEM_DRIVER_INFORMATION {
    PVOID pvAddress;
    DWORD dwUnknown1;
    DWORD dwUnknown2;
    DWORD dwEntryIndex;
    DWORD dwUnknown3;
    char szName[MAX_PATH + 1];
} SYSTEM_DRIVER_INFORMATION, *PSYSTEM_DRIVER_INFORMATION;

/* System Information Class 0x10 */

typedef struct _SYSTEM_HANDLE_ENTRY {
    ULONG  OwnerPid;
    BYTE   ObjectType;
    BYTE   HandleFlags;
    USHORT HandleValue;
    PVOID  ObjectPointer;
    ULONG  AccessMask;
} SYSTEM_HANDLE_ENTRY, *PSYSTEM_HANDLE_ENTRY;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG               Count;
    SYSTEM_HANDLE_ENTRY Handle[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

/* System Information Class 0x15 */

typedef struct _SYSTEM_CACHE_INFORMATION {
    ULONG CurrentSize;
    ULONG PeakSize;
    ULONG PageFaultCount;
    ULONG MinimumWorkingSet;
    ULONG MaximumWorkingSet;
    ULONG unused[4];
} SYSTEM_CACHE_INFORMATION, *PSYSTEM_CACHE_INFORMATION;

/* System Information Class 0x17 */

typedef struct _SYSTEM_INTERRUPT_INFORMATION {
    BYTE Reserved1[24];
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_CONFIGURATION_INFO {
    union {
        ULONG	OemId;
        struct {
	    WORD ProcessorArchitecture;
	    WORD Reserved;
	} tag1;
    } tag2;
    ULONG PageSize;
    PVOID MinimumApplicationAddress;
    PVOID MaximumApplicationAddress;
    ULONG ActiveProcessorMask;
    ULONG NumberOfProcessors;
    ULONG ProcessorType;
    ULONG AllocationGranularity;
    WORD  ProcessorLevel;
    WORD  ProcessorRevision;
} SYSTEM_CONFIGURATION_INFO, *PSYSTEM_CONFIGURATION_INFO;

typedef struct SYSTEM_RANGE_START_INFORMATION {
    PVOID SystemRangeStart;
} SYSTEM_RANGE_START_INFORMATION, *PSYSTEM_RANGE_START_INFORMATION;

typedef struct _SYSTEM_EXCEPTION_INFORMATION {
    BYTE Reserved1[16];
} SYSTEM_EXCEPTION_INFORMATION, *PSYSTEM_EXCEPTION_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION {
    BYTE Reserved1[32];
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
	BOOLEAN  DebuggerEnabled;
	BOOLEAN  DebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

/* System Information Class 0x05 */

typedef struct _VM_COUNTERS_ {
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG  PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _SYSTEM_PROCESS_INFORMATION {
#ifdef __WINESRC__
    DWORD dwOffset;
    DWORD dwThreadCount;
    DWORD dwUnknown1[6];
    FILETIME ftCreationTime;
    FILETIME ftUserTime;
    FILETIME ftKernelTime;
    UNICODE_STRING ProcessName;
    DWORD dwBasePriority;
    DWORD dwProcessID;
    DWORD dwParentProcessID;
    DWORD dwHandleCount;
    DWORD dwUnknown3;
    DWORD dwUnknown4;
    VM_COUNTERS vmCounters;
    IO_COUNTERS ioCounters;
    SYSTEM_THREAD_INFORMATION ti[1];
#else
    ULONG NextEntryOffset;
    BYTE Reserved1[52];
    PVOID Reserved2[3];
    HANDLE UniqueProcessId;
    PVOID Reserved3;
    ULONG HandleCount;
    BYTE Reserved4[4];
    PVOID Reserved5[11];
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved6[6];
#endif
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION {
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    PVOID Reserved1;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_TIME_ADJUSTMENT {
    ULONG   TimeAdjustment;
    BOOLEAN TimeAdjustmentDisabled;
} SYSTEM_TIME_ADJUSTMENT, *PSYSTEM_TIME_ADJUSTMENT;

typedef struct _TIME_FIELDS
{   CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

typedef struct _WINSTATIONINFORMATIONW {
  BYTE Reserved2[70];
  ULONG LogonId;
  BYTE Reserved3[1140];
} WINSTATIONINFORMATIONW, *PWINSTATIONINFORMATIONW;

typedef BOOLEAN (NTAPI * PWINSTATIONQUERYINFORMATIONW)(HANDLE,ULONG,WINSTATIONINFOCLASS,PVOID,ULONG,PULONG);

typedef struct _LDR_RESOURCE_INFO
{
    ULONG_PTR Type;
    ULONG_PTR Name;
    ULONG Language;
} LDR_RESOURCE_INFO, *PLDR_RESOURCE_INFO;


/* debug buffer definitions */

typedef struct _DEBUG_BUFFER {
  HANDLE SectionHandle;
  PVOID  SectionBase;
  PVOID  RemoteSectionBase;
  ULONG  SectionBaseDelta;
  HANDLE EventPairHandle;
  ULONG  Unknown[2];
  HANDLE RemoteThreadHandle;
  ULONG  InfoClassMask;
  ULONG  SizeOfInfo;
  ULONG  AllocatedSize;
  ULONG  SectionSize;
  PVOID  ModuleInformation;
  PVOID  BackTraceInformation;
  PVOID  HeapInformation;
  PVOID  LockInformation;
  PVOID  Reserved[8];
} DEBUG_BUFFER, *PDEBUG_BUFFER;

#define PDI_MODULES                       0x01
#define PDI_BACKTRACE                     0x02
#define PDI_HEAPS                         0x04
#define PDI_HEAP_TAGS                     0x08
#define PDI_HEAP_BLOCKS                   0x10
#define PDI_LOCKS                         0x20

typedef struct _DEBUG_MODULE_INFORMATION {
  ULONG  Reserved[2];
  ULONG  Base;
  ULONG  Size;
  ULONG  Flags;
  USHORT Index;
  USHORT Unknown;
  USHORT LoadCount;
  USHORT ModuleNameOffset;
  CHAR   ImageName[256];
} DEBUG_MODULE_INFORMATION, *PDEBUG_MODULE_INFORMATION;

typedef struct _DEBUG_HEAP_INFORMATION {
  ULONG  Base;
  ULONG  Flags;
  USHORT Granularity;
  USHORT Unknown;
  ULONG  Allocated;
  ULONG  Committed;
  ULONG  TagCount;
  ULONG  BlockCount;
  ULONG  Reserved[7];
  PVOID  Tags;
  PVOID  Blocks;
} DEBUG_HEAP_INFORMATION, *PDEBUG_HEAP_INFORMATION;

typedef struct _DEBUG_LOCK_INFORMATION {
  PVOID  Address;
  USHORT Type;
  USHORT CreatorBackTraceIndex;
  ULONG  OwnerThreadId;
  ULONG  ActiveCount;
  ULONG  ContentionCount;
  ULONG  EntryCount;
  ULONG  RecursionCount;
  ULONG  NumberOfSharedWaiters;
  ULONG  NumberOfExclusiveWaiters;
} DEBUG_LOCK_INFORMATION, *PDEBUG_LOCK_INFORMATION;

typedef struct _PORT_MESSAGE_HEADER {
  USHORT DataSize;
  USHORT MessageSize;
  USHORT MessageType;
  USHORT VirtualRangesOffset;
  CLIENT_ID ClientId;
  ULONG MessageId;
  ULONG SectionSize;
} PORT_MESSAGE_HEADER, *PPORT_MESSAGE_HEADER, PORT_MESSAGE, *PPORT_MESSAGE;

typedef unsigned short RTL_ATOM, *PRTL_ATOM;

typedef struct atom_table *RTL_ATOM_TABLE, **PRTL_ATOM_TABLE;

typedef enum _ATOM_INFORMATION_CLASS {
   AtomBasicInformation         = 0,
   AtomTableInformation         = 1,
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION {
   USHORT       ReferenceCount;
   USHORT       Pinned;
   USHORT       NameLength;
   WCHAR        Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

/* FIXME: names probably not correct */
typedef struct _RTL_HANDLE
{
    struct _RTL_HANDLE * Next;
} RTL_HANDLE;

/* FIXME: names probably not correct */
typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaxHandleCount;  /* 0x00 */
    ULONG HandleSize;      /* 0x04 */
    ULONG Unused[2];       /* 0x08-0x0c */
    PVOID NextFree;        /* 0x10 */
    PVOID FirstHandle;     /* 0x14 */
    PVOID ReservedMemory;  /* 0x18 */
    PVOID MaxHandle;       /* 0x1c */
} RTL_HANDLE_TABLE;

/***********************************************************************
 * Defines
 */

/* flags for NtCreateFile and NtOpenFile */
#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_WRITE_THROUGH              0x00000002
#define FILE_SEQUENTIAL_ONLY            0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT    0x00000020
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_CREATE_TREE_CONNECTION     0x00000080
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_NO_EA_KNOWLEDGE            0x00000200
#define FILE_OPEN_FOR_RECOVERY          0x00000400
#define FILE_RANDOM_ACCESS              0x00000800
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_BY_FILE_ID            0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000
#define FILE_NO_COMPRESSION             0x00008000
#define FILE_RESERVE_OPFILTER           0x00100000
#define FILE_TRANSACTED_MODE            0x00200000
#define FILE_OPEN_OFFLINE_FILE          0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY  0x00800000

#define FILE_ATTRIBUTE_VALID_FLAGS      0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS  0x000031a7

/* status for NtCreateFile or NtOpenFile */
#define FILE_SUPERSEDED                 0
#define FILE_OPENED                     1
#define FILE_CREATED                    2
#define FILE_OVERWRITTEN                3
#define FILE_EXISTS                     4
#define FILE_DOES_NOT_EXIST             5

/* disposition for NtCreateFile */
#define FILE_SUPERSEDE                  0
#define FILE_OPEN                       1
#define FILE_CREATE                     2
#define FILE_OPEN_IF                    3
#define FILE_OVERWRITE                  4
#define FILE_OVERWRITE_IF               5
#define FILE_MAXIMUM_DISPOSITION        5

/* Characteristics of a File System */
#define FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONE_MEDIA            0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define FILE_DEVICE_SECURE_OPEN         0x00000100

/* options for NtCreateNamedPipeFile */
#define FILE_PIPE_INBOUND               0x00000000
#define FILE_PIPE_OUTBOUND              0x00000001
#define FILE_PIPE_FULL_DUPLEX           0x00000002

/* options for pipe's type */
#define FILE_PIPE_TYPE_MESSAGE          0x00000001
#define FILE_PIPE_TYPE_BYTE             0x00000000
/* and client / server end */
#define FILE_PIPE_SERVER_END            0x00000001
#define FILE_PIPE_CLIENT_END            0x00000000

#if (_WIN32_WINNT >= 0x0501)
#define INTERNAL_TS_ACTIVE_CONSOLE_ID ( *((volatile ULONG*)(0x7ffe02d8)) )
#endif /* (_WIN32_WINNT >= 0x0501) */

#define LOGONID_CURRENT    ((ULONG)-1)

#define OBJ_INHERIT          0x00000002L
#define OBJ_PERMANENT        0x00000010L
#define OBJ_EXCLUSIVE        0x00000020L
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_OPENIF           0x00000080L
#define OBJ_OPENLINK         0x00000100L
#define OBJ_KERNEL_HANDLE    0x00000200L
#define OBJ_VALID_ATTRIBUTES 0x000003F2L

#define SERVERNAME_CURRENT ((HANDLE)NULL)

typedef void (CALLBACK *PKNORMAL_ROUTINE)(PVOID,PVOID,PVOID);
typedef void (CALLBACK *PRTL_THREAD_START_ROUTINE)(LPVOID); /* FIXME: not the right name */
typedef DWORD (CALLBACK *PRTL_WORK_ITEM_ROUTINE)(LPVOID); /* FIXME: not the right name */


/* DbgPrintEx default levels */
#define DPFLTR_ERROR_LEVEL     0
#define DPFLTR_WARNING_LEVEL   1
#define DPFLTR_TRACE_LEVEL     2
#define DPFLTR_INFO_LEVEL      3
#define DPFLTR_MASK    0x8000000

/* Well-known LUID values */
#define SE_MIN_WELL_KNOWN_PRIVILEGE       2L
#define SE_CREATE_TOKEN_PRIVILEGE         2L
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   3L
#define SE_LOCK_MEMORY_PRIVILEGE          4L
#define SE_INCREASE_QUOTA_PRIVILEGE       5L
#define SE_UNSOLICITED_INPUT_PRIVILEGE    6L /* obsolete */
#define SE_MACHINE_ACCOUNT_PRIVILEGE      6L
#define SE_TCB_PRIVILEGE                  7L
#define SE_SECURITY_PRIVILEGE             8L
#define SE_TAKE_OWNERSHIP_PRIVILEGE       9L
#define SE_LOAD_DRIVER_PRIVILEGE         10L
#define SE_SYSTEM_PROFILE_PRIVILEGE      11L
#define SE_SYSTEMTIME_PRIVILEGE          12L
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE 13L
#define SE_INC_BASE_PRIORITY_PRIVILEGE   14L
#define SE_CREATE_PAGEFILE_PRIVILEGE     15L
#define SE_CREATE_PERMANENT_PRIVILEGE    16L
#define SE_BACKUP_PRIVILEGE              17L
#define SE_RESTORE_PRIVILEGE             18L
#define SE_SHUTDOWN_PRIVILEGE            19L
#define SE_DEBUG_PRIVILEGE               20L
#define SE_AUDIT_PRIVILEGE               21L
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE  22L
#define SE_CHANGE_NOTIFY_PRIVILLEGE      23L
#define SE_REMOTE_SHUTDOWN_PRIVILEGE     24L
#define SE_UNDOCK_PRIVILEGE              25L
#define SE_SYNC_AGENT_PRIVILEGE          26L
#define SE_ENABLE_DELEGATION_PRIVILEGE   27L
#define SE_MANAGE_VOLUME_PRIVILEGE       28L
#define SE_IMPERSONATE_PRIVILEGE         29L
#define SE_CREATE_GLOBAL_PRIVILEGE       30L
#define SE_MAX_WELL_KNOWN_PRIVILEGE      SE_CREATE_GLOBAL_PRIVILEGE


/* Rtl*Registry* functions structs and defines */
#define RTL_REGISTRY_ABSOLUTE             0
#define RTL_REGISTRY_SERVICES             1
#define RTL_REGISTRY_CONTROL              2
#define RTL_REGISTRY_WINDOWS_NT           3
#define RTL_REGISTRY_DEVICEMAP            4
#define RTL_REGISTRY_USER                 5

#define RTL_REGISTRY_HANDLE       0x40000000
#define RTL_REGISTRY_OPTIONAL     0x80000000

#define RTL_QUERY_REGISTRY_SUBKEY         0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY         0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED       0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE        0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND       0x00000010
#define RTL_QUERY_REGISTRY_DIRECT         0x00000020
#define RTL_QUERY_REGISTRY_DELETE         0x00000040

typedef NTSTATUS (NTAPI *PRTL_QUERY_REGISTRY_ROUTINE)( PCWSTR ValueName,
                                                        ULONG  ValueType,
                                                        PVOID  ValueData,
                                                        ULONG  ValueLength,
                                                        PVOID  Context,
                                                        PVOID  EntryContext);

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
  PRTL_QUERY_REGISTRY_ROUTINE  QueryRoutine;
  ULONG  Flags;
  PWSTR  Name;
  PVOID  EntryContext;
  ULONG  DefaultType;
  PVOID  DefaultData;
  ULONG  DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

typedef struct _KEY_MULTIPLE_VALUE_INFORMATION
{
  PUNICODE_STRING ValueName;
  ULONG DataLength;
  ULONG DataOffset;
  ULONG Type;
} KEY_MULTIPLE_VALUE_INFORMATION, *PKEY_MULTIPLE_VALUE_INFORMATION;

typedef VOID (*PTIMER_APC_ROUTINE) ( PVOID, ULONG, LONG );

typedef enum _EVENT_TYPE {
  NotificationEvent,
  SynchronizationEvent
} EVENT_TYPE, *PEVENT_TYPE;

typedef enum _EVENT_INFORMATION_CLASS {
  EventBasicInformation
} EVENT_INFORMATION_CLASS, *PEVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION {
  EVENT_TYPE EventType;
  LONG SignalState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef enum _SEMAPHORE_INFORMATION_CLASS {
  SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS, *PSEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION {
  ULONG CurrentCount;
  ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS
{
  SectionBasicInformation,
  SectionImageInformation,
} SECTION_INFORMATION_CLASS;

typedef struct _SECTION_BASIC_INFORMATION {
  ULONG BaseAddress;
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

typedef enum _SHUTDOWN_ACTION {
  ShutdownNoReboot,
  ShutdownReboot,
  ShutdownPowerOff
} SHUTDOWN_ACTION, *PSHUTDOWN_ACTION;

typedef enum _KPROFILE_SOURCE {
  ProfileTime,
  ProfileAlignmentFixup,
  ProfileTotalIssues,
  ProfilePipelineDry,
  ProfileLoadInstructions,
  ProfilePipelineFrozen,
  ProfileBranchInstructions,
  ProfileTotalNonissues,
  ProfileDcacheMisses,
  ProfileIcacheMisses,
  ProfileCacheMisses,
  ProfileBranchMispredictions,
  ProfileStoreInstructions,
  ProfileFpInstructions,
  ProfileIntegerInstructions,
  Profile2Issue,
  Profile3Issue,
  Profile4Issue,
  ProfileSpecialInstructions,
  ProfileTotalCycles,
  ProfileIcacheIssues,
  ProfileDcacheAccesses,
  ProfileMemoryBarrierCycles,
  ProfileLoadLinkedIssues,
  ProfileMaximum
} KPROFILE_SOURCE, *PKPROFILE_SOURCE;

typedef struct _DIRECTORY_BASIC_INFORMATION {
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectTypeName;
} DIRECTORY_BASIC_INFORMATION, *PDIRECTORY_BASIC_INFORMATION;

typedef struct _INITIAL_TEB {
  PVOID StackBase;
  PVOID StackLimit;
  PVOID StackCommit;
  PVOID StackCommitMax;
  PVOID StackReserved;
} INITIAL_TEB, *PINITIAL_TEB;

typedef enum _PORT_INFORMATION_CLASS {
  PortNoInformation
} PORT_INFORMATION_CLASS, *PPORT_INFORMATION_CLASS;

typedef enum _IO_COMPLETION_INFORMATION_CLASS {
  IoCompletionBasicInformation
} IO_COMPLETION_INFORMATION_CLASS, *PIO_COMPLETION_INFORMATION_CLASS;

typedef struct _FILE_COMPLETION_INFORMATION {
    HANDLE CompletionPort;
    ULONG_PTR CompletionKey;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

#define IO_COMPLETION_QUERY_STATE  0x0001
#define IO_COMPLETION_MODIFY_STATE 0x0002
#define IO_COMPLETION_ALL_ACCESS   (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

typedef enum _HARDERROR_RESPONSE_OPTION {
  OptionAbortRetryIgnore,
  OptionOk,
  OptionOkCancel,
  OptionRetryCancel,
  OptionYesNo,
  OptionYesNoCancel,
  OptionShutdownSystem
} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;

typedef enum _HARDERROR_RESPONSE {
  ResponseReturnToCaller,
  ResponseNotHandled,
  ResponseAbort,
  ResponseCancel,
  ResponseIgnore,
  ResponseNo,
  ResponseOk,
  ResponseRetry,
  ResponseYes
} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;

typedef enum _SYSDBG_COMMAND {
  SysDbgQueryModuleInformation,
  SysDbgQueryTraceInformation,
  SysDbgSetTracepoint,
  SysDbgSetSpecialCall,
  SysDbgClearSpecialCalls,
  SysDbgQuerySpecialCalls
} SYSDBG_COMMAND, *PSYSDBG_COMMAND;

typedef struct _FILE_USER_QUOTA_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER QuotaUsed;
    LARGE_INTEGER QuotaThreshold;
    LARGE_INTEGER QuotaLimit;
    SID Sid[1];
} FILE_USER_QUOTA_INFORMATION, *PFILE_USER_QUOTA_INFORMATION;

typedef struct _FILE_QUOTA_LIST_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    ULONG Sid[1];
} FILE_QUOTA_LIST_INFORMATION, *PFILE_QUOTA_LIST_INFORMATION;

#define DIRECTORY_QUERY (0x0001)
#define DIRECTORY_TRAVERSE (0x0002)
#define DIRECTORY_CREATE_OBJECT (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

typedef enum _JOBOBJECTINFOCLASS {
	JobObjectBasicAccountingInformation = 1,
	JobObjectBasicLimitInformation,
	JobObjectBasicProcessIdList,
	JobObjectBasicUIRestrictions,
	JobObjectSecurityLimitInformation,
	JobObjectEndOfJobTimeInformation,
	JobObjectAssociateCompletionPortInformation,
	JobObjectBasicAndIoAccountingInformation,
	JobObjectExtendedLimitInformation,
	JobObjectJobSetInformation,
	MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

typedef enum _KEY_SET_INFORMATION_CLASS {
	KeyLastWriteInformation,
} KEY_SET_INFORMATION_CLASS;

/***********************************************************************
 * Function declarations
 */

#if 0
#if defined(__i386__) && defined(__GNUC__)
static inline void NTAPI DbgBreakPoint(void) { __asm__ __volatile__("int3"); }
static inline void NTAPI DbgUserBreakPoint(void) { __asm__ __volatile__("int3"); }
#else  /* __i386__ && __GNUC__ */
void NTAPI DbgBreakPoint(void);
void NTAPI DbgUserBreakPoint(void);
#endif  /* __i386__ && __GNUC__ */
NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...);
NTSTATUS WINAPIV DbgPrintEx(ULONG iComponentId, ULONG Level, LPCSTR fmt, ...);

NTSTATUS  NTAPI LdrAccessResource(HMODULE,const IMAGE_RESOURCE_DATA_ENTRY*,void**,PULONG);
NTSTATUS  NTAPI LdrAddRefDll(ULONG,HMODULE);
NTSTATUS  NTAPI LdrFindResourceDirectory_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DIRECTORY**);
NTSTATUS  NTAPI LdrFindResource_U(HMODULE,const LDR_RESOURCE_INFO*,ULONG,const IMAGE_RESOURCE_DATA_ENTRY**);
NTSTATUS  NTAPI LdrGetDllHandle(LPCWSTR, ULONG, const UNICODE_STRING*, HMODULE*);
NTSTATUS  NTAPI LdrGetProcedureAddress(HMODULE, const ANSI_STRING*, ULONG, void**);
void      NTAPI LdrInitializeThunk(ULONG,ULONG,ULONG,ULONG);
NTSTATUS  NTAPI LdrLoadDll(LPCWSTR, DWORD, const UNICODE_STRING*, HMODULE*);
void      NTAPI LdrShutdownProcess(void);
void      NTAPI LdrShutdownThread(void);
#endif
NTSTATUS  NTAPI NtAcceptConnectPort(PHANDLE,ULONG,PLPC_MESSAGE,BOOLEAN,PLPC_SECTION_WRITE,PLPC_SECTION_READ);
NTSTATUS  NTAPI NtAccessCheck(PSECURITY_DESCRIPTOR,HANDLE,ACCESS_MASK,PGENERIC_MAPPING,PPRIVILEGE_SET,PULONG,PACCESS_MASK,PBOOLEAN);
NTSTATUS  NTAPI NtAccessCheckByType(PSECURITY_DESCRIPTOR,PSID,HANDLE,ACCESS_MASK,POBJECT_TYPE_LIST,ULONG,PGENERIC_MAPPING,PPRIVILEGE_SET,PULONG,PACCESS_MASK,PULONG);
NTSTATUS  NTAPI NtAccessCheckAndAuditAlarm(PUNICODE_STRING,HANDLE,PUNICODE_STRING,PUNICODE_STRING,PSECURITY_DESCRIPTOR,ACCESS_MASK,PGENERIC_MAPPING,BOOLEAN,PACCESS_MASK,PBOOLEAN,PBOOLEAN);
NTSTATUS  NTAPI NtAddAtom(PWSTR,ULONG,PUSHORT);
NTSTATUS  NTAPI NtAdjustGroupsToken(HANDLE,BOOLEAN,PTOKEN_GROUPS,ULONG,PTOKEN_GROUPS,PULONG);
NTSTATUS  NTAPI NtAdjustPrivilegesToken(HANDLE,BOOLEAN,PTOKEN_PRIVILEGES,DWORD,PTOKEN_PRIVILEGES,PDWORD);
NTSTATUS  NTAPI NtAlertResumeThread(HANDLE,PULONG);
NTSTATUS  NTAPI NtAlertThread(HANDLE);
NTSTATUS  NTAPI NtAllocateLocallyUniqueId(PLUID);
NTSTATUS  NTAPI NtAllocateUserPhysicalPages(HANDLE,PULONG,PULONG);
NTSTATUS  NTAPI NtAllocateUuids(PULARGE_INTEGER,PULONG,PULONG);
NTSTATUS  NTAPI NtAllocateVirtualMemory(HANDLE,PVOID*,ULONG,SIZE_T*,ULONG,ULONG);
NTSTATUS  NTAPI NtAreMappedFilesTheSame(PVOID,PVOID);
NTSTATUS  NTAPI NtAssignProcessToJobObject(HANDLE,HANDLE);
NTSTATUS  NTAPI NtCallbackReturn(PVOID,ULONG,NTSTATUS);
NTSTATUS  NTAPI NtCancelDeviceWakeupRequest(HANDLE);
NTSTATUS  NTAPI NtCancelIoFile(HANDLE,PIO_STATUS_BLOCK);
NTSTATUS  NTAPI NtCancelTimer(HANDLE,PBOOLEAN);
NTSTATUS  NTAPI NtClearEvent(HANDLE);
NTSTATUS  NTAPI NtClose(HANDLE);
NTSTATUS  NTAPI NtCloseObjectAuditAlarm(PUNICODE_STRING,HANDLE,BOOLEAN);
NTSTATUS  NTAPI NtCompleteConnectPort(HANDLE);
NTSTATUS  NTAPI NtConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS  NTAPI NtContinue(PCONTEXT,BOOLEAN);
NTSTATUS  NTAPI NtCreateDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtCreateEvent(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,EVENT_TYPE,BOOLEAN);
NTSTATUS  NTAPI NtCreateEventPair(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtCreateFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,PLARGE_INTEGER,ULONG,ULONG,ULONG,ULONG,PVOID,ULONG);
NTSTATUS  NTAPI NtCreateIoCompletion(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG);
NTSTATUS  NTAPI NtCreateJobObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtCreateKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG,PUNICODE_STRING,ULONG,PULONG);
NTSTATUS  NTAPI NtCreateMailslotFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSTATUS  NTAPI NtCreateMutant(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN);
NTSTATUS  NTAPI NtCreateNamedPipeFile(PHANDLE,ULONG,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG,ULONG,BOOLEAN,BOOLEAN,BOOLEAN,ULONG,ULONG,ULONG,PLARGE_INTEGER);
NTSTATUS  NTAPI NtCreatePagingFile(PUNICODE_STRING,PULARGE_INTEGER,PULARGE_INTEGER,ULONG);
NTSTATUS  NTAPI NtCreatePort(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,PULONG);
NTSTATUS  NTAPI NtCreateProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,BOOLEAN,HANDLE,HANDLE,HANDLE);
NTSTATUS  NTAPI NtCreateProfile(PHANDLE,HANDLE,PVOID,ULONG,ULONG,PULONG,ULONG,KPROFILE_SOURCE,ULONG);
NTSTATUS  NTAPI NtCreateSection(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PLARGE_INTEGER,ULONG,ULONG,HANDLE);
NTSTATUS  NTAPI NtCreateSemaphore(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,ULONG,ULONG);
NTSTATUS  NTAPI NtCreateSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PUNICODE_STRING);
NTSTATUS  NTAPI NtCreateThread(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,HANDLE,PCLIENT_ID,PCONTEXT,PINITIAL_TEB,BOOLEAN);
NTSTATUS  NTAPI NtCreateTimer(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,TIMER_TYPE);
NTSTATUS  NTAPI NtCreateToken(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,TOKEN_TYPE,PLUID,PLARGE_INTEGER,PTOKEN_USER,PTOKEN_GROUPS,PTOKEN_PRIVILEGES,PTOKEN_OWNER,PTOKEN_PRIMARY_GROUP,PTOKEN_DEFAULT_DACL,PTOKEN_SOURCE);
NTSTATUS  NTAPI NtCreateWaitablePort(PHANDLE,POBJECT_ATTRIBUTES,ULONG,ULONG,ULONG);
NTSTATUS  NTAPI NtDelayExecution(BOOLEAN,PLARGE_INTEGER);
NTSTATUS  NTAPI NtDeleteAtom(USHORT);
NTSTATUS  NTAPI NtDeleteFile(POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtDeleteKey(HANDLE);
NTSTATUS  NTAPI NtDeleteValueKey(HANDLE,PUNICODE_STRING);
NTSTATUS  NTAPI NtDeviceIoControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  NTAPI NtDisplayString(PUNICODE_STRING);
NTSTATUS  NTAPI NtDuplicateObject(HANDLE,HANDLE,HANDLE,PHANDLE,ACCESS_MASK,ULONG,ULONG);
NTSTATUS  NTAPI NtDuplicateToken(HANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN,TOKEN_TYPE,PHANDLE);
NTSTATUS  NTAPI NtEnumerateKey(HANDLE,ULONG,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtEnumerateValueKey(HANDLE,ULONG,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtExtendSection(HANDLE,PLARGE_INTEGER);
NTSTATUS  NTAPI NtFilterToken(HANDLE,ULONG,PTOKEN_GROUPS,PTOKEN_PRIVILEGES,PTOKEN_GROUPS,PHANDLE);
NTSTATUS  NTAPI NtFindAtom(PWSTR,ULONG,PUSHORT);
NTSTATUS  NTAPI NtFlushBuffersFile(HANDLE,PIO_STATUS_BLOCK);
NTSTATUS  NTAPI NtFlushInstructionCache(HANDLE,PVOID,SIZE_T);
NTSTATUS  NTAPI NtFlushKey(HANDLE);
NTSTATUS  NTAPI NtFlushVirtualMemory(HANDLE,PVOID*,PULONG,PIO_STATUS_BLOCK);
NTSTATUS  NTAPI NtFlushWriteBuffer(VOID);
NTSTATUS  NTAPI NtFreeUserPhysicalPages(PVOID,PULONG,PULONG);
NTSTATUS  NTAPI NtFreeVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSTATUS  NTAPI NtFsControlFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  NTAPI NtGetContextThread(HANDLE,PCONTEXT);
NTSTATUS  NTAPI NtGetDevicePowerState(HANDLE,PDEVICE_POWER_STATE);
NTSTATUS  NTAPI NtGetPlugPlayEvent(ULONG,ULONG,PVOID,ULONG);
ULONG     NTAPI NtGetTickCount(VOID);
NTSTATUS  NTAPI NtGetWriteWatch(HANDLE,ULONG,PVOID,ULONG,PULONG,PULONG,PULONG);
NTSTATUS  NTAPI NtImpersonateAnonymousToken(HANDLE);
NTSTATUS  NTAPI NtImpersonateClientOfPort(HANDLE,PPORT_MESSAGE);
NTSTATUS  NTAPI NtImpersonateThread(HANDLE,HANDLE,PSECURITY_QUALITY_OF_SERVICE);
NTSTATUS  NTAPI NtInitializeRegistry(BOOLEAN);
NTSTATUS  NTAPI NtInitiatePowerAction(POWER_ACTION,SYSTEM_POWER_STATE,ULONG,BOOLEAN);
BOOLEAN  NTAPI NtIsSystemResumeAutomatic(VOID);
NTSTATUS  NTAPI NtListenPort(HANDLE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtLoadDriver(PUNICODE_STRING);
NTSTATUS  NTAPI NtLoadKey(POBJECT_ATTRIBUTES,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtLockFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PULARGE_INTEGER,PULARGE_INTEGER,ULONG,BOOLEAN,BOOLEAN);
NTSTATUS  NTAPI NtLockVirtualMemory(HANDLE,PVOID*,PULONG,ULONG);
NTSTATUS  NTAPI NtMakeTemporaryObject(HANDLE);
NTSTATUS  NTAPI NtMapViewOfSection(HANDLE,HANDLE,PVOID*,ULONG,ULONG,PLARGE_INTEGER,PULONG,SECTION_INHERIT,ULONG,ULONG);
NTSTATUS  NTAPI NtMapUserPhysicalPages(PVOID,PULONG,PULONG);
NTSTATUS  NTAPI NtNotifyChangeDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,ULONG,BOOLEAN);
NTSTATUS  NTAPI NtNotifyChangeKey(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSTATUS  NTAPI NtNotifyChangeMultipleKeys(HANDLE,ULONG,POBJECT_ATTRIBUTES,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,ULONG,BOOLEAN,PVOID,ULONG,BOOLEAN);
NTSTATUS  NTAPI NtOpenDirectoryObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenEvent(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenEventPair(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenFile(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK,ULONG,ULONG);
NTSTATUS  NTAPI NtOpenIoCompletion(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenJobObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenKey(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenKeyedEvent(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenMutant(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenObjectAuditAlarm(PUNICODE_STRING,PHANDLE,PUNICODE_STRING,PUNICODE_STRING,PSECURITY_DESCRIPTOR,HANDLE,ACCESS_MASK,ACCESS_MASK,PPRIVILEGE_SET,BOOLEAN,BOOLEAN,PBOOLEAN);
NTSTATUS  NTAPI NtOpenProcess(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);
NTSTATUS  NTAPI NtOpenProcessToken(HANDLE,DWORD,PHANDLE);
NTSTATUS  NTAPI NtOpenSection(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenSemaphore(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtOpenThread(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PCLIENT_ID);
NTSTATUS  NTAPI NtOpenThreadToken(HANDLE,DWORD,BOOLEAN,PHANDLE);
NTSTATUS  NTAPI NtOpenTimer(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtPowerInformation(POWER_INFORMATION_LEVEL,PVOID,ULONG,PVOID,ULONG);
NTSTATUS  NTAPI NtPrivilegeCheck(HANDLE,PPRIVILEGE_SET,PBOOLEAN);
NTSTATUS  NTAPI NtPrivilegeObjectAuditAlarm(PUNICODE_STRING,PVOID,HANDLE,ACCESS_MASK,PPRIVILEGE_SET,BOOLEAN);
NTSTATUS  NTAPI NtPrivilegedServiceAuditAlarm(PUNICODE_STRING,PUNICODE_STRING,HANDLE,PPRIVILEGE_SET,BOOLEAN);
NTSTATUS  NTAPI NtProtectVirtualMemory(HANDLE,PVOID*,PULONG,ULONG,PULONG);
NTSTATUS  NTAPI NtPulseEvent(HANDLE,PULONG);
NTSTATUS  NTAPI NtQueueApcThread(HANDLE,PKNORMAL_ROUTINE,PVOID,PVOID,PVOID);
NTSTATUS  NTAPI NtQueryAttributesFile(POBJECT_ATTRIBUTES,PFILE_BASIC_INFORMATION);
NTSTATUS  NTAPI NtQueryDebugFilterState(ULONG,ULONG);
NTSTATUS  NTAPI NtQueryDefaultLocale(BOOLEAN,LCID*);
NTSTATUS  NTAPI NtQueryDefaultUILanguage(LANGID*);
NTSTATUS  NTAPI NtQueryDirectoryFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS,BOOLEAN,PUNICODE_STRING,BOOLEAN);
NTSTATUS  NTAPI NtQueryDirectoryObject(HANDLE,PVOID,ULONG,BOOLEAN,BOOLEAN,PULONG,PULONG);
NTSTATUS  NTAPI NtQueryEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,BOOLEAN,PVOID,ULONG,PVOID,BOOLEAN);
NTSTATUS  NTAPI NtQueryEvent(HANDLE,EVENT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryFullAttributesFile(POBJECT_ATTRIBUTES,FILE_NETWORK_OPEN_INFORMATION*);
NTSTATUS  NTAPI NtQueryInformationAtom(USHORT,ATOM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSTATUS  NTAPI NtQueryInformationJobObject(HANDLE,JOBOBJECTINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInformationPort(HANDLE,PORT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInformationProcess(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInformationToken(HANDLE,TOKEN_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryInstallUILanguage(LANGID*);
NTSTATUS  NTAPI NtQueryIntervalProfile(KPROFILE_SOURCE,PULONG);
NTSTATUS  NTAPI NtQueryIoCompletion(HANDLE,IO_COMPLETION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryKey(HANDLE,KEY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryMultipleValueKey(HANDLE,PKEY_MULTIPLE_VALUE_INFORMATION,ULONG,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryMutant(HANDLE,MUTANT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryObject(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryOpenSubKeys(POBJECT_ATTRIBUTES,PULONG);
NTSTATUS  NTAPI NtQueryPerformanceCounter(PLARGE_INTEGER, PLARGE_INTEGER);
NTSTATUS  NTAPI NtQueryQuotaInformationFile(HANDLE,PIO_STATUS_BLOCK,PFILE_USER_QUOTA_INFORMATION,ULONG,BOOLEAN,PFILE_QUOTA_LIST_INFORMATION,ULONG,PSID,BOOLEAN);
NTSTATUS  NTAPI NtQuerySecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,ULONG,PULONG);
NTSTATUS  NTAPI NtQuerySection(HANDLE,SECTION_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQuerySemaphore(HANDLE,SEMAPHORE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQuerySymbolicLinkObject(HANDLE,PUNICODE_STRING,PULONG);
NTSTATUS  NTAPI NtQuerySystemEnvironmentValue(PUNICODE_STRING,PWCHAR,ULONG,PULONG);
NTSTATUS  NTAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQuerySystemTime(PLARGE_INTEGER);
NTSTATUS  NTAPI NtQueryTimer(HANDLE,TIMER_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryTimerResolution(PULONG,PULONG,PULONG);
NTSTATUS  NTAPI NtQueryValueKey(HANDLE,PUNICODE_STRING,KEY_VALUE_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryVirtualMemory(HANDLE,LPCVOID,MEMORY_INFORMATION_CLASS,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtQueryVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS  NTAPI NtRaiseException(PEXCEPTION_RECORD,PCONTEXT,BOOL);
NTSTATUS  NTAPI NtRaiseHardError(NTSTATUS,ULONG,ULONG,PULONG,HARDERROR_RESPONSE_OPTION,PHARDERROR_RESPONSE);
NTSTATUS  NTAPI NtReadFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  NTAPI NtReadFileScatter(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,FILE_SEGMENT_ELEMENT,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  NTAPI NtReadRequestData(HANDLE,PLPC_MESSAGE,ULONG,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtReadVirtualMemory(HANDLE,PVOID,PVOID,SIZE_T,SIZE_T*);
NTSTATUS  NTAPI NtRegisterThreadTerminatePort(HANDLE);
NTSTATUS  NTAPI NtReleaseMutant(HANDLE,PULONG);
NTSTATUS  NTAPI NtReleaseSemaphore(HANDLE,ULONG,PULONG);
NTSTATUS  NTAPI NtRemoveIoCompletion(HANDLE,PULONG,PULONG,PIO_STATUS_BLOCK,PLARGE_INTEGER);
NTSTATUS  NTAPI NtReplaceKey(POBJECT_ATTRIBUTES,HANDLE,POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtReplyPort(HANDLE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtReplyWaitReceivePort(HANDLE,PULONG,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtReplyWaitReceivePortEx(HANDLE,PVOID*,PPORT_MESSAGE,PPORT_MESSAGE,PLARGE_INTEGER);
NTSTATUS  NTAPI NtReplyWaitReplyPort(HANDLE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtRequestPort(HANDLE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtRequestWaitReplyPort(HANDLE,PLPC_MESSAGE,PLPC_MESSAGE);
NTSTATUS  NTAPI NtResetEvent(HANDLE,PULONG);
NTSTATUS  NTAPI NtResetWriteWatch(HANDLE,PVOID,ULONG);
NTSTATUS  NTAPI NtRestoreKey(HANDLE,HANDLE,ULONG);
NTSTATUS  NTAPI NtResumeThread(HANDLE,PULONG);
NTSTATUS  NTAPI NtSaveKey(HANDLE,HANDLE);
NTSTATUS  NTAPI NtSaveMergedKeys(HANDLE,HANDLE,HANDLE);
NTSTATUS  NTAPI NtSecureConnectPort(PHANDLE,PUNICODE_STRING,PSECURITY_QUALITY_OF_SERVICE,PLPC_SECTION_WRITE,PSID,PLPC_SECTION_READ,PULONG,PVOID,PULONG);
NTSTATUS  NTAPI NtSetContextThread(HANDLE,PCONTEXT);
NTSTATUS  NTAPI NtSetDefaultHardErrorPort(HANDLE);
NTSTATUS  NTAPI NtSetDefaultLocale(BOOLEAN,LCID);
NTSTATUS  NTAPI NtSetDefaultUILanguage(LANGID);
NTSTATUS  NTAPI NtSetEaFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG);
NTSTATUS  NTAPI NtSetEvent(HANDLE,PULONG);
NTSTATUS  NTAPI NtSetQuotaInformationFile(HANDLE,PIO_STATUS_BLOCK,PFILE_USER_QUOTA_INFORMATION,ULONG);
NTSTATUS  NTAPI NtSetHighEventPair(HANDLE);
NTSTATUS  NTAPI NtSetHighWaitLowEventPair(HANDLE);
NTSTATUS  NTAPI NtSetHighWaitLowThread(VOID);
NTSTATUS  NTAPI NtSetInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FILE_INFORMATION_CLASS);
NTSTATUS  NTAPI NtSetInformationJobObject(HANDLE,JOBOBJECTINFOCLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetInformationKey(HANDLE,KEY_SET_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetInformationObject(HANDLE,OBJECT_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetInformationProcess(HANDLE,PROCESS_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetInformationThread(HANDLE,THREADINFOCLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetInformationToken(HANDLE,TOKEN_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetIntervalProfile(ULONG,KPROFILE_SOURCE);
NTSTATUS  NTAPI NtSetIoCompletion(HANDLE,ULONG,ULONG,NTSTATUS,ULONG);
NTSTATUS  NTAPI NtSetLdtEntries(ULONG,LDT_ENTRY,ULONG,LDT_ENTRY);
NTSTATUS  NTAPI NtSetLowEventPair(HANDLE);
NTSTATUS  NTAPI NtSetLowWaitHighEventPair(HANDLE);
NTSTATUS  NTAPI NtSetLowWaitHighThread(VOID);
NTSTATUS  NTAPI NtSetSecurityObject(HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
NTSTATUS  NTAPI NtSetSystemEnvironmentValue(PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  NTAPI NtSetSystemInformation(SYSTEM_INFORMATION_CLASS,PVOID,ULONG);
NTSTATUS  NTAPI NtSetSystemPowerState(POWER_ACTION,SYSTEM_POWER_STATE,ULONG);
NTSTATUS  NTAPI NtSetSystemTime(PLARGE_INTEGER,PLARGE_INTEGER);
NTSTATUS  NTAPI NtSetThreadExecutionState(EXECUTION_STATE,PEXECUTION_STATE);
NTSTATUS  NTAPI NtSetTimer(HANDLE,PLARGE_INTEGER,PTIMER_APC_ROUTINE,PVOID,BOOLEAN,LONG,PBOOLEAN);
NTSTATUS  NTAPI NtSetTimerResolution(ULONG,BOOLEAN,PULONG);
NTSTATUS  NTAPI NtSetUuidSeed(PUCHAR);
NTSTATUS  NTAPI NtSetValueKey(HANDLE,PUNICODE_STRING,ULONG,ULONG,PVOID,ULONG);
NTSTATUS  NTAPI NtSetVolumeInformationFile(HANDLE,PIO_STATUS_BLOCK,PVOID,ULONG,FS_INFORMATION_CLASS);
NTSTATUS  NTAPI NtSignalAndWaitForSingleObject(HANDLE,HANDLE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS  NTAPI NtShutdownSystem(SHUTDOWN_ACTION);
NTSTATUS  NTAPI NtStartProfile(HANDLE);
NTSTATUS  NTAPI NtStopProfile(HANDLE);
NTSTATUS  NTAPI NtSuspendThread(HANDLE,PULONG);
NTSTATUS  NTAPI NtSystemDebugControl(SYSDBG_COMMAND,PVOID,ULONG,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtTerminateJobObject(HANDLE,NTSTATUS);
NTSTATUS  NTAPI NtTerminateProcess(HANDLE,LONG);
NTSTATUS  NTAPI NtTerminateThread(HANDLE,LONG);
NTSTATUS  NTAPI NtTestAlert(VOID);
NTSTATUS  NTAPI NtUnloadDriver(PUNICODE_STRING);
NTSTATUS  NTAPI NtUnloadKey(POBJECT_ATTRIBUTES);
NTSTATUS  NTAPI NtUnloadKeyEx(POBJECT_ATTRIBUTES,HANDLE);
NTSTATUS  NTAPI NtUnlockFile(HANDLE,PIO_STATUS_BLOCK,PULARGE_INTEGER,PULARGE_INTEGER,ULONG);
NTSTATUS  NTAPI NtUnlockVirtualMemory(HANDLE,PVOID*,SIZE_T*,ULONG);
NTSTATUS  NTAPI NtUnmapViewOfSection(HANDLE,PVOID);
NTSTATUS  NTAPI NtVdmControl(ULONG,PVOID);
NTSTATUS  NTAPI NtWaitForSingleObject(HANDLE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS  NTAPI NtWaitForMultipleObjects(ULONG,PHANDLE,WAIT_TYPE,BOOLEAN,PLARGE_INTEGER);
NTSTATUS  NTAPI NtWaitHighEventPair(HANDLE);
NTSTATUS  NTAPI NtWaitLowEventPair(HANDLE);
NTSTATUS  NTAPI NtWriteFile(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  NTAPI NtWriteFileGather(HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,PIO_STATUS_BLOCK,FILE_SEGMENT_ELEMENT,ULONG,PLARGE_INTEGER,PULONG);
NTSTATUS  NTAPI NtWriteRequestData(HANDLE,PLPC_MESSAGE,ULONG,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtWriteVirtualMemory(HANDLE,PVOID,PVOID,ULONG,PULONG);
NTSTATUS  NTAPI NtYieldExecution(VOID);

#if 0
void      NTAPI RtlAcquirePebLock(void);
BYTE      NTAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK,BYTE);
BYTE      NTAPI RtlAcquireResourceShared(LPRTL_RWLOCK,BYTE);
NTSTATUS  NTAPI RtlAddAce(PACL,DWORD,DWORD,PACE_HEADER,DWORD);
NTSTATUS  NTAPI RtlAddAccessAllowedAce(PACL,DWORD,DWORD,PSID);
NTSTATUS  NTAPI RtlAddAccessAllowedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
NTSTATUS  NTAPI RtlAddAccessDeniedAce(PACL,DWORD,DWORD,PSID);
NTSTATUS  NTAPI RtlAddAccessDeniedAceEx(PACL,DWORD,DWORD,DWORD,PSID);
NTSTATUS  NTAPI RtlAddAtomToAtomTable(RTL_ATOM_TABLE,const WCHAR*,RTL_ATOM*);
NTSTATUS  NTAPI RtlAddAuditAccessAce(PACL,DWORD,DWORD,PSID,BOOL,BOOL);
PVOID     NTAPI RtlAddVectoredExceptionHandler(ULONG,PVECTORED_EXCEPTION_HANDLER);
NTSTATUS  NTAPI RtlAdjustPrivilege(ULONG,BOOLEAN,BOOLEAN,PBOOLEAN);
NTSTATUS  NTAPI RtlAllocateAndInitializeSid(PSID_IDENTIFIER_AUTHORITY,BYTE,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,PSID *);
RTL_HANDLE * NTAPI RtlAllocateHandle(RTL_HANDLE_TABLE *,ULONG *);
PVOID     NTAPI RtlAllocateHeap(HANDLE,ULONG,SIZE_T);
WCHAR     NTAPI RtlAnsiCharToUnicodeChar(LPSTR *);
DWORD     NTAPI RtlAnsiStringToUnicodeSize(const STRING *);
NTSTATUS  NTAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING,PCANSI_STRING,BOOLEAN);
NTSTATUS  NTAPI RtlAppendAsciizToString(STRING *,LPCSTR);
NTSTATUS  NTAPI RtlAppendStringToString(STRING *,const STRING *);
NTSTATUS  NTAPI RtlAppendUnicodeStringToString(UNICODE_STRING *,const UNICODE_STRING *);
NTSTATUS  NTAPI RtlAppendUnicodeToString(UNICODE_STRING *,LPCWSTR);
BOOLEAN   NTAPI RtlAreAllAccessesGranted(ACCESS_MASK,ACCESS_MASK);
BOOLEAN   NTAPI RtlAreAnyAccessesGranted(ACCESS_MASK,ACCESS_MASK);
BOOLEAN   NTAPI RtlAreBitsSet(PCRTL_BITMAP,ULONG,ULONG);
BOOLEAN   NTAPI RtlAreBitsClear(PCRTL_BITMAP,ULONG,ULONG);

NTSTATUS  NTAPI RtlCharToInteger(PCSZ,ULONG,PULONG);
NTSTATUS  NTAPI RtlCheckRegistryKey(ULONG, PWSTR);
void      NTAPI RtlClearAllBits(PRTL_BITMAP);
void      NTAPI RtlClearBits(PRTL_BITMAP,ULONG,ULONG);
NTSTATUS  NTAPI RtlCreateActivationContext(HANDLE*,const void*);
PDEBUG_BUFFER NTAPI RtlCreateQueryDebugBuffer(ULONG,BOOLEAN);
ULONG     NTAPI RtlCompactHeap(HANDLE,ULONG);
LONG      NTAPI RtlCompareString(const STRING*,const STRING*,BOOLEAN);
LONG      NTAPI RtlCompareUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
DWORD     NTAPI RtlComputeCrc32(DWORD,PBYTE,INT);
NTSTATUS  NTAPI RtlConvertSidToUnicodeString(PUNICODE_STRING,PSID,BOOLEAN);
LONGLONG  NTAPI RtlConvertLongToLargeInteger(LONG);
ULONGLONG NTAPI RtlConvertUlongToLargeInteger(ULONG);
void      NTAPI RtlCopyLuid(PLUID,const LUID*);
void      NTAPI RtlCopyLuidAndAttributesArray(ULONG,const LUID_AND_ATTRIBUTES*,PLUID_AND_ATTRIBUTES);
BOOLEAN   NTAPI RtlCopySid(DWORD,PSID,PSID);
NTSTATUS  NTAPI RtlCopySecurityDescriptor(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR);
void      NTAPI RtlCopyString(STRING*,const STRING*);
void      NTAPI RtlCopyUnicodeString(UNICODE_STRING*,const UNICODE_STRING*);
NTSTATUS  NTAPI RtlCreateAcl(PACL,DWORD,DWORD);
NTSTATUS  NTAPI RtlCreateAtomTable(ULONG,RTL_ATOM_TABLE*);
NTSTATUS  NTAPI RtlCreateEnvironment(BOOLEAN, PWSTR*);
HANDLE    NTAPI RtlCreateHeap(ULONG,PVOID,SIZE_T,SIZE_T,PVOID,PRTL_HEAP_DEFINITION);
NTSTATUS  NTAPI RtlCreateProcessParameters(RTL_USER_PROCESS_PARAMETERS**,const UNICODE_STRING*,
                                            const UNICODE_STRING*,const UNICODE_STRING*,
                                            const UNICODE_STRING*,PWSTR,const UNICODE_STRING*,
                                            const UNICODE_STRING*,const UNICODE_STRING*,
                                            const UNICODE_STRING*);
NTSTATUS  NTAPI RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR,DWORD);
BOOLEAN   NTAPI RtlCreateUnicodeString(PUNICODE_STRING,LPCWSTR);
BOOLEAN   NTAPI RtlCreateUnicodeStringFromAsciiz(PUNICODE_STRING,LPCSTR);
NTSTATUS  NTAPI RtlCreateUserThread(HANDLE,const SECURITY_DESCRIPTOR*,BOOLEAN,PVOID,SIZE_T,SIZE_T,PRTL_THREAD_START_ROUTINE,void*,HANDLE*,CLIENT_ID*);

void      NTAPI RtlDeactivateActivationContext(DWORD,ULONG_PTR);
NTSTATUS  NTAPI RtlDeleteAce(PACL,DWORD);
NTSTATUS  NTAPI RtlDeleteAtomFromAtomTable(RTL_ATOM_TABLE,RTL_ATOM);
NTSTATUS  NTAPI RtlDeleteCriticalSection(RTL_CRITICAL_SECTION *);
NTSTATUS  NTAPI RtlDeleteRegistryValue(ULONG, PCWSTR, PCWSTR);
void      NTAPI RtlDeleteResource(LPRTL_RWLOCK);
NTSTATUS  NTAPI RtlDeleteSecurityObject(PSECURITY_DESCRIPTOR*);
PRTL_USER_PROCESS_PARAMETERS NTAPI RtlDeNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
NTSTATUS  NTAPI RtlDestroyAtomTable(RTL_ATOM_TABLE);
NTSTATUS  NTAPI RtlDestroyEnvironment(PWSTR);
NTSTATUS  NTAPI RtlDestroyHandleTable(RTL_HANDLE_TABLE *);
HANDLE    NTAPI RtlDestroyHeap(HANDLE);
void      NTAPI RtlDestroyProcessParameters(RTL_USER_PROCESS_PARAMETERS*);
NTSTATUS  NTAPI RtlDestroyQueryDebugBuffer(PDEBUG_BUFFER);
DOS_PATHNAME_TYPE NTAPI RtlDetermineDosPathNameType_U(PCWSTR);
BOOLEAN   NTAPI RtlDllShutdownInProgress(void);
BOOLEAN   NTAPI RtlDoesFileExists_U(LPCWSTR);
BOOLEAN   NTAPI RtlDosPathNameToNtPathName_U(PCWSTR,PUNICODE_STRING,PWSTR*,CURDIR*);
ULONG     NTAPI RtlDosSearchPath_U(LPCWSTR, LPCWSTR, LPCWSTR, ULONG, LPWSTR, LPWSTR*);
WCHAR     NTAPI RtlDowncaseUnicodeChar(WCHAR);
NTSTATUS  NTAPI RtlDowncaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
void      NTAPI RtlDumpResource(LPRTL_RWLOCK);
NTSTATUS  NTAPI RtlDuplicateUnicodeString(int,const UNICODE_STRING*,UNICODE_STRING*);

NTSTATUS  NTAPI RtlEmptyAtomTable(RTL_ATOM_TABLE,BOOLEAN);
LONGLONG  NTAPI RtlEnlargedIntegerMultiply(INT,INT);
ULONGLONG NTAPI RtlEnlargedUnsignedMultiply(UINT,UINT);
UINT      NTAPI RtlEnlargedUnsignedDivide(ULONGLONG,UINT,UINT *);
NTSTATUS  NTAPI RtlEnterCriticalSection(RTL_CRITICAL_SECTION *);
void      NTAPI RtlEraseUnicodeString(UNICODE_STRING*);
NTSTATUS  NTAPI RtlEqualComputerName(const UNICODE_STRING*,const UNICODE_STRING*);
NTSTATUS  NTAPI RtlEqualDomainName(const UNICODE_STRING*,const UNICODE_STRING*);
BOOLEAN   NTAPI RtlEqualLuid(const LUID*,const LUID*);
BOOL      NTAPI RtlEqualPrefixSid(PSID,PSID);
BOOL      NTAPI RtlEqualSid(PSID,PSID);
BOOLEAN   NTAPI RtlEqualString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN   NTAPI RtlEqualUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);
void      DECLSPEC_NORETURN NTAPI RtlExitUserThread(ULONG);
NTSTATUS  NTAPI RtlExpandEnvironmentStrings_U(PWSTR, const UNICODE_STRING*, UNICODE_STRING*, ULONG*);
LONGLONG  NTAPI RtlExtendedMagicDivide(LONGLONG,LONGLONG,INT);
LONGLONG  NTAPI RtlExtendedIntegerMultiply(LONGLONG,INT);
LONGLONG  NTAPI RtlExtendedLargeIntegerDivide(LONGLONG,INT,INT *);

NTSTATUS  NTAPI RtlFindActivationContextSectionString(ULONG,const GUID*,ULONG,const UNICODE_STRING*,PVOID);
NTSTATUS  NTAPI RtlFindCharInUnicodeString(int,const UNICODE_STRING*,const UNICODE_STRING*,USHORT*);
ULONG     NTAPI RtlFindClearBits(PCRTL_BITMAP,ULONG,ULONG);
ULONG     NTAPI RtlFindClearBitsAndSet(PRTL_BITMAP,ULONG,ULONG);
ULONG     NTAPI RtlFindClearRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
ULONG     NTAPI RtlFindLastBackwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
ULONG     NTAPI RtlFindLastBackwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
CCHAR     NTAPI RtlFindLeastSignificantBit(ULONGLONG);
ULONG     NTAPI RtlFindLongestRunSet(PCRTL_BITMAP,PULONG);
ULONG     NTAPI RtlFindLongestRunClear(PCRTL_BITMAP,PULONG);
NTSTATUS  NTAPI RtlFindMessage(HMODULE,ULONG,ULONG,ULONG,const MESSAGE_RESOURCE_ENTRY**);
CCHAR     NTAPI RtlFindMostSignificantBit(ULONGLONG);
ULONG     NTAPI RtlFindNextForwardRunSet(PCRTL_BITMAP,ULONG,PULONG);
ULONG     NTAPI RtlFindNextForwardRunClear(PCRTL_BITMAP,ULONG,PULONG);
ULONG     NTAPI RtlFindSetBits(PCRTL_BITMAP,ULONG,ULONG);
ULONG     NTAPI RtlFindSetBitsAndClear(PRTL_BITMAP,ULONG,ULONG);
ULONG     NTAPI RtlFindSetRuns(PCRTL_BITMAP,PRTL_BITMAP_RUN,ULONG,BOOLEAN);
BOOLEAN   NTAPI RtlFirstFreeAce(PACL,PACE_HEADER *);
NTSTATUS  NTAPI RtlFormatCurrentUserKeyPath(PUNICODE_STRING);
NTSTATUS  NTAPI RtlFormatMessage(LPWSTR,UCHAR,BOOLEAN,BOOLEAN,BOOLEAN,va_list *,LPWSTR,ULONG);
void      NTAPI RtlFreeAnsiString(PANSI_STRING);
BOOLEAN   NTAPI RtlFreeHandle(RTL_HANDLE_TABLE *,RTL_HANDLE *);
BOOLEAN   NTAPI RtlFreeHeap(HANDLE,ULONG,PVOID);
void      NTAPI RtlFreeOemString(POEM_STRING);
DWORD     NTAPI RtlFreeSid(PSID);
void      NTAPI RtlFreeThreadActivationContextStack(void);
void      NTAPI RtlFreeUnicodeString(PUNICODE_STRING);

NTSTATUS  NTAPI RtlGetAce(PACL,DWORD,LPVOID *);
NTSTATUS  NTAPI RtlGetActiveActivationContext(HANDLE*);
NTSTATUS  NTAPI RtlGetControlSecurityDescriptor(PSECURITY_DESCRIPTOR, PSECURITY_DESCRIPTOR_CONTROL,LPDWORD);
NTSTATUS  NTAPI RtlGetCurrentDirectory_U(ULONG, LPWSTR);
PEB *     NTAPI RtlGetCurrentPeb(void);
NTSTATUS  NTAPI RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
ULONG     NTAPI RtlGetFullPathName_U(PCWSTR,ULONG,PWSTR,PWSTR*);
NTSTATUS  NTAPI RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
NTSTATUS  NTAPI RtlGetLastNtStatus(void);
DWORD     NTAPI RtlGetLastWin32Error(void);
DWORD     NTAPI RtlGetLongestNtPathLength(void);
BOOLEAN   NTAPI RtlGetNtProductType(LPDWORD);
NTSTATUS  NTAPI RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID *,PBOOLEAN);
ULONG     NTAPI RtlGetProcessHeaps(ULONG,HANDLE*);
NTSTATUS  NTAPI RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,PBOOLEAN,PACL *,PBOOLEAN);
NTSTATUS  NTAPI RtlGetVersion(RTL_OSVERSIONINFOEXW*);
NTSTATUS  NTAPI RtlGUIDFromString(PUNICODE_STRING,GUID*);

PSID_IDENTIFIER_AUTHORITY NTAPI RtlIdentifierAuthoritySid(PSID);
PVOID     NTAPI RtlImageDirectoryEntryToData(HMODULE,BOOL,WORD,ULONG *);
PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(HMODULE);
PIMAGE_SECTION_HEADER NTAPI RtlImageRvaToSection(const IMAGE_NT_HEADERS *,HMODULE,DWORD);
PVOID     NTAPI RtlImageRvaToVa(const IMAGE_NT_HEADERS *,HMODULE,DWORD,IMAGE_SECTION_HEADER **);
NTSTATUS  NTAPI RtlImpersonateSelf(SECURITY_IMPERSONATION_LEVEL);
void      NTAPI RtlInitString(PSTRING,PCSZ);
void      NTAPI RtlInitAnsiString(PANSI_STRING,PCSZ);
NTSTATUS  NTAPI RtlInitAnsiStringEx(PANSI_STRING,PCSZ);
void      NTAPI RtlInitUnicodeString(PUNICODE_STRING,PCWSTR);
NTSTATUS  NTAPI RtlInitUnicodeStringEx(PUNICODE_STRING,PCWSTR);
NTSTATUS  NTAPI RtlInitializeCriticalSection(RTL_CRITICAL_SECTION *);
NTSTATUS  NTAPI RtlInitializeCriticalSectionAndSpinCount(RTL_CRITICAL_SECTION *,DWORD);
void      NTAPI RtlInitializeBitMap(PRTL_BITMAP,PULONG,ULONG);
void      NTAPI RtlInitializeHandleTable(ULONG,ULONG,RTL_HANDLE_TABLE *);
void      NTAPI RtlInitializeResource(LPRTL_RWLOCK);
BOOL      NTAPI RtlInitializeSid(PSID,PSID_IDENTIFIER_AUTHORITY,BYTE);

NTSTATUS  NTAPI RtlInt64ToUnicodeString(ULONGLONG,ULONG,UNICODE_STRING *);
NTSTATUS  NTAPI RtlIntegerToChar(ULONG,ULONG,ULONG,PCHAR);
NTSTATUS  NTAPI RtlIntegerToUnicodeString(ULONG,ULONG,UNICODE_STRING *);
BOOLEAN   NTAPI RtlIsActivationContextActive(HANDLE);
ULONG     NTAPI RtlIsDosDeviceName_U(PCWSTR);
BOOLEAN   NTAPI RtlIsNameLegalDOS8Dot3(const UNICODE_STRING*,POEM_STRING,PBOOLEAN);
BOOLEAN   NTAPI RtlIsTextUnicode(LPCVOID,INT,INT *);
BOOLEAN   NTAPI RtlIsValidHandle(const RTL_HANDLE_TABLE *, const RTL_HANDLE *);
BOOLEAN   NTAPI RtlIsValidIndexHandle(const RTL_HANDLE_TABLE *, ULONG Index, RTL_HANDLE **);

LONGLONG  NTAPI RtlLargeIntegerAdd(LONGLONG,LONGLONG);
LONGLONG  NTAPI RtlLargeIntegerArithmeticShift(LONGLONG,INT);
ULONGLONG NTAPI RtlLargeIntegerDivide( ULONGLONG,ULONGLONG,ULONGLONG *);
LONGLONG  NTAPI RtlLargeIntegerNegate(LONGLONG);
LONGLONG  NTAPI RtlLargeIntegerShiftLeft(LONGLONG,INT);
LONGLONG  NTAPI RtlLargeIntegerShiftRight(LONGLONG,INT);
LONGLONG  NTAPI RtlLargeIntegerSubtract(LONGLONG,LONGLONG);
NTSTATUS  NTAPI RtlLargeIntegerToChar(const ULONGLONG *,ULONG,ULONG,PCHAR);
NTSTATUS  NTAPI RtlLeaveCriticalSection(RTL_CRITICAL_SECTION *);
DWORD     NTAPI RtlLengthRequiredSid(DWORD);
ULONG     NTAPI RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR);
DWORD     NTAPI RtlLengthSid(PSID);
NTSTATUS  NTAPI RtlLocalTimeToSystemTime(const LARGE_INTEGER*,PLARGE_INTEGER);
BOOLEAN   NTAPI RtlLockHeap(HANDLE);
NTSTATUS  NTAPI RtlLookupAtomInAtomTable(RTL_ATOM_TABLE,const WCHAR*,RTL_ATOM*);

NTSTATUS  NTAPI RtlMakeSelfRelativeSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,LPDWORD);
void      NTAPI RtlMapGenericMask(PACCESS_MASK,const GENERIC_MAPPING*);
NTSTATUS  NTAPI RtlMultiByteToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSTATUS  NTAPI RtlMultiByteToUnicodeSize(DWORD*,LPCSTR,UINT);

NTSTATUS  NTAPI RtlNewSecurityObject(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR*,BOOLEAN,HANDLE,PGENERIC_MAPPING);
PRTL_USER_PROCESS_PARAMETERS NTAPI RtlNormalizeProcessParams(RTL_USER_PROCESS_PARAMETERS*);
ULONG     NTAPI RtlNtStatusToDosError(NTSTATUS);
ULONG     NTAPI RtlNtStatusToDosErrorNoTeb(NTSTATUS);
ULONG     NTAPI RtlNumberOfSetBits(PCRTL_BITMAP);
ULONG     NTAPI RtlNumberOfClearBits(PCRTL_BITMAP);

UINT      NTAPI RtlOemStringToUnicodeSize(const STRING*);
NTSTATUS  NTAPI RtlOemStringToUnicodeString(UNICODE_STRING*,const STRING*,BOOLEAN);
NTSTATUS  NTAPI RtlOemToUnicodeN(LPWSTR,DWORD,LPDWORD,LPCSTR,DWORD);
NTSTATUS  NTAPI RtlOpenCurrentUser(ACCESS_MASK,PHANDLE);

PVOID     NTAPI RtlPcToFileHeader(PVOID,PVOID*);
NTSTATUS  NTAPI RtlPinAtomInAtomTable(RTL_ATOM_TABLE,RTL_ATOM);
BOOLEAN   NTAPI RtlPrefixString(const STRING*,const STRING*,BOOLEAN);
BOOLEAN   NTAPI RtlPrefixUnicodeString(const UNICODE_STRING*,const UNICODE_STRING*,BOOLEAN);

NTSTATUS  NTAPI RtlQueryAtomInAtomTable(RTL_ATOM_TABLE,RTL_ATOM,ULONG*,ULONG*,WCHAR*,ULONG*);
NTSTATUS  NTAPI RtlQueryEnvironmentVariable_U(PWSTR,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  NTAPI RtlQueryInformationAcl(PACL,LPVOID,DWORD,ACL_INFORMATION_CLASS);
NTSTATUS  NTAPI RtlQueryInformationActivationContext(ULONG,HANDLE,PVOID,ULONG,PVOID,SIZE_T,SIZE_T*);
NTSTATUS  NTAPI RtlQueryProcessDebugInformation(ULONG,ULONG,PDEBUG_BUFFER);
NTSTATUS  NTAPI RtlQueryRegistryValues(ULONG, PCWSTR, PRTL_QUERY_REGISTRY_TABLE, PVOID, PVOID);
NTSTATUS  NTAPI RtlQueryTimeZoneInformation(RTL_TIME_ZONE_INFORMATION*);
NTSTATUS  NTAPI RtlQueueWorkItem(PRTL_WORK_ITEM_ROUTINE,PVOID,ULONG);

void      NTAPI RtlRaiseException(PEXCEPTION_RECORD);
void      NTAPI RtlRaiseStatus(NTSTATUS);
ULONG     NTAPI RtlRandom(PULONG);
PVOID     NTAPI RtlReAllocateHeap(HANDLE,ULONG,PVOID,SIZE_T);
void      NTAPI RtlReleaseActivationContext(HANDLE);
void      NTAPI RtlReleasePebLock(void);
void      NTAPI RtlReleaseResource(LPRTL_RWLOCK);
ULONG     NTAPI RtlRemoveVectoredExceptionHandler(PVOID);
void      NTAPI RtlRestoreLastWin32Error(DWORD);

void      NTAPI RtlSecondsSince1970ToTime(DWORD,LARGE_INTEGER *);
void      NTAPI RtlSecondsSince1980ToTime(DWORD,LARGE_INTEGER *);
NTSTATUS  NTAPI RtlSelfRelativeToAbsoluteSD(PSECURITY_DESCRIPTOR,PSECURITY_DESCRIPTOR,
                                             PDWORD,PACL,PDWORD,PACL,PDWORD,PSID,PDWORD,PSID,PDWORD);
void      NTAPI RtlSetAllBits(PRTL_BITMAP);
void      NTAPI RtlSetBits(PRTL_BITMAP,ULONG,ULONG);
ULONG     NTAPI RtlSetCriticalSectionSpinCount(RTL_CRITICAL_SECTION*,ULONG);
NTSTATUS  NTAPI RtlSetCurrentDirectory_U(const UNICODE_STRING*);
void      NTAPI RtlSetCurrentEnvironment(PWSTR, PWSTR*);
NTSTATUS  NTAPI RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSTATUS  NTAPI RtlSetEnvironmentVariable(PWSTR*,PUNICODE_STRING,PUNICODE_STRING);
NTSTATUS  NTAPI RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
NTSTATUS  NTAPI RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR,PSID,BOOLEAN);
void      NTAPI RtlSetLastWin32Error(DWORD);
void      NTAPI RtlSetLastWin32ErrorAndNtStatusFromNtStatus(NTSTATUS);
NTSTATUS  NTAPI RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR,BOOLEAN,PACL,BOOLEAN);
NTSTATUS  NTAPI RtlSetTimeZoneInformation(const RTL_TIME_ZONE_INFORMATION*);
SIZE_T    NTAPI RtlSizeHeap(HANDLE,ULONG,PVOID);
NTSTATUS  NTAPI RtlStringFromGUID(REFGUID,PUNICODE_STRING);
LPDWORD   NTAPI RtlSubAuthoritySid(PSID,DWORD);
LPBYTE    NTAPI RtlSubAuthorityCountSid(PSID);
NTSTATUS  NTAPI RtlSystemTimeToLocalTime(const LARGE_INTEGER*,PLARGE_INTEGER);

void      NTAPI RtlTimeToTimeFields(const LARGE_INTEGER*,PTIME_FIELDS);
BOOLEAN   NTAPI RtlTimeFieldsToTime(PTIME_FIELDS,PLARGE_INTEGER);
void      NTAPI RtlTimeToElapsedTimeFields(const LARGE_INTEGER *,PTIME_FIELDS);
BOOLEAN   NTAPI RtlTimeToSecondsSince1970(const LARGE_INTEGER *,LPDWORD);
BOOLEAN   NTAPI RtlTimeToSecondsSince1980(const LARGE_INTEGER *,LPDWORD);
BOOL      NTAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION *);

ULONGLONG __cdecl RtlUlonglongByteSwap(ULONGLONG);
DWORD     NTAPI RtlUnicodeStringToAnsiSize(const UNICODE_STRING*);
NTSTATUS  NTAPI RtlUnicodeStringToAnsiString(PANSI_STRING,PCUNICODE_STRING,BOOLEAN);
NTSTATUS  NTAPI RtlUnicodeStringToInteger(const UNICODE_STRING *,ULONG,ULONG *);
DWORD     NTAPI RtlUnicodeStringToOemSize(const UNICODE_STRING*);
NTSTATUS  NTAPI RtlUnicodeStringToOemString(POEM_STRING,PCUNICODE_STRING,BOOLEAN);
NTSTATUS  NTAPI RtlUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS  NTAPI RtlUnicodeToMultiByteSize(PULONG,PCWSTR,ULONG);
NTSTATUS  NTAPI RtlUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
ULONG     NTAPI RtlUniform(PULONG);
BOOLEAN   NTAPI RtlUnlockHeap(HANDLE);
void      NTAPI RtlUnwind(PVOID,PVOID,PEXCEPTION_RECORD,PVOID);
#ifdef __ia64__
void      NTAPI RtlUnwind2(FRAME_POINTERS,PVOID,PEXCEPTION_RECORD,PVOID,PCONTEXT);
void      NTAPI RtlUnwindEx(FRAME_POINTERS,PVOID,PEXCEPTION_RECORD,PVOID,PCONTEXT,PUNWIND_HISTORY_TABLE);
#endif
WCHAR     NTAPI RtlUpcaseUnicodeChar(WCHAR);
NTSTATUS  NTAPI RtlUpcaseUnicodeString(UNICODE_STRING*,const UNICODE_STRING *,BOOLEAN);
NTSTATUS  NTAPI RtlUpcaseUnicodeStringToAnsiString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  NTAPI RtlUpcaseUnicodeStringToCountedOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  NTAPI RtlUpcaseUnicodeStringToOemString(STRING*,const UNICODE_STRING*,BOOLEAN);
NTSTATUS  NTAPI RtlUpcaseUnicodeToMultiByteN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
NTSTATUS  NTAPI RtlUpcaseUnicodeToOemN(LPSTR,DWORD,LPDWORD,LPCWSTR,DWORD);
CHAR      NTAPI RtlUpperChar(CHAR);
void      NTAPI RtlUpperString(STRING *,const STRING *);

NTSTATUS  NTAPI RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR);
BOOLEAN   NTAPI RtlValidAcl(PACL);
BOOLEAN   NTAPI RtlValidSid(PSID);
BOOLEAN   NTAPI RtlValidateHeap(HANDLE,ULONG,LPCVOID);
NTSTATUS  NTAPI RtlVerifyVersionInfo(const RTL_OSVERSIONINFOEXW*,DWORD,DWORDLONG);

NTSTATUS  NTAPI RtlWalkHeap(HANDLE,PVOID);
NTSTATUS  NTAPI RtlWriteRegistryValue(ULONG,PCWSTR,PCWSTR,ULONG,PVOID,ULONG);

NTSTATUS  NTAPI RtlpNtCreateKey(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*,ULONG,const UNICODE_STRING*,ULONG,PULONG);
NTSTATUS  NTAPI RtlpNtEnumerateSubKey(HANDLE,UNICODE_STRING *, ULONG);
NTSTATUS  NTAPI RtlpWaitForCriticalSection(RTL_CRITICAL_SECTION *);
NTSTATUS  NTAPI RtlpUnWaitCriticalSection(RTL_CRITICAL_SECTION *);

NTSTATUS NTAPI vDbgPrintEx(ULONG,ULONG,LPCSTR,va_list);
NTSTATUS NTAPI vDbgPrintExWithPrefix(LPCSTR,ULONG,ULONG,LPCSTR,va_list);

/***********************************************************************
 * Inline functions
 */

#define InitializeObjectAttributes(p,n,a,r,s) \
    do { \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
        (p)->RootDirectory = r; \
        (p)->Attributes = a; \
        (p)->ObjectName = n; \
        (p)->SecurityDescriptor = s; \
        (p)->SecurityQualityOfService = NULL; \
    } while (0)

#endif

#define NtCurrentProcess() ((HANDLE)-1)
#define NtCurrentThread() ((HANDLE)-2)

#if 0
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlStoreUlong(p,v)  do { ULONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlStoreUlonglong(p,v) do { ULONGLONG _v = (v); memcpy((p), &_v, sizeof(_v)); } while (0)
#define RtlRetrieveUlong(p,s) memcpy((p), (s), sizeof(ULONG))
#define RtlRetrieveUlonglong(p,s) memcpy((p), (s), sizeof(ULONGLONG))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

static inline BOOLEAN RtlCheckBit(PCRTL_BITMAP lpBits, ULONG ulBit)
{
    if (lpBits && ulBit < lpBits->SizeOfBitMap &&
        lpBits->Buffer[ulBit >> 5] & (1 << (ulBit & 31)))
        return TRUE;
    return FALSE;
}

/* These are implemented as __fastcall, so we can't let Winelib apps link with them */
static inline USHORT RtlUshortByteSwap(USHORT s)
{
    return (s >> 8) | (s << 8);
}
static inline ULONG RtlUlongByteSwap(ULONG i)
{
#if defined(__i386__) && defined(__GNUC__)
    ULONG ret;
    __asm__("bswap %0" : "=r" (ret) : "0" (i) );
    return ret;
#else
    return ((ULONG)RtlUshortByteSwap((USHORT)i) << 16) | RtlUshortByteSwap((USHORT)(i >> 16));
#endif
}

/*************************************************************************
 * Loader functions and structures.
 *
 * Those are not part of standard Winternl.h
 */
typedef struct _LDR_MODULE
{
    LIST_ENTRY          InLoadOrderModuleList;
    LIST_ENTRY          InMemoryOrderModuleList;
    LIST_ENTRY          InInitializationOrderModuleList;
    void*               BaseAddress;
    void*               EntryPoint;
    ULONG               SizeOfImage;
    UNICODE_STRING      FullDllName;
    UNICODE_STRING      BaseDllName;
    ULONG               Flags;
    SHORT               LoadCount;
    SHORT               TlsIndex;
    HANDLE              SectionHandle;
    ULONG               CheckSum;
    ULONG               TimeDateStamp;
    HANDLE              ActivationContext;
} LDR_MODULE, *PLDR_MODULE;

/* those defines are (some of the) regular LDR_MODULE.Flags values */
#define LDR_IMAGE_IS_DLL                0x00000004
#define LDR_LOAD_IN_PROGRESS            0x00001000
#define LDR_UNLOAD_IN_PROGRESS          0x00002000
#define LDR_NO_DLL_CALLS                0x00040000
#define LDR_PROCESS_ATTACHED            0x00080000
#define LDR_MODULE_REBASED              0x00200000

/* FIXME: to be checked */
#define MAXIMUM_FILENAME_LENGTH 256

typedef struct _SYSTEM_MODULE
{
    ULONG               Reserved1;
    ULONG               Reserved2;
    PVOID               ImageBaseAddress;
    ULONG               ImageSize;
    ULONG               Flags;
    WORD                Id;
    WORD                Rank;
    WORD                Unknown;
    WORD                NameOffset;
    BYTE                Name[MAXIMUM_FILENAME_LENGTH];
} SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG               ModulesCount;
    SYSTEM_MODULE       Modules[1]; /* FIXME: should be Modules[0] */
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

NTSTATUS NTAPI LdrDisableThreadCalloutsForDll(HMODULE);
NTSTATUS NTAPI LdrFindEntryForAddress(const void*, PLDR_MODULE*);
NTSTATUS NTAPI LdrLockLoaderLock(ULONG,ULONG*,ULONG*);
NTSTATUS NTAPI LdrQueryProcessModuleInformation(SYSTEM_MODULE_INFORMATION*, ULONG, ULONG*);
NTSTATUS NTAPI LdrUnloadDll(HMODULE);
NTSTATUS NTAPI LdrUnlockLoaderLock(ULONG,ULONG);

/* list manipulation macros */
#define InitializeListHead(le)  (void)((le)->Flink = (le)->Blink = (le))
#define InsertHeadList(le,e)    do { PLIST_ENTRY f = (le)->Flink; (e)->Flink = f; (e)->Blink = (le); f->Blink = (e); (le)->Flink = (e); } while (0)
#define InsertTailList(le,e)    do { PLIST_ENTRY b = (le)->Blink; (e)->Flink = (le); (e)->Blink = b; b->Flink = (e); (le)->Blink = (e); } while (0)
#define IsListEmpty(le)         ((le)->Flink == (le))
#define RemoveEntryList(e)      do { PLIST_ENTRY f = (e)->Flink, b = (e)->Blink; f->Blink = b; b->Flink = f; (e)->Flink = (e)->Blink = NULL; } while (0)
static inline PLIST_ENTRY RemoveHeadList(PLIST_ENTRY le)
{
    PLIST_ENTRY f, b, e;

    e = le->Flink;
    f = le->Flink->Flink;
    b = le->Flink->Blink;
    f->Blink = b;
    b->Flink = f;

    if (e != le) e->Flink = e->Blink = NULL;
    return e;
}
static inline PLIST_ENTRY RemoveTailList(PLIST_ENTRY le)
{
    PLIST_ENTRY f, b, e;

    e = le->Blink;
    f = le->Blink->Flink;
    b = le->Blink->Blink;
    f->Blink = b;
    b->Flink = f;

    if (e != le) e->Flink = e->Blink = NULL;
    return e;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* __WINE_WINTERNL_H */
