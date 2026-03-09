#include <Lmcons.h>
#include <Windows.h>
#include <iostream>
#include <string>
#include <typeinfo>

void PrintSystemIdentifiers() {
  WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = ARRAYSIZE(buffer);

  bool success = GetComputerName(buffer, &size);
  if (success) {
    std::cout << "Computer Name: ";
    std::wcout << buffer << std::endl;
  } else {
    std::cout << "failed to get computer name" << std::endl;
  }

  WCHAR newbuffer[UNLEN + 1];
  DWORD newsize = ARRAYSIZE(newbuffer);

  bool newsuccess = GetUserNameW(newbuffer, &newsize);
  if (newsuccess) {
    std::cout << "username found: ";
    std::wcout << newbuffer << std::endl;
  } else {
    std::cout << "failed to get username";
  }
}
int main() { PrintSystemIdentifiers(); }
