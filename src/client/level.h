#pragma once
#include "mj_math.h"

// -X, -Y, -Z, +X, +Y, +Z
struct Face
{
  enum Enum
  {
    West,
    Bottom,
    South,
    East,
    Top,
    North
  };
};

struct RaycastResult
{
  mjm::int3 position;
  int32_t type;
  Face::Enum face;
};

struct Level
{
  uint8_t width  = 0;
  uint8_t height = 0;
  /// <summary>
  /// Indexing: x * width + z (height)
  /// </summary>
  uint16_t* pBlocks = nullptr;

public:
  static Level Load(const char* path);
  static void Save(Level level, const char* path);
  static void Free(Level level);

  bool FireRay(mjm::vec3 origin, mjm::vec3 direction, float distance, RaycastResult* pResult) const;
  bool IsValid() const;
};
