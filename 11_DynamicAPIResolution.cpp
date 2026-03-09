#include <Windows.h>
#include <iostream>

typedef int(WINAPI *MessageBoxA_t)(HWND, LPCSTR, LPCSTR, UINT);

void DynamicMessageBox() {
  HMODULE user32handle = LoadLibraryA("user32.dll");
  if (user32handle == NULL) {
    std::cout << "failed to import user32.dll" << std::endl;
    return;
  }
  std::cout << "imported user32.dll" << std::endl;
  FARPROC msgboxaddress = GetProcAddress(user32handle, "MessageBoxA");

  if (msgboxaddress == NULL) {
    std::cout << "failed to getprocesadress." << std::endl;
    CloseHandle(user32handle);
    return;
  }

  std::cout << "found message box address: " << msgboxaddress;
  MessageBoxA_t newmessageboxa = (MessageBoxA_t)(msgboxaddress);

  newmessageboxa(NULL, "Hello world", "yes its working sir",
                 MB_OK | MB_ICONINFORMATION);
  CloseHandle(user32handle);
}
int main() {
  std::cout << "dynamic api resolution..\n";
  DynamicMessageBox();
}
