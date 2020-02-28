#include "mj_raytracer_hlsl.h"
#include "game.h"
#include "mj_input.h"

#include "glm/gtc/matrix_transform.hpp"
#include "mj_win32_utils.h"

#include <d3d11.h>
#include <assert.h>

#include "intermediate/CSRaytracer.h"

static ID3D11ComputeShader* s_pComputeShader;
static ID3D11UnorderedAccessView* s_pUnorderedAccessView;
static ID3D11Buffer* s_pConstantBuffer;

static mj::hlsl::Constant s_Constant;
static mj::hlsl::Constant* s_pDevicePtr;

#pragma comment (lib, "dxguid.lib")
void SetDebugName(ID3D11DeviceChild* child, const char* name)
{
  if (child != nullptr && name != nullptr)
    child->SetPrivateData(WKPDID_D3DDebugObjectName,(UINT) strlen(name), name);
}

static void Reset()
{
  s_Constant.s_Camera.position = glm::vec3(0.0f, 0.0f, 0.0f);
  s_Constant.s_Camera.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  CameraInit(MJ_REF s_Constant.s_Camera);
}

bool mj::hlsl::Init(ID3D11Device* pDevice, ID3D11Texture2D* s_pTexture)
{
  assert(!s_pComputeShader);
  WIN32_ASSERT(pDevice->CreateComputeShader(CSRaytracer, sizeof(CSRaytracer), nullptr, &s_pComputeShader));
  SetDebugName(s_pComputeShader, "mj_compute_shader");

  // Texture2D UAV
  D3D11_UNORDERED_ACCESS_VIEW_DESC descView = {};
  descView.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;
  descView.Format                           = DXGI_FORMAT_R32G32B32A32_FLOAT;
  descView.Texture2D.MipSlice               = 0;
  WIN32_ASSERT(pDevice->CreateUnorderedAccessView(s_pTexture, &descView, &s_pUnorderedAccessView));
  SetDebugName(s_pComputeShader, "mj_unordered_access_view");

  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].sphere.origin = glm::vec3(2.0f, 0.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_RedSphere].color         = glm::vec3(1.0f, 0.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.origin = glm::vec3(0.0f, -2.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_YellowSphere].color         = glm::vec3(1.0f, 1.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.origin = glm::vec3(0.0f, 2.0f, 10.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.radius = 1.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_BlueSphere].color         = glm::vec3(0.0f, 0.0f, 1.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].type     = mj::rt::Shape::Shape_AABB;
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.min = glm::vec3(-2.5f, -0.5f, 9.5f);
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.max = glm::vec3(-1.5f, 0.5f, 10.5f);
  s_Constant.s_Shapes[mj::rt::DemoShape_GreenAABB].color    = glm::vec3(0.0f, 1.0f, 0.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].type           = mj::rt::Shape::Shape_Plane;
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].plane.normal   = glm::vec3(0.0f, -1.0f, 0.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].plane.distance = 5.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_WhitePlane].color          = glm::vec3(1.0f, 1.0f, 1.0f);

  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].type           = mj::rt::Shape::Shape_Plane;
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].plane.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].plane.distance = 3.0f;
  s_Constant.s_Shapes[mj::rt::DemoShape_CyanPlane].color          = glm::vec3(0.0f, 1.0f, 1.0f);

  s_Constant.width = MJ_RT_WIDTH;
  s_Constant.height = MJ_RT_HEIGHT;

  // Set constant buffer with shapes and camera
  D3D11_BUFFER_DESC bufferDesc = { 0 };
  bufferDesc.ByteWidth         = sizeof(s_Constant);
  bufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
  bufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
  bufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData;
  InitData.pSysMem          = &s_Constant;
  InitData.SysMemPitch      = 0;
  InitData.SysMemSlicePitch = 0;

  WIN32_ASSERT(pDevice->CreateBuffer(&bufferDesc, &InitData, &s_pConstantBuffer));
  SetDebugName(s_pConstantBuffer, "mj_constant_buffer");

  Reset();

  return true;
}

void mj::hlsl::Update(ID3D11DeviceContext* pDeviceContext)
{
  CameraMovement(MJ_REF s_Constant.s_Camera);

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Reset();
  }

  auto mat       = glm::identity<glm::mat4>();
  s_Constant.mat = glm::translate(mat, s_Constant.s_Camera.position) * glm::mat4_cast(s_Constant.s_Camera.rotation);

  MJ_UNINITIALIZED D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  WIN32_ASSERT(pDeviceContext->Map(s_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));
  memcpy(mappedSubresource.pData, &s_Constant, sizeof(s_Constant));
  pDeviceContext->Unmap(s_pConstantBuffer, 0);

  // Bind
  assert(s_pComputeShader);
  pDeviceContext->CSSetShader(s_pComputeShader, nullptr, 0);
  assert(s_pUnorderedAccessView);
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, &s_pUnorderedAccessView, nullptr);
  pDeviceContext->CSSetConstantBuffers(0, 1, &s_pConstantBuffer);

  // Run compute shader
  pDeviceContext->Dispatch(16, 16, 1);

  // Unbind
  ID3D11Buffer* ppCbNull[1] = { nullptr };
  pDeviceContext->CSSetConstantBuffers(0, 1, ppCbNull);
  ID3D11UnorderedAccessView* ppUavNull[1] = { nullptr };
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUavNull, nullptr);
  pDeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void mj::hlsl::Destroy()
{
}
