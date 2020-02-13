#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d11.h>
#include <string>
#include <tchar.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static constexpr uint16_t sTextureWidth = 640;
static constexpr uint16_t sTextureHeight = 480;

struct Pixel
{
	uint8_t r8;
	uint8_t g8;
	uint8_t b8;
	uint8_t a8;
};
static Pixel p[640 * 480];

struct Global
{
	D3D_DRIVER_TYPE driverType;
	D3D_FEATURE_LEVEL featureLevel;
	HINSTANCE hInst;
	HWND hWnd;
	ComPtr<ID3D11Device> cd3dDevice;
	ComPtr<ID3D11DeviceContext> cImmediateContext;
	ComPtr<IDXGISwapChain> cSwapChain;
	ComPtr<ID3D11RenderTargetView> cRenderTargetView;

	ComPtr<ID3D11Texture2D> cTexture;
};

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(Global& g, HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HRESULT CompileShaderFromFile(WCHAR*, LPCSTR, LPCSTR, ID3DBlob**);
HRESULT InitDevice(Global& g);
void Render(Global& g);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nCmdShow)
{
	(void) hPrevInstance;
	(void) lpCmdLine;

	// TODO: Place code here.
	MSG msg = { 0 };

	// Initialize global strings
	MyRegisterClass(hInstance);

	// Perform application initialization:
	Global g;
	if (!InitInstance(g, hInstance, nCmdShow))
	{
		return FALSE;
	}

	if (FAILED(InitDevice(g)))
	{
		return 0;
	}

	// Main message loop:
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Render(g);
	}

	return (int) msg.wParam;
}



ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = _T("ImGui Platform");
	wcex.hIconSm = nullptr;

	return RegisterClassEx(&wcex);
}

static std::string GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

BOOL InitInstance(Global& g, HINSTANCE hInstance, int nCmdShow)
{
	g.hInst = hInstance; // Store instance handle in our global variable

	g.hWnd = CreateWindow(_T("ImGui Platform"), _T("Untitled"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!g.hWnd)
	{
		auto str = GetLastErrorAsString();
		return FALSE;
	}

	ShowWindow(g.hWnd, nCmdShow);
	UpdateWindow(g.hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


HRESULT InitDevice(Global& g)
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g.hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

#ifdef _DEBUG
	// If the project is in a debug build, enable debugging via SDK Layers with this flag.
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes [] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	// This array defines the ordering of feature levels that D3D should attempt to create.
	D3D_FEATURE_LEVEL featureLevels [] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g.hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g.driverType = driverTypes[driverTypeIndex];

		hr = D3D11CreateDeviceAndSwapChain(nullptr, g.driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, g.cSwapChain.ReleaseAndGetAddressOf(), g.cd3dDevice.ReleaseAndGetAddressOf(), &g.featureLevel, g.cImmediateContext.ReleaseAndGetAddressOf());

		if (hr == E_INVALIDARG)
		{
			// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
			hr = D3D11CreateDeviceAndSwapChain(nullptr, g.driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
				D3D11_SDK_VERSION, &sd, g.cSwapChain.ReleaseAndGetAddressOf(), g.cd3dDevice.ReleaseAndGetAddressOf(), &g.featureLevel, g.cImmediateContext.ReleaseAndGetAddressOf());
		}

		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = p;
	textureData.SysMemPitch = sTextureWidth * sizeof(Pixel);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = sTextureWidth;
	desc.Height = sTextureHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;

	g.cd3dDevice->CreateTexture2D(&desc, &textureData, g.cTexture.ReleaseAndGetAddressOf());

	// Create a render target view
	ComPtr<ID3D11Texture2D> cBackBuffer;
	hr = g.cSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*) cBackBuffer.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = g.cd3dDevice->CreateRenderTargetView(cBackBuffer.Get(), nullptr, g.cRenderTargetView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return hr;

	g.cImmediateContext->OMSetRenderTargets(1, g.cRenderTargetView.GetAddressOf(), nullptr);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT) width;
	vp.Height = (FLOAT) height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g.cImmediateContext->RSSetViewports(1, &vp);

	return S_OK;
}

//
//
//
void Render(Global& g)
{
	float a [] = { 1.0f, 0.0f, 1.0f, 1.0f };
	// Clear the back buffer to a solid color
	g.cImmediateContext->ClearRenderTargetView(g.cRenderTargetView.Get(), a);


	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	g.cImmediateContext->Map(g.cTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	BYTE* buffer = reinterpret_cast<BYTE*>(p);
	size_t rowspan = sTextureWidth * sizeof(Pixel);
	for (UINT i = 0; i < sTextureHeight; ++i)
	{
		memcpy(mappedData, p, rowspan);
		mappedData += mappedResource.RowPitch;
		buffer += rowspan;
	}

	g.cImmediateContext->Unmap(g.cTexture.Get(), 0);

	// Present our back buffer to our front buffer
	g.cSwapChain->Present(0, 0);
}
