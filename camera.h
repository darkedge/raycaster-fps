#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Camera
{
  glm::vec3 position;
  glm::quat rotation;

  glm::vec3 topLeft;
  glm::vec3 topRight;
  glm::vec3 bottomLeft;
  glm::vec3 bottomRight;
};

void CameraMovement(Camera& camera);
