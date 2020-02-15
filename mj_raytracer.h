#pragma once
#include <stdint.h>
#include "mj_common.h"
#include "glm/glm.hpp"

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

    struct Ray
    {
      glm::vec3 origin;
      glm::vec3 direction;
      float length;
    };

    struct RaycastResult
    {
      glm::vec3 intersection;
      glm::vec3 normal;
      float length;
    };

    struct Sphere
    {
      glm::vec3 origin;
      float radius;
    };

    struct Plane
    {
      glm::vec3 normal;
      glm::vec3 distance;
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