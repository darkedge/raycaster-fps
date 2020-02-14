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