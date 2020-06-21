#include "stdafx.h"
#include "game.h"
#include "camera.h"
#include "mj_input.h"
#include "mj_common.h" // MJ_RT_WIDTH / MJ_RT_HEIGHT
#include <glm/gtx/euler_angles.hpp>
#include "main.h"

static const float MOVEMENT_FACTOR = 3.0f;
static const float ROT_SPEED       = 0.0025f;

static float s_LastMousePos;
static float s_CurrentMousePos;
static float s_Yaw;
static glm::quat s_Rotation;

static Camera s_Camera;

void game::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) true));
  // ImGui::GetIO().WantCaptureMouse    = s_MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = s_MouseLook;

  s_Camera.yFov = 60.0f;

  s_LastMousePos    = 0.0f;
  s_CurrentMousePos = 0.0f;
  s_Camera.position = glm::vec3(54.5f, 0.5f, 34.5f);
  s_Rotation        = glm::quat(glm::vec3(0.0f, -s_CurrentMousePos, 0));
  s_Yaw             = -s_CurrentMousePos;
}

void game::Do(Camera** ppCamera)
{
  ZoneScoped;
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    glm::vec3 vec = s_Rotation * axis::FORWARD;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    glm::vec3 vec = s_Rotation * axis::LEFT;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    glm::vec3 vec = s_Rotation * axis::BACKWARD;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    glm::vec3 vec = s_Rotation * axis::RIGHT;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }

  MJ_UNINITIALIZED int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  s_CurrentMousePos -= ROT_SPEED * dx;
  if (s_CurrentMousePos != s_LastMousePos)
  {
    s_Rotation     = glm::quat(glm::vec3(0.0f, -s_CurrentMousePos, 0));
    s_Yaw          = -s_CurrentMousePos;
    s_LastMousePos = s_CurrentMousePos;
  }

  glm::mat4 rotate    = glm::transpose(glm::eulerAngleY(s_Yaw));
  glm::mat4 translate = glm::identity<glm::mat4>();
  translate           = glm::translate(translate, -glm::vec3(s_Camera.position));

  s_Camera.view        = rotate * translate;
  s_Camera.viewport[0] = 0.0f;
  s_Camera.viewport[1] = 0.0f;
  mj::GetWindowSize(&s_Camera.viewport[2], &s_Camera.viewport[3]);
  s_Camera.projection =
      glm::perspective(glm::radians(s_Camera.yFov), s_Camera.viewport[2] / s_Camera.viewport[3], 0.01f, 100.0f);

  *ppCamera = &s_Camera;
}
