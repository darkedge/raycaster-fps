#pragma once
#include "game.h"

namespace rs
{
  void Init(ID3D11Device* pDevice);
  void Resize(int width, int height);
  void Update(ID3D11DeviceContext* pDeviceContext, int width, int height, game::Data* pData);
  void Destroy();
} // namespace rs
