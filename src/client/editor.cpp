#include "stdafx.h"
#include "mj_input.h"
#include "camera.h"
#include "main.h"
#include "mj_common.h"
#include "meta.h"

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

  ImGui::End();
}
#endif

void EditorState::Resize(float w, float h)
{
  this->camera.viewport[2] = w;
  this->camera.viewport[3] = h;
}

static void ResetCamera(Camera& camera)
{
  MJ_UNINITIALIZED float w, h;
  mj::GetWindowSize(&w, &h);

  camera.position = mjm::vec3(32.0f, 10.5f, 0.0f);

  camera.viewport[0] = 0.0f;
  camera.viewport[1] = 0.0f;
  camera.viewport[2] = w;
  camera.viewport[3] = h;

  camera.floor.show            = true;
  camera.floor.backFaceCulling = true;
  camera.walls.show            = true;
  camera.walls.backFaceCulling = true;
}

void EditorState::Entry()
{
  MJ_DISCARD(SDL_SetRelativeMouseMode((SDL_bool) false));
  // ImGui::GetIO().WantCaptureMouse    = this->MouseLook;
  // ImGui::GetIO().WantCaptureKeyboard = this->MouseLook;
  // Only works if relative mouse mode is off
  MJ_UNINITIALIZED float w, h;
  mj::GetWindowSize(&w, &h);
  mj::MoveMouse((int)w / 2, (int)h / 2);

  this->rotation = mjm::quatLookAtLH(mjm::normalize(axis::FORWARD + 2.0f * axis::DOWN), axis::UP);

  ResetCamera(this->camera);

  this->inputComboNew    = { InputCombo::KEYBOARD, Key::KeyN, Modifier::LeftCtrl, MouseButton::None };
  this->inputComboOpen   = { InputCombo::KEYBOARD, Key::KeyO, Modifier::None, MouseButton::None };
  this->inputComboSave   = { InputCombo::KEYBOARD, Key::KeyS, Modifier::LeftCtrl, MouseButton::None };
  this->inputComboSaveAs = { InputCombo::KEYBOARD, Key::KeyS, Modifier::LeftCtrl | Modifier::LeftShift,
                             MouseButton::None };
}

static void SaveFileDialog()
{
  MJ_UNINITIALIZED IFileSaveDialog* pFileSave;

  // Create the FileSaveDialog object.
  HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog,
                                reinterpret_cast<void**>(&pFileSave));

  if (SUCCEEDED(hr))
  {
    // Show the Save dialog box.
    hr = pFileSave->Show(nullptr);

    // Get the file name from the dialog box.
    if (SUCCEEDED(hr))
    {
      MJ_UNINITIALIZED IShellItem* pItem;
      hr = pFileSave->GetResult(&pItem);
      if (SUCCEEDED(hr))
      {
        MJ_UNINITIALIZED PWSTR pszFilePath;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

        if (SUCCEEDED(hr))
        {
          // MessageBoxW(nullptr, pszFilePath, L"File Path", MB_OK);
          CoTaskMemFree(pszFilePath);
        }
        pItem->Release();
      }
    }
    pFileSave->Release();
  }
}

static void OpenFileDialog()
{
  MJ_UNINITIALIZED IFileOpenDialog* pFileOpen;

  // Create the FileOpenDialog object.
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                reinterpret_cast<void**>(&pFileOpen));

  if (SUCCEEDED(hr))
  {
    // Show the Open dialog box.
    // Does not work... requires full name. TODO: Frame-alloc the current working directory.
    // IShellItem* pSI;
    // SHCreateItemFromParsingName(L"./", nullptr, IID_IShellItem, (void**)&pSI);
    // pFileOpen->SetFolder(pSI);
    hr = pFileOpen->Show(nullptr);

    // Get the file name from the dialog box.
    if (SUCCEEDED(hr))
    {
      MJ_UNINITIALIZED IShellItem* pItem;
      hr = pFileOpen->GetResult(&pItem);
      if (SUCCEEDED(hr))
      {
        MJ_UNINITIALIZED PWSTR pszFilePath;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

        if (SUCCEEDED(hr))
        {
          // MessageBoxW(nullptr, pszFilePath, L"File Path", MB_OK);
          CoTaskMemFree(pszFilePath);
        }
        pItem->Release();
      }
    }
    pFileOpen->Release();
  }
}

void EditorState::DoMenu()
{
  bool newLevel = mj::input::GetControlDown(this->inputComboNew);
  bool open     = mj::input::GetControlDown(this->inputComboOpen);
  bool save     = mj::input::GetControlDown(this->inputComboSave);
  bool saveAs   = mj::input::GetControlDown(this->inputComboSaveAs);
  if (ImGui::BeginMainMenuBar())
  {
    if (ImGui::BeginMenu("File"))
    {
      newLevel |= ImGui::MenuItem("New", "Ctrl+N");
      open |= ImGui::MenuItem("Open...", "Ctrl+Z");
      save |= ImGui::MenuItem("Save", "Ctrl+S");
      saveAs |= ImGui::MenuItem("Save As...", "Ctrl+Shift+S");
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit"))
    {
      if (ImGui::MenuItem("Undo", "Ctrl+Z"))
      {
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y"))
      {
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View"))
    {
      if (ImGui::BeginMenu("Floor"))
      {
        ImGui::MenuItem("Show", "", &this->camera.floor.show);
        ImGui::MenuItem("Back-Face Culling", "", &this->camera.floor.backFaceCulling);
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Walls"))
      {
        ImGui::MenuItem("Show", "", &this->camera.walls.show);
        ImGui::MenuItem("Back-Face Culling", "", &this->camera.walls.backFaceCulling);
        ImGui::EndMenu();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (newLevel)
  {
    pMeta->NewLevel();
    ResetCamera(this->camera);
  }
  if (open)
  {
    OpenFileDialog();
  }
  if (save)
  {
    // TODO
  }
  if (saveAs)
  {
    SaveFileDialog();
  }
}

void EditorState::DoInput()
{
  const float dt = mj::GetDeltaTime();

  if (mj::input::GetMouseButton(MouseButton::Right))
  {
    float speed = EditorState::MOVEMENT_FACTOR;
    if (mj::input::GetKey(Key::LeftShift))
    {
      speed *= 3.0f;
    }
    if (mj::input::GetKey(Key::KeyW))
    {
      this->camera.position += this->rotation * axis::FORWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyA))
    {
      this->camera.position += this->rotation * axis::LEFT * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyS))
    {
      this->camera.position += this->rotation * axis::BACKWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyD))
    {
      this->camera.position += this->rotation * axis::RIGHT * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyQ))
    {
      this->camera.position += this->rotation * axis::DOWN * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyE))
    {
      this->camera.position += this->rotation * axis::UP * dt * speed;
    }

    {
      // mouse x = yaw (left/right), mouse y = pitch (up/down)
      MJ_UNINITIALIZED int32_t dx, dy;
      mj::input::GetRelativeMouseMovement(&dx, &dy);
      this->rotation = mjm::angleAxis(EditorState::MOUSE_LOOK_FACTOR * dx, axis::UP) * this->rotation;
      this->rotation =
          mjm::angleAxis(EditorState::MOUSE_LOOK_FACTOR * dy, this->rotation * axis::RIGHT) * this->rotation;
    }
  }

  if (mj::input::GetKey(Key::LeftAlt) && mj::input::GetMouseButton(MouseButton::Left))
  {
    // Arcball rotation
    MJ_UNINITIALIZED int32_t dx, dy;
    mj::input::GetRelativeMouseMovement(&dx, &dy);
    if (dx != 0 || dy != 0)
    {
      mjm::vec3 center(this->camera.position.x, 0.0f, this->camera.position.z);

      // Unit vector center->current
      float x, y;
      mj::input::GetMousePosition(&x, &y);
      mjm::vec3 currentPos(x, y, 0.0f);
      mjm::vec3 to =
          mjm::unProjectZO(currentPos, mjm::identity<mjm::mat4>(), this->camera.projection, this->camera.viewport) -
          center;

      // Unit vector center->old
      mjm::vec3 oldPos(currentPos.x - dx, currentPos.y - dy, 0.0f);
      mjm::vec3 from =
          mjm::unProjectZO(oldPos, mjm::identity<mjm::mat4>(), this->camera.projection, this->camera.viewport) - center;

      ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
      ImGui::Begin("Arcball");
      ImGui::InputFloat3("center", mjm::value_ptr(center), 7);
      auto diff = currentPos - oldPos;
      ImGui::InputFloat3("diff", mjm::value_ptr(diff), 7);
      ImGui::InputFloat3("from", mjm::value_ptr(from), 7);
      ImGui::InputFloat3("to", mjm::value_ptr(to), 7);
      ImGui::End();

      this->rotation *= mjm::quat(from, to);
    }
  }

  if (mj::input::GetMouseButton(MouseButton::Middle))
  {
    MJ_UNINITIALIZED int32_t x, y;
    mj::input::GetRelativeMouseMovement(&x, &y);
    // this->camera.position += mjm::vec3((float)-x * MOUSE_DRAG_FACTOR, 0.0f, (float)y * MOUSE_DRAG_FACTOR);
    // this->camera.position += this->rotation * axis::RIGHT
  }

  this->mouseScrollFactor -= mj::input::GetMouseScroll();
  if (this->mouseScrollFactor <= 1)
  {
    this->mouseScrollFactor = 1;
  }
}

void EditorState::Do(Camera** ppCamera)
{
  this->DoMenu();
  this->DoInput();

  mjm::mat4 rotate    = mjm::transpose(mjm::mat4_cast(this->rotation));
  mjm::mat4 translate = mjm::identity<mjm::mat4>();
  translate           = mjm::translate(translate, -mjm::vec3(this->camera.position));

  this->camera.view = rotate * translate;
  this->camera.yFov = 90.0f;
#if 0
  float aspect      = this->camera.viewport[2] / this->camera.viewport[3];
  this->camera.projection = mjm::ortho(-10.0f * this->mouseScrollFactor * aspect, //
                                   10.0f * this->mouseScrollFactor * aspect,  //
                                   -10.0f * this->mouseScrollFactor,          //
                                   10.0f * this->mouseScrollFactor,           //
                                   0.1f,                                  //
                                   1000.0f);
#else
  this->camera.projection = mjm::perspectiveLH_ZO(mjm::radians(this->camera.yFov),
                                                  this->camera.viewport[2] / this->camera.viewport[3], 0.01f, 100.0f);
#endif

  *ppCamera = &this->camera;
}

void EditorState::Exit()
{
}
