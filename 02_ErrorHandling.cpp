#include <Windows.h>
#include <iostream>


void DemonstrateErrorHandling() {
  HANDLE handle = CreateFileW(L"D:\\test.txt",       // lpfilename
                              GENERIC_READ,          // desired access
                              FILE_SHARE_READ,       // share mode
                              NULL,                  // lp security attributes
                              OPEN_EXISTING,         // dw creation deposition
                              FILE_ATTRIBUTE_NORMAL, // dw flags and attributes
                              NULL                   // hTemplate file
  );
  bool failed = (handle == INVALID_HANDLE_VALUE);
  if (failed) {
    DWORD lasterror = GetLastError();

    LPWSTR buffer = nullptr;

    DWORD charsin = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        lasterror, LANG_NEUTRAL, (LPWSTR)&buffer, 0, NULL);
    if (charsin > 0) {
      std::wcout << buffer;
      LocalFree(buffer);
    } else {
      std::cout << "couldnt format";
    }

  } else {
    "handle created";
  }
}

int main() {
  std::cout << "Hello World!\n";
  DemonstrateErrorHandling();
}
