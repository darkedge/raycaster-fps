// Win32 platform implementation
#include "mj_platform.h"
#include "mj_win32.h"
#include "mj_common.h"

#include <imgui.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h> // CommandLineToArgvW
#include <Shlwapi.h>  // PathRemoveFileSpecW
#include <stdio.h>

void mj::win32::CreateConsoleWindow()
{
  MJ_DISCARD(AllocConsole());
  MJ_DISCARD(freopen("CONIN$", "r", stdin));
  MJ_DISCARD(freopen("CONOUT$", "w", stdout));
  MJ_DISCARD(freopen("CONOUT$", "w", stderr));
}
