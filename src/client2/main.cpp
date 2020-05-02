#include "mj_win32.h"
#include "mj_common.h"
#include "imgui_impl_bgfx.h"
#include "mj_input.h"
#include "raytracer.h"

#include <stdio.h>
#include <bx/bx.h>
#include <bx/timer.h>
#include <bx/spscqueue.h>
#include <bx/thread.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "logo.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include "..\..\3rdparty\tracy\Tracy.hpp"

// WARNING: global variable
static float mj_DeltaTime;
namespace mj
{
  float GetDeltaTime()
  {
    return mj_DeltaTime;
  }
} // namespace mj

int32_t CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine,
                          _In_ int nCmdShow)
{
  MJ_DISCARD(hInstance);
  MJ_DISCARD(hPrevInstance);
  MJ_DISCARD(pCmdLine);
  MJ_DISCARD(nCmdShow);
#if BX_PLATFORM_WINDOWS && defined(_DEBUG)
  mj::win32::CreateConsoleWindow();
#endif

  SDL_SetMainReady();

  // Window
  SDL_Window* pWindow = SDL_CreateWindow("raycaster-fps", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         MJ_WND_WIDTH, MJ_WND_HEIGHT, SDL_WINDOW_RESIZABLE);
  if (!pWindow)
  {
    return 1;
  }

  // Initialize bgfx using the native window handle and window resolution.
  bgfx::Init init;

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(pWindow, &wmInfo);
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
  init.platformData.ndt = wmInfo.info.x11.display;
  init.platformData.nwh = (void*)(uintptr_t)wmInfo.info.x11.window;
#elif BX_PLATFORM_OSX
  init.platformData.nwh = wmInfo.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
  init.platformData.nwh = wmInfo.info.win.window;
#endif
#if defined(MJ_DEBUG)
  init.debug = true;
#elif defined(MJ_PROFILE)
  init.profile          = true;
#endif

  int width, height;
  SDL_GetWindowSize(pWindow, &width, &height);

  uint32_t resetFlags = BGFX_RESET_VSYNC | BGFX_RESET_FLIP_AFTER_RENDER | BGFX_RESET_FLUSH_AFTER_RENDER;

  init.resolution.width           = width;
  init.resolution.height          = height;
  init.resolution.reset           = resetFlags;
  init.resolution.numBackBuffers  = 1;
  init.resolution.maxFrameLatency = 1;
  if (!bgfx::init(init))
  {
    return 1;
  }

  rt::Init();

  // Set view 0 to the same dimensions as the window and to clear the color buffer.
  const bgfx::ViewId kClearView = 0;
  bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);

  imguiCreate();

  bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
  int32_t mouseX      = 0;
  int32_t mouseY      = 0;
  int32_t mouseScroll = 0;
  uint8_t mouseMask   = 0;

  mj::input::Init();

  int64_t lastTime = bx::getHPCounter();
  int64_t perfFreq = bx::getHPFrequency();

  // Run message pump.
  bool exit = false;
  while (!exit)
  {
    ZoneScopedNC("Game loop", tracy::Color::CornflowerBlue);
    SDL_Event event;
    {
      ZoneScopedNC("Window message pump", tracy::Color::Aqua);
      while (SDL_PollEvent(&event))
      {
        switch (event.type)
        {
        case SDL_MOUSEWHEEL:
        {
          const SDL_MouseWheelEvent& e = event.wheel;
          mouseScroll += e.y;
        }
        break;
        case SDL_MOUSEBUTTONUP:
        {
          const SDL_MouseButtonEvent& e = event.button;
          switch (e.button)
          {
          case SDL_BUTTON_LEFT:
            mouseMask &= ~IMGUI_MBUT_LEFT;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseMask &= ~IMGUI_MBUT_MIDDLE;
            break;
          case SDL_BUTTON_RIGHT:
            mouseMask &= ~IMGUI_MBUT_RIGHT;
          default:
            break;
          }
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        {
          const SDL_MouseButtonEvent& e = event.button;
          switch (e.button)
          {
          case SDL_BUTTON_LEFT:
            mouseMask |= IMGUI_MBUT_LEFT;
            break;
          case SDL_BUTTON_MIDDLE:
            mouseMask |= IMGUI_MBUT_MIDDLE;
            break;
          case SDL_BUTTON_RIGHT:
            mouseMask |= IMGUI_MBUT_RIGHT;
            break;
          default:
            break;
          }
        }
        break;
        case SDL_MOUSEMOTION:
        {
          const SDL_MouseMotionEvent& e = event.motion;
          mouseX                        = e.x;
          mouseY                        = e.y;
          mj::input::AddRelativeMouseMovement(event.motion.xrel, event.motion.yrel);
        }
        break;
        case SDL_QUIT:
          exit = true;
          break;
        case SDL_WINDOWEVENT:
        {
          const SDL_WindowEvent& wev = event.window;
          switch (wev.event)
          {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          {
            width  = wev.data1;
            height = wev.data2;

            rt::Resize(width, height);
            bgfx::reset((uint32_t)width, (uint32_t)height, resetFlags);
            bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
          }
          break;
          default:
            break;
          }
        }
        break;
        case SDL_KEYDOWN:
        {
          SDL_KeyboardEvent kev = event.key;
          mj::input::SetKey(kev.keysym.sym, true);
        }
        break;
        case SDL_KEYUP:
        {
          SDL_KeyboardEvent kev = event.key;
          mj::input::SetKey(kev.keysym.sym, false);
          switch (kev.keysym.sym)
          {
          case SDLK_F11:
          {
            static int mode       = 0;
            SDL_WindowFlags flags = (SDL_WindowFlags)0;
            switch (++mode)
            {
            case 2:
              flags = SDL_WINDOW_FULLSCREEN;
              break;
            case 1:
              flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
              break;
            case 0:
            default:
              mode = 0;
            }
            SDL_SetWindowFullscreen(pWindow, flags);
          }
          break;
          case SDLK_F12: // Screenshot (.tga)
            bgfx::requestScreenShot(BGFX_INVALID_HANDLE, "screenshot");
            break;
          }
        }
        break;
        default:
          break;
        }
      }
    }

    mj::input::Update();

    int64_t now     = bx::getHPCounter();
    LONGLONG counts = now - lastTime;
    float dt        = (float)counts / perfFreq;
    if (dt > (1.0f / 60.0f))
    {
      dt = 1.0f / 60.0f;
    }
    mj_DeltaTime = dt;
    lastTime     = now;

    {
      ZoneScopedNC("ImGui NewFrame", tracy::Color::BlueViolet);
      imguiBeginFrame(mouseX, mouseY, mouseMask, mouseScroll, (uint16_t)width, (uint16_t)height);
    }

    rt::Update(width, height);

#if 0
    {
      ZoneScopedNC("ImGui Demo", tracy::Color::Burlywood);
      ImGui::ShowDemoWindow();
    }
#endif

    {
      ZoneScopedNC("ImGui render", tracy::Color::BlanchedAlmond);
      imguiEndFrame();
    }

    // Advance to next frame. Process submitted rendering primitives.
    {
      ZoneScopedNC("bgfx::frame()", tracy::Color::Azure);
      bgfx::frame();
      FrameMark;
    }
  }

  rt::Destroy();

  imguiDestroy();

  bgfx::shutdown();
  SDL_DestroyWindow(pWindow);
  SDL_Quit();

  return 0;
}
