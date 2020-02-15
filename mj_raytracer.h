#pragma once
#include <stdint.h>
#include "mj_common.h"
#include "glm/glm.hpp"

namespace mj
{
  namespace rt
  {
    struct Ray
    {
      glm::vec3 origin;
      glm::vec3 direction;
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
      float distance;
    };

    struct Shape {
      enum Type
      {
        Shape_Sphere,
        Shape_Plane
      };
      Type type;
      glm::vec3 color;
      union
      {
        Sphere sphere;
        Plane plane;
      };
    };

    struct Image
    {
      glm::vec4 p[MJ_WIDTH * MJ_HEIGHT];
    };

    bool Init();
    void Update();
    const Image& GetImage();
    void Destroy();
  }
}