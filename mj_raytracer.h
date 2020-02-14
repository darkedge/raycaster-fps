#pragma once
#include <stdint.h>
#include "mj_common.h"

namespace mj
{
  namespace rt
  {
    union Pixel
    {
      struct
      {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
      };
      uint32_t rgba;
    };

    struct float3
    {
      float x;
      float y;
      float z;
    };

    struct Ray
    {
      float3 origin;
      float3 direction;
      float length;
    };

    struct Sphere
    {
      float3 origin;
      float radius;
    };

    struct Plane
    {
      float3 normal;
      float3 distance;
    };

    struct Camera
    {
      float3 origin;
      float3 direction;
    };

    struct Image
    {
      mj::rt::Pixel p[MJ_WIDTH * MJ_HEIGHT];
    };

    bool Init();
    void Update();
    const Image& GetImage();
    void Destroy();
  }
}