#include "rasterizer.h"
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <vector>
#include "mj_common.h"
#include <SDL.h>

// bgfx shaderc outputs
#include "shaders/screen_triangle/vs_screen_triangle.h"
#include "shaders/screen_triangle/fs_screen_triangle.h"

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

static bgfx::ProgramHandle s_ScreenTriangleProgram;
static bgfx::VertexBufferHandle s_VertexBufferHandle;
static bgfx::IndexBufferHandle s_IndexBufferHandle;

static std::vector<Vertex> s_Vertices; // No index buffer for now...

static void InsertRectangle(std::vector<Vertex>& vertices, std::vector<int16_t>& indices, float x0, float z0, float x1,
                            float z1, uint16_t block)
{
  // Get vertex count before adding new ones
  size_t oldVertexCount = vertices.size();
  // 2->3
  //  \
  // 0->1
  MJ_UNINITIALIZED Vertex vertex;
  vertex.position.x = x0;
  vertex.position.y = 0.0f;
  vertex.position.z = z0;
  vertex.texCoord.x = 0.0f;
  vertex.texCoord.y = 0.0f;
  vertex.texCoord.z = block;
  vertices.push_back(vertex);
  vertex.position.x = x1;
  vertex.position.y = 0.0f;
  vertex.position.z = z1;
  vertex.texCoord.x = 1.0f;
  vertex.texCoord.y = 0.0f;
  vertex.texCoord.z = block;
  vertices.push_back(vertex);
  vertex.position.x = x0;
  vertex.position.y = 1.0f;
  vertex.position.z = z0;
  vertex.texCoord.x = 0.0f;
  vertex.texCoord.y = 1.0f;
  vertex.texCoord.z = block;
  vertices.push_back(vertex);
  vertex.position.x = x1;
  vertex.position.y = 1.0f;
  vertex.position.z = z1;
  vertex.texCoord.x = 1.0f;
  vertex.texCoord.y = 1.0f;
  vertex.texCoord.z = block;
  vertices.push_back(vertex);

  indices.push_back(oldVertexCount + 0);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 2);
  indices.push_back(oldVertexCount + 1);
  indices.push_back(oldVertexCount + 3);
}

static void CreateMesh(uint16_t* pData, size_t numElements)
{
  int32_t xyz[] = { 0, 0 }; // xyz yzx zxy

  std::vector<Vertex> vertices;
  std::vector<int16_t> indices;

  // Traversal direction
  // This is also the normal of the triangles
  // -X, -Z, +X, +Z
  for (int32_t i = 0; i < 4; i++)
  {
    int32_t d0 = (i + 0) % 2; //  x  z  x  z
    int32_t d1 = (i + 1) % 2; //  z  x  z  x

    int32_t backface = i / 2 * 2 - 1; // -1 -1 -1 +1 +1 +1
    int32_t dxyz[]   = { 0, 1, 0, -1 };
    int32_t j        = (i + 1) % 4;

    // Traverse the level slice by slice from a single direction
    for (xyz[d0] = 0; xyz[d0] < game::LEVEL_DIM; xyz[d0]++)
    {
      // Check for blocks in this slice
      for (xyz[d1] = 0; xyz[d1] < game::LEVEL_DIM; xyz[d1]++)
      {
        uint16_t block = pData[xyz[0] * game::LEVEL_DIM + xyz[1]];

        if (block < 0x006A) // This block is solid
        {
          // Check the opposite block that is connected to this face
          xyz[d0] += backface;

          if ((xyz[0] < game::LEVEL_DIM) && (xyz[1] < game::LEVEL_DIM)) // Bounds check
          {
            if (pData[xyz[0] * game::LEVEL_DIM + xyz[1]] >= 0x006A) // Is it empty?
            {
              glm::vec3 v = { xyz[0], 0.0f, xyz[1] };
              InsertRectangle(vertices, indices, xyz[0], xyz[1], xyz[0] + dxyz[i], xyz[1] + dxyz[j], block);
            }
          }

          xyz[d0] -= backface;
        }
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

static void LoadLevel()
{
  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("E1M1.bin", &datasize);
  if (pFile)
  {
    uint16_t* pData    = (uint16_t*)pFile;
    size_t numElements = datasize / sizeof(*pData);

    CreateMesh(pData, numElements);

    SDL_free(pFile);
  }
}

void rs::Init()
{
  // Screen shader
  bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::makeRef(vs_screen_triangle, sizeof(vs_screen_triangle)));
  bgfx::setName(vsh, "Rasterizer Screen Triangle Vertex Shader");
  bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::makeRef(fs_screen_triangle, sizeof(fs_screen_triangle)));
  bgfx::setName(fsh, "Rasterizer Screen Triangle Fragment Shader");
  s_ScreenTriangleProgram = bgfx::createProgram(vsh, fsh, true);

  Vertex::Init();
  LoadLevel();
}

void rs::Resize(int width, int height)
{
}

void rs::Update(int width, int height, game::Data* pData)
{
  bgfx::setVertexBuffer(0, s_VertexBufferHandle);
  bgfx::setIndexBuffer(s_IndexBufferHandle);
  bgfx::submit(0, s_ScreenTriangleProgram);
}

void rs::Destroy()
{
  bgfx::destroy(s_ScreenTriangleProgram);
  bgfx::destroy(s_VertexBufferHandle);
  bgfx::destroy(s_IndexBufferHandle);
}
