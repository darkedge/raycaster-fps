#include "pch.h"
#include "level.h"
#include "mj_common.h"

// Level file format (*.mjl)
// 4 byte magic word (MJMF)
// 1 byte version number
// 1 byte width
// 1 byte height
// array of 2 byte values

// TODO: Add Level name, author, date, layers (floor, ceiling, objects)

static constexpr uint32_t s_MagicWord = 0x464D4A4D;
static constexpr uint8_t s_Version    = 0;
static bx::DefaultAllocator s_defaultAllocator;

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

Level Level::Load(const char* path)
{
  MJ_UNINITIALIZED size_t dataSize;
  Level level = {};
  void* pFile = SDL_LoadFile(path, &dataSize);
  if (pFile)
  {
    mj::MemoryBuffer reader(pFile, dataSize);
    MJ_UNINITIALIZED uint32_t magicWord;
    MJ_UNINITIALIZED uint8_t versionNumber;
    MJ_UNINITIALIZED size_t size;
    if (reader
            .Read(magicWord)            //
            .Read(versionNumber)        //
            .Read(level.width)          //
            .Read(level.height)         //
            .Good()                     //
        && (magicWord == s_MagicWord)   //
        && (versionNumber == s_Version) //
        && (reader.SizeLeft() >= (size = sizeof(uint16_t) * level.width * level.height)))
    {
      level.pBlocks = (uint16_t*)bx::alloc(&s_defaultAllocator, size, 0, __FILE__, __LINE__);
      bx::memCopy(level.pBlocks, reader.Position(), size);
    }

    SDL_free(pFile);
  }

  return level;
}

void Level::Save(Level level, const char* path)
{
  size_t size     = sizeof(uint16_t) * level.width * level.height;
  size_t dataSize = 4 + // 4 byte magic word (MJMF)
                    1 + // 1 byte version number
                    1 + // 1 byte width
                    1 + // 1 byte height
                    size;
  char* pData = (char*)bx::alloc(&s_defaultAllocator, dataSize, 0, __FILE__, __LINE__);
  if (pData)
  {
    // Write to pData
    mj::MemoryBuffer writer(pData, dataSize);
    if (writer
            .Write(s_MagicWord)  // 4 byte magic word (MJMF)
            .Write(s_Version)    // 1 byte version number
            .Write(level.width)  // 1 byte width
            .Write(level.height) // 1 byte height
            .Write(level.pBlocks, size)
            .Good())
    {
      // Write pData to file
      SDL_RWops* pFile = SDL_RWFromFile(path, "wb");
      if (pFile)
      {
        MJ_DISCARD(SDL_RWwrite(pFile, pData, dataSize, 1));
        MJ_DISCARD(SDL_RWclose(pFile));
      }
    }
  }
}

void Level::Free(Level level)
{
  if (level.pBlocks)
  {
    bx::free(&s_defaultAllocator, level.pBlocks, 0, __FILE__, __LINE__);
    level.pBlocks = nullptr;
  }
}

bool Level::FireRay(mjm::vec3 origin, mjm::vec3 direction, float distance, RaycastResult* pResult) const
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
    int32_t block = GetBlock(this, pResult->position);

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
        pResult->face = stepX > 0 ? Face::West : Face::East;
        tMaxX += tDeltaX;
        tMax = tMaxX;
      }
      else
      {
        pResult->position.z += stepZ;
        pResult->face = stepZ > 0 ? Face::South : Face::North;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
    else
    {
      if (tMaxY < tMaxZ)
      {
        pResult->position.y += stepY;
        pResult->face = stepY > 0 ? Face::Bottom : Face::Top;

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
        pResult->face = stepZ > 0 ? Face::South : Face::North;
        tMaxZ += tDeltaZ;
        tMax = tMaxZ;
      }
    }
  }

  return false;
}

bool Level::IsValid() const
{
  return (pBlocks && (width > 0) && (height > 0));
}
