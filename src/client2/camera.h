#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Camera
{
  glm::vec4 position;
  glm::quat rotation;
};

void CameraInit(Camera& camera);
void CameraMovement(Camera& camera);