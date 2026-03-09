#include <Windows.h>
#include <iostream>


HKEY hKey;

void QueryWindowsVersion() {

  LSTATUS status = RegOpenKeyExW(
      HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
      KEY_QUERY_VALUE, &hKey);
  if (status != ERROR_SUCCESS) {
    std::cout << "failed to openkey";
    RegCloseKey(hKey);
    return;
  }
  std::cout << "opened key.." << std::endl;

  BYTE buffer[256];
  DWORD bufferSize = sizeof(buffer);

  LSTATUS readstatus =
      RegQueryValueExW(hKey, L"ProductName", NULL, NULL, buffer, &bufferSize);
  if (readstatus != ERROR_SUCCESS) {
    std::cout << "Failed to read productname key value";
    RegCloseKey(hKey);
    return;
  }
  std::wcout << L"Product Name: " << (LPCWSTR)buffer << std::endl;

  RegCloseKey(hKey);
}

int main() {
  std::cout << "version reading..\n";
  QueryWindowsVersion();
}
