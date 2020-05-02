#include "raytracer.h"
#include "mj_platform.h"
#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <bx/allocator.h>
#include <bx/error.h>
#include <glm/gtc/type_ptr.hpp>
#include "mj_common.h"
#include <imgui.h>
#include <SDL.h>
#include "game.h"

#include "../tracy/Tracy.hpp"

// bgfx shaderc outputs
#include "shaders/raytracer/cs_raytracer.h"
#include "shaders/screen_triangle/vs_screen_triangle.h"
#include "shaders/screen_triangle/fs_screen_triangle.h"

static constexpr uint32_t GRID_DIM = 16;

static bx::DefaultAllocator s_defaultAllocator;

static bgfx::ProgramHandle s_RaytracerProgram;
static bgfx::TextureHandle s_RaytracerTextureArray;
static bgfx::ProgramHandle s_ScreenTriangleProgram;
static bgfx::VertexLayout s_ScreenTriangleVertexLayout;
static bgfx::UniformHandle s_ScreenTriangleSampler;
static bgfx::TextureHandle s_RaytracerOutputTexture;

// Grid
static uint32_t s_Grid[64 * 64];
static bgfx::IndexBufferHandle s_GridBuffer;

static bgfx::IndexBufferHandle s_ObjectBuffer;
static uint32_t s_Object[64 * 64 * 64];

static bgfx::IndexBufferHandle s_PaletteBuffer;
static uint32_t s_Palette[256];

// Constants
static bgfx::UniformHandle s_uMat;
static bgfx::UniformHandle s_uCameraPos;
static bgfx::UniformHandle s_uFieldOfView;
static bgfx::UniformHandle s_uWidth;
static bgfx::UniformHandle s_uHeight;
static bgfx::UniformHandle s_uTextureArray;

static bool InitObjectPlaceholder()
{
  bool voxelData   = false;
  bool paletteData = false;
  bool sizeData    = false;

  constexpr int32_t ID_VOX  = 542658390;
  constexpr int32_t ID_MAIN = 1313423693;
  constexpr int32_t ID_SIZE = 1163544915;
  constexpr int32_t ID_XYZI = 1230657880;
  constexpr int32_t ID_RGBA = 1094862674;

  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("obj_placeholder_small.vox", &datasize);
  if (pFile)
  {
    mj::IStream stream(pFile, datasize);

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
              const bgfx::Memory* pMemory = bgfx::makeRef(s_Object, sizeof(s_Object));
              s_ObjectBuffer = bgfx::createIndexBuffer(pMemory, BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32);
              bgfx::setName(s_ObjectBuffer, "s_ObjectBuffer");
            }
          }
          break;
          case ID_RGBA:
          {
            const bgfx::Memory* pMemory = bgfx::copy(stream.Position(), *pNumBytesChunkContent);
            s_PaletteBuffer = bgfx::createIndexBuffer(pMemory, BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32);
            bgfx::setName(s_PaletteBuffer, "s_PaletteBuffer");
          }
          break;
          default: // Skip irrelevant nodes
            stream.Skip(*pNumBytesChunkContent);
            break;
          }
        }
      }
    }
    SDL_free(pFile);
  }

  return voxelData && paletteData && sizeData;
}

static void InitTexture2DArray()
{
  MJ_UNINITIALIZED size_t datasize;
  void* pFile = SDL_LoadFile("texture_array.dds", &datasize);
  if (pFile)
  {
    mj::IStream stream(pFile, datasize);

    bx::Error error;
    bimg::ImageContainer* pImageContainer = bimg::imageParseDds(&s_defaultAllocator, pFile, (uint32_t)datasize, &error);

    if (pImageContainer)
    {
      const bgfx::Memory* pMemory = bgfx::copy(pImageContainer->m_data, pImageContainer->m_size);
      s_RaytracerTextureArray =
          bgfx::createTexture2D((uint16_t)pImageContainer->m_width, (uint16_t)pImageContainer->m_height, false,
                                pImageContainer->m_numLayers, (bgfx::TextureFormat::Enum)pImageContainer->m_format,
                                BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIN_POINT, pMemory);
      bgfx::setName(s_RaytracerTextureArray, "s_RaytracerTextureArray");
      bimg::imageFree(pImageContainer);
      s_uTextureArray = bgfx::createUniform("s_TextureArray", bgfx::UniformType::Sampler);
    }

    SDL_free(pFile);
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
    for (size_t i = 0; i < numElements; i++)
    {
      uint16_t val = pData[i];
      if (val < 0x006A)
      {
        s_Grid[i] = val;
      }
    }

    const bgfx::Memory* pMemory = bgfx::makeRef(s_Grid, sizeof(s_Grid));
    s_GridBuffer                = bgfx::createIndexBuffer(pMemory, BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32);
    bgfx::setName(s_GridBuffer, "s_GridBuffer");

    SDL_free(pFile);
  }
}

void rt::Init()
{
  // Compute shader
  bgfx::ShaderHandle csh = bgfx::createShader(bgfx::makeRef(cs_raytracer, sizeof(cs_raytracer)));
  bgfx::setName(csh, "compute shader");
  s_RaytracerProgram = bgfx::createProgram(csh, true);
  s_RaytracerOutputTexture =
      bgfx::createTexture2D(MJ_RT_WIDTH, MJ_RT_HEIGHT, false, 1, bgfx::TextureFormat::RGBA32F,
                            BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIN_POINT);
  bgfx::setName(s_RaytracerOutputTexture, "s_RaytracerOutputTexture");

  // Screen shader
  bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::makeRef(vs_screen_triangle, sizeof(vs_screen_triangle)));
  bgfx::setName(vsh, "screen triangle vertex");
  bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::makeRef(fs_screen_triangle, sizeof(fs_screen_triangle)));
  bgfx::setName(fsh, "screen triangle fragment");
  s_ScreenTriangleProgram = bgfx::createProgram(vsh, fsh, true);

  // Set constant buffer with camera
  s_uMat         = bgfx::createUniform("mat", bgfx::UniformType::Mat4);
  s_uCameraPos   = bgfx::createUniform("s_CameraPos", bgfx::UniformType::Vec4);
  s_uFieldOfView = bgfx::createUniform("s_FieldOfView", bgfx::UniformType::Vec4);
  s_uWidth       = bgfx::createUniform("width", bgfx::UniformType::Vec4);
  s_uHeight      = bgfx::createUniform("height", bgfx::UniformType::Vec4);

  LoadLevel();
  s_ScreenTriangleSampler = bgfx::createUniform("SampleType", bgfx::UniformType::Sampler);
  s_ScreenTriangleVertexLayout.begin().add(bgfx::Attrib::Indices, 1, bgfx::AttribType::Float).end();

  InitTexture2DArray();

  MJ_DISCARD(InitObjectPlaceholder());
}

void rt::Resize(int width, int height)
{
  (void)width;
  (void)height;
}

void rt::Update(int width, int height, game::Data* pData)
{
  ZoneScoped;

  {
    // Get thread group and index
    MJ_UNINITIALIZED int x;
    MJ_UNINITIALIZED int y;
    MJ_DISCARD(SDL_GetMouseState(&x, &y));
    int32_t groupX  = x * MJ_RT_WIDTH / width / GRID_DIM;
    int32_t groupY  = y * MJ_RT_HEIGHT / height / GRID_DIM;
    int32_t threadX = x * MJ_RT_WIDTH / width % GRID_DIM;
    int32_t threadY = y * MJ_RT_HEIGHT / height % GRID_DIM;

    ImGui::Begin("Hello, world!");
    ImGui::Text("R to reset, F3 toggles mouselook");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::SliderFloat("Field of view", &pData->s_FieldOfView.x, 5.0f, 170.0f);
    ImGui::Text("Group: [%d, %d, 0], Thread: [%d, %d, 0]", groupX, groupY, threadX, threadY);
    ImGui::End();
  }

  bgfx::setUniform(s_uMat, glm::value_ptr(pData->s_Mat));
  bgfx::setUniform(s_uCameraPos, glm::value_ptr(pData->s_Camera.position));
  bgfx::setUniform(s_uFieldOfView, glm::value_ptr(pData->s_FieldOfView));
  bgfx::setUniform(s_uWidth, glm::value_ptr(pData->s_Width));
  bgfx::setUniform(s_uHeight, glm::value_ptr(pData->s_Height));

  const bgfx::ViewId viewId = 0;
  bgfx::setImage(5, s_RaytracerOutputTexture, 0, bgfx::Access::Write);
  bgfx::setBuffer(0, s_GridBuffer, bgfx::Access::Read);
  bgfx::setTexture(1, s_uTextureArray, s_RaytracerTextureArray);
  bgfx::setBuffer(2, s_ObjectBuffer, bgfx::Access::Read);
  bgfx::setBuffer(3, s_PaletteBuffer, bgfx::Access::Read);

  bgfx::dispatch(viewId, s_RaytracerProgram, (MJ_RT_WIDTH + GRID_DIM - 1) / GRID_DIM,
                 (MJ_RT_HEIGHT + GRID_DIM - 1) / GRID_DIM, 1);

  bgfx::setTexture(0, s_ScreenTriangleSampler, s_RaytracerOutputTexture);
  bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
  bgfx::setVertexCount(3);
  bgfx::submit(viewId, s_ScreenTriangleProgram);
}

void rt::Destroy()
{
  // Vertex buffers
  bgfx::destroy(s_ObjectBuffer);
  bgfx::destroy(s_GridBuffer);
  bgfx::destroy(s_PaletteBuffer);

  // Programs
  bgfx::destroy(s_RaytracerProgram);
  bgfx::destroy(s_ScreenTriangleProgram);

  // Textures
  bgfx::destroy(s_RaytracerTextureArray);
  bgfx::destroy(s_RaytracerOutputTexture);

  // Uniforms
  bgfx::destroy(s_uMat);
  bgfx::destroy(s_uCameraPos);
  bgfx::destroy(s_uFieldOfView);
  bgfx::destroy(s_uWidth);
  bgfx::destroy(s_uHeight);
  bgfx::destroy(s_uTextureArray);
  bgfx::destroy(s_ScreenTriangleSampler);
}
