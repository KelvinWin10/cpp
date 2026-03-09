#include <iostream>
#include <Windows.h>

unsigned char payload[] = { 0x90, 0x90, 0xC3 };

void ExecuteLocalPayload() {
    LPVOID newaddress = VirtualAlloc(
        NULL,
        sizeof(payload),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    std::cout << "Allocated memory" << std::endl;

    RtlMoveMemory(newaddress, payload, sizeof(payload));
    std::cout << "moved memory" << std::endl;

    HANDLE mythread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)newaddress,
        NULL,
        0,
        NULL
    );
    if (mythread != NULL) {
        std::cout << "created thread successfully" << std::endl;
    }
    WaitForSingleObject(
        mythread, INFINITE
    );
    std::cout << "finished. (this is from the main thread)";

    CloseHandle(mythread);
}
int main()
{
    std::cout << "thread..\n";
    ExecuteLocalPayload();
}
