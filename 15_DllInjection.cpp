#include <iostream>
#include <Windows.h>
// dll is in D:\payload.dll

int pid = 17000;
wchar_t path[] = L"D:\\payload.dll";
void InjectDLL() {
    HANDLE pHandle = OpenProcess(
        PROCESS_ALL_ACCESS,
        false,
        pid
    );
    if (pHandle == NULL) {
        std::cout << "couldnt open handle, please recheck PID or privileges.";
        return;
    }
    std::cout << "got handle to process." << std::endl;

    size_t pathSize = (wcslen(path) + 1) * sizeof(wchar_t);
    LPVOID address = VirtualAllocEx(
        pHandle,
        NULL,
        pathSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if (address == NULL) {
        std::cout << "memory allocation failed" << std::endl;
        CloseHandle(pHandle);
        return;
    }
    std::cout << "Memory ALlocated." << std::endl;




    if (!WriteProcessMemory(pHandle, address, path, pathSize, NULL)) {
        std::cout << "failed to write to memory" << std::endl;
        CloseHandle(pHandle);
        return;
    }
    std::cout << "wrote to memory.";
    
    LPVOID loadlib = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    HANDLE remotehandle = CreateRemoteThread(
        pHandle,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)loadlib,
        address,
        0,
        NULL
        );
    if (remotehandle == NULL) {
        std::cout << "failed to inject." << std::endl;
        CloseHandle(pHandle);
    }
    std::cout << "injected successfully." << std::endl;
    WaitForSingleObject(remotehandle, INFINITE);
    CloseHandle(pHandle);
    CloseHandle(remotehandle);
}
int main()
{
    std::cout << "injecting dll.\n";
    InjectDLL();
}
