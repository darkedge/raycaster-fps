#pragma once
#include "camera.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace gfx
{
  void Init(ID3D11Device* pDevice);
  void Resize(int width, int height);
  void Update(ID3D11DeviceContext* pDeviceContext, const Camera* pCamera);
  void Destroy();
  void* GetTileTexture(int x, int y);
} // namespace gfx
