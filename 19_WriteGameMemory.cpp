#include <iostream>
#include <windows.h>


LPCVOID targetAddress = (LPCVOID)0x00007FF6987A5074;
int currentHealth = 0;
int newHealth = 9999;
HANDLE bighandle;
// 00007FF6987A5074 18556

void ReadGameHealth() {
  bighandle = OpenProcess(
      PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, 18556);
  if (!ReadProcessMemory(bighandle, targetAddress, &currentHealth,
                         sizeof(currentHealth), NULL)) {
    std::cout << "failed to read health" << std::endl;
    return;
  }
  std::cout << "health: " << currentHealth << std::endl;
}

void WriteGameHealth() {
  if (!WriteProcessMemory(bighandle, (LPVOID)targetAddress, &newHealth,
                          sizeof(newHealth), NULL)) {
    std::cout << "failed to write memory" << std::endl;
    return;
  }
  std::cout << "wrote memory successfully" << std::endl;
}

int main() {
  std::cout << "game shit.\n";
  ReadGameHealth();
  WriteGameHealth();
  ReadGameHealth();
  CloseHandle(bighandle);
}
