#include "camera.h"
#include "mj_input.h"
#include "game.h"

static const float s_MovementFactor = 5.0f;
static const float ROT_SPEED        = 0.0025f;

static float lastMousePos;
static float currentMousePos;

void CameraInit(Camera& camera)
{
  lastMousePos    = 0.0f;
  currentMousePos = 0.0f;
  camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
}

void CameraMovement(Camera& camera)
{
  const float dt = GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(0, 0, 1);
    vec.y         = 0.0f;
    camera.position += glm::normalize(vec) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(-1, 0, 0);
    vec.y         = 0.0f;
    camera.position += glm::normalize(vec) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(0, 0, -1);
    vec.y         = 0.0f;
    camera.position += glm::normalize(vec) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    glm::vec3 vec = camera.rotation * glm::vec3(1, 0, 0);
    vec.y         = 0.0f;
    camera.position += glm::normalize(vec) * dt * s_MovementFactor;
  }

  int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  currentMousePos -= ROT_SPEED * dx;
  if (currentMousePos != lastMousePos)
  {
    camera.rotation = glm::quat(glm::vec3(0.0f, -currentMousePos, 0));
    lastMousePos    = currentMousePos;
  }
}
