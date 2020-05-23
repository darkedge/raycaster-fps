#pragma once
#include "camera.h"

namespace game
{
  constexpr uint32_t LEVEL_DIM = 64;

  struct Data
  {
    glm::mat4 s_Mat;
    glm::vec4 s_FieldOfView;
    glm::vec4 s_Width;
    glm::vec4 s_Height;
    Camera s_Camera;
  };

  void Init(HWND hwnd);
  void Resize(int width, int height);
  void NewFrame();
  void Update(int width, int height);
  void Destroy();
} // namespace game
