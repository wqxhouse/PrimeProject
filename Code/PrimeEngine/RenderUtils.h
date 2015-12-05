#pragma once
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"

#include <comdef.h>
#include <WCHAR.h>
#include <vector>
#include <d3d9.h>
#include <assert.h>
#include <DxErr.h>
#pragma comment(lib,"DxErr.lib")
#pragma comment(lib, "D3D9.lib")

#define Assert_(x) assert(x)

inline std::wstring GetDXErrorString(HRESULT hr)
{
	std::wstring message = L"DirectX Error: ";
	message += DXGetErrorDescriptionW(hr);
	return message;
}

inline std::string GetDXErrorStringAnsi(HRESULT hr)
{
	std::wstring errorString = GetDXErrorString(hr);

	std::string message;
	for (size_t i = 0; i < errorString.length(); ++i)
		message.append(1, static_cast<char>(errorString[i]));
	return message;
}

#define DXCall(x)                                                       \
do                                                                      \
{                                                                       \
    HRESULT hr_ = x;                                                    \
    Assert_(SUCCEEDED(hr_) || GetDXErrorStringAnsi(hr_).c_str());       \
}                                                                       \
while(0)

// Device
_COM_SMARTPTR_TYPEDEF(ID3D11Device, __uuidof(ID3D11Device));
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceContext, __uuidof(ID3D11DeviceContext));
_COM_SMARTPTR_TYPEDEF(ID3D11DeviceChild, __uuidof(ID3D11DeviceChild));
_COM_SMARTPTR_TYPEDEF(ID3D11Query, __uuidof(ID3D11Query));
_COM_SMARTPTR_TYPEDEF(ID3D11CommandList, __uuidof(ID3D11CommandList));
_COM_SMARTPTR_TYPEDEF(ID3D11Counter, __uuidof(ID3D11Counter));
_COM_SMARTPTR_TYPEDEF(ID3D11InputLayout, __uuidof(ID3D11InputLayout));
_COM_SMARTPTR_TYPEDEF(ID3D11Predicate, __uuidof(ID3D11Predicate));
_COM_SMARTPTR_TYPEDEF(ID3D11Asynchronous, __uuidof(ID3D11Asynchronous));
_COM_SMARTPTR_TYPEDEF(ID3D11InfoQueue, __uuidof(ID3D11InfoQueue));
_COM_SMARTPTR_TYPEDEF(ID3D11Debug, __uuidof(ID3D11Debug));

// States
_COM_SMARTPTR_TYPEDEF(ID3D11BlendState, __uuidof(ID3D11BlendState));
_COM_SMARTPTR_TYPEDEF(ID3D11DepthStencilState, __uuidof(ID3D11DepthStencilState));
_COM_SMARTPTR_TYPEDEF(ID3D11RasterizerState, __uuidof(ID3D11RasterizerState));
_COM_SMARTPTR_TYPEDEF(ID3D11SamplerState, __uuidof(ID3D11SamplerState));

// Resources
_COM_SMARTPTR_TYPEDEF(ID3D11Resource, __uuidof(ID3D11Resource));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture1D, __uuidof(ID3D11Texture1D));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture2D, __uuidof(ID3D11Texture2D));
_COM_SMARTPTR_TYPEDEF(ID3D11Texture3D, __uuidof(ID3D11Texture3D));
_COM_SMARTPTR_TYPEDEF(ID3D11Buffer, __uuidof(ID3D11Buffer));

// Views
_COM_SMARTPTR_TYPEDEF(ID3D11View, __uuidof(ID3D11View));
_COM_SMARTPTR_TYPEDEF(ID3D11RenderTargetView, __uuidof(ID3D11RenderTargetView));
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderResourceView, __uuidof(ID3D11ShaderResourceView));
_COM_SMARTPTR_TYPEDEF(ID3D11DepthStencilView, __uuidof(ID3D11DepthStencilView));
_COM_SMARTPTR_TYPEDEF(ID3D11UnorderedAccessView, __uuidof(ID3D11UnorderedAccessView));

// Shaders
_COM_SMARTPTR_TYPEDEF(ID3D11ComputeShader, __uuidof(ID3D11ComputeShader));
_COM_SMARTPTR_TYPEDEF(ID3D11DomainShader, __uuidof(ID3D11DomainShader));
_COM_SMARTPTR_TYPEDEF(ID3D11GeometryShader, __uuidof(ID3D11GeometryShader));
_COM_SMARTPTR_TYPEDEF(ID3D11HullShader, __uuidof(ID3D11HullShader));
_COM_SMARTPTR_TYPEDEF(ID3D11PixelShader, __uuidof(ID3D11PixelShader));
_COM_SMARTPTR_TYPEDEF(ID3D11VertexShader, __uuidof(ID3D11VertexShader));
_COM_SMARTPTR_TYPEDEF(ID3D11ClassInstance, __uuidof(ID3D11ClassInstance));
_COM_SMARTPTR_TYPEDEF(ID3D11ClassLinkage, __uuidof(ID3D11ClassLinkage));
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderReflection, IID_ID3D11ShaderReflection);
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderReflectionConstantBuffer, IID_ID3D11ShaderReflectionConstantBuffer);

_COM_SMARTPTR_TYPEDEF(ID3D11ShaderReflectionType, IID_ID3D11ShaderReflectionType);
_COM_SMARTPTR_TYPEDEF(ID3D11ShaderReflectionVariable, IID_ID3D11ShaderReflectionVariable);

// D3D10
_COM_SMARTPTR_TYPEDEF(ID3D10Blob, IID_ID3D10Blob);
typedef ID3D10BlobPtr ID3DBlobPtr;

// DXGI
_COM_SMARTPTR_TYPEDEF(IDXGISwapChain, __uuidof(IDXGISwapChain));
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter, __uuidof(IDXGIAdapter));
_COM_SMARTPTR_TYPEDEF(IDXGIAdapter1, __uuidof(IDXGIAdapter1));
_COM_SMARTPTR_TYPEDEF(IDXGIDevice, __uuidof(IDXGIDevice));
_COM_SMARTPTR_TYPEDEF(IDXGIDevice1, __uuidof(IDXGIDevice1));
_COM_SMARTPTR_TYPEDEF(IDXGIDeviceSubObject, __uuidof(IDXGIDeviceSubObject));
_COM_SMARTPTR_TYPEDEF(IDXGIFactory, __uuidof(IDXGIFactory));
_COM_SMARTPTR_TYPEDEF(IDXGIFactory1, __uuidof(IDXGIFactory1));
_COM_SMARTPTR_TYPEDEF(IDXGIKeyedMutex, __uuidof(IDXGIKeyedMutex));
_COM_SMARTPTR_TYPEDEF(IDXGIObject, __uuidof(IDXGIObject));
_COM_SMARTPTR_TYPEDEF(IDXGIOutput, __uuidof(IDXGIOutput));
_COM_SMARTPTR_TYPEDEF(IDXGIResource, __uuidof(IDXGIResource));
_COM_SMARTPTR_TYPEDEF(IDXGISurface1, __uuidof(IDXGISurface1));


struct RenderTarget2D
{
	ID3D11Texture2DPtr Texture;
	ID3D11RenderTargetViewPtr RTView;
	ID3D11ShaderResourceViewPtr SRView;
	ID3D11UnorderedAccessViewPtr UAView;
	int Width;
	int Height;
	int NumMipLevels;
	int MultiSamples;
	int MSQuality;
	DXGI_FORMAT Format;
	bool AutoGenMipMaps;
	int ArraySize;
	bool CubeMap;
	std::vector<ID3D11RenderTargetViewPtr> RTVArraySlices;
	std::vector<ID3D11ShaderResourceViewPtr> SRVArraySlices;


	RenderTarget2D();

	void Initialize(ID3D11Device* device,
		int width,
		int height,
		DXGI_FORMAT format,
		int numMipLevels = 1,
		int multiSamples = 1,
		int msQuality = 0,
		bool autoGenMipMaps = false,
		bool createUAV = false,
		int arraySize = 1,
		bool cubeMap = false);
};

struct DepthStencilBuffer
{
	ID3D11Texture2DPtr Texture;
	ID3D11DepthStencilViewPtr DSView;
	ID3D11DepthStencilViewPtr ReadOnlyDSView;
	ID3D11ShaderResourceViewPtr SRView;
	int Width;
	int Height;
	int MultiSamples;
	int MSQuality;
	DXGI_FORMAT Format;
	int ArraySize;
	bool CubeMap;
	std::vector<ID3D11DepthStencilViewPtr> ArraySlices;
	std::vector<ID3D11ShaderResourceViewPtr> SRVArraySlices;

	DepthStencilBuffer();

	void Initialize(ID3D11Device* device,
		int width,
		int height,
		DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT,
		bool useAsShaderResource = false,
		int multiSamples = 1,
		int msQuality = 0,
		int arraySize = 1,
		bool cubeMap = false);
};

struct RWBuffer
{
	ID3D11BufferPtr Buffer;
	ID3D11ShaderResourceViewPtr SRView;
	ID3D11UnorderedAccessViewPtr UAView;
	int Size;
	int Stride;
	int NumElements;
	bool RawBuffer;
	DXGI_FORMAT Format;

	RWBuffer();

	void Initialize(ID3D11Device* device, DXGI_FORMAT format, int stride, int numElements, bool rawBuffer = false,
		bool vertexBuffer = false, bool indexBuffer = false, bool indirectArgs = false,
		const void* initData = nullptr);
};

struct StagingBuffer
{
	ID3D11BufferPtr Buffer;
	int Size;

	StagingBuffer();

	void Initialize(ID3D11Device* device, int size);
	void* Map(ID3D11DeviceContext* context);
	void Unmap(ID3D11DeviceContext* context);
};

struct StagingTexture2D
{
	ID3D11Texture2DPtr Texture;
	int Width;
	int Height;
	int NumMipLevels;
	int MultiSamples;
	int MSQuality;
	DXGI_FORMAT Format;
	int ArraySize;

	StagingTexture2D();

	void Initialize(ID3D11Device* device,
		int width,
		int height,
		DXGI_FORMAT format,
		int numMipLevels = 1,
		int multiSamples = 1,
		int msQuality = 0,
		int arraySize = 1);

	void* Map(ID3D11DeviceContext* context, int subResourceIndex, int& pitch);
	void Unmap(ID3D11DeviceContext* context, int subResourceIndex);
};

struct StructuredBuffer
{
	ID3D11BufferPtr Buffer;
	ID3D11ShaderResourceViewPtr SRView;
	ID3D11UnorderedAccessViewPtr UAView;
	int Size;
	int Stride;
	int NumElements;
	StagingBuffer DebugBuffer;

	StructuredBuffer();

	void Initialize(ID3D11Device* device, int stride, int numElements, bool dynamic = false, bool useAsUAV = false,
		bool appendConsume = false, bool hiddenCounter = false, const void* initData = nullptr);
};

template<typename T> class ConstantBuffer
{
public:

	T Data;

	ID3D11BufferPtr Buffer;
	bool GPUWritable;

public:

	ConstantBuffer() : GPUWritable(false)
	{
		ZeroMemory(&Data, sizeof(T));
	}

	void Initialize(ID3D11Device* device, bool gpuWritable = false)
	{
		D3D11_BUFFER_DESC desc;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.ByteWidth = static_cast<int>(sizeof(T) + (16 - (sizeof(T) % 16)));

		if (gpuWritable)
		{
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;
		}
		GPUWritable = gpuWritable;

		DXCall(device->CreateBuffer(&desc, nullptr, &Buffer));
	}

	void ApplyChanges(ID3D11DeviceContext* deviceContext)
	{
		Assert_(Buffer != nullptr);

		if (GPUWritable)
		{
			deviceContext->UpdateSubresource(Buffer, 0, nullptr, &Data, 0, 0);
		}
		else
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			DXCall(deviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
			CopyMemory(mappedResource.pData, &Data, sizeof(T));
			deviceContext->Unmap(Buffer, 0);
		}
	}

	void SetVS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);

		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->VSSetConstantBuffers(slot, 1, bufferArray);
	}

	void SetPS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);

		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->PSSetConstantBuffers(slot, 1, bufferArray);
	}

	void SetGS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);


		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->GSSetConstantBuffers(slot, 1, bufferArray);
	}

	void SetHS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);

		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->HSSetConstantBuffers(slot, 1, bufferArray);
	}

	void SetDS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);

		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->DSSetConstantBuffers(slot, 1, bufferArray);
	}

	void SetCS(ID3D11DeviceContext* deviceContext, int slot) const
	{
		Assert_(Buffer != nullptr);

		ID3D11Buffer* bufferArray[1];
		bufferArray[0] = Buffer;
		deviceContext->CSSetConstantBuffers(slot, 1, bufferArray);
	}
};

// For aligning to float4 boundaries
#define Float4Align __declspec(align(16))

class PIXEvent
{
public:

	PIXEvent(const WCHAR* markerName)
	{
		D3DPERF_BeginEvent(0xFFFFFFFF, markerName);
	}

	PIXEvent(const std::wstring& markerName)
	{
		D3DPERF_BeginEvent(0xFFFFFFFF, markerName.c_str());
	}

	~PIXEvent()
	{
		D3DPERF_EndEvent();
	}
};
