#include "stdafx.h"
#include "mj_common.h"
#include "mj_input.h"
#include "game.h"
#include "logo.h"
#include "mj_platform.h"
#include "imgui_impl_sdl.h"

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

struct EventData
{
  int32_t mouseX;
  int32_t mouseY;
  int32_t mouseScroll;
  int width;
  int height;
};

static bool PumpEvents(EventData* pEventData)
{
  ZoneScopedNC("Window message pump", tracy::Color::Aqua);
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    ImGui_ImplSDL2_ProcessEvent(&event);
    switch (event.type)
    {
    case SDL_MOUSEWHEEL:
    {
      const SDL_MouseWheelEvent& e = event.wheel;
      pEventData->mouseScroll += e.y;
    }
    break;
    case SDL_MOUSEBUTTONUP:
    {
      switch (event.button.button)
      {
      case SDL_BUTTON_LEFT:
        break;
      case SDL_BUTTON_MIDDLE:
        break;
      case SDL_BUTTON_RIGHT:
      default:
        break;
      }
    }
    break;
    case SDL_MOUSEBUTTONDOWN:
    {
      switch (event.button.button)
      {
      case SDL_BUTTON_LEFT:
        break;
      case SDL_BUTTON_MIDDLE:
        break;
      case SDL_BUTTON_RIGHT:
        break;
      default:
        break;
      }
    }
    break;
    case SDL_MOUSEMOTION:
    {
      const SDL_MouseMotionEvent& e = event.motion;
      pEventData->mouseX            = e.x;
      pEventData->mouseY            = e.y;
      mj::input::AddRelativeMouseMovement(event.motion.xrel, event.motion.yrel);
    }
    break;
    case SDL_QUIT:
      return false;
    case SDL_WINDOWEVENT:
    {
      const SDL_WindowEvent& wev = event.window;
      switch (wev.event)
      {
      case SDL_WINDOWEVENT_CLOSE:
        if (event.window.windowID == SDL_GetWindowID(s_pWindow)) // Skip ImGui window IDs
        {
          return false;
        }
      case SDL_WINDOWEVENT_RESIZED:
      case SDL_WINDOWEVENT_SIZE_CHANGED:
      {
        if (event.window.windowID == SDL_GetWindowID(s_pWindow)) // Skip ImGui window IDs
        {
          pEventData->width  = wev.data1;
          pEventData->height = wev.data2;
          game::Resize(pEventData->width, pEventData->height);
        }
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
      mj::input::SetKey(s, false);
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
  return true;
}

struct Time
{
  Uint64 lastTime;
  Uint64 perfFreq;
};

static void InitDeltaTime(Time* pTime)
{
  pTime->lastTime = SDL_GetPerformanceCounter();
  pTime->perfFreq = SDL_GetPerformanceFrequency();
}

static void UpdateDeltaTime(Time* pTime)
{
  Uint64 now    = SDL_GetPerformanceCounter();
  Uint64 counts = now - pTime->lastTime;
  float dt      = (float)counts / pTime->perfFreq;
  if (dt > (1.0f / 60.0f))
  {
    dt = 1.0f / 60.0f;
  }
  mj_DeltaTime    = dt;
  pTime->lastTime = now;
}

int32_t CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine,
                          _In_ int nCmdShow)
{
  MJ_DISCARD(hInstance);
  MJ_DISCARD(hPrevInstance);
  MJ_DISCARD(pCmdLine);
  MJ_DISCARD(nCmdShow);
#ifdef _DEBUG
  mj::CreateConsoleWindow();
#endif

  SDL_SetMainReady();

  // Window
  s_pWindow = SDL_CreateWindow("raycaster-fps", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, MJ_WND_WIDTH,
                               MJ_WND_HEIGHT, SDL_WINDOW_RESIZABLE);
  if (!s_pWindow)
  {
    return 1;
  }

  EventData eventData = {};

  SDL_GetWindowSize(s_pWindow, &eventData.width, &eventData.height);

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(s_pWindow, &wmInfo);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  MJ_DISCARD(ImGui::CreateContext());
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
                                                      // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;
  // io.ConfigViewportsNoDefaultParent = true;
  // io.ConfigDockingAlwaysTabBar = true;
  // io.ConfigDockingTransparentPayload = true;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  MJ_DISCARD(ImGui_ImplSDL2_InitForD3D(s_pWindow));
  game::Init(wmInfo.info.win.window);

  mj::input::Init();

  MJ_UNINITIALIZED Time time;
  InitDeltaTime(&time);

  // Run message pump.
  while (true)
  {
    ZoneScopedNC("Game loop", tracy::Color::CornflowerBlue);
    if (!PumpEvents(&eventData))
    {
      break;
    }
    game::NewFrame();
    ImGui_ImplSDL2_NewFrame(s_pWindow);
    mj::input::Update();
    UpdateDeltaTime(&time);
    game::Update(eventData.width, eventData.height);
    FrameMark;
  }

  // Cleanup
  game::Destroy();
  ImGui_ImplSDL2_Shutdown();

  SDL_DestroyWindow(s_pWindow);
  SDL_Quit();

  return 0;
}
