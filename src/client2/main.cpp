#include <stdio.h>
#include <bx/bx.h>
#include <bx/spscqueue.h>
#include <bx/thread.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include "logo.h"
#include <SDL.h>
#include <SDL_syswm.h>

static bx::DefaultAllocator s_allocator;
static bx::SpScUnboundedQueue s_apiThreadEvents(&s_allocator);

enum class EventType
{
  Exit,
  Key,
  Resize
};

struct ExitEvent
{
  EventType type = EventType::Exit;
};

struct KeyEvent
{
  EventType type = EventType::Key;
  int key;
  bool pressed;
};

struct ResizeEvent
{
  EventType type = EventType::Resize;
  uint32_t width;
  uint32_t height;
};

struct ApiThreadArgs
{
  bgfx::PlatformData platformData;
  uint32_t width;
  uint32_t height;
};

static int32_t runApiThread(bx::Thread* self, void* userData)
{
  auto args = (ApiThreadArgs*)userData;
  // Initialize bgfx using the native window handle and window resolution.
  bgfx::Init init;
  init.platformData      = args->platformData;
  init.resolution.width  = args->width;
  init.resolution.height = args->height;
  init.resolution.reset  = BGFX_RESET_VSYNC;
  if (!bgfx::init(init))
    return 1;
  // Set view 0 to the same dimensions as the window and to clear the color buffer.
  const bgfx::ViewId kClearView = 0;
  bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
  bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
  uint32_t width  = args->width;
  uint32_t height = args->height;
  bool showStats  = false;
  bool exit       = false;
  while (!exit)
  {
    // Handle events from the main thread.
    while (auto ev = (EventType*)s_apiThreadEvents.pop())
    {
      if (*ev == EventType::Key)
      {
        auto keyEvent = (KeyEvent*)ev;
        if (keyEvent->key == SDLK_F1 && !keyEvent->pressed)
          showStats = !showStats;
      }
      else if (*ev == EventType::Resize)
      {
        auto resizeEvent = (ResizeEvent*)ev;
        bgfx::reset(resizeEvent->width, resizeEvent->height, BGFX_RESET_VSYNC);
        bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);
        width  = resizeEvent->width;
        height = resizeEvent->height;
      }
      else if (*ev == EventType::Exit)
      {
        exit = true;
      }
      delete ev;
    }
    // This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
    bgfx::touch(kClearView);
    // Use debug font to print information about this example.
    bgfx::dbgTextClear();
    bgfx::dbgTextImage(bx::max<uint16_t>(uint16_t(width / 2 / 8), 20) - 20,
                       bx::max<uint16_t>(uint16_t(height / 2 / 16), 6) - 6, 40, 12, s_logo, 160);
    bgfx::dbgTextPrintf(0, 0, 0x0f, "Press F1 to toggle stats.");
    bgfx::dbgTextPrintf(
        0, 1, 0x0f,
        "Color can be changed with ANSI \x1b[9;me\x1b[10;ms\x1b[11;mc\x1b[12;ma\x1b[13;mp\x1b[14;me\x1b[0m code too.");
    bgfx::dbgTextPrintf(80, 1, 0x0f,
                        "\x1b[;0m    \x1b[;1m    \x1b[; 2m    \x1b[; 3m    \x1b[; 4m    \x1b[; 5m    \x1b[; 6m    "
                        "\x1b[; 7m    \x1b[0m");
    bgfx::dbgTextPrintf(80, 2, 0x0f,
                        "\x1b[;8m    \x1b[;9m    \x1b[;10m    \x1b[;11m    \x1b[;12m    \x1b[;13m    \x1b[;14m    "
                        "\x1b[;15m    \x1b[0m");
    const bgfx::Stats* stats = bgfx::getStats();
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Backbuffer %dW x %dH in pixels, debug text %dW x %dH in characters.", stats->width,
                        stats->height, stats->textWidth, stats->textHeight);
    // Enable stats or debug text.
    bgfx::setDebug(showStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_TEXT);
    // Advance to next frame. Main thread will be kicked to process submitted rendering primitives.
    bgfx::frame();
  }
  bgfx::shutdown();
  return 0;
}

int32_t CALLBACK wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/,
                          int32_t /*nCmdShow*/)
{
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
  {
    return 1;
  }
  SDL_Window* pWindow =
      SDL_CreateWindow("bgfx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, SDL_WINDOW_RESIZABLE);
  if (!pWindow)
  {
    return 1;
  }
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(pWindow, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;

  // Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
  // Most graphics APIs must be used on the same thread that created the window.
  bgfx::renderFrame();

  // Create a thread to call the bgfx API from (except bgfx::renderFrame).
  ApiThreadArgs apiThreadArgs;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    apiThreadArgs.platformData.ndt = wmInfo.info.x11.display;
    apiThreadArgs.platformData.nwh = (void*)(uintptr_t)wmInfo.info.x11.window;
#elif BX_PLATFORM_OSX
    apiThreadArgs.platformData.nwh = wmInfo.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
    apiThreadArgs.platformData.nwh = wmInfo.info.win.window;
#endif

  int width, height;
  SDL_GetWindowSize(pWindow, &width, &height);
  apiThreadArgs.width  = (uint32_t)width;
  apiThreadArgs.height = (uint32_t)height;

  bx::Thread apiThread;
  apiThread.init(runApiThread, &apiThreadArgs);

  // Run message pump.
  bool exit = false;
  while (!exit)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        s_apiThreadEvents.push(new ExitEvent);
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
          auto resize    = new ResizeEvent;
          resize->width  = (uint32_t)wev.data1;
          resize->height = (uint32_t)wev.data2;
          s_apiThreadEvents.push(resize);
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
        auto keyEvent         = new KeyEvent;
        keyEvent->key         = kev.keysym.sym;
        keyEvent->pressed     = true;
        s_apiThreadEvents.push(keyEvent);
      }
      break;
      case SDL_KEYUP:
      {
        SDL_KeyboardEvent kev = event.key;
        auto keyEvent         = new KeyEvent;
        keyEvent->key         = kev.keysym.sym;
        keyEvent->pressed     = false;
        s_apiThreadEvents.push(keyEvent);
      }
      break;
      default:
        break;
      }
    }

    // Wait for the API thread to call bgfx::frame, then process submitted rendering primitives.
    bgfx::renderFrame();
  }

  // Wait for the API thread to finish before shutting down.
  while (bgfx::RenderFrame::NoContext != bgfx::renderFrame())
  {
  }

  apiThread.shutdown();
  SDL_DestroyWindow(pWindow);
  SDL_Quit();

  return apiThread.getExitCode();
}
