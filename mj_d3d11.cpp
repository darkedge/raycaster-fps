#include "mj_d3d11.h"
#include "mj_raytracer.h"
#include "mj_common.h"
#include "mj_win32_utils.h"

#include <d3d11.h>
#include <assert.h>

#include "intermediate/VSQuadOut.h"
#include "intermediate/PSQuadOut.h"

static ID3D11VertexShader* s_pVertexShader;
static ID3D11PixelShader* s_pPixelShader;
static ID3D11Texture2D* s_pTexture;
static ID3D11SamplerState* s_pSamplerState;
static ID3D11ShaderResourceView* s_pShaderResourceView;
static D3D11_VIEWPORT s_Viewport;

bool mj::d3d11::Init(ID3D11Device* device, ID3D11DeviceContext* device_context)
{
	s_Viewport.Width = (FLOAT) MJ_WND_WIDTH;
	s_Viewport.Height = (FLOAT) MJ_WND_HEIGHT;
	s_Viewport.MinDepth = 0.0f;
	s_Viewport.MaxDepth = 1.0f;
	s_Viewport.TopLeftX = 0;
	s_Viewport.TopLeftY = 0;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = mj::rt::GetImage;
	textureData.SysMemPitch = MJ_RT_HEIGHT * sizeof(glm::vec4);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = MJ_RT_WIDTH;
	desc.Height = MJ_RT_HEIGHT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	assert(!s_pTexture);
	WIN32_ASSERT(device->CreateTexture2D(&desc, &textureData, &s_pTexture));

	// Shaders
	assert(!s_pVertexShader);
	WIN32_ASSERT(device->CreateVertexShader(VSQuadOut, sizeof(VSQuadOut), nullptr, &s_pVertexShader));
	assert(!s_pPixelShader);
	WIN32_ASSERT(device->CreatePixelShader(PSQuadOut, sizeof(PSQuadOut), nullptr, &s_pPixelShader));

	// Sampler
	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	assert(!s_pSamplerState);
	WIN32_ASSERT(device->CreateSamplerState(&samplerDesc, &s_pSamplerState));

	// Setup the shader resource view description.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	assert(!s_pShaderResourceView);
	assert(s_pTexture);
	WIN32_ASSERT(device->CreateShaderResourceView(s_pTexture, &srvDesc, &s_pShaderResourceView));

	return mj::rt::Init();
}

void mj::d3d11::Resize(float width, float height)
{
  // Touch window from inside
  const float ratio = (float) MJ_RT_WIDTH / MJ_RT_HEIGHT;
  const float newRatio = width / height;
  if (newRatio > ratio)
  {
	float w = height * ratio;
	s_Viewport.Width = w;
	s_Viewport.Height = height;
	s_Viewport.TopLeftX = (width - w) / 2.0f;
	s_Viewport.TopLeftY = 0;
  } 
  else
  {
	float h = width / ratio;
	s_Viewport.Width = width;
	s_Viewport.Height = h;
	s_Viewport.TopLeftX = 0;
	s_Viewport.TopLeftY = (height - h) / 2.0f;
  }
}

void mj::d3d11::Update(ID3D11DeviceContext* device_context)
{
	device_context->RSSetViewports(1, &s_Viewport);

	MJ_UNINITIALIZED D3D11_MAPPED_SUBRESOURCE texture;
	assert(s_pTexture);
	device_context->Map(s_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &texture);

	mj::rt::Update();

	const glm::vec4* src = mj::rt::GetImage().p;
	char* dst = reinterpret_cast<char*>(texture.pData);
	for (uint16_t i = 0; i < MJ_RT_HEIGHT; i++)
	{
		memcpy(dst, src, MJ_RT_WIDTH * sizeof(glm::vec4));
		dst += texture.RowPitch;
		src += MJ_RT_WIDTH;
	}

	device_context->Unmap(s_pTexture, 0);

	// Full screen triangle
	device_context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	device_context->IASetIndexBuffer(nullptr, (DXGI_FORMAT) 0, 0);
	device_context->IASetInputLayout(nullptr);
	assert(s_pVertexShader);
	device_context->VSSetShader(s_pVertexShader, nullptr, 0);
	assert(s_pPixelShader);
	device_context->PSSetShader(s_pPixelShader, nullptr, 0);
	assert(s_pSamplerState);
	device_context->PSSetSamplers(0, 1, &s_pSamplerState);
	assert(s_pShaderResourceView);
	device_context->PSSetShaderResources(0, 1, &s_pShaderResourceView);
	device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	device_context->Draw(3, 0);
}

void mj::d3d11::Destroy()
{
	mj::rt::Destroy();

	SAFE_RELEASE(s_pVertexShader);
	SAFE_RELEASE(s_pPixelShader);
	SAFE_RELEASE(s_pTexture);
	SAFE_RELEASE(s_pSamplerState);
	SAFE_RELEASE(s_pShaderResourceView);
}