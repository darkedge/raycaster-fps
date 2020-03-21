#pragma once
#include <stdint.h>
#include <wrl/client.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace mj
{
  namespace d3d11
  {
    bool Init(ID3D11Device* pDevice);
    bool Resize(ID3D11Device* pDevice, uint16_t width, uint16_t height);
    void Update(ID3D11DeviceContext* device_context);
    void Destroy();
  } // namespace d3d11
} // namespace mj
