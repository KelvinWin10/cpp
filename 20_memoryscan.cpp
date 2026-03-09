#include <windows.h>
#include <tlhelp32.h> 
#include <iostream>
#include <string>
#include <vector>
PROCESSENTRY32 pe32;


// 00007FF689F75074
DWORD GetProcessIdByName(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    pe32.dwSize = sizeof(PROCESSENTRY32);



    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return 0;
    }
    do {
        if (_wcsicmp(pe32.szExeFile, processName) == 0) {
            pid = pe32.th32ProcessID;
            break;
        }
        
    } while (Process32Next(hSnapshot, &pe32));
    CloseHandle(hSnapshot);
    return pid;


}
uintptr_t FindHealthAddress(DWORD targetPid, int targetValue) {
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, targetPid);
    if (hProcess == NULL) return 0;
    uintptr_t currentAddress = 0;
    MEMORY_BASIC_INFORMATION mbi;

    while (VirtualQueryEx(hProcess, (LPCVOID)currentAddress, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_READONLY || mbi.Protect == PAGE_EXECUTE_READWRITE)) {
            std::vector<BYTE> buffer(mbi.RegionSize);
            if (!ReadProcessMemory(hProcess, (LPCVOID)currentAddress, buffer.data(), mbi.RegionSize, nullptr)) return 0;
            
            for (size_t i = 0; i < mbi.RegionSize - sizeof(int); i++) {
                int* currentinttemplate = (int*)&buffer[i];
                if (*currentinttemplate == targetValue) {
                    uintptr_t foundAddress = currentAddress + i;
                    CloseHandle(hProcess);
                    return foundAddress;
                }
            }

        }
        currentAddress = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    CloseHandle(hProcess);
    return 0;

}

int main()
{
    std::cout << "helo shit.\n";
    DWORD dummypid = GetProcessIdByName(L"DummyApp.exe");
    std::cout << "found pid: " << dummypid << std::endl;
    uintptr_t healthmemoryaddress = FindHealthAddress(dummypid, 88776655);
    

    if (healthmemoryaddress != 0) {
        std::cout << "found health memory address at " << healthmemoryaddress << std::endl;
        HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, dummypid);
        int newhealth = 1337;
        WriteProcessMemory(hProcess, (LPVOID)healthmemoryaddress, &newhealth, sizeof(newhealth), NULL);
        std::cout << "changed to 1337" << std::endl;
    }
}
