#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Camera
{
  glm::vec3 position;
  float pad0;
  glm::quat rotation;

  glm::vec3 topLeft;
  float pad1;
  glm::vec3 topRight;
  float pad2;
  glm::vec3 bottomLeft;
  float pad3;
  glm::vec3 bottomRight;
  float pad4;
};

void CameraInit(Camera& camera);
void CameraMovement(Camera& camera);
