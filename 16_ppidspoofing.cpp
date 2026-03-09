#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>


void spawnfakecmdbyexplorer() {
    DWORD explorerpid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"explorer.exe") == 0) {
                explorerpid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    
    HANDLE hEXP = OpenProcess(
        PROCESS_ALL_ACCESS,
        false,
        explorerpid
    );
    if (hEXP == NULL) {
        std::cout << "couldnt open handle to explorer.exe";
    }
    std::cout << "got handle to explorer.exe" << std::endl;
    SIZE_T attributeSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attributeSize);
    PPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeSize);
    if (!InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &attributeSize)) {
        std::cout << "Failed to initialize attribute list." << std::endl;
        return;
    }

    if (!UpdateProcThreadAttribute(
        lpAttributeList,
        0,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
        &hEXP, // Pointer to the handle of the parent
        sizeof(HANDLE),
        NULL,
        NULL)) {
        std::cout << "Failed to update attribute." << std::endl;
        return;
    }

    STARTUPINFOEXW siEx = { 0 };
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    siEx.lpAttributeList = lpAttributeList;

    PROCESS_INFORMATION pi = { 0 };
    WCHAR command[] = L"C:\\Windows\\System32\\notepad.exe";

    BOOL success = CreateProcessW(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &siEx.StartupInfo, 
        &pi
    );

    if (success) {
        std::cout << "succes, notepad created with pid: " << pi.dwProcessId << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else {
        std::cout << "CreateProcessW failed: " << GetLastError() << std::endl;
    }
    DeleteProcThreadAttributeList(lpAttributeList);
    HeapFree(GetProcessHeap(), 0, lpAttributeList);
    CloseHandle(hEXP);

}

int main()
{
    std::cout << "creating fake cmd with explorer parent pid.\n";
    spawnfakecmdbyexplorer();
}
