// dear imgui - standalone example application for DirectX 11
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

#include "mj_d3d11.h"
#include "mj_common.h"
#include "mj_input.h"
#include "mj_win32_utils.h"

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void InitKeymap();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    ImGui_ImplWin32_EnableDpiAwareness();

    // Create application window
    RECT desktopRect;
    GetClientRect(GetDesktopWindow(), &desktopRect);

    // Get window rectangle
    RECT windowRect = { 0, 0, MJ_WIDTH, MJ_HEIGHT };
    auto dwStyle = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    AdjustWindowRect(&windowRect, dwStyle, FALSE);

    // Calculate window dimensions
    LONG windowWidth = windowRect.right - windowRect.left;
    LONG windowHeight = windowRect.bottom - windowRect.top;

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("raycaster-fps"), dwStyle, 100, 100, windowWidth, windowHeight, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigViewportsNoDefaultParent = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;
#if 1
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;     // FIXME-DPI: THIS CURRENTLY DOESN'T WORK AS EXPECTED. DON'T USE IN USER APP!
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports; // FIXME-DPI
#endif

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    if (!mj::d3d11::Init(g_pd3dDevice, g_pd3dDeviceContext))
    {
        return EXIT_FAILURE;
    }
    InitKeymap();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    //ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    mj::input::Init();
    while (msg.message != WM_QUIT)
    {
        mj::input::Update();
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        //g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_color);
        mj::d3d11::Update(g_pd3dDeviceContext);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
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

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

static uint8_t s_translateKey[256];
static void initTranslateKey(uint8_t vk, Key::Enum _key)
{
  //BX_CHECK(vk < BX_COUNTOF(s_translateKey), "Out of bounds %d.", vk);
  s_translateKey[vk & 0xff] = (uint8_t) _key;

  //SDL_Keycode keyCode = SDL_GetKeyFromScancode(vk);
  //const char* keyName = SDL_GetKeyName(keyCode);
  //mj::input::SetKeyName(_key, keyName);
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
    device[0].usUsage = 0x06;
    // RIDEV_NOLEGACY disables WM_KEYDOWN, but also breaks hotkeys like PrintScreen
    device[0].dwFlags = 0;
    device[0].hwndTarget = nullptr;

    // Mouse
    device[1].usUsagePage = 0x01;
    device[1].usUsage = 0x02;
    device[1].dwFlags = 0;
    device[1].hwndTarget = nullptr;

    WIN32_FAIL_IF_ZERO(RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE)));
}

//LRESULT WINAPI MjWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
void MjWndProc(HWND hwnd, LPARAM lParam)
{
  RAWINPUT raw;
  UINT size = sizeof(RAWINPUT);
  WIN32_FAIL_IF(GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)), (UINT) -1);

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
    UINT scanCode = rawKB.MakeCode;
    UINT flags = rawKB.Flags;

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
        virtualKey = MJ_NUMPAD_ENTER; // normally unassigned
      }

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

    // a key can either produce a "make" or "break" scancode. this is used to differentiate between down-presses and releases
    // see http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
    if (flags & RI_KEY_BREAK)
    {
      mj::input::SetKey(translateKey(virtualKey), false);
    }
    else
    {
      mj::input::SetKey(translateKey(virtualKey), true);
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
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
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
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
