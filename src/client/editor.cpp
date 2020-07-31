#include "pch.h"
#include "mj_input.h"
#include "camera.h"
#include "main.h"
#include "mj_common.h"
#include "meta.h"

#include "generated/block_cursor_vs.h"
#include "generated/block_cursor_ps.h"

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
  camera.rotation = mjm::quatLookAtLH(mjm::normalize(axis::FORWARD + 2.0f * axis::DOWN), axis::UP);

  camera.viewport[0] = 0.0f;
  camera.viewport[1] = 0.0f;
  camera.viewport[2] = w;
  camera.viewport[3] = h;

  camera.floor.show            = true;
  camera.floor.backFaceCulling = true;
  camera.walls.show            = true;
  camera.walls.backFaceCulling = true;
}

void EditorState::Init(ComPtr<ID3D11Device> pDevice)
{
  this->inputComboNew    = { InputCombo::KEYBOARD, Key::KeyN, Modifier::LeftCtrl, MouseButton::None };
  this->inputComboOpen   = { InputCombo::KEYBOARD, Key::KeyO, Modifier::None, MouseButton::None };
  this->inputComboSave   = { InputCombo::KEYBOARD, Key::KeyS, Modifier::LeftCtrl, MouseButton::None };
  this->inputComboSaveAs = { InputCombo::KEYBOARD, Key::KeyS, Modifier::LeftCtrl | Modifier::LeftShift,
                             MouseButton::None };
  ResetCamera(this->camera);
  CreateBlockCursor(pDevice);

  this->blockCursorMatrix = mjm::mat4(1.0f, 0.0f, 0.0f, 0.0f, //
                                      0.0f, 1.0f, 0.0f, 0.0f, //
                                      0.0f, 0.0f, 1.0f, 0.0f, //
                                      0.0f, 0.0f, 0.0f, 1.0f);
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

void EditorState::CreateBlockCursor(ComPtr<ID3D11Device> pDevice)
{
  MJ_DISCARD(pDevice->CreateVertexShader(block_cursor_vs, sizeof(block_cursor_vs), nullptr,
                                         this->pVertexShader.ReleaseAndGetAddressOf()));
  MJ_DISCARD(pDevice->CreatePixelShader(block_cursor_ps, sizeof(block_cursor_ps), nullptr,
                                        this->pPixelShader.ReleaseAndGetAddressOf()));

  // w = 1.0f;
  // rgb = 0.0f;
  // a = 0.4f;
  float vertices[] = {
    -0.002f, -0.002f, -0.002f, // 0
    -0.002f, -0.002f, 1.002f,  // 1
    -0.002f, 1.002f,  -0.002f, // 2
    -0.002f, 1.002f,  1.002f,  // 3
    1.002f,  -0.002f, -0.002f, // 4
    1.002f,  -0.002f, 1.002f,  // 5
    1.002f,  1.002f,  -0.002f, // 6
    1.002f,  1.002f,  1.002f   // 7
  };

  // Line list (24)
  uint16_t indices[] = { 4, 5, 5, 7, 7, 6, 5, 1, 1, 3, 3, 7, 1, 0, 0, 2, 2, 3, 0, 4, 4, 6, 6, 2 };

  mj::ArrayListView<float> vertexList(vertices);
  mj::ArrayListView<uint16_t> indexList(indices);

  // Create static index buffer.
  this->blockCursor                   = Graphics::CreateMesh(pDevice, vertexList, 3, indexList);
  this->blockCursor.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

  {
    D3D11_INPUT_ELEMENT_DESC desc = {};

    // float3 a_position : POSITION
    desc.SemanticName         = "POSITION";
    desc.SemanticIndex        = 0;
    desc.Format               = DXGI_FORMAT_R32G32B32_FLOAT;
    desc.InputSlot            = 0;
    desc.AlignedByteOffset    = 0;
    desc.InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
    desc.InstanceDataStepRate = 0;

    MJ_DISCARD(pDevice->CreateInputLayout(&desc, 1, block_cursor_vs, sizeof(block_cursor_vs),
                                          this->blockCursor.inputLayout.ReleaseAndGetAddressOf()));
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
  auto& cam      = this->camera;

  if (mj::input::GetMouseButton(MouseButton::Right))
  {
    float speed = EditorState::MOVEMENT_FACTOR;
    if (mj::input::GetKey(Key::LeftShift))
    {
      speed *= 3.0f;
    }
    if (mj::input::GetKey(Key::KeyW))
    {
      cam.position += cam.rotation * axis::FORWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyA))
    {
      cam.position += cam.rotation * axis::LEFT * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyS))
    {
      cam.position += cam.rotation * axis::BACKWARD * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyD))
    {
      cam.position += cam.rotation * axis::RIGHT * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyQ))
    {
      cam.position += cam.rotation * axis::DOWN * dt * speed;
    }
    if (mj::input::GetKey(Key::KeyE))
    {
      cam.position += cam.rotation * axis::UP * dt * speed;
    }

    {
      // mouse x = yaw (left/right), mouse y = pitch (up/down)
      MJ_UNINITIALIZED int32_t dx, dy;
      mj::input::GetRelativeMouseMovement(&dx, &dy);
      cam.rotation = mjm::angleAxis(EditorState::MOUSE_LOOK_FACTOR * dx, axis::UP) * cam.rotation;
      cam.rotation = mjm::angleAxis(EditorState::MOUSE_LOOK_FACTOR * dy, cam.rotation * axis::RIGHT) * cam.rotation;
    }
  }

#if 0
  if (mj::input::GetKey(Key::LeftAlt) && mj::input::GetMouseButton(MouseButton::Left))
  {
    // Arcball rotation
    MJ_UNINITIALIZED int32_t dx, dy;
    mj::input::GetRelativeMouseMovement(&dx, &dy);
    if (dx != 0 || dy != 0)
    {
      mjm::vec3 center(cam.position.x, 0.0f, cam.position.z);

      // Unit vector center->current
      float x, y;
      mj::input::GetMousePosition(&x, &y);
      mjm::vec3 currentPos(x, y, 0.0f);
      mjm::vec3 to =
          mjm::unProjectZO(currentPos, mjm::identity<mjm::mat4>(), cam.projection, cam.viewport) -
          center;

      // Unit vector center->old
      mjm::vec3 oldPos(currentPos.x - dx, currentPos.y - dy, 0.0f);
      mjm::vec3 from =
          mjm::unProjectZO(oldPos, mjm::identity<mjm::mat4>(), cam.projection, cam.viewport) - center;

      ImGui::SetNextWindowSize(ImVec2(400.0f, 400.0f));
      ImGui::Begin("Arcball");
      ImGui::InputFloat3("center", mjm::value_ptr(center), 7);
      auto diff = currentPos - oldPos;
      ImGui::InputFloat3("diff", mjm::value_ptr(diff), 7);
      ImGui::InputFloat3("from", mjm::value_ptr(from), 7);
      ImGui::InputFloat3("to", mjm::value_ptr(to), 7);
      ImGui::End();

      cam.rotation *= mjm::quat(from, to);
    }
  }
#endif

  if (mj::input::GetMouseButton(MouseButton::Middle))
  {
    MJ_UNINITIALIZED int32_t x, y;
    mj::input::GetRelativeMouseMovement(&x, &y);
    // cam.position += mjm::vec3((float)-x * MOUSE_DRAG_FACTOR, 0.0f, (float)y * MOUSE_DRAG_FACTOR);
    // cam.position += this->rotation * axis::RIGHT
  }

  this->mouseScrollFactor -= mj::input::GetMouseScroll();
  if (this->mouseScrollFactor <= 1)
  {
    this->mouseScrollFactor = 1;
  }
}

// -X, -Y, -Z, +X, +Y, +Z
enum EFace
{
  EFace_West,
  EFace_Bottom,
  EFace_South,
  EFace_East,
  EFace_Top,
  EFace_North
};

struct TraversalResult
{
  mjm::int3 position;
  int32_t type;
  EFace face;
};

static int32_t GetBlock(const Level* pLevel, const mjm::int3& position)
{
  if (position.x >= 0 &&            //
      position.x < pLevel->width && //
      position.z >= 0 &&            //
      position.z < pLevel->height)
  {
    auto Block = [&] { return (int32_t)pLevel->pBlocks[position.x * pLevel->width + position.z]; };
    if (position.y == -1)
    {
      int32_t block = Block();
      if (block >= 0x006A)
      {
        return block;
      }
    }
    else if (position.y == 0)
    {
      int32_t block = Block();
      if (block < 0x006A)
      {
        return block;
      }
    }
  }

  return -1;
}

static bool Traverse(const Level* pLevel, mjm::vec3 origin, mjm::vec3 direction, float distance,
                     TraversalResult* pResult)
{
  // Step values
  int32_t stepX = direction.x < 0 ? -1 : 1;
  int32_t stepY = direction.y < 0 ? -1 : 1;
  int32_t stepZ = direction.z < 0 ? -1 : 1;

  // Inverse direction
  float tDeltaX = fabsf(1.0f / direction.x);
  float tDeltaY = fabsf(1.0f / direction.y);
  float tDeltaZ = fabsf(1.0f / direction.z);

  // t values
  float tMax  = 0.0f;
  float tMaxX = (direction.x > 0 ? ceilf(origin.x) - origin.x : origin.x - floorf(origin.x)) * tDeltaX;
  float tMaxY = (direction.y > 0 ? ceilf(origin.y) - origin.y : origin.y - floorf(origin.y)) * tDeltaY;
  float tMaxZ = (direction.z > 0 ? ceilf(origin.z) - origin.z : origin.z - floorf(origin.z)) * tDeltaZ;

  pResult->position.x = (int32_t)floorf(origin.x);
  pResult->position.y = (int32_t)floorf(origin.y);
  pResult->position.z = (int32_t)floorf(origin.z);
  pResult->type       = -1;

  MJ_UNINITIALIZED mjm::int3 endPos;
  endPos.x = (int32_t)floorf(origin.x + distance * direction.x);
  endPos.y = (int32_t)floorf(origin.y + distance * direction.y);
  endPos.z = (int32_t)floorf(origin.z + distance * direction.z);

  // Advance ray to level layer
  // int32_t diff = (int32_t)floorf(1 - pResult->position.y);
  // tMaxY += diff * tDeltaY;
  // tMax = tMaxY;

  while (pResult->position != endPos)
  {
    int32_t block = GetBlock(pLevel, pResult->position);

    if (block != -1)
    {
      pResult->type = block;

      return true;
    }

    if (tMaxX < tMaxY)
    {
      if (tMaxX < tMaxZ)
      {
        pResult->position.x += stepX;
        pResult->face = stepX > 0 ? EFace_West : EFace_East;
        tMaxX += tDeltaX;
        tMax = tMaxX;
      }
      else
      {
        pResult->position.z += stepZ;
        pResult->face = stepZ > 0 ? EFace_South : EFace_North;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
    else
    {
      if (tMaxY < tMaxZ)
      {
        pResult->position.y += stepY;
        pResult->face = stepY > 0 ? EFace_Bottom : EFace_Top;

        if ((direction.y < 0 && pResult->position.y < 0) || (direction.y > 0 && pResult->position.y >= 2)) // TODO
        {
          return false;
        }

        tMaxY += tDeltaY;
        tMax = tMaxY;
      }
      else
      {
        pResult->position.z += stepZ;
        pResult->face = stepZ > 0 ? EFace_South : EFace_North;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
  }

  return false;
}

void EditorState::UpdateBlockCursor(const mjm::mat4& vp)
{
  MJ_UNINITIALIZED float mouseX;
  MJ_UNINITIALIZED float mouseY;
  mj::input::GetMousePosition(&mouseX, &mouseY);

  MJ_UNINITIALIZED float windowWidth;
  MJ_UNINITIALIZED float windowHeight;
  mj::GetWindowSize(&windowWidth, &windowHeight);

  MJ_UNINITIALIZED mjm::vec4 ndcNear;
  ndcNear.x = mouseX / windowWidth * 2.0f - 1.0f;
  ndcNear.y = 1.0f - mouseY / windowHeight * 2.0f;
  ndcNear.z = 0.0f;
  ndcNear.w = 1.0f;

  mjm::vec4 ndcFar(ndcNear.x, ndcNear.y, 1.0f, 1.0f);

  auto inv       = mjm::inverse(vp);
  auto worldNear = inv * ndcNear;
  auto worldFar  = inv * ndcFar;
  worldNear /= worldNear.w;
  worldFar /= worldFar.w;
  mjm::vec4 ray = mjm::normalize(worldFar - worldNear);

  // Ray-voxel intersection (2 y-layers: walls and floors)
  MJ_UNINITIALIZED TraversalResult result;
  if (Traverse(this->pLevel, worldNear, ray, -1.0f, &result))
  {
    this->blockCursorMatrix[3][0] = (float)result.position.x;
    this->blockCursorMatrix[3][2] = (float)result.position.z;
  }
}

void EditorState::Update(mj::ArrayList<DrawCommand>& drawList)
{
  auto& cam = this->camera;

  this->DoMenu();
  this->DoInput();
  this->UpdateBlockCursor(cam.viewProjection);

  mjm::mat4 rotate    = mjm::transpose(mjm::mat4_cast(cam.rotation));
  mjm::mat4 translate = mjm::identity<mjm::mat4>();
  translate           = mjm::translate(translate, -mjm::vec3(cam.position));

  auto view          = rotate * translate;
  cam.yFov           = 90.0f;
  auto projection    = mjm::perspectiveLH_ZO(mjm::radians(cam.yFov), cam.viewport[2] / cam.viewport[3], 0.01f, 100.0f);
  cam.viewProjection = projection * view;

  {
    // Level
    DrawCommand* pCmd = drawList.EmplaceSingle();
    if (pCmd)
    {
      pCmd->pCamera      = &this->camera;
      pCmd->pMesh        = &this->levelMesh;
      pCmd->vertexShader = Graphics::GetVertexShader();
      pCmd->pixelShader  = Graphics::GetPixelShader();
    }
  }

  {
    // Block cursor
    DrawCommand* pCmd = drawList.EmplaceSingle();
    if (pCmd)
    {
      pCmd->vertexShader = this->pVertexShader;
      pCmd->pixelShader  = this->pPixelShader;
      pCmd->pCamera      = &this->camera;
      pCmd->pMatrix      = &this->blockCursorMatrix;
      pCmd->pMesh        = &this->blockCursor;
    }
  }
}

void EditorState::SetLevel(const Level* pLvl, ComPtr<ID3D11Device> pDevice)
{
  this->pLevel = pLvl;

  mj::ArrayList<Vertex> vertices;
  mj::ArrayList<uint16_t> indices;

  Graphics::InsertWalls(vertices, indices, pLevel);

  // Floor/ceiling pass
  for (size_t z = 0; z < Meta::LEVEL_DIM; z++)
  {
    // Check for blocks in this slice
    for (size_t x = 0; x < Meta::LEVEL_DIM; x++)
    {
      if (this->pLevel->pBlocks[z * this->pLevel->width + x] >= 0x006A)
      {
        Graphics::InsertFloor(vertices, indices, (float)x, 0.0f, (float)z, 136.0f);
        Graphics::InsertCeiling(vertices, indices, (float)x, (float)z, 138.0f);
      }
      else
      {
        // Editor: draw top of level for clarity
        Graphics::InsertFloor(vertices, indices, (float)x, 1.0f, (float)z, 138.0f);
      }
    }
  }

  {
    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage          = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth      = vertices.ByteWidth();
    bufferDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags      = 0;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = vertices.Get();
    InitData.SysMemPitch      = vertices.ElemSize();
    this->levelMesh.stride    = InitData.SysMemPitch;
    InitData.SysMemSlicePitch = 0;

    // Create the vertex buffer.
    MJ_DISCARD(pDevice->CreateBuffer(&bufferDesc, &InitData, this->levelMesh.vertexBuffer.ReleaseAndGetAddressOf()));
  }
  {
    this->levelMesh.indexCount  = indices.Size();
    this->levelMesh.inputLayout = Graphics::GetInputLayout();

    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage          = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth      = indices.ByteWidth();
    bufferDesc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags      = 0;

    // Define the resource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = indices.Get();
    InitData.SysMemPitch      = indices.ElemSize();
    InitData.SysMemSlicePitch = 0;

    // Create the buffer with the device.
    MJ_DISCARD(pDevice->CreateBuffer(&bufferDesc, &InitData, this->levelMesh.indexBuffer.ReleaseAndGetAddressOf()));
  }
}
