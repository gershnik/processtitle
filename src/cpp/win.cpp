// Copyright (c) 2026, Eugene Gershnik
// SPDX-License-Identifier: BSD-3-Clause

#include "win.h"
#include "logger.h"

#pragma comment(lib, "ntdll.lib")

namespace undoc { extern "C" {

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define RTL_MAX_DRIVE_LETTERS 32

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PVOID Environment;

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

    ULONG_PTR EnvironmentSize;
    ULONG_PTR EnvironmentVersion;

    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    //More stuff here. This is a vriable length struct
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;;


NTSYSAPI PRTL_USER_PROCESS_PARAMETERS NTAPI
RtlNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params);

NTSYSAPI PRTL_USER_PROCESS_PARAMETERS NTAPI
RtlDeNormalizeProcessParams(PRTL_USER_PROCESS_PARAMETERS Params);

NTSYSAPI PVOID NTAPI 
RtlAllocateHeap(PVOID  HeapHandle, ULONG  Flags, SIZE_T Size);

}}

void windowsPrepare() {

}
    
bool windowsSetProcessTitle(const char * title) {
    int size = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
    if (size <= 0) {
        logDebug("MultiByteToWideChar failed: " + std::to_string(GetLastError()));
        return false;
    }

    std::vector<wchar_t> buf(size);
    MultiByteToWideChar(CP_UTF8, 0, title, -1, buf.data(), int(buf.size()));
    size_t length = wcslen(buf.data());

    PEB * peb = NtCurrentTeb()->ProcessEnvironmentBlock;
    auto params = (undoc::RTL_USER_PROCESS_PARAMETERS *)peb->ProcessParameters;

    if (length < params->CommandLine.MaximumLength / sizeof(wchar_t)) {
        memcpy(params->CommandLine.Buffer, buf.data(), length * sizeof(wchar_t));
        params->CommandLine.Buffer[length] = 0;
        params->CommandLine.Length = USHORT(length * sizeof(wchar_t));
    } else {
        // !!DANGER ZONE!!
        // We are going to replace peb->ProcessParameters.
        // Because somebody could have a pointer to the insides of the previous one
        // we never deallocate it. This creates a small memory leak. Hopefully nobody
        // is crazy enough to change process title to ever increasing values more 
        // than once or twice per process lifetime, but, if he does, he deserves what he gets.
        auto newParams = (undoc::RTL_USER_PROCESS_PARAMETERS *)undoc::RtlAllocateHeap(
                            GetProcessHeap(), 
                            HEAP_ZERO_MEMORY, 
                            params->Length + (length + 1) * sizeof(wchar_t));
        if (!newParams) {
            logDebug("RtlAllocateHeap failed");
            return false;
        }
        RtlDeNormalizeProcessParams(params);
        memcpy(newParams, params, params->Length);
        RtlNormalizeProcessParams(params);
        RtlNormalizeProcessParams(newParams);
        newParams->CommandLine.Buffer = PWSTR((BYTE*)newParams + params->Length);
        memcpy(newParams->CommandLine.Buffer, buf.data(), length * sizeof(wchar_t));
        newParams->CommandLine.Buffer[length] = 0;
        newParams->CommandLine.Length = USHORT(length * sizeof(wchar_t));
        newParams->CommandLine.MaximumLength = USHORT((length + 1) * sizeof(wchar_t));
        peb->ProcessParameters = (RTL_USER_PROCESS_PARAMETERS *)newParams;
    }

    return true;
}
