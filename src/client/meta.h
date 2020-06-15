#pragma once
#include "camera.h"

namespace meta
{
  constexpr uint32_t LEVEL_DIM = 64;

  void Init(HWND hwnd);
  void Resize(int width, int height);
  void NewFrame();
  void Update(int width, int height);
  void Destroy();
} // namespace meta
