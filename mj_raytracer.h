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

    struct AABB
    {
      glm::vec3 min;
      glm::vec3 max;
    };

    struct Shape
    {
      enum Type
      {
        Shape_Sphere,
        Shape_Plane,
        Shape_AABB,
        Shape_Octree
      };
      int type;
      glm::vec3 color;
      union {
        Sphere sphere;
        Plane plane;
        AABB aabb;
      };
    };

    enum DemoShape
    {
      DemoShape_RedSphere,
      DemoShape_YellowSphere,
      DemoShape_BlueSphere,
      DemoShape_GreenAABB,
      DemoShape_WhitePlane,
      DemoShape_CyanPlane,
      DemoShape_Count
    };
  } // namespace rt
} // namespace mj