#pragma once
#include "camera.h"
#include "map.h"

struct ID3D11Device;
struct ID3D11DeviceContext;

class Graphics
{
public:
  void Init(ID3D11Device* pDevice);
  void Resize(int width, int height);
  void Update(ID3D11DeviceContext* pDeviceContext, const Camera* pCamera);
  void Destroy();
  void* GetTileTexture(int x, int y);
  void CreateMesh(map::map_t map, ID3D11Device* pDevice);

private:
  void InitTexture2DArray(ID3D11Device* pDevice);

  ID3D11Texture2D* pTextureArray;
  ID3D11SamplerState* pTextureSamplerState;
  ID3D11Buffer* pVertexBuffer;
  ID3D11Buffer* pIndexBuffer;
  ID3D11VertexShader* pVertexShader;
  ID3D11PixelShader* pPixelShader;
  ID3D11ShaderResourceView* pShaderResourceView; // Texture array SRV
  ID3D11InputLayout* pInputLayout;
  ID3D11RasterizerState* pRasterizerState;
  ID3D11RasterizerState* pRasterizerStateCullNone;
  ID3D11BlendState* pBlendState;
  ID3D11Buffer* pResource;
  UINT Indices;

  bx::DefaultAllocator defaultAllocator;
};
