#pragma once
#include "game.h"
#include "camera.h"
#include "editor.h"

namespace meta
{
  constexpr uint32_t LEVEL_DIM = 64;

  struct Global
  {
    ID3D11Device* pDevice;
    ID3D11DeviceContext* pContext;
    IDXGISwapChain* pSwapChain;
    ID3D11RenderTargetView* pRenderTargetView;
    ID3D11DepthStencilState* pDepthStencilState;
    ID3D11DepthStencilView* pDepthStencilView;
    ID3D11Texture2D* pDepthStencilBuffer;

    game::Game StateGame;
    editor::Editor StateEditor;

    Camera* pCamera;

    StateMachine StateMachine;
  };

  void Init(Global* pGlobal, HWND hwnd);
  void Resize(Global* pGlobal, int width, int height);
  void NewFrame();
  void Update(Global* pGlobal);
  void Destroy(Global* pGlobal);
} // namespace meta
