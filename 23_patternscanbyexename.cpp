#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <iostream>
#include <sstream>

std::wstring target = L"explorer.exe";

DWORD modsize;
void* addr;
HANDLE h;
DWORD pid;

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
		std::cout << " process 32 first failed." << GetLastError() <<  std::endl;
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

void getbounds(DWORD pid) {
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (h == INVALID_HANDLE_VALUE) {
		std::cout << "failed to get handle to snapshot. exiting" << std::endl;
		return;
	}

	MODULEENTRY32 me;
	me.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(h, &me)) {
		do {
			if ((std::wstring)me.szModule == target) {
				std::cout << "found module in process successfully." << std::endl;
				modsize = me.modBaseSize;
				addr = me.modBaseAddr;
				CloseHandle(h);
				return;
			}
			
		} while (Module32Next(h, &me));
		CloseHandle(h);
	}
	else
	{
		std::cout << "failed to get first module, exiting as it failed." << GetLastError() << std::endl;
		CloseHandle(h);
		return;
	}

}

std::vector<uint8_t> phase2() {
	std::vector<uint8_t> buffer(modsize + 1);
	HANDLE realh = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (realh == NULL) {
		std::cout << "failed to open process handle" << GetLastError() << std::endl;
		return {};
	}
	if (ReadProcessMemory(
		realh,
		addr,
		buffer.data(),
		modsize,
		NULL
	)) {
		std::cout << "read memory into buffer successfully." << std::endl;
		CloseHandle(realh);
		return buffer;
	}
	else {
		std::cout << "failed to readprocessmemory.." << GetLastError() << std::endl;
		CloseHandle(realh);
		return {};
	}

}

void findaddressbypattern(const std::vector<uint8_t>& buffer, const std::string& pattern) {
	std::vector<int> signature;
	std::stringstream ss(pattern);
	std::string word;


	while (ss >> word) {
		if (word == "?") signature.push_back(-1);
		else signature.push_back(std::stoi(word, nullptr, 16));
	}


	for (size_t i = 0; i < buffer.size() - signature.size(); i++) {

		bool mismatch = false;
		for (size_t j = 0; j < signature.size(); j++) {
			if (signature[j] != -1 && signature[j] != buffer[i + j]) {
				mismatch = true;
				break; 
			}
		}
		if (!mismatch) {
			uintptr_t foundAddr = (uintptr_t)addr + i;
			std::cout << "Pattern found at: 0x" << std::hex << foundAddr << std::dec << std::endl;
			return; 
		}
	}

	std::cout << "Pattern not found." << std::endl;


}

int main()
{
	pid = getpid();
	if (pid == 0) return 0; // i know system is 0, but i dont care since no one is going to literally read system memory..


	std::cout << "found Target pid:  " << pid << std::endl;
	getbounds(pid);
	std::vector<uint8_t> buffer = phase2();

	findaddressbypattern(buffer, "55 8B EC ? ? ? ? 83 EC");


}
