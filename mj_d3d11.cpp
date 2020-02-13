#include "mj_d3d11.h"
#include "mj_raytracer.h"
#include "mj_common.h"

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

struct Pixel
{
	uint8_t r8;
	uint8_t g8;
	uint8_t b8;
	uint8_t a8;
};
static Pixel p[MJ_WIDTH * MJ_HEIGHT];

bool mj::d3d11::Init(ID3D11Device* device, ID3D11DeviceContext* device_context)
{
	MJ_UNINITIALIZED HRESULT hr;

	s_Viewport.Width = (FLOAT) MJ_WIDTH;
	s_Viewport.Height = (FLOAT) MJ_HEIGHT;
	s_Viewport.MinDepth = 0.0f;
	s_Viewport.MaxDepth = 1.0f;
	s_Viewport.TopLeftX = 0;
	s_Viewport.TopLeftY = 0;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = p;
	textureData.SysMemPitch = MJ_HEIGHT * sizeof(Pixel);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = MJ_WIDTH;
	desc.Height = MJ_HEIGHT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	assert(!s_pTexture);
	device->CreateTexture2D(&desc, &textureData, &s_pTexture);

	// Shaders
	assert(!s_pVertexShader);
	device->CreateVertexShader(VSQuadOut, sizeof(VSQuadOut), nullptr, &s_pVertexShader);
	assert(!s_pPixelShader);
	device->CreatePixelShader(PSQuadOut, sizeof(PSQuadOut), nullptr, &s_pPixelShader);

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
	hr = device->CreateSamplerState(&samplerDesc, &s_pSamplerState);
	if (FAILED(hr))
	{
		return false;
	}

	// Setup the shader resource view description.
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	// Create the shader resource view for the texture.
	assert(!s_pShaderResourceView);
	assert(s_pTexture);
	hr = device->CreateShaderResourceView(s_pTexture, &srvDesc, &s_pShaderResourceView);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

void mj::d3d11::Update(ID3D11DeviceContext* device_context)
{
	device_context->RSSetViewports(1, &s_Viewport);

	MJ_UNINITIALIZED D3D11_MAPPED_SUBRESOURCE mappedResource;
	assert(s_pTexture);
	device_context->Map(s_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	BYTE* buffer = reinterpret_cast<BYTE*>(p);
	size_t rowspan = MJ_WIDTH * sizeof(Pixel);
	for (UINT i = 0; i < MJ_HEIGHT; ++i)
	{
		memcpy(mappedData, p, rowspan);
		mappedData += mappedResource.RowPitch;
		buffer += rowspan;
	}
	device_context->Unmap(s_pTexture, 0);

	//device_context->OMSetRenderTargets(1, s_pRenderTargetView.GetAddressOf(), nullptr);

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
	if (s_pVertexShader) { s_pVertexShader->Release(); s_pVertexShader = nullptr; }
	if (s_pPixelShader) { s_pPixelShader->Release(); s_pPixelShader = nullptr; }
	if (s_pTexture) { s_pTexture->Release(); s_pTexture = nullptr; }
	if (s_pSamplerState) { s_pSamplerState->Release(); s_pSamplerState = nullptr; }
	if (s_pShaderResourceView) { s_pShaderResourceView->Release(); s_pShaderResourceView = nullptr; }
}