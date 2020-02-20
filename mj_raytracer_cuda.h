#pragma once
#include <stdint.h>
#include "mj_common.h"
#include "mj_raytracer.h"
#include "camera.h"
#include "glm/glm.hpp"

struct ID3D11Texture2D;

namespace mj
{
  namespace cuda
  {
    struct Constant
    {
      glm::mat4 mat;
      rt::Shape s_Shapes[rt::DemoShape_Count];
      Camera s_Camera;
      const float s_FieldOfView = 45.0f; // Degrees
    };

    void Init(ID3D11Texture2D* s_pTexture);
    void Update();
    void Destroy();
  }
}