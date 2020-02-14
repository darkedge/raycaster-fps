#pragma once
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Dbghelp.h> // Minidump

#define __FILENAME__ (strrchr("\\" __FILE__, '\\') + 1)

// Reserved (but unused) in WinUser.h
#define MJ_NUMPAD_ENTER 0x07

#ifdef _DEBUG
#define DEBUG_BREAK() DebugBreak()
#else
#define DEBUG_BREAK() exit(1)
#endif

#define WIN32_FAIL_IF(_expr, x) \
if ((_expr) == (x)) { \
  mj::win32::Win32PrintLastError(__FILENAME__, __LINE__, #_expr); \
  DEBUG_BREAK(); \
}

#define WIN32_FAIL_IF_NOT(_expr, x) \
if ((_expr) != (x)) { \
  mj::win32::Win32PrintLastError(__FILENAME__, __LINE__, #_expr); \
  DEBUG_BREAK(); \
}

#define WIN32_TRY_OR_FAIL(_expr) \
do { \
  HRESULT _hr = (_expr); \
  if (_hr != S_OK && _hr != S_FALSE) { \
    mj::win32::Win32PrintError(__FILENAME__, __LINE__, #_expr, _hr); \
    DEBUG_BREAK(); \
  } \
} while (0)

#define WIN32_ASSERT(_expr) \
do { \
  HRESULT _hr = (_expr); \
  if (_hr != S_OK && _hr != S_FALSE) { \
    mj::win32::Win32PrintError(__FILENAME__, __LINE__, #_expr, _hr); \
    assert(false); \
  } \
} while (0)

#define WIN32_FAIL_IF_ZERO(_expr) WIN32_FAIL_IF(_expr, 0)

#define SAFE_RELEASE(_ptr) \
if ((_ptr) != nullptr) { \
  (_ptr)->Release(); \
  (_ptr) = nullptr; \
}

namespace mj
{
  namespace win32 {

  class SleepTimer
  {
  public:
    SleepTimer();
    ~SleepTimer();

    void SleepUntilNextPeriod(LONGLONG frequency);
    LONGLONG GetPerformanceFrequency() const;

  private:
    HANDLE waitableTimer;
    LARGE_INTEGER perfFreq;
    LARGE_INTEGER periodBegin;
  };

#pragma pack(push,8)
  typedef struct tagTHREADNAME_INFO
  {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
  } THREADNAME_INFO;
#pragma pack(pop)

  typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

  void Win32PrintLastError(const char* file, int32_t line, const char* expr);
  void Win32PrintError(const char* file, int32_t line, const char* expr, DWORD err);
  int32_t Win32Narrow(char* dst, const wchar_t* src, int32_t bufferSize);
  int32_t Win32Widen(wchar_t* dst, const char* src, int32_t bufferSize);
  void SetThreadName(DWORD dwThreadID, const char* threadName);
  RECT CenteredWindowRectangle(HWND hwnd, LONG width, LONG height, bool frame);
  LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT* pNumArgs);
  }
}
