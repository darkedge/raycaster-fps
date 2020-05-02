#pragma once
#include <glm/glm.hpp>
#include "camera.h"

namespace game
{
  struct Data
  {
    glm::mat4 s_Mat;
    glm::vec4 s_FieldOfView;
    glm::vec4 s_Width;
    glm::vec4 s_Height;
    Camera s_Camera;
  };

  void Init();
  void Resize(int width, int height);
  void Update(int width, int height);
  void Destroy();
} // namespace game
