#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <imgui.h>

#include <shobjidl.h> // Save/Load dialogs
#include <shlobj.h> // Save/Load dialogs
#include <d3d11.h>

#include <bx/bx.h>
#include <bx/timer.h>
#include <bx/thread.h>
#include <bx/allocator.h>
#include <bx/error.h>

#include <bimg/bimg.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include "..\..\3rdparty\tracy\Tracy.hpp"

#include <stdio.h>
#include <stdint.h>
