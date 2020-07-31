#include "pch.h"
// Win32 platform implementation
#include "mj_platform.h"
#include "mj_common.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>

void mj::CreateConsoleWindow()
{
  MJ_DISCARD(AllocConsole());
  MJ_DISCARD(freopen("CONIN$", "r", stdin));
  MJ_DISCARD(freopen("CONOUT$", "w", stdout));
  MJ_DISCARD(freopen("CONOUT$", "w", stderr));
}