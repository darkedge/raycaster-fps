#include "stdafx.h"
#include "rasterizer.h"
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <vector>
#include "mj_common.h"
#include "map.h"
#include <SDL.h>
#include <bimg/bimg.h>
#include <bx/allocator.h>
#include <bx/error.h>
#include <d3d11.h>

// Texture2DArray
static ID3D11Texture2D* s_pTexture;
static ID3D11SamplerState* s_pTextureSamplerState;
static ID3D11ShaderResourceView* s_pTextureSrv;

#if 0
// bgfx shaderc outputs
#include "shaders/rasterizer/vs_rasterizer.h"
#include "shaders/rasterizer/fs_rasterizer.h"

struct Vertex
{
  glm::vec3 position;
  // glm::vec3 normal;
  glm::vec3 texCoord;
  static void Init()
  {
    s_VertexLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        //.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 3, bgfx::AttribType::Float)
        .end();
  }
  static bgfx::VertexLayout s_VertexLayout;
};
bgfx::VertexLayout Vertex::s_VertexLayout;

static bgfx::VertexLayout s_ScreenTriangleVertexLayout;
static bgfx::UniformHandle s_ScreenTriangleSampler;

static bgfx::ProgramHandle s_RasterizerProgram;
static bgfx::VertexBufferHandle s_VertexBufferHandle;
static bgfx::IndexBufferHandle s_IndexBufferHandle;
static bgfx::TextureHandle s_RasterizerTextureArray;

static bx::DefaultAllocator s_defaultAllocator;

static bgfx::UniformHandle s_uTextureArray;

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

static void CreateMesh(map::map_t map)
{
  int32_t xz[] = { 0, 0 }; // xz yzx zxy

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

    int32_t arr_xz[] = { 0, 0, 1, 1 };
    int32_t neighbor = arr_xz[i] * 2 - 1; // -1 -1 +1 +1
    int32_t cur_z    = (i + 3) & 3;       // 1, 0, 0, 1
    int32_t next_x   = (i + 1) & 3;       // 0, 1, 1, 0

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
              glm::vec3 v = { xz[0], 0.0f, xz[1] };
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
  for (size_t z = 0; z < game::LEVEL_DIM; z++)
  {
    // Check for blocks in this slice
    for (size_t x = 0; x < game::LEVEL_DIM; x++)
    {
      if (map.pBlocks[z * map.width + x] >= 0x006A)
      {
        InsertFloor(vertices, indices, (float)x, (float)z);
        InsertCeiling(vertices, indices, (float)x, (float)z);
      }
    }
  }

  {
    // Create vertex buffer.
    const bgfx::Memory* mem = bgfx::copy(vertices.data(), uint32_t(vertices.size() * sizeof(vertices[0])));
    if (mem)
    {
      s_VertexBufferHandle = bgfx::createVertexBuffer(mem, Vertex::s_VertexLayout);
      bgfx::setName(s_VertexBufferHandle, "Rasterizer Vertex Buffer");
    }
  }
  {
    // Create index buffer.
    const bgfx::Memory* mem = bgfx::copy(indices.data(), uint32_t(indices.size() * sizeof(indices[0])));
    if (mem)
    {
      s_IndexBufferHandle = bgfx::createIndexBuffer(mem);
      bgfx::setName(s_IndexBufferHandle, "Rasterizer Index Buffer");
    }
  }
}

static void InitTexture2DArray()
{
  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("texture_array.dds", &datasize);
  if (pFile)
  {
    bx::Error error;
    bimg::ImageContainer* pImageContainer = bimg::imageParseDds(&s_defaultAllocator, pFile, (uint32_t)datasize, &error);

    if (pImageContainer)
    {
      const bgfx::Memory* pMemory = bgfx::copy(pImageContainer->m_data, pImageContainer->m_size);
      s_RasterizerTextureArray =
          bgfx::createTexture2D((uint16_t)pImageContainer->m_width, (uint16_t)pImageContainer->m_height, false,
                                pImageContainer->m_numLayers, (bgfx::TextureFormat::Enum)pImageContainer->m_format,
                                BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIN_POINT, pMemory);
      bgfx::setName(s_RasterizerTextureArray, "s_RasterizerTextureArray");
      bimg::imageFree(pImageContainer);
      s_uTextureArray = bgfx::createUniform("s_TextureArray", bgfx::UniformType::Sampler);
    }

    SDL_free(pFile);
  }
}

static void LoadLevel()
{
  auto map = map::Load("e1m1.mjm");
  if (map::Valid(map))
  {
    CreateMesh(map);
    map::Free(map);
  }
}
#endif

void rs::Init(ID3D11Device* pDevice)
{
#if 0
  // Rasterizer shader
  auto vsh = bgfx::createShader(bgfx::makeRef(vs_rasterizer, sizeof(vs_rasterizer)));
  bgfx::setName(vsh, "Rasterizer Vertex Shader");
  auto fsh = bgfx::createShader(bgfx::makeRef(fs_rasterizer, sizeof(fs_rasterizer)));
  bgfx::setName(fsh, "Rasterizer Fragment Shader");
  s_RasterizerProgram = bgfx::createProgram(vsh, fsh, true);

  Vertex::Init();
  LoadLevel();

  s_ScreenTriangleSampler = bgfx::createUniform("SampleType", bgfx::UniformType::Sampler);
  s_ScreenTriangleVertexLayout.begin().add(bgfx::Attrib::Indices, 1, bgfx::AttribType::Float).end();

  InitTexture2DArray();
#endif
}

void rs::Resize(int width, int height)
{
  MJ_DISCARD(width);
  MJ_DISCARD(height);
}

void rs::Update(ID3D11DeviceContext* pDeviceContext, int width, int height, game::Data* pData)
{
}

#if 0
void rs::Update(bgfx::ViewId viewId, int width, int height, game::Data* pData)
{
  glm::mat4 translate = glm::identity<glm::mat4>();
  translate           = glm::translate(translate, -glm::vec3(pData->s_Camera.position));
  glm::mat4 rotate    = glm::transpose(glm::eulerAngleY(pData->s_Camera.yaw));

  glm::mat4 view       = rotate * translate;
  glm::mat4 projection = glm::perspective(glm::radians(pData->s_FieldOfView.x), (float)width / height, 0.01f, 100.0f);

  bgfx::setViewRect(viewId, 0, 0, bgfx::BackbufferRatio::Equal);
  bgfx::setViewTransform(viewId, &view, &projection);

  bgfx::setTexture(0, s_uTextureArray, s_RasterizerTextureArray);
  bgfx::setVertexBuffer(0, s_VertexBufferHandle);
  bgfx::setIndexBuffer(s_IndexBufferHandle);
  bgfx::setState(BGFX_STATE_CULL_CW | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                 BGFX_STATE_DEPTH_TEST_LESS);

  bgfx::submit(viewId, s_RasterizerProgram);
}
#endif

void rs::Destroy()
{
#if 0
  bgfx::destroy(s_RasterizerProgram);
  bgfx::destroy(s_RasterizerTextureArray);
  bgfx::destroy(s_VertexBufferHandle);
  bgfx::destroy(s_IndexBufferHandle);
  bgfx::destroy(s_uTextureArray);
  bgfx::destroy(s_ScreenTriangleSampler);
#endif
}
