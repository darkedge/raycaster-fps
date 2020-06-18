#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace axis
{
  const auto POS_X = glm::vec3(1.0f, 0.0f, 0.0f);
  const auto POS_Y = glm::vec3(0.0f, 1.0f, 0.0f);
  const auto POS_Z = glm::vec3(0.0f, 0.0f, 1.0f);
  const auto NEG_X = glm::vec3(-1.0f, 0.0f, 0.0f);
  const auto NEG_Y = glm::vec3(0.0f, -1.0f, 0.0f);
  const auto NEG_Z = glm::vec3(0.0f, 0.0f, -1.0f);
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
