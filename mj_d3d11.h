#pragma once
#include <wrl/client.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace mj
{
  namespace d3d11
  {
    bool Init(ID3D11Device* pDevice);
    void Resize(float width, float height);
    void Update(ID3D11DeviceContext* device_context);
    void Destroy();
  } // namespace d3d11
} // namespace mj
