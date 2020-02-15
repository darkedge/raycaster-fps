#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace mj
{
	namespace d3d11
	{
		bool Init(ID3D11Device* device, ID3D11DeviceContext* device_context);
		void Resize(float width, float height);
		void Update(ID3D11DeviceContext* device_context);
		void Destroy();
	}
}
