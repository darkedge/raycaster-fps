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

bool Level::IsValid() const
{
  return (pBlocks && (width > 0) && (height > 0));
}
