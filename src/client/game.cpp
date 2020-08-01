#include "pch.h"
#include "mj_input.h"
#include "mj_common.h" // MJ_RT_WIDTH / MJ_RT_HEIGHT
#include "main.h"
#include "meta.h"

void GameState::SetLevel(Level level, ComPtr<ID3D11Device> pDevice)
{
  uint8_t xz[] = { 0, 0 }; // xz yzx zxy

  mj::ArrayList<Vertex> vertices;
  mj::ArrayList<uint16_t> indices;

  Graphics::InsertWalls(vertices, indices, &level);

  // Floor/ceiling pass
  for (size_t z = 0; z < Meta::LEVEL_DIM; z++)
  {
    // Check for blocks in this slice
    for (size_t x = 0; x < Meta::LEVEL_DIM; x++)
    {
      if (level.pBlocks[z * level.width + x] >= 0x006A)
      {
        Graphics::InsertFloor(vertices, indices, (float)x, 0.0f, (float)z, 136.0f);
        Graphics::InsertCeiling(vertices, indices, (float)x, (float)z, 138.0f);
      }
    }
  }

  this->levelMesh             = Graphics::CreateMesh(pDevice, vertices.Cast<float>(), 6, indices, D3D11_USAGE_IMMUTABLE);
  this->levelMesh.inputLayout = Graphics::GetInputLayout();
}

void GameState::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) true));
  // ImGui::GetIO().WantCaptureMouse    = this->MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = this->MouseLook;

  this->camera.yFov = 60.0f;

  this->lastMousePos    = 0.0f;
  this->currentMousePos = 0.0f;
  this->camera.position = mjm::vec3(54.5f, 0.5f, 34.5f);
  this->camera.rotation = mjm::quat(mjm::vec3(0.0f, -this->currentMousePos, 0));
  this->yaw             = -this->currentMousePos;
}

void GameState::Update(mj::ArrayList<DrawCommand>& drawList)
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

  auto& cam = this->camera;
  if (mj::input::GetKey(Key::KeyW))
  {
    mjm::vec3 vec = cam.rotation * axis::FORWARD;
    vec.y         = 0.0f;
    cam.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    mjm::vec3 vec = cam.rotation * axis::LEFT;
    vec.y         = 0.0f;
    cam.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    mjm::vec3 vec = cam.rotation * axis::BACKWARD;
    vec.y         = 0.0f;
    cam.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    mjm::vec3 vec = cam.rotation * axis::RIGHT;
    vec.y         = 0.0f;
    cam.position += mjm::vec3(mjm::normalize(vec) * dt * GameState::MOVEMENT_FACTOR);
  }

  MJ_UNINITIALIZED int32_t dx, dy;
  mj::input::GetRelativeMouseMovement(&dx, &dy);
  this->currentMousePos -= GameState::ROT_SPEED * dx;
  if (this->currentMousePos != this->lastMousePos)
  {
    cam.rotation       = mjm::quat(mjm::vec3(0.0f, -this->currentMousePos, 0));
    this->yaw          = -this->currentMousePos;
    this->lastMousePos = this->currentMousePos;
  }

  mjm::mat4 rotate    = mjm::transpose(mjm::eulerAngleY(this->yaw));
  mjm::mat4 translate = mjm::identity<mjm::mat4>();
  translate           = mjm::translate(translate, -mjm::vec3(cam.position));

  auto view       = rotate * translate;
  cam.viewport[0] = 0.0f;
  cam.viewport[1] = 0.0f;
  mj::GetWindowSize(&cam.viewport[2], &cam.viewport[3]);
  auto projection = mjm::perspectiveLH_ZO(mjm::radians(cam.yFov), cam.viewport[2] / cam.viewport[3], 0.01f, 100.0f);

  cam.viewProjection = projection * view;

  {
    auto* pCmd = drawList.EmplaceSingle();
    if (pCmd)
    {
      pCmd->pCamera      = &this->camera;
      pCmd->pMesh        = &this->levelMesh;
      pCmd->vertexShader = Graphics::GetVertexShader();
      pCmd->pixelShader  = Graphics::GetPixelShader();
    }
  }
}
