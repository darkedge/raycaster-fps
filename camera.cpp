#include "camera.h"
#include "mj_input.h"
#include "game.h"

static const float s_MovementFactor = 5.0f;
static const float ROT_SPEED        = 0.0025f;

static glm::vec2 lastMousePos;
static glm::vec2 currentMousePos;

void CameraInit(Camera& camera)
{
  lastMousePos    = glm::zero<glm::vec2>();
  currentMousePos = glm::zero<glm::vec2>();
  camera.rotation = glm::quat(glm::vec3(-currentMousePos.y, -currentMousePos.x, 0));
}

void CameraMovement(Camera& camera)
{
  const float dt = GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    camera.position += camera.rotation * glm::vec3(0, 0, 1) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    camera.position += camera.rotation * glm::vec3(-1, 0, 0) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    camera.position += camera.rotation * glm::vec3(0, 0, -1) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    camera.position += camera.rotation * glm::vec3(1, 0, 0) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::Space))
  {
    camera.position += camera.rotation * glm::vec3(0, 1, 0) * dt * s_MovementFactor;
  }
  if (mj::input::GetKey(Key::LeftCtrl))
  {
    camera.position += camera.rotation * glm::vec3(0, -1, 0) * dt * s_MovementFactor;
  }

  int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  currentMousePos -= ROT_SPEED * glm::vec2(dx, dy);

  if (currentMousePos.y < glm::radians(-89.0f))
  {
    currentMousePos.y = glm::radians(-89.0f);
  }
  if (currentMousePos.y > glm::radians(89.0f))
  {
    currentMousePos.y = glm::radians(89.0f);
  }
  if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y)
  {
    camera.rotation = glm::quat(glm::vec3(-currentMousePos.y, -currentMousePos.x, 0));
    lastMousePos    = currentMousePos;
  }
}
