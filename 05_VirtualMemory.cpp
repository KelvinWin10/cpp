#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <typeinfo>

void AllocateAndWriteMemory() {
  LPVOID myptr =
      VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (myptr == NULL) {
    std::cout << "allocation failed, its a null";
    return;
  }
  char *mypt = (char *)myptr;

  strcpy_s(mypt, 4096, "Hello Kernel!");

  std::wcout << "address: " << (void *)mypt << "string i wrote: " << mypt
             << std::endl;

  if (VirtualFree(mypt, 0, MEM_RELEASE)) {
    std::cout << "memory freed";
  }
}
int main() {
  std::cout << "memory\n";
  AllocateAndWriteMemory();
}
