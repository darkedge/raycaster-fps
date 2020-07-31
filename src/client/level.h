#pragma once

struct Level
{
  uint8_t width = 0;
  uint8_t height = 0;
  /// <summary>
  /// Indexing: x * width + z (height)
  /// </summary>
  uint16_t* pBlocks = nullptr;

public:
  static Level Load(const char* path);
  static void Save(Level level, const char* path);
  static void Free(Level level);
  bool IsValid() const;
};
