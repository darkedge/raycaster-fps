#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Camera
{
  glm::vec3 position;
  float pad0;
  glm::quat rotation;
};

void CameraInit(Camera& camera);
void CameraMovement(Camera& camera);
