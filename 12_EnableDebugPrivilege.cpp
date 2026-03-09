#include <Windows.h>
#include <iostream>

void enabletoken() {
  HANDLE thishandle = GetCurrentProcess();
  HANDLE hToken;
  bool success = OpenProcessToken(
      thishandle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
  if (!success) {
    std::cout << "failed to get token" << std::endl;
    return;
  }
  std::cout << "got token." << std::endl;
  LUID luid;
  bool looksuccess = LookupPrivilegeValueW(NULL, L"SeDebugPrivilege", &luid);
  if (!looksuccess) {
    std::cout << "failed to lookup privilege value." << std::endl;
    return;
  }
  std::cout << "got LUID" << std::endl;
  TOKEN_PRIVILEGES tp;
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  bool adjustsuccess = AdjustTokenPrivileges(
      hToken, false, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
  if (!adjustsuccess) {
    std::cout << "failed to adjust token privileges." << std::endl;
    return;
  }
  std::cout << "adjusted token privileges." << std::endl;
}
int main() {
  std::cout << "token privileges change..\n";
  enabletoken();
}
