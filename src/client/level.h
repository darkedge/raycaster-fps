#pragma once

struct Level
{
  uint8_t width;
  uint8_t height;
  /// <summary>
  /// Indexing: x * width + z (height)
  /// </summary>
  uint16_t* pBlocks;

public:
  static Level Load(const char* path);
  static void Save(Level level, const char* path);
  static void Free(Level level);
  static bool Valid(Level level);
};
