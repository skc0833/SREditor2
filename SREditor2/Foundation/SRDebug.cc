#if 0
#include <Windows.h> // for OutputDebugStringA
#include "SRDebug.h" // windows.h 뒤에 포함(error C2371: 'LPCWSTR' : 재정의. 기본 형식이 다릅니다)
#include <stdio.h>
#include <tchar.h> // for _tcscpy

#ifndef countof
  #define countof(t)	(sizeof((t)) / sizeof((t)[0]))
#endif	//countof

void _cdecl SR_DebugTraceLineAndFileA(const char* pcszFilename, int nLine, const char* pcszFormat, ...) {
  OutputDebugStringA(pcszFilename);

  char szLineNumber[30] = "(";
  //ltoa(nLine, szLineNumber + 1, 10); // _ltoa
  _ltoa(nLine, szLineNumber + 1, 10);
  strcat(szLineNumber , ") : ");
  OutputDebugStringA(szLineNumber);

  char szBuf[1024] = {0,};
  va_list args = NULL;
  va_start(args, pcszFormat);
  _vsnprintf(szBuf, 1024, pcszFormat, args);
  va_end(args);
  szBuf[1023] = 0;

  OutputDebugStringA(szBuf);
  OutputDebugStringA("\n");
}

void _cdecl SR_DebugTraceA(const char* pcszFormat, ...) {
  char szBuf[1024] = {0,};
  va_list args = NULL;
  va_start(args, pcszFormat);
  _vsnprintf(szBuf, 1024, pcszFormat, args);
  va_end(args);
  szBuf[1023] = 0;

  OutputDebugStringA(szBuf);
  OutputDebugStringA("\n");
}

void _cdecl SR_DebugTraceLineAndFileW(const wchar_t* pcszFilename, int nLine, const wchar_t* pcszFormat, ...) {
  OutputDebugStringW(pcszFilename);

  WCHAR szLineNumber[30] = L"(";
  _ltow(nLine, szLineNumber + 1, 10);
  wcscat(szLineNumber , L") : ");
  OutputDebugStringW(szLineNumber);

  WCHAR szBuf[1024] = {0,};
  va_list args = NULL;
  va_start(args, pcszFormat);
  _vsnwprintf(szBuf, 1024, pcszFormat, args);
  va_end(args);
  szBuf[1023] = 0;

  OutputDebugStringW(szBuf);
  OutputDebugStringW(L"\n");
}

void _cdecl SR_DebugTraceW(const wchar_t* pcszFormat, ...) {
  WCHAR szBuf[1024] = {0,};
  va_list args = NULL;
  va_start(args, pcszFormat);
  _vsnwprintf(szBuf, 1024, pcszFormat, args);
  va_end(args);
  szBuf[1023] = 0;

  OutputDebugStringW(szBuf);
  OutputDebugStringW(L"\n");
}

bool _cdecl SR_AssertFailed(const char* pcszFilename, int nLine, const char* pcszExpression) {
  TCHAR szExeName[MAX_PATH];
  if (!GetModuleFileName(NULL, szExeName, countof(szExeName))) 
    _tcscpy(szExeName, _T("<No Program Name>"));

  char szMessage[1024];
  _snprintf(szMessage, countof(szMessage)
    , "Assertion Failure!"
    "\nProgram: %s"
    "\n"
    "\nFile %s"
    "\nLine %d"
    "\n"
    "\nExpression %s"
    "\n\nPress Retry to debug the application"
    , szExeName
    , pcszFilename
    , nLine
    , pcszExpression
    );

  HWND hwndParent = GetActiveWindow();
  hwndParent = GetLastActivePopup(hwndParent);
  int nCode = MessageBoxA(hwndParent, szMessage, "Debug Helper", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

  if (nCode == IDABORT) {
    exit(nCode);
  }

  if (nCode == IDRETRY)
    return 1;

  return FALSE;
}

bool _cdecl SR_ApiFailure(const char* pcszFilename, int nLine, const char* pcszExpression) {
  const DWORD dwLastError = ::GetLastError();
  LPCTSTR lpMsgBuf;
  (void)::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS
    , NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);

  TCHAR szExeName[MAX_PATH];
  if (!GetModuleFileName(NULL, szExeName, countof(szExeName)))
    _tcscpy(szExeName, _T("<No Program Name>"));

  char szMessage[1024];
  _snprintf(szMessage, countof(szMessage)
    , "API SR_VERIFY Failure!"
    "\nProgram: %s"
    "\n"
    "\nFile %s"
    "\nLine %d"
    "\n"
    "\nExpression %s"
    "\n"
    "\nLast Error %d"
    "\n           %s"
    "\n\nPress Retry to debug the application"
    , szExeName
    , pcszFilename
    , nLine
    , pcszExpression
    , dwLastError
    , lpMsgBuf
    );

  (void)LocalFree((LPVOID)lpMsgBuf);
  HWND hwndParent = ::GetActiveWindow();
  hwndParent = ::GetLastActivePopup(hwndParent);
  int nCode = ::MessageBoxA(hwndParent, szMessage, "Debug Helper", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

  if (nCode == IDABORT) {
    exit(nCode);
  }

  if (nCode == IDRETRY)
    return 1;

  return FALSE;
}
#endif
