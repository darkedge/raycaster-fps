#include "stdafx.h"
#include "game.h"
#include "camera.h"
#include "mj_input.h"
#include "mj_common.h" // MJ_RT_WIDTH / MJ_RT_HEIGHT
#include "main.h"

void GameState::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) true));
  // ImGui::GetIO().WantCaptureMouse    = this->MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = this->MouseLook;

  this->camera.yFov = 60.0f;

  this->lastMousePos    = 0.0f;
  this->currentMousePos = 0.0f;
  this->camera.position = mjm::vec3(54.5f, 0.5f, 34.5f);
  this->rotation        = mjm::quat(mjm::vec3(0.0f, -this->currentMousePos, 0));
  this->yaw             = -this->currentMousePos;
}

void GameState::Do(Camera** ppCamera)
{
  ZoneScoped;

  // Reset button
  if (!ImGui::IsAnyWindowFocused() && mj::input::GetKeyDown(Key::KeyR))
  {
    Entry();
  }

  {
    ImGui::Begin("Debug");
    ImGui::Text("R to reset, F3 toggles editor");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::End();
  }

  const float dt = mj::GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    mjm::vec3 vec = this->rotation * axis::FORWARD;
    vec.y         = 0.0f;
    this->camera.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    mjm::vec3 vec = this->rotation * axis::LEFT;
    vec.y         = 0.0f;
    this->camera.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    mjm::vec3 vec = this->rotation * axis::BACKWARD;
    vec.y         = 0.0f;
    this->camera.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    mjm::vec3 vec = this->rotation * axis::RIGHT;
    vec.y         = 0.0f;
    this->camera.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }

  MJ_UNINITIALIZED int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  this->currentMousePos -= GameState::ROT_SPEED * dx;
  if (this->currentMousePos != this->lastMousePos)
  {
    this->rotation     = mjm::quat(mjm::vec3(0.0f, -this->currentMousePos, 0));
    this->yaw          = -this->currentMousePos;
    this->lastMousePos = this->currentMousePos;
  }

  mjm::mat4 rotate    = mjm::transpose(mjm::eulerAngleY(this->yaw));
  mjm::mat4 translate = mjm::identity<mjm::mat4>();
  translate           = mjm::translate(translate, -mjm::vec3(this->camera.position));

  this->camera.view        = rotate * translate;
  this->camera.viewport[0] = 0.0f;
  this->camera.viewport[1] = 0.0f;
  mj::GetWindowSize(&this->camera.viewport[2], &this->camera.viewport[3]);
  this->camera.projection = mjm::perspectiveLH_ZO(mjm::radians(this->camera.yFov),
                                                  this->camera.viewport[2] / this->camera.viewport[3], 0.01f, 100.0f);

  *ppCamera = &this->camera;
}
