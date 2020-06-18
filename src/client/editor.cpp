#include "stdafx.h"
#include "editor.h"
#include "mj_input.h"
#include "graphics.h"
#include "camera.h"
#include "main.h"
#include "mj_common.h"

static const float MOVEMENT_FACTOR = 3.0f;

struct ETool
{
  enum Enum
  {
    Select,
    Paint,
    Erase,
    Dropper,
    COUNT
  };
};

static Camera s_Camera;

#if 0
void editor::Show()
{
  ImGui::Begin("Editor");
  static int selectedTool = ETool::Select;

  if (ImGui::IsWindowFocused())
  {
    if (mj::input::GetKeyDown(Key::KeyQ))
    {
      selectedTool = ETool::Select;
    }
    else if (mj::input::GetKeyDown(Key::KeyW))
    {
      selectedTool = ETool::Paint;
    }
    else if (mj::input::GetKeyDown(Key::KeyE))
    {
      selectedTool = ETool::Erase;
    }
    else if (mj::input::GetKeyDown(Key::KeyR))
    {
      selectedTool = ETool::Dropper;
    }
  }

  ImGui::RadioButton("[Q] Select", &selectedTool, ETool::Select);
  ImGui::RadioButton("[W] Paint", &selectedTool, ETool::Paint);
  ImGui::RadioButton("[E] Erase", &selectedTool, ETool::Erase);
  ImGui::RadioButton("[R] Dropper", &selectedTool, ETool::Dropper);

  static int selected[64 * 64] = {};
  for (int i = 0; i < 64 * 64; i++)
  {
    ImGui::PushID(i);

    void* pTexture = rs::GetTileTexture(i % 64, i / 64);
    if (pTexture)
    {
      ImGui::Image(pTexture, ImVec2(50, 50));
    }
#if 0
    if (ImGui::Selectable("##dummy", selected[i] != 0, 0, ImVec2(50, 50)))
    {
      
    }
#endif
    if ((i % 64) < 63)
      ImGui::SameLine();
    ImGui::PopID();
  }
  ImGui::End();
}
#endif

void editor::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) false));
  // ImGui::GetIO().WantCaptureMouse    = s_MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = s_MouseLook;
  // Only works if relative mouse mode is off
  float w, h;
  mj::GetWindowSize(&w, &h);
  SDL_WarpMouseInWindow(nullptr, w / 2, h / 2);

  s_Camera.position = glm::vec3(32.0f, 10.5f, 0.0f);
}

void editor::Do(Camera** ppCamera)
{
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetKey(Key::KeyW))
  {
    glm::vec3 vec = axis::POS_Z;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyA))
  {
    glm::vec3 vec = axis::NEG_X;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyS))
  {
    glm::vec3 vec = axis::NEG_Z;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }
  if (mj::input::GetKey(Key::KeyD))
  {
    glm::vec3 vec = axis::POS_X;
    vec.y         = 0.0f;
    s_Camera.position += glm::vec3(glm::normalize(vec) * dt * MOVEMENT_FACTOR);
  }

  glm::mat4 rotate =
      glm::transpose(glm::mat4_cast(glm::quatLookAt(glm::normalize(axis::POS_Z + axis::NEG_Y), axis::POS_Y)));
  glm::mat4 translate = glm::identity<glm::mat4>();
  translate           = glm::translate(translate, -glm::vec3(s_Camera.position));

  s_Camera.view        = rotate * translate;
  s_Camera.yFov        = 90.0f;
  s_Camera.viewport[0] = 0.0f;
  s_Camera.viewport[1] = 0.0f;
  mj::GetWindowSize(&s_Camera.viewport[2], &s_Camera.viewport[3]);
  s_Camera.projection = glm::perspective(glm::radians(s_Camera.yFov),                 //
                                         s_Camera.viewport[2] / s_Camera.viewport[3], //
                                         0.01f,                                       //
                                         100.0f);
#if 0
  s_Camera.projection = glm::ortho(s_Camera.viewport[0], //
                                   s_Camera.viewport[1], //
                                   s_Camera.viewport[2], //
                                   s_Camera.viewport[3], //
                                   0.01f,                //
                                   100.0f);
#endif

  // auto bla = glm::unProject(mj::input::GetMousePosition(), s_Camera.model, s_Camera.projection, s_Camera.viewport);

  *ppCamera = &s_Camera;
}

void editor::Exit()
{
}
