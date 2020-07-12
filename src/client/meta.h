#pragma once
#include "game.h"
#include "camera.h"
#include "editor.h"
#include "graphics.h"
#include "map.h"

class Meta
{
public:
  static constexpr uint32_t LEVEL_DIM = 64;

  void Init(HWND hwnd);
  void Resize(int width, int height);
  void NewFrame();
  void Update();
  void Destroy();

private:
  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();
  void CreateRenderTargetView();
  void CleanupRenderTarget();
  void LoadLevel();

  ID3D11Device* pDevice;
  ID3D11DeviceContext* pContext;
  IDXGISwapChain* pSwapChain;
  ID3D11RenderTargetView* pRenderTargetView;
  ID3D11DepthStencilState* pDepthStencilState;
  ID3D11DepthStencilView* pDepthStencilView;
  ID3D11Texture2D* pDepthStencilBuffer;

  GameState stateGame;
  EditorState stateEditor;
  Graphics graphics;
  map::map_t map;

  Camera* pCamera;

  StateMachine stateMachine;
};
