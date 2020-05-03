#include "camera.h"
#include "mj_input.h"

#include "../../3rdparty/tracy/Tracy.hpp"

namespace mj
{
  extern float GetDeltaTime();
}

static const float s_MovementFactor = 3.0f;
static const float ROT_SPEED        = 0.0025f;

static float lastMousePos;
static float currentMousePos;

void CameraInit(Camera& camera)
{
  lastMousePos    = 0.0f;
  currentMousePos = 0.0f;
  camera.position = glm::vec4(54.5f, 0.5f, 34.5f, 0.0f);
  camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
  camera.yaw      = -currentMousePos;
}

void CameraMovement(Camera& camera)
{
  ZoneScoped;
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(0, 0, 1);
    vec.y         = 0.0f;
    camera.position += glm::vec4(glm::normalize(vec) * dt * s_MovementFactor, 0.0f);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(-1, 0, 0);
    vec.y         = 0.0f;
    camera.position += glm::vec4(glm::normalize(vec) * dt * s_MovementFactor, 0.0f);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(0, 0, -1);
    vec.y         = 0.0f;
    camera.position += glm::vec4(glm::normalize(vec) * dt * s_MovementFactor, 0.0f);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(1, 0, 0);
    vec.y         = 0.0f;
    camera.position += glm::vec4(glm::normalize(vec) * dt * s_MovementFactor, 0.0f);
  }

  int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  currentMousePos -= ROT_SPEED * dx;
  if (currentMousePos != lastMousePos)
  {
    camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
    camera.yaw      = -currentMousePos;
    lastMousePos    = currentMousePos;
  }
}
