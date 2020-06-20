#pragma once

struct Camera;

namespace editor
{
  void Resize(float w, float h);
  void Entry();
  void Do(Camera** ppCamera);
  void Exit();
} // namespace editor
