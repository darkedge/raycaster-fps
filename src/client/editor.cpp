#include "stdafx.h"
#include "editor.h"
#include "mj_input.h"
#include "graphics.h"
#include "camera.h"
#include "main.h"
#include "mj_common.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#if 0
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

void editor::Editor::Resize(float w, float h)
{
  this->camera.viewport[2] = w;
  this->camera.viewport[3] = h;
}

void editor::Editor::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) false));
  // ImGui::GetIO().WantCaptureMouse    = pEditor->MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = pEditor->MouseLook;
  // Only works if relative mouse mode is off
  float w, h;
  mj::GetWindowSize(&w, &h);
  SDL_WarpMouseInWindow(nullptr, (int)w / 2, (int)h / 2);

  this->camera.position = glm::vec3(32.0f, 10.5f, 0.0f);

  this->camera.viewport[0] = 0.0f;
  this->camera.viewport[1] = 0.0f;
  this->camera.viewport[2] = w;
  this->camera.viewport[3] = h;

  this->rotation = glm::quatLookAt(glm::normalize(axis::FORWARD + 2.0f * axis::DOWN), axis::UP);
}

static void DoMenu()
{
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }
}

static void DoInput(editor::Editor* pEditor)
{
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetMouseButton(MouseButton::Right))
  {
    float speed = editor::Editor::MOVEMENT_FACTOR;
    if (mj::input::GetKey(Key::LeftShift))
    {
      speed *= 3.0f;
    }
    if (mj::input::GetKey(Key::KeyW))
    {
      pEditor->camera.position += pEditor->rotation * axis::FORWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyA))
    {
      pEditor->camera.position += pEditor->rotation * axis::LEFT * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyS))
    {
      pEditor->camera.position += pEditor->rotation * axis::BACKWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyD))
    {
      pEditor->camera.position += pEditor->rotation * axis::RIGHT * dt * speed;
    }

    {
      // mouse x = yaw (left/right), mouse y = pitch (up/down)
      MJ_UNINITIALIZED int32_t dx, dy;
      mj::input::GetRelativeMouseMovement(&dx, &dy);
      pEditor->rotation = glm::angleAxis(editor::Editor::MOUSE_LOOK_FACTOR * dx, axis::UP) * pEditor->rotation;
      pEditor->rotation =
          glm::angleAxis(editor::Editor::MOUSE_LOOK_FACTOR * dy, pEditor->rotation * axis::RIGHT) * pEditor->rotation;
    }
  }

  if (mj::input::GetKey(Key::LeftAlt) && mj::input::GetMouseButton(MouseButton::Left))
  {
    // Arcball rotation
    MJ_UNINITIALIZED int32_t dx, dy;
    mj::input::GetRelativeMouseMovement(&dx, &dy);
    if (dx != 0 || dy != 0)
    {
      glm::vec3 center(pEditor->camera.position.x, 0.0f, pEditor->camera.position.z);

      // Unit vector center->current
      glm::vec3 currentPos = mj::input::GetMousePosition();
      glm::vec3 to =
          glm::unProject(currentPos, glm::identity<glm::mat4>(), pEditor->camera.projection, pEditor->camera.viewport) -
          center;

      // Unit vector center->old
      glm::vec3 oldPos(currentPos.x - dx, currentPos.y - dy, 0.0f);
      glm::vec3 from =
          glm::unProject(oldPos, glm::identity<glm::mat4>(), pEditor->camera.projection, pEditor->camera.viewport) -
          center;

      ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
      ImGui::Begin("Arcball");
      ImGui::InputFloat3("center", glm::value_ptr(center), 7);
      auto diff = currentPos - oldPos;
      ImGui::InputFloat3("diff", glm::value_ptr(diff), 7);
      ImGui::InputFloat3("from", glm::value_ptr(from), 7);
      ImGui::InputFloat3("to", glm::value_ptr(to), 7);
      ImGui::End();

      pEditor->rotation *= glm::quat(from, to);
    }
  }

  if (mj::input::GetMouseButton(MouseButton::Middle))
  {
    MJ_UNINITIALIZED int32_t x, y;
    mj::input::GetRelativeMouseMovement(&x, &y);
    // pEditor->camera.position += glm::vec3((float)-x * MOUSE_DRAG_FACTOR, 0.0f, (float)y * MOUSE_DRAG_FACTOR);
    // pEditor->camera.position += pEditor->rotation * axis::RIGHT
  }

  pEditor->mouseScrollFactor -= mj::input::GetMouseScroll();
  if (pEditor->mouseScrollFactor <= 1)
  {
    pEditor->mouseScrollFactor = 1;
  }
}

void editor::Editor::Do(Camera** ppCamera)
{
  DoMenu();
  DoInput(this);

  glm::mat4 rotate    = glm::transpose(glm::mat4_cast(this->rotation));
  glm::mat4 translate = glm::identity<glm::mat4>();
  translate           = glm::translate(translate, -glm::vec3(this->camera.position));

  this->camera.view = rotate * translate;
  this->camera.yFov = 90.0f;
#if 0
  float aspect      = this->camera.viewport[2] / this->camera.viewport[3];
  pEditor->camera.projection = glm::ortho(-10.0f * pEditor->mouseScrollFactor * aspect, //
                                   10.0f * pEditor->mouseScrollFactor * aspect,  //
                                   -10.0f * pEditor->mouseScrollFactor,          //
                                   10.0f * pEditor->mouseScrollFactor,           //
                                   0.1f,                                  //
                                   1000.0f);
#else
  this->camera.projection = glm::perspective(glm::radians(this->camera.yFov),
                                             this->camera.viewport[2] / this->camera.viewport[3], 0.01f, 100.0f);
#endif

  *ppCamera = &this->camera;
}

void editor::Editor::Exit()
{
}
