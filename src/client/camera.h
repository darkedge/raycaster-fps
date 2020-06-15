#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Camera
{
  glm::vec4 position;
  glm::quat rotation;
  glm::vec3 yFov;
  float yaw;

  glm::vec4 s_FieldOfView;
  glm::vec4 s_Width;
  glm::vec4 s_Height;
};
