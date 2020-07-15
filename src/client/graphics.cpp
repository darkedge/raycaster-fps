#include "stdafx.h"
#include "graphics.h"
#include "mj_common.h"
#include "map.h"
#include "camera.h"
#include "meta.h"
#include "main.h"

#include "generated/rasterizer_vs.h"
#include "generated/rasterizer_ps.h"

static void CreateDefaultTexture(ComPtr<ID3D11Device> pDevice)
{
#if 0
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = 2;
  desc.Height               = 2;
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count     = 1;
  desc.SampleDesc.Quality   = 0;
  desc.Usage                = D3D11_USAGE_IMMUTABLE;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags       = 0;
  desc.MiscFlags            = 0;

  uint32_t data[] = { 0xFF00FFFF, 0x000000FF, 0x000000FF, 0xFF00FFFF };

  D3D11_SUBRESOURCE_DATA srd = {};
  srd.pSysMem                = data;
  srd.SysMemPitch            = 8;
  srd.SysMemSlicePitch       = 0;

  MJ_UNINITIALIZED ID3D11Texture2D* pTexture2D;
  MJ_DISCARD(pDevice->CreateTexture2D(&desc, &srd, &pTexture2D));

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format                          = desc.Format;
  srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip       = 0;
  srvDesc.Texture2D.MipLevels             = 1;

  ID3D11ShaderResourceView* pSrv;
  MJ_DISCARD(pDevice->CreateShaderResourceView(pTexture2D, &srvDesc, &pSrv));
#endif
}

struct Vertex
{
  mjm::vec3 position;
  mjm::vec3 texCoord;
};

static void InsertCeiling(std::vector<Vertex>& vertices, std::vector<int16_t>& indices, float x, float z)
{
  int16_t oldVertexCount = (int16_t)vertices.size();
  MJ_UNINITIALIZED Vertex vertex;

  vertex.position.x = x;
  vertex.position.y = 1.0f;
  vertex.position.z = z;
  vertex.texCoord.x = 0.0f;
  vertex.texCoord.y = 0.0f;
  vertex.texCoord.z = 138.0f;
  vertices.push_back(vertex);
  vertex.position.x = x;
  vertex.position.z = z + 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);
  vertex.position.x = x + 1.0f;
  vertex.position.z = z;
  vertex.texCoord.x = 1.0f;
  vertex.texCoord.y = 0.0f;
  vertices.push_back(vertex);
  vertex.position.x = x + 1.0f;
  vertex.position.z = z + 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);

  indices.push_back(oldVertexCount + 0);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 3);
  indices.push_back(oldVertexCount + 3);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 0);
}

static void InsertFloor(std::vector<Vertex>& vertices, std::vector<int16_t>& indices, float x, float z)
{
  int16_t oldVertexCount = (int16_t)vertices.size();
  MJ_UNINITIALIZED Vertex vertex;

  vertex.position.x = x;
  vertex.position.y = 0.0f;
  vertex.position.z = z;
  vertex.texCoord.x = 0.0f;
  vertex.texCoord.y = 0.0f;
  vertex.texCoord.z = 136.0f;
  vertices.push_back(vertex);
  vertex.position.x = x;
  vertex.position.z = z + 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);
  vertex.position.x = x + 1.0f;
  vertex.position.z = z;
  vertex.texCoord.x = 1.0f;
  vertex.texCoord.y = 0.0f;
  vertices.push_back(vertex);
  vertex.position.x = x + 1.0f;
  vertex.position.z = z + 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);

  indices.push_back(oldVertexCount + 0);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 3);
}

static void InsertRectangle(std::vector<Vertex>& vertices, std::vector<int16_t>& indices, float x0, float z0, float x1,
                            float z1, uint16_t block)
{
  // Get vertex count before adding new ones
  int16_t oldVertexCount = (int16_t)vertices.size();
  // 1  3
  //  \ |
  // 0->2
  MJ_UNINITIALIZED Vertex vertex;
  vertex.position.x = x0;
  vertex.position.y = 0.0f;
  vertex.position.z = z0;
  vertex.texCoord.x = 0.0f;
  vertex.texCoord.y = 0.0f;
  vertex.texCoord.z = block;
  vertices.push_back(vertex);
  vertex.position.y = 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);
  vertex.position.x = x1;
  vertex.position.y = 0.0f;
  vertex.position.z = z1;
  vertex.texCoord.x = 1.0f;
  vertex.texCoord.y = 0.0f;
  vertices.push_back(vertex);
  vertex.position.y = 1.0f;
  vertex.texCoord.y = 1.0f;
  vertices.push_back(vertex);

  indices.push_back(oldVertexCount + 0);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 3);
}

void Graphics::CreateMesh(map::map_t map, ComPtr<ID3D11Device> pDevice)
{
  uint8_t xz[] = { 0, 0 }; // xz yzx zxy

  std::vector<Vertex> vertices;
  std::vector<int16_t> indices;

  // Traversal direction
  // This is also the normal of the triangles
  // -X, -Z, +X, +Z
  for (int32_t i = 0; i < 4; i++)
  {
    int32_t mapDim[]      = { map.width, map.height };
    int32_t primaryAxis   = i & 1;           //  x  z  x  z
    int32_t secondaryAxis = primaryAxis ^ 1; //  z  x  z  x

    int8_t arr_xz[] = { 0, 0, 1, 1 };
    int8_t neighbor = arr_xz[i] * 2 - 1; // -1 -1 +1 +1
    int32_t cur_z   = (i + 3) & 3;       // 1, 0, 0, 1
    int32_t next_x  = (i + 1) & 3;       // 0, 1, 1, 0

    // Traverse the level slice by slice from a single direction
    for (xz[primaryAxis] = 0; xz[primaryAxis] < mapDim[primaryAxis]; xz[primaryAxis]++)
    {
      // Check for blocks in this slice
      for (xz[secondaryAxis] = 0; xz[secondaryAxis] < mapDim[secondaryAxis]; xz[secondaryAxis]++)
      {
        uint16_t block = map.pBlocks[xz[1] * map.width + xz[0]];

        if (block < 0x006A) // This block is solid
        {
          // Check the opposite block that is connected to this face
          xz[primaryAxis] += neighbor;

          if ((xz[1] < map.height) && (xz[0] < map.width)) // Bounds check
          {
            if (map.pBlocks[xz[1] * map.width + xz[0]] >= 0x006A) // Is it empty?
            {
              xz[primaryAxis] -= neighbor;
              mjm::vec3 v((float)xz[0], 0.0f, (float)xz[1]);
              InsertRectangle(vertices, indices, (float)xz[0] + arr_xz[i], (float)xz[1] + arr_xz[cur_z],
                              (float)xz[0] + arr_xz[next_x], (float)xz[1] + arr_xz[i], 2 * block - 1);
              xz[primaryAxis] += neighbor;
            }
          }

          xz[primaryAxis] -= neighbor;
        }
      }
    }
  }

  // Floor/ceiling pass
  for (size_t z = 0; z < Meta::LEVEL_DIM; z++)
  {
    // Check for blocks in this slice
    for (size_t x = 0; x < Meta::LEVEL_DIM; x++)
    {
      if (map.pBlocks[z * map.width + x] >= 0x006A)
      {
        InsertFloor(vertices, indices, (float)x, (float)z);
        InsertCeiling(vertices, indices, (float)x, (float)z);
      }
    }
  }

  {
    // Fill in a buffer description.
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage          = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth      = uint32_t(vertices.size() * sizeof(vertices[0]));
    bufferDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags      = 0;

    // Fill in the subresource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = vertices.data();
    InitData.SysMemPitch      = sizeof(vertices[0]);
    InitData.SysMemSlicePitch = 0;

    // Create the vertex buffer.
    MJ_DISCARD(pDevice->CreateBuffer(&bufferDesc, &InitData, this->pVertexBuffer.ReleaseAndGetAddressOf()));
  }
  {
    this->Indices = (UINT)indices.size();
    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.Usage          = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth      = uint32_t(indices.size() * sizeof(indices[0]));
    bufferDesc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags      = 0;

    // Define the resource data.
    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = indices.data();
    InitData.SysMemPitch      = sizeof(indices[0]);
    InitData.SysMemSlicePitch = 0;

    // Create the buffer with the device.
    MJ_DISCARD(pDevice->CreateBuffer(&bufferDesc, &InitData, this->pIndexBuffer.ReleaseAndGetAddressOf()));
  }
}

void Graphics::InitTexture2DArray(ComPtr<ID3D11Device> pDevice)
{
  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("texture_array.dds", &datasize);
  if (pFile)
  {
    bx::Error error;
    bimg::ImageContainer* pImageContainer =
        bimg::imageParseDds(&this->defaultAllocator, pFile, (uint32_t)datasize, &error);

    if (pImageContainer)
    {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width                = pImageContainer->m_width;
      desc.Height               = pImageContainer->m_height;
      desc.MipLevels            = pImageContainer->m_numMips;
      desc.ArraySize            = pImageContainer->m_numLayers;
      desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count     = 1;
      desc.SampleDesc.Quality   = 0;
      desc.Usage                = D3D11_USAGE_IMMUTABLE;
      desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags       = 0;
      desc.MiscFlags            = 0;

      pImageContainer->m_offset = UINT32_MAX;
      D3D11_SUBRESOURCE_DATA* srd =
          (D3D11_SUBRESOURCE_DATA*)alloca(pImageContainer->m_numLayers * sizeof(D3D11_SUBRESOURCE_DATA));
      MJ_UNINITIALIZED bimg::ImageMip mip;
      for (uint16_t i = 0; i < pImageContainer->m_numLayers; i++)
      {
        constexpr uint8_t lod = 0;
        if (bimg::imageGetRawData(*pImageContainer, i, lod, nullptr, 0, MJ_REF mip))
        {
          srd[i].pSysMem          = mip.m_data;
          srd[i].SysMemPitch      = mip.m_width * mip.m_bpp / 8;
          srd[i].SysMemSlicePitch = 0;
        }
      }

      MJ_DISCARD(pDevice->CreateTexture2D(&desc, srd, this->pTextureArray.ReleaseAndGetAddressOf()));

      D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Format                          = desc.Format;
      srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
      srvDesc.Texture2DArray.MostDetailedMip  = 0;
      srvDesc.Texture2DArray.MipLevels        = 1;
      srvDesc.Texture2DArray.FirstArraySlice  = 0;
      srvDesc.Texture2DArray.ArraySize        = pImageContainer->m_numLayers;

      MJ_DISCARD(pDevice->CreateShaderResourceView(this->pTextureArray.Get(), &srvDesc, this->pShaderResourceView.ReleaseAndGetAddressOf()));

      D3D11_SAMPLER_DESC samplerDesc = {};
      samplerDesc.Filter             = D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
      samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
      samplerDesc.MipLODBias         = 0.0f;
      samplerDesc.MaxAnisotropy      = 16;
      samplerDesc.ComparisonFunc     = D3D11_COMPARISON_LESS_EQUAL;
      samplerDesc.BorderColor[0]     = 0.0f;
      samplerDesc.BorderColor[1]     = 0.0f;
      samplerDesc.BorderColor[2]     = 0.0f;
      samplerDesc.BorderColor[3]     = 0.0f;
      samplerDesc.MinLOD             = 0.0f;
      samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;
      pDevice->CreateSamplerState(&samplerDesc, this->pTextureSamplerState.ReleaseAndGetAddressOf());

      bimg::imageFree(pImageContainer);
    }

    SDL_free(pFile);
  }
}

void Graphics::Init(ComPtr<ID3D11Device> pDevice)
{
  MJ_DISCARD(pDevice->CreateVertexShader(rasterizer_vs, sizeof(rasterizer_vs), nullptr, this->pVertexShader.ReleaseAndGetAddressOf()));
  // SetDebugName(this->pVertexShader, "this->pVertexShader");
  MJ_DISCARD(pDevice->CreatePixelShader(rasterizer_ps, sizeof(rasterizer_ps), nullptr, this->pPixelShader.ReleaseAndGetAddressOf()));
  // SetDebugName(this->pPixelShader, "this->pPixelShader");

  {
    D3D11_INPUT_ELEMENT_DESC desc[2] = {};

    // float3 a_position : POSITION
    desc[0].SemanticName         = "POSITION";
    desc[0].SemanticIndex        = 0;
    desc[0].Format               = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[0].InputSlot            = 0;
    desc[0].AlignedByteOffset    = 0;
    desc[0].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
    desc[0].InstanceDataStepRate = 0;

    // float3 a_texcoord0 : TEXCOORD0
    desc[1].SemanticName         = "TEXCOORD";
    desc[1].SemanticIndex        = 0;
    desc[1].Format               = DXGI_FORMAT_R32G32B32_FLOAT;
    desc[1].InputSlot            = 0;
    desc[1].AlignedByteOffset    = offsetof(Vertex, texCoord);
    desc[1].InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA;
    desc[1].InstanceDataStepRate = 0;

    MJ_DISCARD(pDevice->CreateInputLayout(desc, 2, rasterizer_vs, sizeof(rasterizer_vs), this->pInputLayout.ReleaseAndGetAddressOf()));
  }

  {
    D3D11_BUFFER_DESC desc   = {};
    desc.ByteWidth           = sizeof(mjm::mat4);
    desc.Usage               = D3D11_USAGE_DEFAULT;
    desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags      = 0;
    desc.MiscFlags           = 0;
    desc.StructureByteStride = 0;
    MJ_DISCARD(pDevice->CreateBuffer(&desc, nullptr, this->pResource.ReleaseAndGetAddressOf()));
  }

  CreateDefaultTexture(pDevice);
  InitTexture2DArray(pDevice);

  {
    D3D11_BLEND_DESC bs = {};
    for (size_t i = 0; i < 8; i++)
    {
      bs.RenderTarget[i].BlendEnable           = TRUE;
      bs.RenderTarget[i].BlendOp               = D3D11_BLEND_OP_ADD;
      bs.RenderTarget[i].BlendOpAlpha          = D3D11_BLEND_OP_MAX;
      bs.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      bs.RenderTarget[i].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
      bs.RenderTarget[i].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
      bs.RenderTarget[i].SrcBlendAlpha         = D3D11_BLEND_ONE;
      bs.RenderTarget[i].DestBlendAlpha        = D3D11_BLEND_ONE;
    }

    MJ_DISCARD(pDevice->CreateBlendState(&bs, this->pBlendState.ReleaseAndGetAddressOf()));
  }

  {
    // Rasterizer State
    D3D11_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode              = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = TRUE;
    rasterizerDesc.DepthBias             = 0;
    rasterizerDesc.DepthBiasClamp        = 0.0f;
    rasterizerDesc.SlopeScaledDepthBias  = 0.0f;
    rasterizerDesc.DepthClipEnable       = TRUE;
    rasterizerDesc.ScissorEnable         = FALSE;
    rasterizerDesc.MultisampleEnable     = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    pDevice->CreateRasterizerState(&rasterizerDesc, this->pRasterizerState.ReleaseAndGetAddressOf());

    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    pDevice->CreateRasterizerState(&rasterizerDesc, this->pRasterizerStateCullNone.ReleaseAndGetAddressOf());
  }
}

void Graphics::Resize(int width, int height)
{
  MJ_DISCARD(width);
  MJ_DISCARD(height);
}

/// <summary>
/// Note: Eventually, we will have to renderer a list of draw commands (command buffer).
/// Draw commands will contain matrices, textures, buffers, and all other bindings.
/// </summary>
/// <param name="pContext"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="pCamera"></param>
void Graphics::Update(ComPtr<ID3D11DeviceContext> pContext, const Camera* pCamera)
{
  mjm::mat4 vp = pCamera->projection * pCamera->view;
  pContext->UpdateSubresource(this->pResource.Get(), 0, 0, &vp, 0, 0);

  // Input Assembler
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pContext->IASetInputLayout(this->pInputLayout.Get());
  pContext->IASetIndexBuffer(this->pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0); // Index type
  UINT strides[] = { sizeof(Vertex) };
  UINT offsets[] = { 0 };
  pContext->IASetVertexBuffers(0, 1, this->pVertexBuffer.GetAddressOf(), strides, offsets);

  // Vertex Shader
  pContext->VSSetShader(this->pVertexShader.Get(), nullptr, 0);
  pContext->VSSetConstantBuffers(0, 1, this->pResource.GetAddressOf());

  // Rasterizer
  D3D11_VIEWPORT viewport = {};
  viewport.TopLeftX       = 0;
  viewport.TopLeftY       = 0;
  viewport.MinDepth       = 0.0f;
  viewport.MaxDepth       = 1.0f;
  mj::GetWindowSize(&viewport.Width, &viewport.Height);

  pContext->RSSetViewports(1, &viewport);
  pContext->RSSetState(this->pRasterizerState.Get());

  // Pixel Shader
  pContext->PSSetShaderResources(0, 1, this->pShaderResourceView.GetAddressOf());
  pContext->PSSetShader(this->pPixelShader.Get(), nullptr, 0);
  pContext->PSSetSamplers(0, 1, this->pTextureSamplerState.GetAddressOf());

  // Output Merger
  pContext->OMSetBlendState(this->pBlendState.Get(), nullptr, 0xFFFFFFFF);

  pContext->DrawIndexed(this->Indices, 0, 0);
}

void* Graphics::GetTileTexture(int x, int y)
{
  (void)x;
  (void)y;
  return this->pShaderResourceView.Get();
}
