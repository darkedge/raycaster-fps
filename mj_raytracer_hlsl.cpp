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

static ID3D11ComputeShader* s_pComputeShader;
static ID3D11UnorderedAccessView* s_pUnorderedAccessView;

// Constant buffer
static ID3D11Buffer* s_pConstantBuffer;
static mj::hlsl::Constant s_Constant;

// Grid
static ID3D11ShaderResourceView* s_pGridSrv;
static ID3D11Buffer* s_pGridBuffer;
static uint32_t s_Grid[64 * 64];

// Texture2DArray
static ID3D11Texture2D* s_pTexture;
static ID3D11SamplerState* s_pSamplerState;
static ID3D11ShaderResourceView* s_pShaderResourceView;

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
  s_Constant.fovDeg            = 45.0f;
  CameraInit(MJ_REF s_Constant.s_Camera);
}

static bool InitObjectPlaceholder()
{
  bool voxelData   = false;
  bool paletteData = false;

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
            if (stream.Fetch(MJ_REF pId)
                    .Fetch(MJ_REF pNumBytesChunkContent)
                    .Fetch(MJ_REF pNumBytesChildrenChunks)
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
                if (stream.Fetch(MJ_REF pSizeX).Fetch(MJ_REF pSizeY).Fetch(MJ_REF pSizeZ).Good())
                {
                }
              }
              break;
              case ID_XYZI:
              {
                MJ_UNINITIALIZED uint32_t* pNumVoxels;
                if (stream.Fetch(MJ_REF pNumVoxels).Good())
                {
                  struct Voxel
                  {
                    uint8_t x;
                    uint8_t y;
                    uint8_t z;
                    uint8_t colorIndex;
                  };
                  auto pVoxels = (Voxel*)stream.Position();
                  stream.Skip(*pNumVoxels * 4);
                  voxelData = true;
                }
              }
              break;
              case ID_RGBA:
              {
                struct RGBA
                {
                  uint8_t R;
                  uint8_t G;
                  uint8_t B;
                  uint8_t A;
                };

                auto pPalette = (RGBA*)stream.Position();
                stream.Skip(256 * sizeof(RGBA));
                paletteData = true;
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

  return voxelData && paletteData;
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
            WIN32_ASSERT(pDevice->CreateTexture2D(&desc, pSubResourceData, &s_pTexture));
            SetDebugName(s_pTexture, "s_pTexture");

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
              assert(!s_pSamplerState);
              WIN32_ASSERT(pDevice->CreateSamplerState(&samplerDesc, &s_pSamplerState));
              SetDebugName(s_pSamplerState, "s_pSamplerState");
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
              assert(!s_pShaderResourceView);
              assert(s_pTexture);
              WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pTexture, &srvDesc, &s_pShaderResourceView));
              SetDebugName(s_pShaderResourceView, "s_pShaderResourceView");
            }

            success = true;
          }
        }
      }
    }
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

      WIN32_ASSERT(pDevice->CreateBuffer(&gridBufferDesc, &GridData, &s_pGridBuffer));
      SetDebugName(s_pGridBuffer, "s_pGridBuffer");
    }
  }

  {
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.FirstElement             = 0;
    srvDesc.Buffer.NumElements              = MJ_COUNTOF(s_Grid);
    WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pGridBuffer, &srvDesc, &s_pGridSrv));
    SetDebugName(s_pGridSrv, "s_pGridSrv");
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

bool mj::hlsl::Init(ID3D11Device* pDevice, ID3D11Texture2D* pTexture)
{
  assert(!s_pComputeShader);
  WIN32_ASSERT(pDevice->CreateComputeShader(CSRaytracer, sizeof(CSRaytracer), nullptr, &s_pComputeShader));
  SetDebugName(s_pComputeShader, "mj_compute_shader");

  // Texture2D UAV
  D3D11_UNORDERED_ACCESS_VIEW_DESC descView = {};
  descView.ViewDimension                    = D3D11_UAV_DIMENSION_TEXTURE2D;
  descView.Format                           = DXGI_FORMAT_R32G32B32A32_FLOAT;
  descView.Texture2D.MipSlice               = 0;
  WIN32_ASSERT(pDevice->CreateUnorderedAccessView(pTexture, &descView, &s_pUnorderedAccessView));
  SetDebugName(s_pUnorderedAccessView, "mj_unordered_access_view");

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

  if (!LoadLevel(pDevice))
  {
    return false;
  }

  if (!InitTexture2DArray(pDevice))
  {
    return false;
  }

  if (!InitObjectPlaceholder())
  {
    return false;
  }

  return true;
}

void mj::hlsl::Update(ID3D11DeviceContext* pDeviceContext)
{
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
    ImGui::Begin("Hello, world!");
    ImGui::Text("R to reset, F3 toggles mouselook");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Field of view", &s_Constant.fovDeg, 5.0f, 170.0f);
    ImGui::End();
  }

  MJ_UNINITIALIZED D3D11_MAPPED_SUBRESOURCE mappedSubresource;
  WIN32_ASSERT(pDeviceContext->Map(s_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));
  memcpy(mappedSubresource.pData, &s_Constant, sizeof(s_Constant));
  pDeviceContext->Unmap(s_pConstantBuffer, 0);

  // Bind
  assert(s_pComputeShader);
  pDeviceContext->CSSetShader(s_pComputeShader, nullptr, 0);
  assert(s_pUnorderedAccessView);
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, &s_pUnorderedAccessView, nullptr);
  assert(s_pConstantBuffer);
  pDeviceContext->CSSetConstantBuffers(0, 1, &s_pConstantBuffer);
  assert(s_pSamplerState);
  pDeviceContext->CSSetSamplers(0, 1, &s_pSamplerState);
  ID3D11ShaderResourceView* ppSrv[2] = { s_pGridSrv, s_pShaderResourceView };
  pDeviceContext->CSSetShaderResources(0, MJ_COUNTOF(ppSrv), ppSrv);

  // Run compute shader
  const UINT GRID_DIM = 16;
  pDeviceContext->Dispatch((MJ_RT_WIDTH + GRID_DIM - 1) / GRID_DIM, (MJ_RT_HEIGHT + GRID_DIM - 1) / GRID_DIM, 1);

  // Unbind
  ID3D11ShaderResourceView* ppSrvNull[2] = {};
  pDeviceContext->CSSetShaderResources(0, MJ_COUNTOF(ppSrvNull), ppSrvNull);
  ID3D11SamplerState* ppSsNull[1] = {};
  pDeviceContext->CSSetSamplers(0, MJ_COUNTOF(ppSsNull), ppSsNull);
  ID3D11Buffer* ppCbNull[1] = { nullptr };
  pDeviceContext->CSSetConstantBuffers(0, 1, ppCbNull);
  ID3D11UnorderedAccessView* ppUavNull[1] = {};
  pDeviceContext->CSSetUnorderedAccessViews(0, 1, ppUavNull, nullptr);
  pDeviceContext->CSSetShader(nullptr, nullptr, 0);
}

void mj::hlsl::Destroy()
{
  SAFE_RELEASE(s_pComputeShader);
  SAFE_RELEASE(s_pUnorderedAccessView);
  SAFE_RELEASE(s_pConstantBuffer);
  SAFE_RELEASE(s_pGridSrv);
  SAFE_RELEASE(s_pGridBuffer);
  SAFE_RELEASE(s_pTexture);
  SAFE_RELEASE(s_pSamplerState);
  SAFE_RELEASE(s_pShaderResourceView);
}
