#include "mj_d3d11.h"
#include "mj_raytracer.h"
#include "mj_raytracer_hlsl.h"
#include "mj_common.h"
#include "mj_win32_utils.h"

#include <d3d11.h>
#include <assert.h>

#include "intermediate/VSQuadOut.h"
#include "intermediate/PSQuadOut.h"

#include "resource.h"

static ID3D11VertexShader* s_pVertexShader;
static ID3D11PixelShader* s_pPixelShader;
static ID3D11Texture2D* s_pTexture;
static ID3D11SamplerState* s_pSamplerState;
static ID3D11ShaderResourceView* s_pShaderResourceView;
static D3D11_VIEWPORT s_Viewport;

bool mj::d3d11::Init(ID3D11Device* pDevice)
{
  s_Viewport.Width    = (FLOAT)MJ_WND_WIDTH;
  s_Viewport.Height   = (FLOAT)MJ_WND_HEIGHT;
  s_Viewport.MinDepth = 0.0f;
  s_Viewport.MaxDepth = 1.0f;
  s_Viewport.TopLeftX = 0;
  s_Viewport.TopLeftY = 0;

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width                = MJ_RT_WIDTH;
  desc.Height               = MJ_RT_HEIGHT;
  desc.MipLevels            = 1;
  desc.ArraySize            = 1;
  desc.Format               = DXGI_FORMAT_R32G32B32A32_FLOAT;
  desc.SampleDesc.Count     = 1;
  desc.Usage                = D3D11_USAGE_DEFAULT;
  desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
  desc.MiscFlags            = 0;

  assert(!s_pTexture);
  WIN32_ASSERT(pDevice->CreateTexture2D(&desc, nullptr, &s_pTexture));

  // Shaders
  assert(!s_pVertexShader);
  WIN32_ASSERT(pDevice->CreateVertexShader(VSQuadOut, sizeof(VSQuadOut), nullptr, &s_pVertexShader));
  assert(!s_pPixelShader);
  WIN32_ASSERT(pDevice->CreatePixelShader(PSQuadOut, sizeof(PSQuadOut), nullptr, &s_pPixelShader));

  // Sampler
  D3D11_SAMPLER_DESC samplerDesc = {};
  samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
  samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
  samplerDesc.MipLODBias         = 0.0f;
  samplerDesc.MaxAnisotropy      = 1;
  samplerDesc.ComparisonFunc     = D3D11_COMPARISON_ALWAYS;
  samplerDesc.BorderColor[0]     = 0;
  samplerDesc.BorderColor[1]     = 0;
  samplerDesc.BorderColor[2]     = 0;
  samplerDesc.BorderColor[3]     = 0;
  samplerDesc.MinLOD             = 0;
  samplerDesc.MaxLOD             = D3D11_FLOAT32_MAX;

  // Create the texture sampler state.
  assert(!s_pSamplerState);
  WIN32_ASSERT(pDevice->CreateSamplerState(&samplerDesc, &s_pSamplerState));

  // Setup the shader resource view description.
  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format                          = desc.Format;
  srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MostDetailedMip       = 0;
  srvDesc.Texture2D.MipLevels             = 1;

  // Create the shader resource view for the texture.
  assert(!s_pShaderResourceView);
  assert(s_pTexture);
  WIN32_ASSERT(pDevice->CreateShaderResourceView(s_pTexture, &srvDesc, &s_pShaderResourceView));

  return mj::hlsl::Init(pDevice, s_pTexture);
}

void mj::d3d11::Resize(float width, float height)
{
  // Touch window from inside
  const float ratio    = (float)MJ_RT_WIDTH / MJ_RT_HEIGHT;
  const float newRatio = width / height;
  if (newRatio > ratio)
  {
    float w             = height * ratio;
    s_Viewport.Width    = w;
    s_Viewport.Height   = height;
    s_Viewport.TopLeftX = (width - w) / 2.0f;
    s_Viewport.TopLeftY = 0;
  }
  else
  {
    float h             = width / ratio;
    s_Viewport.Width    = width;
    s_Viewport.Height   = h;
    s_Viewport.TopLeftX = 0;
    s_Viewport.TopLeftY = (height - h) / 2.0f;
  }
}

void mj::d3d11::Update(ID3D11DeviceContext* pDeviceContext)
{
  pDeviceContext->RSSetViewports(1, &s_Viewport);

  mj::hlsl::Update(pDeviceContext);

  // Full screen triangle

  // Bind
  pDeviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
  pDeviceContext->IASetIndexBuffer(nullptr, (DXGI_FORMAT)0, 0);
  pDeviceContext->IASetInputLayout(nullptr);
  assert(s_pVertexShader);
  pDeviceContext->VSSetShader(s_pVertexShader, nullptr, 0);
  assert(s_pPixelShader);
  pDeviceContext->PSSetShader(s_pPixelShader, nullptr, 0);
  assert(s_pSamplerState);
  pDeviceContext->PSSetSamplers(0, 1, &s_pSamplerState);
  assert(s_pShaderResourceView);
  pDeviceContext->PSSetShaderResources(0, 1, &s_pShaderResourceView);
  pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  pDeviceContext->Draw(3, 0);

  // Unbind
  ID3D11ShaderResourceView* ppSrcNull[1] = { nullptr };
  pDeviceContext->PSSetShaderResources(0, 1, ppSrcNull);
}

void mj::d3d11::Destroy()
{
  mj::hlsl::Destroy();

  SAFE_RELEASE(s_pVertexShader);
  SAFE_RELEASE(s_pPixelShader);
  SAFE_RELEASE(s_pTexture);
  SAFE_RELEASE(s_pSamplerState);
  SAFE_RELEASE(s_pShaderResourceView);
}
