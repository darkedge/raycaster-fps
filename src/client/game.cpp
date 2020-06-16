#include "stdafx.h"
#include "game.h"
#include "camera.h"
#include "mj_input.h"
#include "mj_common.h" // MJ_RT_WIDTH / MJ_RT_HEIGHT

namespace mj
{
  extern float GetDeltaTime();
}

static const float s_MovementFactor = 3.0f;
static const float ROT_SPEED        = 0.0025f;

static float lastMousePos;
static float currentMousePos;

static Camera s_Camera;

void game::Entry()
{
  s_Camera.yFov     = 60.0f;

  lastMousePos      = 0.0f;
  currentMousePos   = 0.0f;
  s_Camera.position = glm::vec3(54.5f, 0.5f, 34.5f);
  s_Camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
  s_Camera.yaw      = -currentMousePos;
}

void game::Do(Camera** ppCamera)
{
  ZoneScoped;
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    glm::vec3 vec = s_Camera.rotation * axis::POS_Z;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * s_MovementFactor);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    glm::vec3 vec = s_Camera.rotation * axis::NEG_X;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * s_MovementFactor);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    glm::vec3 vec = s_Camera.rotation * axis::NEG_Z;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * s_MovementFactor);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    glm::vec3 vec = s_Camera.rotation * axis::POS_X;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * s_MovementFactor);
  }

  MJ_UNINITIALIZED int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  currentMousePos -= ROT_SPEED * dx;
  if (currentMousePos != lastMousePos)
  {
    s_Camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
    s_Camera.yaw      = -currentMousePos;
    lastMousePos      = currentMousePos;
  }

  *ppCamera = &s_Camera;
}
