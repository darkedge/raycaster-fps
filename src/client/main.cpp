#include "stdafx.h"
#include "mj_common.h"
#include "mj_input.h"
#include "game.h"
#include "logo.h"

#if BX_PLATFORM_WINDOWS
#include "mj_win32.h"
#include "imgui_win32.h"
#endif

// WARNING: global variable
static float mj_DeltaTime;
static SDL_Window* s_pWindow;

namespace mj
{
  float GetDeltaTime()
  {
    return mj_DeltaTime;
  }

  bool IsWindowMouseFocused()
  {
    Uint32 flags = SDL_GetWindowFlags(s_pWindow);
    return (flags & SDL_WINDOW_MOUSE_FOCUS);
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
  s_pWindow = SDL_CreateWindow("raycaster-fps", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MJ_WND_WIDTH,
                               MJ_WND_HEIGHT, SDL_WINDOW_RESIZABLE);
  if (!s_pWindow)
  {
    return 1;
  }

  int width, height;
  SDL_GetWindowSize(s_pWindow, &width, &height);

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(s_pWindow, &wmInfo);

  game::Init(wmInfo.info.win.window);

  int32_t mouseX      = 0;
  int32_t mouseY      = 0;
  int32_t mouseScroll = 0;
  uint8_t mouseMask   = 0;

  mj::input::Init();

  Uint64 lastTime = SDL_GetPerformanceCounter();
  Uint64 perfFreq = SDL_GetPerformanceFrequency();

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
          auto io = ImGui::GetIO();
          switch (event.button.button)
          {
          case SDL_BUTTON_LEFT:
            io.MouseDown[ImGuiMouseButton_Left] = false;
            break;
          case SDL_BUTTON_MIDDLE:
            io.MouseDown[ImGuiMouseButton_Middle] = false;
            break;
          case SDL_BUTTON_RIGHT:
            io.MouseDown[ImGuiMouseButton_Right] = false;
          default:
            break;
          }
        }
        break;
        case SDL_MOUSEBUTTONDOWN:
        {
          auto io = ImGui::GetIO();
          switch (event.button.button)
          {
          case SDL_BUTTON_LEFT:
            io.MouseDown[ImGuiMouseButton_Left] = true;
            break;
          case SDL_BUTTON_MIDDLE:
            io.MouseDown[ImGuiMouseButton_Middle] = true;
            break;
          case SDL_BUTTON_RIGHT:
            io.MouseDown[ImGuiMouseButton_Right] = true;
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

            game::Resize(width, height);
          }
          break;
          default:
            break;
          }
        }
        break;
        case SDL_KEYDOWN:
        {
          SDL_Scancode s = event.key.keysym.scancode;
          auto io        = ImGui::GetIO();
          mj::input::SetKey(s, true);
          if (s < MJ_COUNTOF(io.KeysDown))
          {
            io.KeysDown[s] = true;
          }
        }
        break;
        case SDL_KEYUP:
        {
          SDL_Scancode s = event.key.keysym.scancode;
          auto io        = ImGui::GetIO();
          mj::input::SetKey(s, false);
          if (s < MJ_COUNTOF(io.KeysDown))
          {
            io.KeysDown[s] = false;
          }
          switch (s)
          {
          case SDL_SCANCODE_F11:
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
            SDL_SetWindowFullscreen(s_pWindow, flags);
          }
          break;
          case SDL_SCANCODE_F12: // Screenshot (.tga)
            // bgfx::requestScreenShot(BGFX_INVALID_HANDLE, "screenshot");
            break;
          }
        }
        break;
        case SDL_TEXTINPUT:
        {
          // char[32] text = the null-terminated input text in UTF-8 encoding
          ImGui::GetIO().AddInputCharactersUTF8(event.text.text);
        }
        break;
        default:
          break;
        }
      }
    }

    mj::input::Update();

    Uint64 now    = SDL_GetPerformanceCounter();
    Uint64 counts = now - lastTime;
    float dt      = (float)counts / perfFreq;
    if (dt > (1.0f / 60.0f))
    {
      dt = 1.0f / 60.0f;
    }
    mj_DeltaTime = dt;
    lastTime     = now;

    game::Update(width, height);
  }

  // Cleanup
  game::Destroy();

  SDL_DestroyWindow(s_pWindow);
  SDL_Quit();

  return 0;
}
