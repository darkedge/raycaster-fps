#pragma once
#include "mj_math.h"
#include "mj_common.h"

using block_t = uint16_t;

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

struct BlockPos
{
  MJ_UNINITIALIZED int32_t x;
  MJ_UNINITIALIZED int32_t z;
};

bool operator==(const BlockPos& a, const BlockPos& b);
bool operator!=(const BlockPos& a, const BlockPos& b);

struct RaycastResult
{
  BlockPos position;
  block_t block;
  Face::Enum face;
};

struct Level
{
  uint8_t width  = 0;
  uint8_t height = 0;
  /// <summary>
  /// Indexing: z * width + x (height)
  /// </summary>
  block_t* pBlocks = nullptr;

public:
  static Level Load(const char* path);
  static void Save(Level level, const char* path);
  static void Free(Level level);

  bool FireRay(mjm::vec3 origin, mjm::vec3 direction, float distance, RaycastResult* pResult) const;
  bool IsValid() const;
};
