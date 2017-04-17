#include "SRLog.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>   // for _ltoa
#include <stdarg.h>   // for va_start

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  // for OutputDebugStringA

#if 0
inline void NSLog(std::wstring format, ...) {
    wchar_t buffer[256];
    va_list args;

    format += L"\n";

    va_start(args, format);
#ifdef _WIN32_WCE
    // CE5 doesn't have _s variants of sprintf but these are
    // suppose to be "safe"
    StringCchVPrintf(buffer, 256, format.c_str(), args);
#else
    vswprintf_s(buffer, 256, format.c_str(), args);
#endif
    va_end(args);
        
// IMPORTANT: the va_list isn't working correctly in CE 5..
// its outputting an address instead? Removing until
// I can figure it out..that means NO NSLog will do anything
#ifndef _WIN32_WCE
    // Output to Visual Studio Console
    OutputDebugStringW(buffer);
#endif
}
#endif

#define LOG_BUF_SIZE	1024
#define LOG_FILENAME_LINE

#ifndef countof
#define countof(t)	(sizeof((t)) / sizeof((t)[0]))
#endif //countof

// Apply color attribute to the console trace.
#define TRACE_RED	  (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define TRACE_GREEN (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define TRACE_BLUE	(FOREGROUND_BLUE | FOREGROUND_INTENSITY)

static int assert_failed_a(const char* filename, int line, const char* expression) {
  char szExeName[MAX_PATH];
  if (!GetModuleFileNameA(NULL, szExeName, countof(szExeName))) 
    strcpy(szExeName, "<No Program Name>");

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
    , filename
    , line
    , expression
    );

  HWND hwndParent = GetActiveWindow();
  hwndParent = GetLastActivePopup(hwndParent);
  int nCode = MessageBoxA(hwndParent, szMessage, "Debug Helper", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

  if (nCode == IDABORT) {
    exit(nCode);
  }
  if (nCode == IDRETRY)
    return 1;

  return 0; // false
}

static int assert_failed_w(const wchar_t* filename, int line, const wchar_t* expression) {
  wchar_t szExeName[MAX_PATH];
  if (!GetModuleFileNameW(NULL, szExeName, countof(szExeName))) 
    wcscpy(szExeName, L"<No Program Name>");

  wchar_t szMessage[1024];
  _snwprintf(szMessage, countof(szMessage)
    , L"Assertion Failure!"
    L"\nProgram: %s"
    L"\n"
    L"\nFile %s"
    L"\nLine %d"
    L"\n"
    L"\nExpression %s"
    L"\n\nPress Retry to debug the application"
    , szExeName
    , filename
    , line
    , expression
    );

  HWND hwndParent = GetActiveWindow();
  hwndParent = GetLastActivePopup(hwndParent);
  int nCode = MessageBoxW(hwndParent, szMessage, L"Debug Helper", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

  if (nCode == IDABORT) {
    exit(nCode);
  }
  if (nCode == IDRETRY)
    return 1;

  return 0; // false
}

static int not_impl_w(const wchar_t* filename, int line, const wchar_t* function) {
  wchar_t szExeName[MAX_PATH];
  if (!GetModuleFileNameW(NULL, szExeName, countof(szExeName)))
    wcscpy(szExeName, L"<No Program Name>");

  wchar_t szMessage[1024];
  _snwprintf(szMessage, countof(szMessage)
    , L"Not Implemented!"
    L"\nProgram: %s"
    L"\n"
    L"\nFile %s"
    L"\nLine %d"
    L"\n"
    L"\nFunction %s"
    L"\n\nPress Retry to debug the application"
    , szExeName
    , filename
    , line
    , function
  );

  HWND hwndParent = GetActiveWindow();
  hwndParent = GetLastActivePopup(hwndParent);
  int nCode = MessageBoxW(hwndParent, szMessage, L"Debug Helper", MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

  if (nCode == IDABORT) {
    exit(nCode);
  }
  if (nCode == IDRETRY)
    return 1;

  return 0; // false
}

static void print_to_console_a(int prio, const char* buf) {
  static char* conBuf;
  if (!conBuf) {
    AllocConsole();
    HANDLE hHeap = GetProcessHeap();
    conBuf = (char*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, LOG_BUF_SIZE);
  }
  strcpy(conBuf, buf);

  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

  //	색상 정보 추가.
  WORD wColor = 0; // for default white color???
  WORD wOldColorAttrs = 0;
  switch (prio) {
    case SR_LOG_ERROR:
    case SR_LOG_FATAL:
      wColor = TRACE_RED;
      break;
  }

  if (wColor > 0) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hStdout, &csbi)) {
      //ASSERT(0);
      return;
    }
    wOldColorAttrs = csbi.wAttributes;
    if (!SetConsoleTextAttribute(hStdout, wColor)) {
      //ASSERT(0);
      return;
    }
  }

  DWORD dw;
  WriteFile(hStdout, conBuf, strlen(conBuf), &dw, NULL);
  //HeapFree(GetProcessHeap(), 0, conBuf); // don't need because HeapAlloc() called once.

  if (wOldColorAttrs > 0) {
    if (!SetConsoleTextAttribute(hStdout, wOldColorAttrs)) {
      //ASSERT(0);
      return;
    }
  }
}

static void print_to_console_w(int prio, const wchar_t* buf) {
  static wchar_t* conBuf;
  if (!conBuf) {
    AllocConsole();
    HANDLE hHeap = GetProcessHeap();
    conBuf = (wchar_t*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, LOG_BUF_SIZE * sizeof(wchar_t));
  }
  wcscpy(conBuf, buf);

  HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

  //	색상 정보 추가.
  WORD wColor = 0; // for default white color???
  WORD wOldColorAttrs = 0;
  switch (prio) {
    case SR_LOG_ERROR:
    case SR_LOG_FATAL:
      wColor = TRACE_RED;
      break;
  }

  if (wColor > 0) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hStdout, &csbi)) {
      //ASSERT(0);
      return;
    }
    wOldColorAttrs = csbi.wAttributes;
    if (!SetConsoleTextAttribute(hStdout, wColor)) {
      //ASSERT(0);
      return;
    }
  }

  DWORD dw;
  //WriteFile(hStdout, conBuf, wcslen(conBuf), &dw, NULL);
  WriteConsoleW(hStdout, conBuf, wcslen(conBuf), &dw, 0);
  //HeapFree(GetProcessHeap(), 0, conBuf); // don't need because HeapAlloc() called once.

  if (wOldColorAttrs > 0) {
    if (!SetConsoleTextAttribute(hStdout, wOldColorAttrs)) {
      //ASSERT(0);
      return;
    }
  }
}

void __sr_log_print_a(const char* filename, int line, int prio, const char* tag, const char* fmt, ...) {
  char buf[LOG_BUF_SIZE] = {0,};
  char* p = buf;

#ifdef LOG_FILENAME_LINE
  if (prio >= SR_LOG_FATAL) {
    // print "filename (line): "
    strcat(buf, filename);
    p = buf + strlen(buf);
    strcat(buf, "(");
    p = buf + strlen(buf);
    _ltoa(line, p, 10);
    strcat(buf, ") : ");
    p = buf + strlen(buf);
  }
#endif

  // print "prio/tag: "
  char level[3] = {'U', '/', 0};
  switch (prio) {
    case SR_LOG_DEBUG:
      *level = 'D';
      break;
    case SR_LOG_ERROR:
      *level = 'E';
      break;
    case SR_LOG_FATAL:
      *level = 'F';
      break;
  }
  strcat(buf, level);

  if (tag) {
    strcat(buf, tag);
  }
  strcat(buf, ": ");

  int used_len = strlen(buf);
  p = buf + used_len;

  va_list args = NULL;
  va_start(args, fmt);
  _vsnprintf(p, LOG_BUF_SIZE - used_len - 2, fmt, args); // -1 for '\n'
  va_end(args);
  strcat(buf, "\n");

  OutputDebugStringA(buf);

  // print to Console
  //print_to_console_a(prio, buf);
}

void __sr_log_print_w(const wchar_t* filename, int line, int prio, const wchar_t* tag, const wchar_t* fmt, ...) {
  wchar_t buf[LOG_BUF_SIZE] = {0,};
  wchar_t* p = buf;

#ifdef LOG_FILENAME_LINE
  if (prio >= SR_LOG_FATAL) {
    // print "filename (line): "
    wcscat(buf, filename);
    p = buf + wcslen(buf);
    wcscat(buf, L"(");
    p = buf + wcslen(buf);
    _ltow(line, p, 10);
    wcscat(buf, L") : ");
    p = buf + wcslen(buf);
  }
#endif

  // print "prio/tag: "
  wchar_t level[3] = {'U', '/', 0};
  switch (prio) {
    case SR_LOG_DEBUG:
      *level = 'D';
      break;
    case SR_LOG_ERROR:
      *level = 'E';
      break;
    case SR_LOG_FATAL:
      *level = 'F';
      break;
  }
  wcscat(buf, level);

  if (tag) {
    wcscat(buf, tag);
  }
  // thread id
  p = buf + wcslen(buf);
  wcscat(buf, L"(");
  p = buf + wcslen(buf);
  _ltow(GetCurrentThreadId(), p, 10);
  wcscat(buf, L") : ");

  //wcscat(buf, L": "); // 이게 왜 있었지???

  int used_len = wcslen(buf);
  p = buf + used_len;

  va_list args = NULL;
  va_start(args, fmt);
  _vsnwprintf(p, LOG_BUF_SIZE - used_len - 2, fmt, args); // -1 for '\n'
  va_end(args);
  wcscat(buf, L"\n");

  OutputDebugStringW(buf);

  // print to Console
  //print_to_console_w(prio, buf);
}

void __sr_log_assert_a(const char* filename, int line, const char* cond, const char* tag, const char* fmt, ...) {
  if (cond) {
    char buf[LOG_BUF_SIZE] = {0,};
    va_list args = NULL;
    va_start(args, fmt);
    _vsnprintf(buf, LOG_BUF_SIZE - 1, fmt, args); // -1 for '\n'
    va_end(args);
    __sr_log_print_a(filename, line, SR_LOG_FATAL, tag, buf);
    assert_failed_a(filename, line, cond);
  }
}

void __sr_log_assert_w(const wchar_t* filename, int line, const wchar_t* cond, const wchar_t* tag, const wchar_t* fmt, ...) {
  if (cond) {
    wchar_t buf[LOG_BUF_SIZE] = {0,};
    va_list args = NULL;
    va_start(args, fmt);
    _vsnwprintf(buf, LOG_BUF_SIZE - 1, fmt, args); // -1 for '\n'
    va_end(args);
    __sr_log_print_w(filename, line, SR_LOG_FATAL, tag, buf);
    assert_failed_w(filename, line, cond);
  }
}

void __sr_not_impl_w(const wchar_t* filename, int line, const wchar_t* func) {
  __sr_log_print_w(filename, line, SR_LOG_FATAL, nullptr, L"Not Implemented!");
  not_impl_w(filename, line, func);
}
