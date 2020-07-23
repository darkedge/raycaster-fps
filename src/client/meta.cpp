#include "stdafx.h"
#include "meta.h"
#include "mj_common.h"
#include "graphics.h"
#include "mj_input.h"
#include "imgui_impl_dx11.h"
#include "editor.h"
#include "state_machine.h"
#include "game.h"
#include "main.h"

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
  if (D3D11CreateDeviceAndSwapChain(nullptr,                                   //
                                    D3D_DRIVER_TYPE_HARDWARE,                  //
                                    nullptr,                                   //
                                    createDeviceFlags,                         //
                                    featureLevelArray,                         //
                                    2,                                         //
                                    D3D11_SDK_VERSION,                         //
                                    &sd,                                       //
                                    this->pSwapChain.ReleaseAndGetAddressOf(), //
                                    this->pDevice.ReleaseAndGetAddressOf(),    //
                                    &featureLevel,                             //
                                    this->pContext.ReleaseAndGetAddressOf()) != S_OK)
  {
    return false;
  }

  return true;
}

void Meta::CreateRenderTargetView()
{
  MJ_UNINITIALIZED ID3D11Texture2D* pBackBuffer;
  this->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  this->pDevice->CreateRenderTargetView(pBackBuffer, nullptr, this->pRenderTargetView.ReleaseAndGetAddressOf());
  pBackBuffer->Release();

  MJ_UNINITIALIZED float w, h;
  mj::GetWindowSize(&w, &h);

  // Depth Stencil
  D3D11_TEXTURE2D_DESC desc = {};

  desc.ArraySize        = 1;
  desc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
  desc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
  desc.Width            = (UINT)w;
  desc.Height           = (UINT)h;
  desc.MipLevels        = 1;
  desc.SampleDesc.Count = 1;
  desc.Usage            = D3D11_USAGE_DEFAULT;

  D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
  descDSV.Format                        = desc.Format;
  descDSV.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
  descDSV.Flags                         = 0;
  descDSV.Texture2D.MipSlice            = 0;

  this->pDevice->CreateTexture2D(&desc, nullptr, this->pDepthStencilBuffer.ReleaseAndGetAddressOf());
  this->pDevice->CreateDepthStencilView(this->pDepthStencilBuffer.Get(), &descDSV,
                                        this->pDepthStencilView.ReleaseAndGetAddressOf());
}

void Meta::LoadLevel()
{
  Level level = Level::Load("e1m1.mjm");
  if (Level::Valid(level))
  {
    game.SetLevel(level, pDevice);
    editor.SetLevel(level, pDevice);
    Level::Free(level);
  }
}

void Meta::Init(HWND hwnd)
{
  // Setup Platform/Renderer bindings
  MJ_DISCARD(CreateDeviceD3D(hwnd));

  MJ_DISCARD(ImGui_ImplDX11_Init(this->pDevice.Get(), this->pContext.Get()));

  this->CreateRenderTargetView();

  graphics.Init(this->pDevice);
  this->editor.Init();
  this->game.Init();

  LoadLevel();

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

    MJ_DISCARD(this->pDevice->CreateDepthStencilState(&dsDesc, this->pDepthStencilState.ReleaseAndGetAddressOf()));
  }

  // Mouse capture behavior
  if (mj::IsWindowMouseFocused())
  {
    MJ_DISCARD(SDL_SetRelativeMouseMode(SDL_TRUE));
    ImGui::GetIO().WantCaptureMouse    = true;
    ImGui::GetIO().WantCaptureKeyboard = true;
    this->stateMachine.pStateNext      = &this->game;
  }
  else
  {
    ImGui::GetIO().WantCaptureMouse    = false;
    ImGui::GetIO().WantCaptureKeyboard = false;
    this->stateMachine.pStateNext      = &this->editor;
  }

  // Fire Entry action for next state
  this->stateMachine.Update(drawList);
}

void Meta::Resize(int width, int height)
{
  this->pRenderTargetView.Reset();
  this->pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
  this->CreateRenderTargetView();
  graphics.Resize(width, height);
  this->stateMachine.Resize((float)width, (float)height);
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
    this->stateMachine.pStateNext = (this->stateMachine.pStateCurrent == &this->game)
                                        ? (StateBase*)&this->editor
                                        : (StateBase*)&this->game;
  }

  this->stateMachine.Update(this->drawList);

  this->pContext->OMSetRenderTargets(1, this->pRenderTargetView.GetAddressOf(), this->pDepthStencilView.Get());
  this->pContext->ClearDepthStencilView(this->pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
  FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  this->pContext->ClearRenderTargetView(this->pRenderTargetView.Get(), clearColor);
  this->pContext->OMSetDepthStencilState(this->pDepthStencilState.Get(), 1);

  graphics.Update(this->pContext, this->drawList);
  this->drawList.Clear();

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

Meta::~Meta()
{
  ImGui_ImplDX11_Shutdown();
  ImGui::DestroyContext();
}

void Meta::NewLevel()
{
  LoadLevel();
}
