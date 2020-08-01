#pragma once
#include "game.h"
#include "camera.h"
#include "editor.h"
#include "graphics.h"
#include "level.h"

class Meta
{
public:
  Meta()
  {
    game.SetMeta(this);
    editor.SetMeta(this);
  }
  ~Meta();
  static constexpr uint32_t LEVEL_DIM = 64;

  void Init(HWND hwnd);
  void Resize(int width, int height);
  void NewFrame();
  void Update();
  void NewLevel();
  void GainFocus();

private:
  bool CreateDeviceD3D(HWND hWnd);
  void CreateRenderTargetView();
  void LoadLevel();

  ComPtr<ID3D11Device> pDevice;
  ComPtr<ID3D11DeviceContext> pContext;
  ComPtr<IDXGISwapChain> pSwapChain;
  ComPtr<ID3D11RenderTargetView> pRenderTargetView;
  ComPtr<ID3D11DepthStencilState> pDepthStencilState;
  ComPtr<ID3D11DepthStencilView> pDepthStencilView;
  ComPtr<ID3D11Texture2D> pDepthStencilBuffer;

  Level level;
  GameState game;
  EditorState editor;
  Graphics graphics;
  mj::ArrayList<DrawCommand> drawList;

  StateMachine stateMachine;
};
