#include "raytracer.h"
#include "mj_platform.h"
#include <string_view>
#include <stdint.h>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include "mj_common.h"
#include <imgui.h>
#include <SDL.h>

#include "../tracy/Tracy.hpp"

// bgfx shaderc outputs
#include "shaders/raytracer/cs_raytracer.h"

static constexpr uint32_t GRID_DIM = 16;

static bgfx::ProgramHandle s_RaytracerProgram;

// Grid
static uint32_t s_Grid[64 * 64];
static bgfx::DynamicVertexBufferHandle s_GridBuffer;

static bgfx::DynamicVertexBufferHandle s_ObjectBuffer;
static uint32_t s_Object[64 * 64 * 64];

static bgfx::DynamicVertexBufferHandle s_PaletteBuffer;
static uint32_t s_Palette[256];


static void ParseLevel(void* pFile, size_t datasize)
{
  uint16_t* pData    = (uint16_t*)pFile;
  size_t numElements = datasize / sizeof(*pData);
  for (size_t i = 0; i < numElements; i++)
  {
    uint16_t val = pData[i];
    if (val < 0x006A)
    {
      s_Grid[i] = 1;
    }
  }

  // Setup compute buffers
  bgfx::VertexLayout computeVertexLayout;
  computeVertexLayout.begin().add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float).end();

  s_GridBuffer = bgfx::createDynamicVertexBuffer(1 << 15, computeVertexLayout, BGFX_BUFFER_COMPUTE_READ);
  //bgfx::createTexture2D()
  s_ObjectBuffer = bgfx::createDynamicVertexBuffer(1 << 15, computeVertexLayout, BGFX_BUFFER_COMPUTE_READ);
  s_PaletteBuffer = bgfx::createDynamicVertexBuffer(1 << 15, computeVertexLayout, BGFX_BUFFER_COMPUTE_READ);

  bgfx::ShaderHandle csh = bgfx::createShader(bgfx::makeRef(cs_raytracer, sizeof(cs_raytracer)));
  s_RaytracerProgram = bgfx::createProgram(csh, true);

  bgfx::setBuffer(0, s_GridBuffer, bgfx::Access::Read);
  //bgfx::setTexture(1, s_TextureArrayBuffer, bgfx::Access::Read);
  bgfx::setBuffer(2, s_ObjectBuffer, bgfx::Access::Read);
  bgfx::setBuffer(3, s_PaletteBuffer, bgfx::Access::Read);
}

void rt::Init()
{
  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("E1M1.bin", &datasize);
  if (pFile)
  {
    ParseLevel(pFile, datasize);
    SDL_free(pFile);
  }
}

void rt::Resize(int width, int height)
{
  (void)width;
  (void)height;
}

void rt::Update()
{
  bgfx::dispatch(0, s_RaytracerProgram,(MJ_RT_WIDTH + GRID_DIM - 1) / GRID_DIM, (MJ_RT_HEIGHT + GRID_DIM - 1) / GRID_DIM, 1);
}

void rt::Destroy()
{
}
