#include <stdio.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <bx/thread.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "logo.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_syswm.h>

#include "mj_common.h"
#include "imgui_impl_bgfx.h"
#include "raytracer.h"

#include "..\..\3rdparty\tracy\Tracy.hpp"

static bool showStats = false;

int32_t CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int32_t nCmdShow)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)pCmdLine;
  (void)nCmdShow;

  SDL_SetMainReady();

  // SDL initialization
#if 0
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    return 1;
  }
#endif

  // Window
  SDL_Window* pWindow = SDL_CreateWindow("raycaster-fps", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         MJ_WND_WIDTH, MJ_WND_HEIGHT, SDL_WINDOW_RESIZABLE);
  if (!pWindow)
  {
    return 1;
  }

  // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
  // Most graphics APIs must be used on the same thread that created the window.
  bgfx::renderFrame();
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
        case SDL_KEYUP:
        {
          SDL_KeyboardEvent kev = event.key;
          switch (kev.keysym.sym)
          {
          case SDLK_F1:
            showStats = !showStats;
            break;
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
          }
        }
        break;
        default:
          break;
        }
      }
    }

    {
      ZoneScopedNC("bgfx debug", tracy::Color::Beige);
      // This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view
      // 0.
      bgfx::touch(kClearView);
      // Use debug font to print information about this example.
      bgfx::dbgTextClear();
      bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20,
                         bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
      bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
      bgfx::dbgTextPrintf(0, 1, 0x0f,
                          "Color can be changed with ANSI "
                          "\x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
      bgfx::dbgTextPrintf(80, 1, 0x0f,
                          "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    "
                          "\x1b[; 7m    \x1b[0m");
      bgfx::dbgTextPrintf(80, 2, 0x0f,
                          "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    "
                          "\x1b[;15m    \x1b[0m");
      const bgfx::Stats* stats = bgfx::getStats();
      bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.",
                          stats->width, stats->height, stats->textWidth, stats->textHeight);
      // Enable stats or debug text.
      bgfx::setDebug(showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
    }

    {
      ZoneScopedNC("ImGui NewFrame", tracy::Color::BlueViolet);
      imguiBeginFrame(mouseX, mouseY, mouseMask, mouseScroll, (uint16_t)width, (uint16_t)height);
    }

    rt::Update();

    {
      ZoneScopedNC("ImGui Demo", tracy::Color::Burlywood);
      ImGui::ShowDemoWindow();
    }

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
