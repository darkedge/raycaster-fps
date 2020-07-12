#include "stdafx.h"
#include "meta.h"
#include "mj_common.h"
#include "graphics.h"
#include "mj_input.h"
#include "imgui_impl_dx11.h"
#include "editor.h"
#include "state_machine.h"
#include "game.h"

// Helper functions

bool Meta::CreateDeviceD3D(HWND hWnd)
{
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount                        = 1;
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
  sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

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
                                    &this->pSwapChain,        //
                                    &this->pDevice,           //
                                    &featureLevel,            //
                                    &this->pContext) != S_OK)
  {
    return false;
  }

  return true;
}

void Meta::CleanupDeviceD3D()
{
  SAFE_RELEASE(this->pSwapChain);
  SAFE_RELEASE(this->pContext);
  SAFE_RELEASE(this->pRenderTargetView);
  SAFE_RELEASE(this->pDevice);
  SAFE_RELEASE(this->pDepthStencilState);
  SAFE_RELEASE(this->pDepthStencilView);
  SAFE_RELEASE(this->pDepthStencilBuffer);
}

#if 0
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
    ImVec2 window_popivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_popivot);
    ImGui::SetNextWindowViewport(viewport->ID);
  }
  ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
  if (ImGui::Begin("Overlay", nullptr,
                   (corner != -1 ? ImGuiWindowFlagNoMove : 0) | ImGuiWindowFlagNoDocking |
                       ImGuiWindowFlagNoTitleBar | ImGuiWindowFlagNoResize | ImGuiWindowFlagAlwaysAutoResize |
                       ImGuiWindowFlagNoSavedSettings | ImGuiWindowFlagNoFocusOnAppearing | ImGuiWindowFlagNoNav))
  {
    ImGui::Text("%s, %s (%s #%s), %s\nStaged:%s\nUnstaged:%s", mj::txt::pBuildConfiguration, mj::txt::pGitCommitId,
                mj::txt::pGitBranch, mj::txt::pGitRevision, mj::txt::pDateTime, mj::txt::pGitDiffStaged,
                mj::txt::pGitDiff);
  }
  ImGui::End();
}
#endif

void Meta::CreateRenderTargetView()
{
  MJ_UNINITIALIZED ID3D11Texture2D* pBackBuffer;
  this->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  this->pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &this->pRenderTargetView);
  pBackBuffer->Release();
}

void Meta::LoadLevel()
{
  map = map::Load("e1m1.mjm");
  if (map::Valid(map))
  {
    graphics.CreateMesh(map, pDevice);
    map::Free(map);
  }
}

void Meta::Init(HWND hwnd)
{
  // Setup Platform/Renderer bindings
  MJ_DISCARD(CreateDeviceD3D(hwnd));

  MJ_DISCARD(ImGui_ImplDX11_Init(this->pDevice, this->pContext));

  this->CreateRenderTargetView();

  graphics.Init(this->pDevice);

  LoadLevel();

  {
    // Depth Stencil
    D3D11_TEXTURE2D_DESC desc = {};

    desc.ArraySize        = 1;
    desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
    desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.Width            = 1600;
    desc.Height           = 1000;
    desc.MipLevels        = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DEFAULT;

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format                        = desc.Format; // DXGI_FORMAT_D32_FLOAT; // DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    descDSV.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Flags                         = 0;
    descDSV.Texture2D.MipSlice            = 0;

    this->pDevice->CreateTexture2D(&desc, nullptr, &this->pDepthStencilBuffer);
    this->pDevice->CreateDepthStencilView(this->pDepthStencilBuffer, &descDSV, &this->pDepthStencilView);
  }

  {
    D3D11_DEPTH_STENCIL_DESC dsDesc     = {};
    dsDesc.DepthEnable                  = TRUE;
    dsDesc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc                    = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable                = TRUE;
    dsDesc.StencilReadMask              = D3D11_DEFAULT_STENCIL_READ_MASK;
    dsDesc.StencilWriteMask             = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    dsDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    dsDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;
    dsDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;

    MJ_DISCARD(this->pDevice->CreateDepthStencilState(&dsDesc, &this->pDepthStencilState));
  }

  // Mouse capture behavior
  if (mj::IsWindowMouseFocused())
  {
    MJ_DISCARD(SDL_SetRelativeMouseMode(SDL_TRUE));
    ImGui::GetIO().WantCaptureMouse    = true;
    ImGui::GetIO().WantCaptureKeyboard = true;
    this->stateMachine.pStateNext      = &this->stateGame;
  }
  else
  {
    ImGui::GetIO().WantCaptureMouse    = false;
    ImGui::GetIO().WantCaptureKeyboard = false;
    this->stateMachine.pStateNext      = &this->stateEditor;
  }

  // Fire Entry action for next state
  StateMachineUpdate(&this->stateMachine, nullptr);
}

void Meta::CleanupRenderTarget()
{
  SAFE_RELEASE(this->pRenderTargetView);
}

void Meta::Resize(int width, int height)
{
  this->CleanupRenderTarget();
  this->pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
  this->CreateRenderTargetView();
  graphics.Resize(width, height);
  StateMachineResize(&this->stateMachine, (float)width, (float)height);
}

void Meta::NewFrame()
{
  // This is a little awkward, but we must call the DX11 NewFrame before calling SDL2 NewFrame.
  // SDL2 NewFrame is called in main.cpp, but we do not have the DX11 ImGui implementation header included there.
  ImGui_ImplDX11_NewFrame();
}

void Meta::Update()
{
  ImGui::NewFrame();

  if (mj::input::GetKeyDown(Key::F3))
  {
    this->stateMachine.pStateNext = (this->stateMachine.pStateCurrent == &this->stateGame)
                                        ? (StateBase*)&this->stateEditor
                                        : (StateBase*)&this->stateGame;
  }

  StateMachineUpdate(&this->stateMachine, &this->pCamera);

  this->pContext->OMSetRenderTargets(1, &this->pRenderTargetView, this->pDepthStencilView);
  this->pContext->ClearDepthStencilView(this->pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
  FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  this->pContext->ClearRenderTargetView(this->pRenderTargetView, clearColor);
  this->pContext->OMSetDepthStencilState(this->pDepthStencilState, 1);

  graphics.Update(this->pContext, this->pCamera);

#if 0
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

    // Update and Render additional Platform Windows
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }
  }

  {
    ZoneScopedNC("Swap Chain Present", tracy::Color::Azure);
    this->pSwapChain->Present(1, 0); // Present with vsync
    // pSwapChain->Present(0, 0); // Present without vsync
  }
}

void Meta::Destroy()
{
  graphics.Destroy();
  this->CleanupDeviceD3D();
  ImGui_ImplDX11_Shutdown();
  ImGui::DestroyContext();
}
