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
static ID3D11ShaderResourceView* s_pShaderResourceView;
static ID3D11Buffer* s_pShapeBuffer;

static mj::rt::Shape s_Shapes[mj::rt::DemoShape_Count];

static mj::hlsl::Constant s_Constant;
static mj::hlsl::Constant* s_pDevicePtr;

#pragma comment(lib, "dxguid.lib")
void SetDebugName(ID3D11DeviceChild* child, const char* name)
{
  if (child != nullptr && name != nullptr)
    child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
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

  s_Shapes[mj::rt::DemoShape_RedSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Shapes[mj::rt::DemoShape_RedSphere].color         = glm::vec3(1.0f, 0.0f, 0.0f);
  s_Shapes[mj::rt::DemoShape_RedSphere].sphere.origin = glm::vec3(2.0f, 0.0f, 10.0f);
  s_Shapes[mj::rt::DemoShape_RedSphere].sphere.radius = 1.0f;

  s_Shapes[mj::rt::DemoShape_YellowSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Shapes[mj::rt::DemoShape_YellowSphere].color         = glm::vec3(1.0f, 1.0f, 0.0f);
  s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.origin = glm::vec3(0.0f, -2.0f, 10.0f);
  s_Shapes[mj::rt::DemoShape_YellowSphere].sphere.radius = 1.0f;

  s_Shapes[mj::rt::DemoShape_BlueSphere].type          = mj::rt::Shape::Shape_Sphere;
  s_Shapes[mj::rt::DemoShape_BlueSphere].color         = glm::vec3(0.0f, 0.0f, 1.0f);
  s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.origin = glm::vec3(0.0f, 2.0f, 10.0f);
  s_Shapes[mj::rt::DemoShape_BlueSphere].sphere.radius = 1.0f;

  s_Shapes[mj::rt::DemoShape_GreenAABB].type     = mj::rt::Shape::Shape_AABB;
  s_Shapes[mj::rt::DemoShape_GreenAABB].color    = glm::vec3(0.0f, 1.0f, 0.0f);
  s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.min = glm::vec3(-2.5f, -0.5f, 9.5f);
  s_Shapes[mj::rt::DemoShape_GreenAABB].aabb.max = glm::vec3(-1.5f, 0.5f, 10.5f);

  s_Shapes[mj::rt::DemoShape_WhitePlane].type           = mj::rt::Shape::Shape_Plane;
  s_Shapes[mj::rt::DemoShape_WhitePlane].color          = glm::vec3(1.0f, 1.0f, 1.0f);
  s_Shapes[mj::rt::DemoShape_WhitePlane].plane.normal   = glm::vec3(0.0f, -1.0f, 0.0f);
  s_Shapes[mj::rt::DemoShape_WhitePlane].plane.distance = 5.0f;

  s_Shapes[mj::rt::DemoShape_CyanPlane].type           = mj::rt::Shape::Shape_Plane;
  s_Shapes[mj::rt::DemoShape_CyanPlane].color          = glm::vec3(0.0f, 1.0f, 1.0f);
  s_Shapes[mj::rt::DemoShape_CyanPlane].plane.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
  s_Shapes[mj::rt::DemoShape_CyanPlane].plane.distance = 3.0f;

  s_Constant.width  = MJ_RT_WIDTH;
  s_Constant.height = MJ_RT_HEIGHT;

  Reset();

  // Set constant buffer with camera
  D3D11_BUFFER_DESC bufferDesc = {};
  bufferDesc.ByteWidth         = sizeof(s_Constant);
  bufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
  bufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
  bufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

  D3D11_SUBRESOURCE_DATA InitData = {};
  InitData.pSysMem                = &s_Constant;
  InitData.SysMemPitch            = 0;
  InitData.SysMemSlicePitch       = 0;

  WIN32_ASSERT(pDevice->CreateBuffer(&bufferDesc, &InitData, &s_pConstantBuffer));
  SetDebugName(s_pConstantBuffer, "mj_constant_buffer");

  // Set constant buffer with shapes
  D3D11_BUFFER_DESC shapeBufferDesc   = {};
  shapeBufferDesc.ByteWidth           = sizeof(s_Shapes);
  shapeBufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
  shapeBufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
  shapeBufferDesc.CPUAccessFlags      = 0;
  shapeBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  shapeBufferDesc.StructureByteStride = sizeof(s_Shapes[0]);

  D3D11_SUBRESOURCE_DATA ShapeData = {};
  ShapeData.pSysMem                = &s_Shapes;
  ShapeData.SysMemPitch            = 0;
  ShapeData.SysMemSlicePitch       = 0;

  WIN32_ASSERT(pDevice->CreateBuffer(&shapeBufferDesc, &ShapeData, &s_pShapeBuffer));
  SetDebugName(s_pShapeBuffer, "mj_shape_buffer");

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
  srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
  srvDesc.Buffer.FirstElement             = 0;
  srvDesc.Buffer.NumElements              = MJ_COUNTOF(s_Shapes);
  WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pShapeBuffer, &srvDesc, &s_pShaderResourceView));
  SetDebugName(s_pShaderResourceView, "mj_shader_resource_view");

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
  pDeviceContext->CSSetShaderResources(0, 1, &s_pShaderResourceView);

  // Run compute shader
  pDeviceContext->Dispatch(16, 16, 1);

  // Unbind
  ID3D11ShaderResourceView* ppSrvNull[1] = { nullptr };
  pDeviceContext->CSSetShaderResources(0, 1, ppSrvNull);
  ID3D11Buffer* ppCbNull[1] = { nullptr };
  pDeviceContext->CSSetConstantBuffers(0, 1, ppCbNull);
  ID3D11UnorderedAccessView* ppUavNull[1] = { nullptr };
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUavNull, nullptr);
  pDeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void mj::hlsl::Destroy()
{
  SAFE_RELEASE(s_pComputeShader);
  SAFE_RELEASE(s_pUnorderedAccessView);
  SAFE_RELEASE(s_pConstantBuffer);
  SAFE_RELEASE(s_pShaderResourceView);
  SAFE_RELEASE(s_pShapeBuffer);
}
