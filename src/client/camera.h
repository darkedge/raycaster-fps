#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace axis
{
  const auto RIGHT    = glm::vec3(1.0f, 0.0f, 0.0f);
  const auto UP       = glm::vec3(0.0f, 1.0f, 0.0f);
  const auto FORWARD  = glm::vec3(0.0f, 0.0f, 1.0f);
  const auto LEFT     = glm::vec3(-1.0f, 0.0f, 0.0f);
  const auto DOWN     = glm::vec3(0.0f, -1.0f, 0.0f);
  const auto BACKWARD = glm::vec3(0.0f, 0.0f, -1.0f);
} // namespace axis

struct Camera
{
  glm::vec3 position;
  // glm::mat4 model;
  glm::mat4 projection;
  glm::mat4 view;
  glm::vec4 viewport; // x, y, w, h
  float yFov;         // Degrees
};
