#pragma once

namespace mj
{
  float GetDeltaTime();
  void GetWindowSize(float* w, float* h);
  void MoveMouse(int x, int y);
  bool IsWindowMouseFocused();
} // namespace mj
