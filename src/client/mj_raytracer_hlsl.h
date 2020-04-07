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

    void SetTexture(ID3D11Device* pDevice, ID3D11Texture2D* pTexture);
    [[nodiscard]] bool Init(ID3D11Device* pDevice);
    void Update(ID3D11DeviceContext* pDeviceContext, uint16_t width, uint16_t height);
    void Destroy();
  } // namespace hlsl
} // namespace mj
