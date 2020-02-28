#include "mj_win32_utils.h"

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <shellapi.h> // CommandLineToArgvW

#include <stdio.h>
#include <assert.h>
#include <algorithm>

static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#ifdef AUDCLNT_E_NOT_INITIALIZED
static const char* FormatAudioClientMessage(HRESULT hr)
{
  switch (hr)
  {
  case AUDCLNT_E_NOT_INITIALIZED:
    return "The IAudioClient object is not initialized.";

  case AUDCLNT_E_ALREADY_INITIALIZED:
    return "The IAudioClient object is already initialized.";

  case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
    return "The AUDCLNT_STREAMFLAGS_LOOPBACK flag is set but the endpoint device is a capture device, not a rendering "
           "device.";

  case AUDCLNT_E_DEVICE_INVALIDATED:
    return "The audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have "
           "been reconfigured, disabled, removed, or otherwise made unavailable for use.";

  case AUDCLNT_E_NOT_STOPPED:
    return "The audio stream was not stopped at the time of the Start call.";

  case AUDCLNT_E_BUFFER_TOO_LARGE:
    return "The NumFramesRequested value exceeds the available buffer space (buffer size minus padding size).";

  case AUDCLNT_E_OUT_OF_ORDER:
    return "A previous IAudioRenderClient::GetBuffer call is still in effect.";

  case AUDCLNT_E_UNSUPPORTED_FORMAT:
    return "The audio engine (shared mode) or audio endpoint device (exclusive mode) does not support the specified "
           "format.";

  case AUDCLNT_E_INVALID_SIZE:
    return "The NumFramesWritten value exceeds the NumFramesRequested value specified in the previous "
           "IAudioRenderClient::GetBuffer call";

  case AUDCLNT_E_DEVICE_IN_USE:
    return "The endpoint device is already in use. Either the device is being used in exclusive mode, or the device is "
           "being used in shared mode and the caller asked to use the device in exclusive mode.";

  case AUDCLNT_E_BUFFER_OPERATION_PENDING:
    return "Buffer cannot be accessed because a stream reset is in progress.";

  case AUDCLNT_E_THREAD_NOT_REGISTERED:
    return "The thread is not registered.";

  case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
    return "The caller is requesting exclusive-mode use of the endpoint device, but the user has disabled "
           "exclusive-mode use of the device.";

  case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
    return "The method failed to create the audio endpoint for the render or the capture device. This can occur if the "
           "audio endpoint device has been unplugged, or the audio hardware or associated hardware resources have been "
           "reconfigured, disabled, removed, or otherwise made unavailable for use.";

  case AUDCLNT_E_SERVICE_NOT_RUNNING:
    return "The Windows audio service is not running.";

  case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
    return "The audio stream was not initialized for event-driven buffering.";

  case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
    return "Exclusive mode only.";

  case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
    return "The AUDCLNT_STREAMFLAGS_EVENTCALLBACK flag is set but parameters hnsBufferDuration and hnsPeriodicity are "
           "not equal.";

  case AUDCLNT_E_EVENTHANDLE_NOT_SET:
    return "The audio stream is configured to use event-driven buffering, but the caller has not called "
           "IAudioClient::SetEventHandle to set the event handle on the stream.";

  case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
    return "Indicates that the buffer has an incorrect size.";

  case AUDCLNT_E_BUFFER_SIZE_ERROR:
    return "Indicates that the buffer duration value requested by an exclusive-mode client is out of range. The "
           "requested duration value for pull mode must not be greater than 500 milliseconds; for push mode the "
           "duration value must not be greater than 2 seconds.";

  case AUDCLNT_E_CPUUSAGE_EXCEEDED:
    return "The audio endpoint device has been unplugged, or the audio hardware or associated hardware rIndicates that "
           "the process-pass duration exceeded the maximum CPU usage. The audio engine keeps track of CPU usage by "
           "maintaining the number of times the process-pass duration exceeds the maximum CPU usage. The maximum CPU "
           "usage is calculated as a percent of the engine's periodicity. The percentage value is the system's CPU "
           "throttle value (within the range of 10% and 90%). If this value is not found, then the default value of "
           "40% is used to calculate the maximum CPU usage.esources have been reconfigured, disabled, removed, or "
           "otherwise made unavailable for use.";

  case AUDCLNT_E_BUFFER_ERROR:
    return "GetBuffer failed to retrieve a data buffer and *ppData points to NULL. For more information, see Remarks.";

  case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
    return "The requested buffer size is not aligned. This code can be returned for a render or a capture device if "
           "the caller specified AUDCLNT_SHAREMODE_EXCLUSIVE and the AUDCLNT_STREAMFLAGS_EVENTCALLBACK flags. The "
           "caller must call Initialize again with the aligned buffer size. For more information, see Remarks.";

  default:
    return "Unknown error message";
  }
}
#endif

// Retrieve the system error message for the last-error code
void mj::win32::Win32PrintLastError(const char* file, int32_t line, const char* expr)
{
  DWORD err = GetLastError();
  mj::win32::Win32PrintError(file, line, expr, err);
}

void mj::win32::Win32PrintError(const char* file, int32_t line, const char* expr, DWORD err)
{
  if (HRESULT_FACILITY(err) == FACILITY_AUDCLNT)
  {
#ifdef AUDCLNT_E_NOT_INITIALIZED
    char buf[1024] = {};
    snprintf(buf, 1024, "%s:%d - %s failed with error 0x%08lx: %s\n", file, line, expr, err,
             FormatAudioClientMessage(err));
    printf(buf);
#endif
  }
  else
  {
    void* lpMsgBuf;

    // Potential recursion issue if FormatMessageW keeps failing
    WIN32_FAIL_IF_ZERO(
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, nullptr));

    // Display the error message
    char buf[1024] = {};
    snprintf(buf, 1024, "%s:%d - %s failed with error 0x%08lx: %ws\n", file, line, expr, err, (LPCWSTR)lpMsgBuf);
    printf(buf);

    LocalFree(lpMsgBuf);
  }
}

int32_t mj::win32::Win32Narrow(char* dst, const wchar_t* src, int32_t bufferSize)
{
  int32_t numBytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, -1, nullptr, 0, nullptr, nullptr);
  WIN32_FAIL_IF_ZERO(numBytes);

  // Buffer overrun protection
  if (numBytes > bufferSize)
  {
    numBytes            = bufferSize - 1;
    dst[bufferSize - 1] = '\0';
  }

  WIN32_FAIL_IF_ZERO(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, -1, dst, numBytes, nullptr, nullptr));
  return numBytes;
}

int32_t mj::win32::Win32Widen(wchar_t* dst, const char* src, int32_t bufferSize)
{
  int32_t numBytes = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, nullptr, 0);
  WIN32_FAIL_IF_ZERO(numBytes);

  // Buffer overrun protection
  if (numBytes > bufferSize)
  {
    numBytes            = bufferSize - 1;
    dst[bufferSize - 1] = '\0';
  }

  WIN32_FAIL_IF_ZERO(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, dst, numBytes));
  return numBytes;
}

void mj::win32::SetThreadName(DWORD dwThreadID, const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType     = 0x1000;
  info.szName     = threadName;
  info.dwThreadID = dwThreadID;
  info.dwFlags    = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)

  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }

#pragma warning(pop)
}

RECT mj::win32::CenteredWindowRectangle(HWND hwnd, LONG width, LONG height, bool frame)
{
  RECT desktopRect = {};

  if (hwnd)
  {
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    MONITORINFO info = {};
    info.cbSize      = sizeof(MONITORINFO);
    WIN32_FAIL_IF_ZERO(GetMonitorInfo(monitor, &info));
    // rcWork takes task bar into account
    desktopRect = info.rcMonitor;
  }
  else
  {
    WIN32_FAIL_IF_ZERO(GetClientRect(GetDesktopWindow(), &desktopRect));
  }

  LONG x = (desktopRect.left + desktopRect.right) / 2 - width / 2;
  LONG y = (desktopRect.bottom + desktopRect.top) / 2 - height / 2;

  RECT windowRect   = {};
  windowRect.left   = x;
  windowRect.top    = y;
  windowRect.right  = x + width;
  windowRect.bottom = y + height;

  if (frame)
  {
    WIN32_FAIL_IF_ZERO(AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE));
  }

  return windowRect;
}

// https://stackoverflow.com/a/4023686
LPSTR* mj::win32::CommandLineToArgvA(LPSTR lpCmdLine, INT* pNumArgs)
{
  int32_t retval;
  retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);

  if (!SUCCEEDED(retval))
  {
    return NULL;
  }

  LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));

  if (lpWideCharStr == NULL)
  {
    return NULL;
  }

  retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);

  if (!SUCCEEDED(retval))
  {
    free(lpWideCharStr);
    return NULL;
  }

  int32_t numArgs;
  LPWSTR* args;
  args = CommandLineToArgvW(lpWideCharStr, &numArgs);
  free(lpWideCharStr);

  if (args == NULL)
  {
    return NULL;
  }

  int32_t storage = numArgs * sizeof(LPSTR);

  for (int32_t i = 0; i < numArgs; ++i)
  {
    BOOL lpUsedDefaultChar = FALSE;
    retval                 = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);

    if (!SUCCEEDED(retval))
    {
      LocalFree(args);
      return NULL;
    }

    storage += retval;
  }

  LPSTR* result = (LPSTR*)LocalAlloc(LMEM_FIXED, storage);

  if (result == NULL)
  {
    LocalFree(args);
    return NULL;
  }

  int32_t bufLen = storage - numArgs * sizeof(LPSTR);
  LPSTR buffer   = ((LPSTR)result) + numArgs * sizeof(LPSTR);

  for (int32_t i = 0; i < numArgs; ++i)
  {
    assert(bufLen > 0);
    BOOL lpUsedDefaultChar = FALSE;
    retval                 = WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);

    if (!SUCCEEDED(retval))
    {
      LocalFree(result);
      LocalFree(args);
      return NULL;
    }

    result[i] = buffer;
    buffer += retval;
    bufLen -= retval;
  }

  LocalFree(args);

  *pNumArgs = numArgs;
  return result;
}

mj::win32::SleepTimer::SleepTimer()
{
  this->waitableTimer = CreateWaitableTimerW(nullptr, true, nullptr);
  WIN32_FAIL_IF_ZERO(this->waitableTimer);
  WIN32_FAIL_IF_ZERO(QueryPerformanceFrequency(&this->perfFreq));
  WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&this->periodBegin));
}

mj::win32::SleepTimer::~SleepTimer()
{
  WIN32_FAIL_IF_ZERO(CloseHandle(this->waitableTimer));
}

// Use a combination of WaitableTimer and YieldProcessor
// Special thanks to Jonathan Blow for this method
void mj::win32::SleepTimer::SleepUntilNextPeriod(LONGLONG frequency)
{
  const LONGLONG frameTime = this->perfFreq.QuadPart / frequency;

  // Get remaining time for this period
  LARGE_INTEGER time;
  WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&time));
  LONGLONG intervalCounts = frameTime - (time.QuadPart - this->periodBegin.QuadPart);

  // Convert to 100 nanosecond intervals
  // Subtract 800 microseconds (8000 hundreds of nanoseconds)
  // to account for WaitableTimer synchronization
  LONGLONG numIntervals = 10000000 * intervalCounts / this->perfFreq.QuadPart - 8000;

  // Negative value implies relative time (as opposed to absolute time)
  time.QuadPart = -std::max(numIntervals, LONGLONG(0));

  WIN32_FAIL_IF_ZERO(SetWaitableTimer(this->waitableTimer, &time, 0, nullptr, nullptr, FALSE));
  WIN32_FAIL_IF_NOT(WaitForSingleObject(this->waitableTimer, INFINITE), WAIT_OBJECT_0);

  // Spin the remaining time using a YieldProcessor loop
  WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&time));

  while ((time.QuadPart - this->periodBegin.QuadPart) < frameTime)
  {
    YieldProcessor();
    WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&time));
  }

  WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&this->periodBegin));
}

LONGLONG mj::win32::SleepTimer::GetPerformanceFrequency() const
{
  return this->perfFreq.QuadPart;
}
