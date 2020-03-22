#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11_1.h>
#include <dxgi1_6.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "mj_d3d11.h"
#include "mj_common.h"
#include "mj_input.h"
#include "mj_win32_utils.h"
#include "game.h"

#include "tracy/Tracy.hpp"

using Microsoft::WRL::ComPtr;

// Data
static ComPtr<ID3D11Device> s_pDevice;
static ComPtr<ID3D11DeviceContext> s_pDeviceContext;
static ComPtr<IDXGISwapChain2> s_pSwapChain;
static ComPtr<ID3D11RenderTargetView> s_pRenderTargetView;
static HANDLE s_WaitableObject;
static UINT s_SwapChainFlags;
static ComPtr<IDXGIFactory3> s_pFactory;
static bool s_FullScreen;

static uint16_t s_Width;
static uint16_t s_Height;

// Forward declarations of helper functions
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void CreateRenderTarget();
static void InitKeymap();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void ShowReferences()
{
#ifdef _DEBUG
  ComPtr<ID3D11Debug> pDebug;
  WIN32_ASSERT(s_pDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(pDebug.GetAddressOf())));
  WIN32_ASSERT(pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL));
  // D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL
#endif
}

static void CreateSwapChainFullscreen(HWND hWnd)
{
  s_SwapChainFlags = 0;

  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Width                 = MJ_FS_WIDTH;
  sd.Height                = MJ_FS_HEIGHT;
  sd.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.Stereo                = FALSE;
  sd.SampleDesc.Count      = 1;
  sd.SampleDesc.Quality    = 0;
  sd.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount           = 2;
  sd.Scaling               = DXGI_SCALING_STRETCH;
  sd.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.AlphaMode             = DXGI_ALPHA_MODE_IGNORE;
  sd.Flags                 = s_SwapChainFlags;

  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsd = {};
  fsd.RefreshRate.Numerator           = 1000;
  fsd.RefreshRate.Denominator         = 60000;
  fsd.Scaling                         = DXGI_MODE_SCALING_UNSPECIFIED;
  fsd.Windowed                        = false;

  ShowReferences();
  MJ_UNINITIALIZED IDXGISwapChain1* pSwapChain;
  WIN32_ASSERT(s_pFactory->CreateSwapChainForHwnd(s_pDevice.Get(), hWnd, &sd, &fsd, nullptr, &pSwapChain));
  s_pSwapChain.Attach((IDXGISwapChain2*)pSwapChain);
  s_WaitableObject = 0;
}

static void CreateSwapChainWindowed(HWND hWnd)
{
  s_SwapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Width                 = MJ_WND_WIDTH;
  sd.Height                = MJ_WND_HEIGHT;
  sd.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.Stereo                = FALSE;
  sd.SampleDesc.Count      = 1;
  sd.SampleDesc.Quality    = 0;
  sd.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount           = 2;
  sd.Scaling               = DXGI_SCALING_STRETCH;
  sd.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sd.AlphaMode             = DXGI_ALPHA_MODE_IGNORE;
  sd.Flags                 = s_SwapChainFlags;

  ShowReferences();
  MJ_UNINITIALIZED IDXGISwapChain1* pSwapChain;
  WIN32_ASSERT(s_pFactory->CreateSwapChainForHwnd(s_pDevice.Get(), hWnd, &sd, nullptr, nullptr, &pSwapChain));
  s_pSwapChain.Attach((IDXGISwapChain2*)pSwapChain);
  s_WaitableObject = s_pSwapChain->GetFrameLatencyWaitableObject();
  s_pSwapChain->SetMaximumFrameLatency(1);
}

// Main code
int32_t CALLBACK wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/,
                          int32_t /*nCmdShow*/)
{
  tracy::SetThreadName("Main Thread");
#ifdef _DEBUG
  WIN32_FAIL_IF_ZERO(AllocConsole());
  MJ_DISCARD(freopen("CONIN$", "r", stdin));
  MJ_DISCARD(freopen("CONOUT$", "w", stdout));
  MJ_DISCARD(freopen("CONOUT$", "w", stderr));
#endif
  ImGui_ImplWin32_EnableDpiAwareness();

  // Create application window
  RECT desktopRect;
  GetClientRect(GetDesktopWindow(), &desktopRect);

  // Get window rectangle
  RECT windowRect = { 0, 0, MJ_WND_WIDTH, MJ_WND_HEIGHT };
  // auto dwStyle    = WS_POPUP;
  auto dwStyle = WS_OVERLAPPEDWINDOW;
  AdjustWindowRect(&windowRect, dwStyle, FALSE);

  // Calculate window dimensions
  LONG windowWidth  = windowRect.right - windowRect.left;
  LONG windowHeight = windowRect.bottom - windowRect.top;

  WNDCLASSEX wc = { sizeof(WNDCLASSEX),       CS_CLASSDC, WndProc, 0L,      0L,
                    GetModuleHandle(nullptr), nullptr,    nullptr, nullptr, nullptr,
                    _T("ImGui Example"),      nullptr };
  ::RegisterClassEx(&wc);
  HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("raycaster-fps"), dwStyle, CW_USEDEFAULT, 0, windowWidth,
                             windowHeight, nullptr, nullptr, wc.hInstance, nullptr);

  // Initialize Direct3D
  if (!CreateDeviceD3D(hwnd))
  {
    CleanupDeviceD3D();
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
    return 1;
  }

  // Show the window
  ShowWindow(hwnd, SW_SHOWDEFAULT);
  UpdateWindow(hwnd);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;
  // io.ConfigViewportsNoDefaultParent = true;
  // io.ConfigDockingAlwaysTabBar = true;
  // io.ConfigDockingTransparentPayload = true;
#if 1
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts; // FIXME-DPI: THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T
                                                          // USE IN USER APP!
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI
#endif

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  ImGuiStyle& style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer bindings
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(s_pDevice.Get(), s_pDeviceContext.Get());

  if (!mj::d3d11::Init(s_pDevice.Get()))
  {
    return EXIT_FAILURE;
  }
  mj::d3d11::Resize(s_pDevice.Get(), MJ_WND_WIDTH, MJ_WND_HEIGHT);
  InitKeymap();

  // Main loop
  MSG msg = {};
  mj::input::Init();
  LARGE_INTEGER lastTime, perfFreq;
  WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&lastTime));
  WIN32_FAIL_IF_ZERO(QueryPerformanceFrequency(&perfFreq));
  while (true)
  {
    ZoneScopedN("Game Loop");
    if (!s_FullScreen)
    {
      ZoneScopedNC("WaitableObject", tracy::Color::Crimson);
      MJ_DISCARD(WaitForSingleObjectEx(s_WaitableObject, 1000, true));
    }
    FrameMark;

    // Poll and handle messages (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your
    // inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two
    // flags.
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT)
      {
        break;
      }
    }
    if (msg.message == WM_QUIT)
    {
      break;
    }

    mj::input::Update();

    if (mj::input::GetKeyDown(Key::F1))
    {
      s_FullScreen = !s_FullScreen;

      if (!s_FullScreen)
      {
        WIN32_ASSERT(s_pSwapChain->SetFullscreenState(FALSE, nullptr));
        s_pSwapChain->ResizeBuffers(0, s_Width, s_Height, DXGI_FORMAT_UNKNOWN, s_SwapChainFlags);
      }

      ID3D11RenderTargetView* ppRtvNull[] = { nullptr };
      s_pDeviceContext->OMSetRenderTargets(1, ppRtvNull, nullptr);
      s_pRenderTargetView.Reset();
      s_pSwapChain.Reset();
      if (s_FullScreen)
      {
        s_Width  = MJ_FS_WIDTH;
        s_Height = MJ_FS_HEIGHT;
        CreateSwapChainFullscreen(hwnd);
        mj::d3d11::Resize(s_pDevice.Get(), s_Width, s_Height);
      }
      else
      {
        CreateSwapChainWindowed(hwnd);
      }
      CreateRenderTarget();
    }

    LARGE_INTEGER now;
    WIN32_FAIL_IF_ZERO(QueryPerformanceCounter(&now));
    LONGLONG counts = now.QuadPart - lastTime.QuadPart;
    float dt        = (float)counts / perfFreq.QuadPart;
    if (dt > (1.0f / 60.0f))
    {
      dt = 1.0f / 60.0f;
    }
    SetDeltaTime(dt);
    lastTime = now;

    {
      ZoneScopedNC("ImGuiNewFrame", tracy::Color::CornflowerBlue);
      // Start the Dear ImGui frame
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      ImGui::NewFrame();
    }

    s_pDeviceContext->OMSetRenderTargets(1, s_pRenderTargetView.GetAddressOf(), nullptr);
    mj::d3d11::Update(s_pDeviceContext.Get());

    {
      ZoneScopedNC("ImGuiRender", tracy::Color::CornflowerBlue);
      ImGui::Render();
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

      // Update and Render additional Platform Windows
      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
      }
    }

    {
      ZoneScopedNC("Present", tracy::Color::DarkRed);
      DXGI_PRESENT_PARAMETERS presentParams = {};
      presentParams.DirtyRectsCount         = 0;
      presentParams.pDirtyRects             = nullptr;
      presentParams.pScrollOffset           = nullptr;
      presentParams.pScrollRect             = nullptr;
      WIN32_ASSERT(s_pSwapChain->Present1(1, 0, &presentParams)); // Present with vsync
    }
  }

  // Cleanup
  mj::d3d11::Destroy();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  CleanupDeviceD3D();
  ::DestroyWindow(hwnd);
  ::UnregisterClass(wc.lpszClassName, wc.hInstance);

  return 0;
}

static void CreateDevice(HWND hWnd)
{
  UINT createDeviceFlags = 0;
  DWORD dxgiFactoryFlags = 0;
#ifdef _DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[] = {
    D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,
  };

  WIN32_ASSERT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(s_pFactory.ReleaseAndGetAddressOf())));
  WIN32_ASSERT(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray,
                                 MJ_COUNTOF(featureLevelArray), D3D11_SDK_VERSION, s_pDevice.ReleaseAndGetAddressOf(),
                                 &featureLevel, s_pDeviceContext.ReleaseAndGetAddressOf()));
  WIN32_ASSERT(s_pFactory->MakeWindowAssociation(hWnd, 0));
}

static bool CreateDeviceD3D(HWND hWnd)
{
  CreateDevice(hWnd);
  CreateSwapChainWindowed(hWnd);
  CreateRenderTarget();
  return true;
}

static void CleanupDeviceD3D()
{
  s_pRenderTargetView.Reset();
  s_pSwapChain.Reset();
  s_pDeviceContext.Reset();
  s_pFactory.Reset();
  s_pDevice.Reset();
}

static void CreateRenderTarget()
{
  ComPtr<ID3D11Texture2D> pBackBuffer;
  s_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  WIN32_ASSERT(
      s_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, s_pRenderTargetView.ReleaseAndGetAddressOf()));
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

static uint8_t s_translateKey[256];
static void initTranslateKey(uint8_t vk, Key::Enum _key)
{
  s_translateKey[vk & 0xff] = (uint8_t)_key;
}

static Key::Enum translateKey(uint8_t vk)
{
  return (Key::Enum)s_translateKey[vk & 0xff];
}

static void InitKeymap()
{
  initTranslateKey(VK_ESCAPE, Key::Esc);
  initTranslateKey(VK_RETURN, Key::Return);
  initTranslateKey(VK_TAB, Key::Tab);
  initTranslateKey(VK_BACK, Key::Backspace);
  initTranslateKey(VK_SPACE, Key::Space);
  initTranslateKey(VK_UP, Key::Up);
  initTranslateKey(VK_DOWN, Key::Down);
  initTranslateKey(VK_LEFT, Key::Left);
  initTranslateKey(VK_RIGHT, Key::Right);
  initTranslateKey(VK_PRIOR, Key::PageUp);
  initTranslateKey(VK_NEXT, Key::PageDown);
  initTranslateKey(VK_HOME, Key::Home);
  initTranslateKey(VK_END, Key::End);
  initTranslateKey(VK_SNAPSHOT, Key::Print);
  initTranslateKey(VK_ADD, Key::Plus);
  initTranslateKey(VK_OEM_PLUS, Key::Plus);
  initTranslateKey(VK_SUBTRACT, Key::Minus);
  initTranslateKey(VK_OEM_MINUS, Key::Minus);
  initTranslateKey(VK_OEM_3, Key::Tilde);
  initTranslateKey(VK_OEM_COMMA, Key::Comma);
  initTranslateKey(VK_DECIMAL, Key::Period);
  initTranslateKey(VK_OEM_PERIOD, Key::Period);
  initTranslateKey(VK_OEM_2, Key::Slash);
  initTranslateKey(VK_F1, Key::F1);
  initTranslateKey(VK_F2, Key::F2);
  initTranslateKey(VK_F3, Key::F3);
  initTranslateKey(VK_F4, Key::F4);
  initTranslateKey(VK_F5, Key::F5);
  initTranslateKey(VK_F6, Key::F6);
  initTranslateKey(VK_F7, Key::F7);
  initTranslateKey(VK_F8, Key::F8);
  initTranslateKey(VK_F9, Key::F9);
  initTranslateKey(VK_F10, Key::F10);
  initTranslateKey(VK_F11, Key::F11);
  initTranslateKey(VK_F12, Key::F12);
  initTranslateKey(VK_NUMPAD0, Key::NumPad0);
  initTranslateKey(VK_NUMPAD1, Key::NumPad1);
  initTranslateKey(VK_NUMPAD2, Key::NumPad2);
  initTranslateKey(VK_NUMPAD3, Key::NumPad3);
  initTranslateKey(VK_NUMPAD4, Key::NumPad4);
  initTranslateKey(VK_NUMPAD5, Key::NumPad5);
  initTranslateKey(VK_NUMPAD6, Key::NumPad6);
  initTranslateKey(VK_NUMPAD7, Key::NumPad7);
  initTranslateKey(VK_NUMPAD8, Key::NumPad8);
  initTranslateKey(VK_NUMPAD9, Key::NumPad9);
  initTranslateKey('0', Key::Key0);
  initTranslateKey('1', Key::Key1);
  initTranslateKey('2', Key::Key2);
  initTranslateKey('3', Key::Key3);
  initTranslateKey('4', Key::Key4);
  initTranslateKey('5', Key::Key5);
  initTranslateKey('6', Key::Key6);
  initTranslateKey('7', Key::Key7);
  initTranslateKey('8', Key::Key8);
  initTranslateKey('9', Key::Key9);
  initTranslateKey('A', Key::KeyA);
  initTranslateKey('B', Key::KeyB);
  initTranslateKey('C', Key::KeyC);
  initTranslateKey('D', Key::KeyD);
  initTranslateKey('E', Key::KeyE);
  initTranslateKey('F', Key::KeyF);
  initTranslateKey('G', Key::KeyG);
  initTranslateKey('H', Key::KeyH);
  initTranslateKey('I', Key::KeyI);
  initTranslateKey('J', Key::KeyJ);
  initTranslateKey('K', Key::KeyK);
  initTranslateKey('L', Key::KeyL);
  initTranslateKey('M', Key::KeyM);
  initTranslateKey('N', Key::KeyN);
  initTranslateKey('O', Key::KeyO);
  initTranslateKey('P', Key::KeyP);
  initTranslateKey('Q', Key::KeyQ);
  initTranslateKey('R', Key::KeyR);
  initTranslateKey('S', Key::KeyS);
  initTranslateKey('T', Key::KeyT);
  initTranslateKey('U', Key::KeyU);
  initTranslateKey('V', Key::KeyV);
  initTranslateKey('W', Key::KeyW);
  initTranslateKey('X', Key::KeyX);
  initTranslateKey('Y', Key::KeyY);
  initTranslateKey('Z', Key::KeyZ);

  initTranslateKey(VK_LMENU, Key::LeftAlt);
  initTranslateKey(VK_RMENU, Key::RightAlt);
  initTranslateKey(VK_LCONTROL, Key::LeftCtrl);
  initTranslateKey(VK_RCONTROL, Key::RightCtrl);
  initTranslateKey(VK_LSHIFT, Key::LeftShift);
  initTranslateKey(VK_RSHIFT, Key::RightShift);
  initTranslateKey(VK_LWIN, Key::LeftMeta);
  initTranslateKey(VK_RWIN, Key::RightMeta);

  // Raw input
  RAWINPUTDEVICE device[2];

  // Keyboard
  device[0].usUsagePage = 0x01;
  device[0].usUsage     = 0x06;
  // RIDEV_NOLEGACY disables WM_KEYDOWN, but also breaks hotkeys like PrintScreen
  device[0].dwFlags    = 0;
  device[0].hwndTarget = nullptr;

  // Mouse
  device[1].usUsagePage = 0x01;
  device[1].usUsage     = 0x02;
  device[1].dwFlags     = 0;
  device[1].hwndTarget  = nullptr;

  WIN32_FAIL_IF_ZERO(RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE)));
}

void MjWndProc(HWND /*hwnd*/, LPARAM lParam)
{
  RAWINPUT raw;
  UINT size = sizeof(RAWINPUT);
  WIN32_FAIL_IF(GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)),
                (UINT)-1);

  // extract keyboard raw input data
  if (raw.header.dwType == RIM_TYPEMOUSE)
  {
    const RAWMOUSE& rawMouse = raw.data.mouse;

    mj::input::AddRelativeMouseMovement(rawMouse.lLastX, rawMouse.lLastY);

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
    {
      mj::input::SetMouseButton(MouseButton::Left, true);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)
    {
      mj::input::SetMouseButton(MouseButton::Left, false);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
    {
      mj::input::SetMouseButton(MouseButton::Right, true);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)
    {
      mj::input::SetMouseButton(MouseButton::Right, false);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
    {
      mj::input::SetMouseButton(MouseButton::Middle, true);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
    {
      mj::input::SetMouseButton(MouseButton::Middle, false);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
    {
      mj::input::SetMouseButton(MouseButton::Back, true);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
    {
      mj::input::SetMouseButton(MouseButton::Back, false);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
    {
      mj::input::SetMouseButton(MouseButton::Forward, true);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
    {
      mj::input::SetMouseButton(MouseButton::Forward, false);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_WHEEL)
    {
      // TODO: Mouse wheel delta (rawMouse.usButtonData)
      // signed value that specifies the wheel delta
    }
  }
  else if (raw.header.dwType == RIM_TYPEKEYBOARD)
  {
    const RAWKEYBOARD& rawKB = raw.data.keyboard;
    // do something with the data here

    UINT virtualKey = rawKB.VKey;
    UINT scanCode   = rawKB.MakeCode;
    UINT flags      = rawKB.Flags;

    if (virtualKey == 0xFF)
    {
      // discard "fake keys" which are part of an escaped sequence
      return;
    }
    else if (virtualKey == VK_SHIFT)
    {
      // correct left-hand / right-hand SHIFT
      virtualKey = MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX);
    }
    else if (virtualKey == VK_NUMLOCK)
    {
      // correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
      scanCode = (MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
    }

    const bool isE0 = ((flags & RI_KEY_E0) != 0);
    const bool isE1 = ((flags & RI_KEY_E1) != 0);

    if (isE1)
    {
      // for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
      // however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
      if (virtualKey == VK_PAUSE)
      {
        scanCode = 0x45;
      }
      else
      {
        scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
      }
    }

    switch (virtualKey)
    {
      // right-hand CONTROL and ALT have their e0 bit set
    case VK_CONTROL:
      if (isE0)
      {
        virtualKey = VK_RCONTROL;
      }
      else
      {
        virtualKey = VK_LCONTROL;
      }
      break;
    case VK_MENU:
      if (isE0)
      {
        virtualKey = VK_RMENU;
      }
      else
      {
        virtualKey = VK_LMENU;
      }
      break;

      // NUMPAD ENTER has its e0 bit set
    case VK_RETURN:
      if (isE0)
      {
        virtualKey = MJ_NUMPAD_ENTER;
      } // normally unassigned
      break;

      // the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
      // corresponding keys on the NUMPAD will not.
    case VK_INSERT:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD0;
      }
      break;
    case VK_DELETE:
      if (!isE0)
      {
        virtualKey = VK_DECIMAL;
      }
      break;
    case VK_HOME:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD7;
      }
      break;
    case VK_END:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD1;
      }
      break;
    case VK_PRIOR:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD9;
      }
      break;
    case VK_NEXT:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD3;
      }
      break;

      // the standard arrow keys will always have their e0 bit set, but the
      // corresponding keys on the NUMPAD will not.
    case VK_LEFT:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD4;
      }
      break;
    case VK_RIGHT:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD6;
      }
      break;
    case VK_UP:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD8;
      }
      break;
    case VK_DOWN:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD2;
      }
      break;

      // NUMPAD 5 doesn't have its e0 bit set
    case VK_CLEAR:
      if (!isE0)
      {
        virtualKey = VK_NUMPAD5;
      }
      break;
    }

    // a key can either produce a "make" or "break" scancode. this is used to differentiate between down-presses and
    // releases see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    if (flags & RI_KEY_BREAK)
    {
      mj::input::SetKey(translateKey((uint8_t)virtualKey), false);
    }
    else
    {
      mj::input::SetKey(translateKey((uint8_t)virtualKey), true);
    }
  }
}

// Win32 message handler
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_INPUT)
  {
    MjWndProc(hWnd, lParam);
  }
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg)
  {
  case WM_SIZE:
    s_Width  = (uint16_t)LOWORD(lParam);
    s_Height = (uint16_t)HIWORD(lParam);
    if (s_pDevice && s_pSwapChain && wParam != SIZE_MINIMIZED)
    {
      s_pRenderTargetView.Reset();
      s_pSwapChain->ResizeBuffers(0, s_Width, s_Height, DXGI_FORMAT_UNKNOWN, s_SwapChainFlags);
      mj::d3d11::Resize(s_pDevice.Get(), (uint16_t)LOWORD(lParam), (uint16_t)HIWORD(lParam));
      CreateRenderTarget();
    }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  case WM_DPICHANGED:
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
    {
      // const int dpi = HIWORD(wParam);
      // printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
      const RECT* suggested_rect = (RECT*)lParam;
      ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top,
                     suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    break;
  }
  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
