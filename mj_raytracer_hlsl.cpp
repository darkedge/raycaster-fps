#include "mj_raytracer_hlsl.h"
#include "game.h"
#include "mj_input.h"

#include "glm/gtc/matrix_transform.hpp"
#include "mj_win32_utils.h"

#include <d3d11.h>
#include <assert.h>
#include "dds.h"

#include "intermediate/CSRaytracer.h"
#include "resource.h"
#include "imgui.h"

#include "tracy/Tracy.hpp"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static constexpr UINT GRID_DIM = 16;

static ComPtr<ID3D11ComputeShader> s_pComputeShader;
static ComPtr<ID3D11UnorderedAccessView> s_pUnorderedAccessView;

// Constant buffer
static ComPtr<ID3D11Buffer> s_pConstantBuffer;
static mj::hlsl::Constant s_Constant;

// Grid
static ComPtr<ID3D11ShaderResourceView> s_pGridSrv;
static ComPtr<ID3D11Buffer> s_pGridBuffer;
static uint32_t s_Grid[64 * 64];

// Object placeholder
static ComPtr<ID3D11ShaderResourceView> s_pObjectSrv;
static ComPtr<ID3D11Buffer> s_pObjectBuffer;
static uint32_t s_Object[64 * 64 * 64];

// Palette
static ComPtr<ID3D11ShaderResourceView> s_pPaletteSrv;
static ComPtr<ID3D11Buffer> s_pPaletteBuffer;
static uint32_t s_Palette[256];

// Texture2DArray
static ComPtr<ID3D11Texture2D> s_pTexture;
static ComPtr<ID3D11SamplerState> s_pTextureSamplerState;
static ComPtr<ID3D11ShaderResourceView> s_pTextureSrv;

static bool s_MouseLook = true;

#pragma comment(lib, "dxguid.lib")
void SetDebugName(ID3D11DeviceChild* child, const char* name)
{
  if (child && name)
  {
    child->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)strlen(name), name);
  }
}

static void Reset()
{
  s_Constant.s_Camera.position = glm::vec3(54.5f, 0.5f, 34.5f);
  s_Constant.s_Camera.rotation = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
  s_Constant.s_Camera.frame    = 0;
  s_Constant.fovDeg            = 60.0f;
  CameraInit(MJ_REF s_Constant.s_Camera);
}

static bool InitObjectPlaceholder(ID3D11Device* pDevice)
{
  bool voxelData   = false;
  bool paletteData = false;
  bool sizeData    = false;

  constexpr int32_t ID_VOX  = 542658390;
  constexpr int32_t ID_MAIN = 1313423693;
  constexpr int32_t ID_SIZE = 1163544915;
  constexpr int32_t ID_XYZI = 1230657880;
  constexpr int32_t ID_RGBA = 1094862674;

  HRSRC resourceHandle = FindResource(0, MAKEINTRESOURCE(MJ_RC_OBJ_PLACEHOLDER), RT_RCDATA);
  if (resourceHandle)
  {
    HGLOBAL dataHandle = LoadResource(nullptr, resourceHandle);
    if (dataHandle)
    {
      void* resourceData = LockResource(dataHandle);
      if (resourceData)
      {
        DWORD resourceSize = SizeofResource(nullptr, resourceHandle);
        mj::IStream stream(resourceData, resourceSize);

        MJ_UNINITIALIZED int32_t* pId;
        MJ_UNINITIALIZED uint32_t* versionNumber;

        // Check header
        if (stream.Fetch(MJ_REF pId).Fetch(MJ_REF versionNumber).Good() && (*pId == ID_VOX) && (*versionNumber == 150))
        {
          MJ_UNINITIALIZED uint32_t* pNumBytesChunkContent;
          MJ_UNINITIALIZED uint32_t* pNumBytesChildrenChunks;
          while (stream.SizeLeft() > 0)
          {
            if (stream
                    .Fetch(MJ_REF pId)                     //
                    .Fetch(MJ_REF pNumBytesChunkContent)   //
                    .Fetch(MJ_REF pNumBytesChildrenChunks) //
                    .Good())
            {
              switch (*pId)
              {
              case ID_MAIN: // Root node
                break;
              case ID_SIZE:
              {
                MJ_UNINITIALIZED uint32_t* pSizeX;
                MJ_UNINITIALIZED uint32_t* pSizeY;
                MJ_UNINITIALIZED uint32_t* pSizeZ;
                if (stream
                        .Fetch(MJ_REF pSizeX) //
                        .Fetch(MJ_REF pSizeY) //
                        .Fetch(MJ_REF pSizeZ) //
                        .Good())
                {
                  if ((*pSizeX == 64) && (*pSizeY == 64) && (*pSizeZ == 64))
                  {
                    sizeData = true;
                  }
                }
              }
              break;
              case ID_XYZI:
              {
                MJ_UNINITIALIZED uint32_t* pNumVoxels;
                if (stream
                        .Fetch(MJ_REF pNumVoxels) //
                        .Good())
                {
                  for (uint32_t i = 0; i < *pNumVoxels; i++)
                  {
                    uint8_t* pX;
                    uint8_t* pY;
                    uint8_t* pZ;
                    uint8_t* pColorIndex;
                    if (stream
                            .Fetch(MJ_REF pX)          //
                            .Fetch(MJ_REF pY)          //
                            .Fetch(MJ_REF pZ)          //
                            .Fetch(MJ_REF pColorIndex) //
                            .Good())
                    {
                      // Flip MagicaVoxel's right-handed Z-up to left-handed Y-up
                      s_Object[*pY * 64 * 64 + *pZ * 64 + *pX] = *pColorIndex;
                    }
                  }
                  {
                    D3D11_BUFFER_DESC bufferDesc   = {};
                    bufferDesc.ByteWidth           = sizeof(s_Object);
                    bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
                    bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
                    bufferDesc.CPUAccessFlags      = 0;
                    bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                    bufferDesc.StructureByteStride = sizeof(*s_Object);

                    {
                      D3D11_SUBRESOURCE_DATA subresourceData = {};
                      subresourceData.pSysMem                = s_Object;
                      subresourceData.SysMemPitch            = 0;
                      subresourceData.SysMemSlicePitch       = 0;

                      assert(!s_pObjectBuffer);
                      WIN32_ASSERT(pDevice->CreateBuffer(&bufferDesc, &subresourceData,
                                                         s_pObjectBuffer.ReleaseAndGetAddressOf()));
                      SetDebugName(s_pObjectBuffer.Get(), "s_pObjectBuffer");
                    }
                  }

                  {
                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
                    srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
                    srvDesc.Buffer.FirstElement             = 0;
                    srvDesc.Buffer.NumElements              = MJ_COUNTOF(s_Object);

                    assert(!s_pObjectSrv);
                    WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pObjectBuffer.Get(), &srvDesc,
                                                                   s_pObjectSrv.ReleaseAndGetAddressOf()));
                    SetDebugName(s_pObjectSrv.Get(), "s_pObjectSrv");
                  }
                  if (s_pObjectBuffer && s_pObjectSrv)
                  {
                    voxelData = true;
                  }
                  else
                  {
                    s_pObjectBuffer.Reset();
                    s_pObjectSrv.Reset();
                    ;
                  }
                }
              }
              break;
              case ID_RGBA:
              {
                {
                  D3D11_BUFFER_DESC bufferDesc   = {};
                  bufferDesc.ByteWidth           = *pNumBytesChunkContent;
                  bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
                  bufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
                  bufferDesc.CPUAccessFlags      = 0;
                  bufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                  bufferDesc.StructureByteStride = 4; // (R, G, B, A) : 1 byte for each component

                  {
                    D3D11_SUBRESOURCE_DATA subresourceData = {};
                    subresourceData.pSysMem                = stream.Position();
                    subresourceData.SysMemPitch            = 0;
                    subresourceData.SysMemSlicePitch       = 0;

                    assert(!s_pPaletteBuffer);
                    WIN32_ASSERT(pDevice->CreateBuffer(&bufferDesc, &subresourceData,
                                                       s_pPaletteBuffer.ReleaseAndGetAddressOf()));
                    SetDebugName(s_pPaletteBuffer.Get(), "s_pPaletteBuffer");
                  }
                }

                {
                  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                  srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
                  srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
                  srvDesc.Buffer.FirstElement             = 0;
                  srvDesc.Buffer.NumElements              = 256;

                  assert(!s_pPaletteSrv);
                  WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pPaletteBuffer.Get(), &srvDesc,
                                                                 s_pPaletteSrv.ReleaseAndGetAddressOf()));
                  SetDebugName(s_pPaletteSrv.Get(), "s_pPaletteSrv");
                }
                stream.Skip(*pNumBytesChunkContent);
                if (s_pPaletteBuffer && s_pPaletteSrv)
                {
                  paletteData = true;
                }
                else
                {
                  s_pPaletteBuffer.Reset();
                  s_pPaletteSrv.Reset();
                }
              }
              break;
              default: // Skip irrelevant nodes
                stream.Skip(*pNumBytesChunkContent);
                break;
              }
            }
          }
        }
      }
    }
  }

  return voxelData && paletteData && sizeData;
}

static bool InitTexture2DArray(ID3D11Device* pDevice)
{
  bool success = false;

  HRSRC resourceHandle = FindResource(0, MAKEINTRESOURCE(MJ_RC_TEXTURE_ARRAY), RT_RCDATA);
  if (resourceHandle)
  {
    HGLOBAL dataHandle = LoadResource(nullptr, resourceHandle);
    if (dataHandle)
    {
      void* resourceData = LockResource(dataHandle);
      if (resourceData)
      {
        DWORD resourceSize = SizeofResource(nullptr, resourceHandle);
        mj::IStream stream(resourceData, resourceSize);

        // DDS header
        MJ_UNINITIALIZED DWORD* dwMagic;
        MJ_UNINITIALIZED DirectX::DDS_HEADER* header;
        MJ_UNINITIALIZED DirectX::DDS_HEADER_DXT10* header10;

        stream.Fetch(MJ_REF dwMagic).Fetch(MJ_REF header).Fetch(MJ_REF header10);

        if (stream.Good())
        {
          D3D11_TEXTURE2D_DESC desc = {};
          desc.Width                = header->width;
          desc.Height               = header->height;
          desc.MipLevels            = header->mipMapCount;
          desc.ArraySize            = header10->arraySize;
          desc.Format               = header10->dxgiFormat;
          desc.SampleDesc.Count     = 1;
          desc.Usage                = D3D11_USAGE_IMMUTABLE;
          desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;

          UINT numDesc = desc.MipLevels * desc.ArraySize;
          D3D11_SUBRESOURCE_DATA* pSubResourceData =
              (D3D11_SUBRESOURCE_DATA*)alloca(numDesc * sizeof(D3D11_SUBRESOURCE_DATA));
          if (pSubResourceData)
          {
            char* pData = stream.Position();
            for (size_t i = 0; i < numDesc; i++)
            {
              pSubResourceData[i].pSysMem          = pData;
              pSubResourceData[i].SysMemPitch      = header->pitchOrLinearSize;
              pSubResourceData[i].SysMemSlicePitch = 0;
              pData += (size_t)header->height * header->pitchOrLinearSize;
            }

            assert(!s_pTexture);
            WIN32_ASSERT(pDevice->CreateTexture2D(&desc, pSubResourceData, s_pTexture.ReleaseAndGetAddressOf()));
            SetDebugName(s_pTexture.Get(), "s_pTexture");

            {
              // Sampler
              D3D11_SAMPLER_DESC samplerDesc = {};
              samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
              samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_CLAMP;
              samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_CLAMP;
              samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_CLAMP;
              samplerDesc.MipLODBias         = 0.0f;
              samplerDesc.MaxAnisotropy      = 1;
              samplerDesc.ComparisonFunc     = D3D11_COMPARISON_ALWAYS;
              samplerDesc.BorderColor[0]     = 0.0f;
              samplerDesc.BorderColor[1]     = 0.0f;
              samplerDesc.BorderColor[2]     = 0.0f;
              samplerDesc.BorderColor[3]     = 0.0f;
              samplerDesc.MinLOD             = 0.0f;
              samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

              // Create the texture sampler state.
              assert(!s_pTextureSamplerState);
              WIN32_ASSERT(pDevice->CreateSamplerState(&samplerDesc, s_pTextureSamplerState.ReleaseAndGetAddressOf()));
              SetDebugName(s_pTextureSamplerState.Get(), "s_pTextureSamplerState");
            }

            {
              D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
              srvDesc.Format                          = desc.Format;
              srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
              srvDesc.Texture2DArray.ArraySize        = header10->arraySize;
              srvDesc.Texture2DArray.FirstArraySlice  = 0;
              srvDesc.Texture2DArray.MipLevels        = 1;
              srvDesc.Texture2DArray.MostDetailedMip  = 0;

              // Create the shader resource view for the texture.
              assert(!s_pTextureSrv);
              assert(s_pTexture);
              WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pTexture.Get(), &srvDesc,
                                                             s_pTextureSrv.ReleaseAndGetAddressOf()));
              SetDebugName(s_pTextureSrv.Get(), "s_pTextureSrv");
            }

            success = true;
          }
        }
      }
    }
  }

  if (!success)
  {
    s_pTexture.Reset();
    s_pTextureSamplerState.Reset();
    s_pTextureSrv.Reset();
  }

  return success;
}

static bool ParseLevel(ID3D11Device* pDevice, uint16_t* arr, DWORD arraySize)
{
  for (size_t i = 0; i < arraySize; i++)
  {
    uint16_t val = arr[i];
    if (val < 0x006A)
    {
      s_Grid[i] = 1;
    }
  }

  {
    D3D11_BUFFER_DESC gridBufferDesc   = {};
    gridBufferDesc.ByteWidth           = sizeof(s_Grid);
    gridBufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    gridBufferDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    gridBufferDesc.CPUAccessFlags      = 0;
    gridBufferDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    gridBufferDesc.StructureByteStride = sizeof(s_Grid[0]);

    {
      D3D11_SUBRESOURCE_DATA GridData = {};
      GridData.pSysMem                = &s_Grid;
      GridData.SysMemPitch            = 0;
      GridData.SysMemSlicePitch       = 0;

      assert(!s_pGridBuffer);
      WIN32_ASSERT(pDevice->CreateBuffer(&gridBufferDesc, &GridData, s_pGridBuffer.ReleaseAndGetAddressOf()));
      SetDebugName(s_pGridBuffer.Get(), "s_pGridBuffer");
    }
  }

  {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement             = 0;
    srvDesc.Buffer.NumElements              = MJ_COUNTOF(s_Grid);

    assert(!s_pGridSrv);
    WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pGridBuffer.Get(), &srvDesc, s_pGridSrv.ReleaseAndGetAddressOf()));
    SetDebugName(s_pGridSrv.Get(), "s_pGridSrv");
  }

  if (!(s_pGridBuffer && s_pGridSrv))
  {
    s_pGridBuffer.Get();
    s_pGridSrv.Reset();
    return false;
  }

  return true;
}

static bool LoadLevel(ID3D11Device* pDevice)
{
  bool success = false;

  HRSRC resourceHandle = FindResource(0, MAKEINTRESOURCE(MJ_RC_RESOURCE), RT_RCDATA);
  if (resourceHandle)
  {
    HGLOBAL dataHandle = LoadResource(nullptr, resourceHandle);
    if (dataHandle)
    {
      uint16_t* resourceData = (uint16_t*)LockResource(dataHandle);
      DWORD resourceSize     = SizeofResource(nullptr, resourceHandle) / sizeof(*resourceData);

      if (ParseLevel(pDevice, resourceData, resourceSize))
      {
        success = true;
      }
    }
  }

  return success;
}

void mj::hlsl::SetTexture(ID3D11Device* pDevice, ID3D11Texture2D* pTexture)
{
  // Texture2D UAV
  D3D11_UNORDERED_ACCESS_VIEW_DESC descView = {};
  descView.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;
  descView.Format                           = DXGI_FORMAT_R32G32B32A32_FLOAT;
  descView.Texture2D.MipSlice               = 0;
  WIN32_ASSERT(
      pDevice->CreateUnorderedAccessView(pTexture, &descView, s_pUnorderedAccessView.ReleaseAndGetAddressOf()));
  SetDebugName(s_pUnorderedAccessView.Get(), "mj_unordered_access_view");
}

bool mj::hlsl::Init(ID3D11Device* pDevice)
{
  WIN32_ASSERT(pDevice->CreateComputeShader(CSRaytracer, sizeof(CSRaytracer), nullptr,
                                            s_pComputeShader.ReleaseAndGetAddressOf()));
  SetDebugName(s_pComputeShader.Get(), "mj_compute_shader");

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

  WIN32_ASSERT(pDevice->CreateBuffer(&bufferDesc, &InitData, s_pConstantBuffer.ReleaseAndGetAddressOf()));
  SetDebugName(s_pConstantBuffer.Get(), "mj_constant_buffer");

  if (!LoadLevel(pDevice))
  {
    return false;
  }

  if (!InitTexture2DArray(pDevice))
  {
    return false;
  }

  if (!InitObjectPlaceholder(pDevice))
  {
    return false;
  }

  return true;
}

void mj::hlsl::Update(ID3D11DeviceContext* pDeviceContext, uint16_t width, uint16_t height)
{
  ZoneScoped;
  if (mj::input::GetKeyDown(Key::F3))
  {
    s_MouseLook = !s_MouseLook;
  }
  if (s_MouseLook)
  {
    CameraMovement(MJ_REF s_Constant.s_Camera);
  }

  // Reset button
  if (mj::input::GetKeyDown(Key::KeyR))
  {
    Reset();
  }

  auto mat       = glm::identity<glm::mat4>();
  s_Constant.mat = glm::translate(mat, s_Constant.s_Camera.position) * glm::mat4_cast(s_Constant.s_Camera.rotation);
  s_Constant.s_Camera.frame++;

  {
    // Get thread group and index
    POINT point = {};
    MJ_DISCARD(GetCursorPos(&point));
    MJ_DISCARD(ScreenToClient(GetActiveWindow(), &point));
    int32_t groupX  = point.x * MJ_RT_WIDTH / width / GRID_DIM;
    int32_t groupY  = point.y * MJ_RT_HEIGHT / height / GRID_DIM;
    int32_t threadX = point.x * MJ_RT_WIDTH / width % GRID_DIM;
    int32_t threadY = point.y * MJ_RT_HEIGHT / height % GRID_DIM;

    ImGui::Begin("Hello, world!");
    ImGui::Text("R to reset, F3 toggles mouselook");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Field of view", &s_Constant.fovDeg, 5.0f, 170.0f);
    ImGui::Text("Group: [%d, %d, 0], Thread: [%d, %d, 0]", groupX, groupY, threadX, threadY);
    ImGui::End();
  }

  MJ_UNINITIALIZED D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  WIN32_ASSERT(pDeviceContext->Map(s_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));
  memcpy(mappedSubresource.pData, &s_Constant, sizeof(s_Constant));
  pDeviceContext->Unmap(s_pConstantBuffer.Get(), 0);

  // Bind
  assert(s_pComputeShader);
  pDeviceContext->CSSetShader(s_pComputeShader.Get(), nullptr, 0);
  assert(s_pUnorderedAccessView);
  ID3D11UnorderedAccessView* ppUav[] = { s_pUnorderedAccessView.Get() };
  pDeviceContext->CSSetUnorderedAccessViews(0, MJ_COUNTOF(ppUav), ppUav, nullptr);
  assert(s_pConstantBuffer);
  ID3D11Buffer* ppCb[] = { s_pConstantBuffer.Get() };
  pDeviceContext->CSSetConstantBuffers(0, MJ_COUNTOF(ppCb), ppCb);
  assert(s_pTextureSamplerState);
  ID3D11SamplerState* ppSs[] = { s_pTextureSamplerState.Get() };
  pDeviceContext->CSSetSamplers(0, MJ_COUNTOF(ppSs), ppSs);
  assert(s_pGridSrv);
  assert(s_pTextureSrv);
  assert(s_pObjectSrv);
  assert(s_pPaletteSrv);
  ID3D11ShaderResourceView* ppSrv[] = { s_pGridSrv.Get(), s_pTextureSrv.Get(), s_pObjectSrv.Get(),
                                        s_pPaletteSrv.Get() };
  pDeviceContext->CSSetShaderResources(0, MJ_COUNTOF(ppSrv), ppSrv);

  // Run compute shader
  if (mj::input::GetMouseButton(MouseButton::Left))
  {
    const UINT values[4] = { 0, 0, 0, 0 };
    pDeviceContext->ClearUnorderedAccessViewUint(s_pUnorderedAccessView.Get(), values);
  }
  else
  {
    pDeviceContext->Dispatch((MJ_RT_WIDTH + GRID_DIM - 1) / GRID_DIM, (MJ_RT_HEIGHT + GRID_DIM - 1) / GRID_DIM, 1);
  }

  // Unbind
  ID3D11ShaderResourceView* ppSrvNull[MJ_COUNTOF(ppSrv)] = {};
  pDeviceContext->CSSetShaderResources(0, MJ_COUNTOF(ppSrvNull), ppSrvNull);
  ID3D11SamplerState* ppSsNull[MJ_COUNTOF(ppSs)] = {};
  pDeviceContext->CSSetSamplers(0, MJ_COUNTOF(ppSsNull), ppSsNull);
  ID3D11Buffer* ppCbNull[MJ_COUNTOF(ppCb)] = {};
  pDeviceContext->CSSetConstantBuffers(0, 1, ppCbNull);
  ID3D11UnorderedAccessView* ppUavNull[MJ_COUNTOF(ppUav)] = {};
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUavNull, nullptr);
  pDeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void mj::hlsl::Destroy()
{
  s_pComputeShader.Reset();
  s_pUnorderedAccessView.Reset();
  s_pConstantBuffer.Reset();
  s_pGridSrv.Reset();
  s_pGridBuffer.Reset();
  s_pTexture.Reset();
  s_pTextureSamplerState.Reset();
  s_pTextureSrv.Reset();
  s_pObjectSrv.Reset();
  s_pObjectBuffer.Reset();
  s_pPaletteSrv.Reset();
  s_pPaletteBuffer.Reset();
}
