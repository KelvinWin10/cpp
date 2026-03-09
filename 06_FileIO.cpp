#include <Windows.h>
#include <iostream>
#include <typeinfo>


void ReadWriteFileAPI() {
  HANDLE filehandle =
      CreateFileW(L"D:\\test.txt",              // file name
                  GENERIC_WRITE | GENERIC_READ, // desired access
                  0,                            // share mode
                  NULL,                         // security attributes
                  CREATE_ALWAYS,                // creation deposition
                  FILE_ATTRIBUTE_NORMAL, NULL);
  const char buffer[] = "Writing native bytes to disk!";
  DWORD bytestowrite = (DWORD)strlen(buffer);
  DWORD bytesWritten = 0;
  bool result =
      WriteFile(filehandle, buffer, bytestowrite, &bytesWritten, NULL);
  if (!result) {
    std::cout << "failed to write to file";
    return;
  }
  SetFilePointer(filehandle, 0, NULL, FILE_BEGIN);
  std::cout << "wrote to file" << std::endl;
  char readBuffer[1024] = {0};
  DWORD bytesread = 0;

  bool newres = ReadFile(filehandle, readBuffer, sizeof(readBuffer) - 1,
                         &bytesread, NULL);
  if (!newres) {
    std::cout << "failed to read, " << newres << std::endl;
    return;
  }
  std::cout << "success reading " << bytesread
            << "bytes, content: " << readBuffer << std::endl;
  CloseHandle(filehandle);
}
int main() {
  std::cout << "file write\n";
  ReadWriteFileAPI();
}
