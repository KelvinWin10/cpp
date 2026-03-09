#include <iostream>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

void dostuff() {

  HINTERNET session =
      WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  HINTERNET connection = WinHttpConnect(session, L"127.0.0.1", 8000, 0);
  HINTERNET request =
      WinHttpOpenRequest(connection, NULL, L"/test", NULL, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, NULL);
  bool requeststatus =
      WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (!requeststatus) {
    std::cout << "request failed." << std::endl;
    return;
  }
  std::cout << "request successful" << std::endl;

  bool responsesuccess = WinHttpReceiveResponse(request, NULL);
  if (!responsesuccess) {
    std::cout << "receive response failed";
    return;
  }
  std::cout << "got response." << std::endl;
  DWORD dwSize = 0;
  DWORD dwDownloaded = 0;
  LPSTR pszOutBuffer;

  do {
    dwSize = 0;
    WinHttpQueryDataAvailable(request, &dwSize);
    pszOutBuffer = new char[dwSize + 1];
    if (!pszOutBuffer) {
      std::cout << "out of memory" << std::endl;
      dwSize = 0;
      break;
    }
    ZeroMemory(pszOutBuffer, dwSize + 1);
    bool status =
        WinHttpReadData(request, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);
    if (status)
      std::cout << pszOutBuffer;
    delete[] pszOutBuffer;

  } while (dwSize > 0);

  // Cleanup handles
  if (request)
    WinHttpCloseHandle(request);
  if (connection)
    WinHttpCloseHandle(connection);
  if (session)
    WinHttpCloseHandle(session);
}

int main() {
  std::cout << "http shit.\n";
  dostuff();
}
