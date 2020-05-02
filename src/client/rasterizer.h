#pragma once
#include "game.h"

namespace rs
{
  void Init();
  void Resize(int width, int height);
  void Update(int width, int height, game::Data* pData);
  void Destroy();
} // namespace rs
