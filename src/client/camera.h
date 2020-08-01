#pragma once
#include "mj_math.h"

namespace axis
{
  const auto RIGHT    = mjm::vec3(1.0f, 0.0f, 0.0f);
  const auto UP       = mjm::vec3(0.0f, 1.0f, 0.0f);
  const auto FORWARD  = mjm::vec3(0.0f, 0.0f, 1.0f);
  const auto LEFT     = mjm::vec3(-1.0f, 0.0f, 0.0f);
  const auto DOWN     = mjm::vec3(0.0f, -1.0f, 0.0f);
  const auto BACKWARD = mjm::vec3(0.0f, 0.0f, -1.0f);
} // namespace axis

struct Camera
{
  mjm::vec3 position;
  mjm::mat4 viewProjection;
  mjm::vec4 viewport; // x, y, w, h
  mjm::quat rotation;
  float yFov;         // Degrees
};
