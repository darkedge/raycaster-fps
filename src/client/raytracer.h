#pragma once
#include "game.h"

namespace rt
{
  void Init();
  void Resize(int width, int height);
  void Update(int width, int height, game::Data* pData);
  void Destroy();
} // namespace rt
