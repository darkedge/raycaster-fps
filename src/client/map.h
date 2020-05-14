#pragma once
#include <stdint.h>

namespace map
{
  struct map_t
  {
    uint8_t width;
    uint8_t height;
    uint16_t* pBlocks;
  };

  map_t Load(const char* path);
  void Save(map_t map, const char* path);
  void Free(map_t map);
  bool Valid(map_t map);
} // namespace map
