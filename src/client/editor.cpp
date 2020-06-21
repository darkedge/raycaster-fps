#include "stdafx.h"
#include "editor.h"
#include "mj_input.h"
#include "graphics.h"
#include "camera.h"
#include "main.h"
#include "mj_common.h"
#include <glm/gtc/type_ptr.hpp>

static const float MOVEMENT_FACTOR   = 3.0f;
static const float MOUSE_DRAG_FACTOR = 0.025f;

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
static int32_t s_MouseScrollFactor = 1;
static glm::quat s_Rotation;

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

void editor::Resize(float w, float h)
{
  s_Camera.viewport[2] = w;
  s_Camera.viewport[3] = h;
}

void editor::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) false));
  // ImGui::GetIO().WantCaptureMouse    = s_MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = s_MouseLook;
  // Only works if relative mouse mode is off
  float w, h;
  mj::GetWindowSize(&w, &h);
  SDL_WarpMouseInWindow(nullptr, (int)w / 2, (int)h / 2);

  s_Camera.position = glm::vec3(32.0f, 10.5f, 0.0f);

  s_Camera.viewport[0] = 0.0f;
  s_Camera.viewport[1] = 0.0f;
  s_Camera.viewport[2] = w;
  s_Camera.viewport[3] = h;

  s_Rotation = glm::quatLookAt(glm::normalize(axis::POS_Z + 2.0f * axis::NEG_Y), axis::POS_Y);
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

static void DoInput()
{
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetMouseButton(MouseButton::Right))
  {
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

    // Arcball rotation
    MJ_UNINITIALIZED int32_t dx, dy;
    mj::input::GetRelativeMouseMovement(&dx, &dy);
    if (dx != 0 || dy != 0)
    {
      glm::vec3 center(s_Camera.position.x, 0.0f, s_Camera.position.z);

      // Unit vector center->current
      glm::vec3 currentPos = mj::input::GetMousePosition();
      glm::vec3 to =
          glm::unProject(currentPos, glm::identity<glm::mat4>(), s_Camera.projection, s_Camera.viewport) - center;

      // Unit vector center->old
      glm::vec3 oldPos(currentPos.x - dx, currentPos.y - dy, 0.0f);
      glm::vec3 from =
          glm::unProject(oldPos, glm::identity<glm::mat4>(), s_Camera.projection, s_Camera.viewport) - center;

      ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
      ImGui::Begin("Arcball");
      ImGui::InputFloat3("center", glm::value_ptr(center), 7);
      auto diff = currentPos - oldPos;
      ImGui::InputFloat3("diff", glm::value_ptr(diff), 7);
      ImGui::InputFloat3("from", glm::value_ptr(from), 7);
      ImGui::InputFloat3("to", glm::value_ptr(to), 7);
      ImGui::End();

      s_Rotation *= glm::quat(from, to);
    }
  }

  if (mj::input::GetMouseButton(MouseButton::Middle))
  {
    MJ_UNINITIALIZED int32_t x, y;
    mj::input::GetRelativeMouseMovement(&x, &y);
    s_Camera.position += glm::vec3((float)-x * MOUSE_DRAG_FACTOR, 0.0f, (float)y * MOUSE_DRAG_FACTOR);
  }

  s_MouseScrollFactor -= mj::input::GetMouseScroll();
  if (s_MouseScrollFactor <= 1)
  {
    s_MouseScrollFactor = 1;
  }
}

void editor::Do(Camera** ppCamera)
{
  DoMenu();
  DoInput();

  glm::mat4 rotate    = glm::transpose(glm::mat4_cast(s_Rotation));
  glm::mat4 translate = glm::identity<glm::mat4>();
  translate           = glm::translate(translate, -glm::vec3(s_Camera.position));

  s_Camera.view = rotate * translate;
  s_Camera.yFov = 90.0f;
  float aspect  = s_Camera.viewport[2] / s_Camera.viewport[3];
#if 0
  s_Camera.projection = glm::ortho(-10.0f * s_MouseScrollFactor * aspect, //
                                   10.0f * s_MouseScrollFactor * aspect,  //
                                   -10.0f * s_MouseScrollFactor,          //
                                   10.0f * s_MouseScrollFactor,           //
                                   0.1f,                                  //
                                   1000.0f);
#else
  s_Camera.projection =
      glm::perspective(glm::radians(s_Camera.yFov), s_Camera.viewport[2] / s_Camera.viewport[3], 0.01f, 100.0f);
#endif

  *ppCamera = &s_Camera;
}

void editor::Exit()
{
}
