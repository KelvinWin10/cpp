#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

using namespace std;



extern "C" NTSTATUS Indirect(
    HANDLE hProcess,
    PVOID lpBaseAddress,
    PVOID lpBuffer,
    SIZE_T nSize,
    PSIZE_T lpNumberOfBytesWritten,
    DWORD ssn,           
    UINT_PTR gadgetAddr   
);


DWORD getpid(const wchar_t* processname) {
    DWORD foundPid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    Process32First(hSnap, &pe);
    do {
        std::wstring currentProcessName = pe.szExeFile;

        if (currentProcessName == processname) {
            foundPid = pe.th32ProcessID;
            std::cout << "found the target's PID: " << foundPid << endl;
            break;
        }
    } while (Process32Next(hSnap, &pe));


    CloseHandle(hSnap);
    return foundPid;
}


LPVOID findNtWriteVirtualMemory() {
    LPVOID found = NULL;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == NULL) {
        cout << "failed to get handle to ntdll.dll, error: " << GetLastError() << endl;
        return found;
    }
    std::cout << "Got handle to ntdll.dll" << endl;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ntdll;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        std::cout << "file's magic number doesn't match.." << endl;
        
        return found;
    }
    
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE*)ntdll + dosHeader->e_lfanew);
    if (ntHeader->Signature != IMAGE_NT_SIGNATURE) {
        cout << "file signature doesn't match nt signature. " << endl;
        return found;
    }
    PIMAGE_OPTIONAL_HEADER64 optionalHeader = (PIMAGE_OPTIONAL_HEADER64) & (ntHeader->OptionalHeader);
    IMAGE_DATA_DIRECTORY datadirectory = (IMAGE_DATA_DIRECTORY)optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];


    PIMAGE_EXPORT_DIRECTORY exportdirec = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)ntdll + datadirectory.VirtualAddress);
    PDWORD NamesArray = (PDWORD)((BYTE*)ntdll + exportdirec->AddressOfNames);
    PDWORD pdwFunctions = (PDWORD)((BYTE*)ntdll + exportdirec->AddressOfFunctions);
    PWORD pwOrdinals = (PWORD)((BYTE*)ntdll + exportdirec->AddressOfNameOrdinals);

    for (int i = 0; i < exportdirec->NumberOfNames; i++) {
        char* functionName = (char*)((BYTE*)ntdll + NamesArray[i]);
        if (strcmp(functionName, "NtWriteVirtualMemory") == 0) {
            WORD ordinal = pwOrdinals[i];
            DWORD functionRVA = pdwFunctions[ordinal];

            found = (LPVOID)((BYTE*)ntdll + functionRVA);
            break;
        }
    }
    return found;
    

}

DWORD extractssn(LPVOID addy) {
    DWORD ssn = 0;
    BYTE* newaddy = (BYTE*)addy;

    for (int i = 0; i < 12; i++) {
        if (newaddy[i] == 0xB8) {

            ssn = *(DWORD*)(&newaddy[i + 1]);
            cout << "found the ssn: 0x" << hex << ssn << endl;
            break;
        }
    }
    return ssn;
}

UINT_PTR findsyscall(LPVOID functionAddress) {
    BYTE* functionaddy = (BYTE*)functionAddress;
    UINT_PTR found = 0x1;
    for (int i = 0; i < 32; i++) {
        if (functionaddy[i] == 0x0F && functionaddy[i + 1] == 0x05 && functionaddy[i + 2] == 0xC3) {
            found = (UINT_PTR)&functionaddy[i];
            break;
        }
    }
    return found;
}
int main()
{
    
    cout << "indirect syscall" << endl;
    LPVOID ntwrite = findNtWriteVirtualMemory();
    DWORD ssn = extractssn(ntwrite);
    if (ssn == 0) {
        cout << "failed to extract ssn. its equal to 0.." << endl;
        return 0;
    }

    UINT_PTR gadget = findsyscall(ntwrite);
    if (gadget == 0x1) {
        cout << "failed to find the syscall address." << endl;
        return 0;
    }
    cout << "Found the syscall address: 0x" << hex << gadget << endl;

    DWORD pid = getpid(L"DummyApp.exe");
    if (pid == 0) {
        return 1;
    }
    LPVOID targetAddress = (LPVOID)0x00007FF643185074;
    int newValue = 70;
    HANDLE h = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, pid);

    NTSTATUS status = Indirect(h, targetAddress, &newValue, sizeof(newValue), NULL, ssn, gadget);
    cout << "NTSTATUS: 0x" << hex << status << endl;


    return 0;
}


syscalls.asm: 
.code

Indirect PROC
    mov r10, rcx          
    mov eax, [rsp + 48]  
    mov r11, [rsp + 56]    
    jmp r11
Indirect ENDP

end