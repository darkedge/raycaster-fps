#include "stdafx.h"
#include "game.h"
#include "mj_common.h"
#include "rasterizer.h"
#include "generated/text.h"
#include "mj_input.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

namespace mj
{
  extern bool IsWindowMouseFocused();
}

static ID3D11Device* g_pd3dDevice;
static ID3D11DeviceContext* g_pd3dDeviceContext;
static IDXGISwapChain* g_pSwapChain;
static ID3D11RenderTargetView* g_mainRenderTargetView;

// Helper functions

static void CreateRenderTarget()
{
  ID3D11Texture2D* pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
  pBackBuffer->Release();
}

static bool CreateDeviceD3D(HWND hWnd)
{
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount                        = 2;
  sd.BufferDesc.Width                   = 0;
  sd.BufferDesc.Height                  = 0;
  sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator   = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow                       = hWnd;
  sd.SampleDesc.Count                   = 1;
  sd.SampleDesc.Quality                 = 0;
  sd.Windowed                           = TRUE;
  sd.SwapEffect                         = DXGI_SWAP_EFFECT_FLIP_DISCARD;

  UINT createDeviceFlags = 0;
#ifdef _DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_0,
  };
  if (D3D11CreateDeviceAndSwapChain(nullptr,                  //
                                    D3D_DRIVER_TYPE_HARDWARE, //
                                    nullptr,                  //
                                    createDeviceFlags,        //
                                    featureLevelArray,        //
                                    2,                        //
                                    D3D11_SDK_VERSION,        //
                                    &sd,                      //
                                    &g_pSwapChain,            //
                                    &g_pd3dDevice,            //
                                    &featureLevel,            //
                                    &g_pd3dDeviceContext) != S_OK)
  {
    return false;
  }

  CreateRenderTarget();
  return true;
}

static void CleanupRenderTarget()
{
  if (g_mainRenderTargetView)
  {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

static void CleanupDeviceD3D()
{
  CleanupRenderTarget();
  SAFE_RELEASE(g_pSwapChain);
  SAFE_RELEASE(g_pd3dDeviceContext);
  SAFE_RELEASE(g_pd3dDevice);
}

static bool s_MouseLook = true;
static game::Data s_Data;

static void Reset()
{
  s_Data.s_FieldOfView.x = 60.0f;
  CameraInit(MJ_REF s_Data.s_Camera);
}

static void ShowBuildInfo()
{
  // FIXME-VIEWPORT: Select a default viewport
  const float DISTANCE = 10.0f;
  static int corner    = 0;
  if (corner != -1)
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 window_pos =
        ImVec2((corner & 1) ? (viewport->Pos.x + viewport->Size.x - DISTANCE) : (viewport->Pos.x + DISTANCE),
               (corner & 2) ? (viewport->Pos.y + viewport->Size.y - DISTANCE) : (viewport->Pos.y + DISTANCE));
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowViewport(viewport->ID);
  }
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (ImGui::Begin("Overlay", nullptr,
                   (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking |
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
  {
    ImGui::Text("%s, %s (%s #%s), %s\nStaged:%s\nUnstaged:%s", mj::txt::pBuildConfiguration, mj::txt::pGitCommitId,
                mj::txt::pGitBranch, mj::txt::pGitRevision, mj::txt::pDateTime, mj::txt::pGitDiffStaged,
                mj::txt::pGitDiff);
  }
  ImGui::End();
}

void game::Init(HWND hwnd)
{
  s_Data.s_Width  = glm::vec4(MJ_RT_WIDTH, 0.0f, 0.0f, 0.0f);
  s_Data.s_Height = glm::vec4(MJ_RT_HEIGHT, 0.0f, 0.0f, 0.0f);

  Reset();

  // Setup Platform/Renderer bindings
  MJ_DISCARD(CreateDeviceD3D(hwnd));
  MJ_DISCARD(ImGui::CreateContext());
  MJ_DISCARD(ImGui_ImplWin32_Init(hwnd));
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

  rs::Init(g_pd3dDevice);

  // Allow mouse movement tracking outside the window
  if (s_MouseLook)
  {
    if (mj::IsWindowMouseFocused())
    {
      MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool)s_MouseLook));
    }
    else
    {
      s_MouseLook = false;
    }
  }
}

void game::Resize(int width, int height)
{
  rs::Resize(width, height);
}

void game::Update(int width, int height)
{
  // Start the Dear ImGui frame
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  if (mj::input::GetKeyDown(Key::F3) || mj::input::GetKeyDown(Key::KeyE))
  {
    s_MouseLook = !s_MouseLook;
    SDL_SetRelativeMouseMode((SDL_bool)s_MouseLook);
    if (!s_MouseLook)
    {
      // Only works if relative mouse mode is off
      SDL_WarpMouseInWindow(nullptr, width / 2, height / 2);
    }
  }
  if (s_MouseLook)
  {
    CameraMovement(MJ_REF s_Data.s_Camera);
  }

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Reset();
  }

  ShowBuildInfo();

  {
    ImGui::Begin("Debug");
    ImGui::Text("R to reset, F3 or E toggles mouselook");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Field of view", &s_Data.s_FieldOfView.x, 5.0f, 170.0f);
    ImGui::Text("Player position: x=%.3f, z=%.3f", s_Data.s_Camera.position.x, s_Data.s_Camera.position.z);
    ImGui::End();
  }

  auto mat     = glm::identity<glm::mat4>();
  s_Data.s_Mat = glm::translate(mat, glm::vec3(s_Data.s_Camera.position)) * glm::mat4_cast(s_Data.s_Camera.rotation);

  g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
  rs::Update(g_pd3dDeviceContext, width, height, &s_Data);

#if 1
  {
    ZoneScopedNC("ImGui Demo", tracy::Color::Burlywood);
    ImGui::ShowDemoWindow();
  }
#endif

  {
    ZoneScopedNC("ImGui render", tracy::Color::BlanchedAlmond);

    // Rendering
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  }

  {
    ZoneScopedNC("Swap Chain Present", tracy::Color::Azure);
    g_pSwapChain->Present(1, 0); // Present with vsync
    // g_pSwapChain->Present(0, 0); // Present without vsync
    FrameMark;
  }
}

void game::Destroy()
{
  CleanupDeviceD3D();
  rs::Destroy();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
}
