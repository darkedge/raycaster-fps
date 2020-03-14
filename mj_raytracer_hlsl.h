#pragma once
#include <stdint.h>
#include "mj_common.h"
#include "mj_raytracer.h"
#include "camera.h"
#include "glm/glm.hpp"

struct ID3D11Device;
struct ID3D11Texture2D;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

namespace mj
{
  namespace hlsl
  {
    struct Constant
    {
      glm::mat4 mat;
      Camera s_Camera;
      float fovDeg;
      int width;
      int height;
      float padding;
    };

    [[nodiscard]] bool Init(ID3D11Device* pDevice, ID3D11Texture2D* s_pTexture);
    void Update(ID3D11DeviceContext* device_context);
    void Destroy();
  } // namespace hlsl
} // namespace mj
