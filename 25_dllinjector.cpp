#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

using namespace std;
wchar_t dllpath[] = L"D:\\msgbox.dll";
DWORD getpid(WCHAR* processname) {
    DWORD foundPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    Process32First(hSnap, &pe);
    do {
        std::wstring currentProcessName = pe.szExeFile;
        if (currentProcessName == processname) {
            foundPid = pe.th32ProcessID;
            std::cout << "found notepad's PID: " << foundPid << endl;
            break;
        }
    } while (Process32Next(hSnap, &pe));


    CloseHandle(hSnap);
    return foundPid;
}


BOOL inject(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    if (h == NULL) {
        cout << "failed to open handle to process PID: " << pid << endl;
        return FALSE;
    }
    cout << "Successfully Got A Handle To The Process." << endl;


    LPVOID addy = VirtualAllocEx(h, NULL, sizeof(dllpath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (addy == NULL) {
        cout << "failed to allocate space in process memory. Error code: " << GetLastError() << endl;
        CloseHandle(h);
        return FALSE;
    }
    cout << "successfully allocated memory at: " << hex << addy <<endl;

    if (!WriteProcessMemory(h, addy, dllpath, sizeof(dllpath), NULL)) {
        cout << "failed to write the dll path string to the process's memory" << GetLastError() << endl;
        CloseHandle(h);
        return FALSE;
    }
    std::cout << "successfully wrote the DLL path into the process's memory. " << endl;
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    FARPROC WLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(h, NULL, 0, (LPTHREAD_START_ROUTINE)WLoadLibraryW, addy, 0, NULL);
    if (hThread == NULL) {
        cout << "failed to create remote thread. error: " << GetLastError() << endl;
        CloseHandle(h);
        return FALSE;
    }
    std::cout << "successfully injected dll into process." << endl;

    CloseHandle(h);
    return TRUE;

}

int main()
{
    
    std::cout << "Dll injector\n";
    DWORD pid = getpid((WCHAR*)L"notepad.exe");
    if (pid == 0) {
        std::cout << "failed to find the target's pid. make sure its running" << endl;
        return 0;
    }

    inject(pid);
    return 0;
}

