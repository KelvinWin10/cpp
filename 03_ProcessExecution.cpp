#include <Windows.h>
#include <iostream>
#include <typeinfo>


STARTUPINFOW si = {0};

PROCESS_INFORMATION pi = {0};

void LaunchAndAwaitProcess() {

  WCHAR cmdLine[] = L"notepad.exe";
  bool startsuccess = CreateProcessW(NULL,    // application name
                                     cmdLine, // command line
                                     NULL,    // process attributes
                                     NULL,    // thread attributes,
                                     FALSE,   // inherit handles
                                     0, NULL, NULL, &si, &pi);
  if (!startsuccess) {
    std::cout << "failed to open notepad";
    return;
  }
  std::cout << "opened notepad" << std::endl;
  DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
  if (result == WAIT_OBJECT_0) {
    std::cout << "notepad was closed" << std::endl;
  }

  if (CloseHandle(pi.hProcess) & CloseHandle(pi.hThread)) {
    std::cout << "handle and thread all closed" << std::endl;
  }
}
int main() {
  std::cout << "Hello World!\n";
  si.cb = sizeof(si);
  LaunchAndAwaitProcess();
}
