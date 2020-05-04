#pragma once
#include "game.h"
#include <bgfx/bgfx.h>

namespace rt
{
  static constexpr uint32_t GRID_DIM = 16;

  void Init();
  void Resize(int width, int height);
  void Update(bgfx::ViewId viewId, int width, int height, game::Data* pData);
  void Destroy();
} // namespace rt
