#include "RenderUtils.h"


// == RenderTarget2D ==============================================================================

RenderTarget2D::RenderTarget2D() : Width(0),
	Height(0),
	Format(DXGI_FORMAT_UNKNOWN),
	NumMipLevels(0),
	MultiSamples(0),
	MSQuality(0),
	AutoGenMipMaps(false),
	UAView(nullptr),
	ArraySize(1)
{

}

void RenderTarget2D::Initialize(ID3D11Device* device,
	int width,
	int height,
	DXGI_FORMAT format,
	int numMipLevels,
	int multiSamples,
	int msQuality,
	bool autoGenMipMaps,
	bool createUAV,
	int arraySize,
	bool cubeMap)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = arraySize;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	if (createUAV)
		desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

	desc.CPUAccessFlags = 0;
	desc.Format = format;
	desc.MipLevels = numMipLevels;
	desc.MiscFlags = (autoGenMipMaps && numMipLevels != 1) ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;
	desc.SampleDesc.Count = multiSamples;
	desc.SampleDesc.Quality = msQuality;
	desc.Usage = D3D11_USAGE_DEFAULT;

	if (cubeMap)
	{
		Assert_(arraySize % 6 == 0);
		desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	}

	DXCall(device->CreateTexture2D(&desc, nullptr, &Texture));

	RTVArraySlices.clear();
	RTVArraySlices.reserve(arraySize);
	for (int i = 0; i < arraySize; ++i)
	{
		ID3D11RenderTargetViewPtr rtView;
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
		rtDesc.Format = format;

		if (arraySize == 1)
		{
			if (multiSamples > 1)
			{
				rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				rtDesc.Texture2D.MipSlice = 0;
			}
		}
		else
		{
			if (multiSamples > 1)
			{
				rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
				rtDesc.Texture2DMSArray.ArraySize = 1;
				rtDesc.Texture2DMSArray.FirstArraySlice = i;
			}
			else
			{
				rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				rtDesc.Texture2DArray.ArraySize = 1;
				rtDesc.Texture2DArray.FirstArraySlice = i;
				rtDesc.Texture2DArray.MipSlice = 0;
			}
		}
		DXCall(device->CreateRenderTargetView(Texture, &rtDesc, &rtView));

		RTVArraySlices.push_back(rtView);
	}

	RTView = RTVArraySlices[0];

	DXCall(device->CreateShaderResourceView(Texture, nullptr, &SRView));

	SRVArraySlices.clear();
	for (int i = 0; i < arraySize; ++i)
	{
		ID3D11ShaderResourceViewPtr srView;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = format;

		if (arraySize == 1)
		{
			if (multiSamples > 1)
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = -1;
				srvDesc.Texture2D.MostDetailedMip = 0;
			}
		}
		else
		{
			if (multiSamples > 1)
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.ArraySize = 1;
				srvDesc.Texture2DMSArray.FirstArraySlice = i;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.ArraySize = 1;
				srvDesc.Texture2DArray.FirstArraySlice = i;
				srvDesc.Texture2DArray.MipLevels = -1;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
			}
		}

		DXCall(device->CreateShaderResourceView(Texture, &srvDesc, &srView));
		SRVArraySlices.push_back(srView);
	}

	Width = width;
	Height = height;
	NumMipLevels = numMipLevels;
	MultiSamples = multiSamples;
	Format = format;
	AutoGenMipMaps = autoGenMipMaps;
	ArraySize = arraySize;
	CubeMap = cubeMap;

	if (createUAV)
		DXCall(device->CreateUnorderedAccessView(Texture, nullptr, &UAView));
};

// == DepthStencilBuffer ==========================================================================

DepthStencilBuffer::DepthStencilBuffer() : Width(0),
	Height(0),
	MultiSamples(0),
	MSQuality(0),
	Format(DXGI_FORMAT_UNKNOWN),
	ArraySize(1),
	CubeMap(false)
{

}

void DepthStencilBuffer::Initialize(ID3D11Device* device,
	int width,
	int height,
	DXGI_FORMAT format,
	bool useAsShaderResource,
	int multiSamples,
	int msQuality,
	int arraySize,
	bool cubeMap)
{
	Width = width;
	Height = height;
	MultiSamples = multiSamples;
	Format = format;
	ArraySize = arraySize;
	CubeMap = cubeMap;

	int bindFlags = D3D11_BIND_DEPTH_STENCIL;
	if (useAsShaderResource)
		bindFlags |= D3D11_BIND_SHADER_RESOURCE;

	DXGI_FORMAT dsTexFormat;
	if (!useAsShaderResource)
		dsTexFormat = format;
	else if (format == DXGI_FORMAT_D16_UNORM)
		dsTexFormat = DXGI_FORMAT_R16_TYPELESS;
	else if (format == DXGI_FORMAT_D24_UNORM_S8_UINT)
		dsTexFormat = DXGI_FORMAT_R24G8_TYPELESS;
	else
		dsTexFormat = DXGI_FORMAT_R32_TYPELESS;

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = arraySize;
	desc.BindFlags = bindFlags;
	desc.CPUAccessFlags = 0;
	desc.Format = dsTexFormat;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	if (cubeMap)
	{
		Assert_(arraySize == 6);
		Assert_(multiSamples == 1);
		desc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
	}
	desc.SampleDesc.Count = multiSamples;
	desc.SampleDesc.Quality = msQuality;
	desc.Usage = D3D11_USAGE_DEFAULT;
	DXCall(device->CreateTexture2D(&desc, nullptr, &Texture));

	ArraySlices.clear();
	for (int i = 0; i < arraySize; ++i)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		ID3D11DepthStencilViewPtr dsView;
		dsvDesc.Format = format;

		if (arraySize == 1)
		{
			dsvDesc.ViewDimension = multiSamples > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Texture2D.MipSlice = 0;
		}
		else
		{
			if (multiSamples > 1)
			{
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
				dsvDesc.Texture2DMSArray.ArraySize = 1;
				dsvDesc.Texture2DMSArray.FirstArraySlice = i;
			}
			else
			{
				dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Texture2DArray.ArraySize = 1;
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				dsvDesc.Texture2DArray.MipSlice = 0;
			}
		}

		dsvDesc.Flags = 0;
		DXCall(device->CreateDepthStencilView(Texture, &dsvDesc, &dsView));
		ArraySlices.push_back(dsView);

		if (i == 0)
		{
			// Also create a read-only DSV
			dsvDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;
			if (format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
				dsvDesc.Flags |= D3D11_DSV_READ_ONLY_STENCIL;
			DXCall(device->CreateDepthStencilView(Texture, &dsvDesc, &ReadOnlyDSView));
			dsvDesc.Flags = 0;
		}
	}

	DSView = ArraySlices[0];

	if (useAsShaderResource)
	{
		DXGI_FORMAT dsSRVFormat;
		if (format == DXGI_FORMAT_D16_UNORM)
			dsSRVFormat = DXGI_FORMAT_R16_UNORM;
		else if (format == DXGI_FORMAT_D24_UNORM_S8_UINT)
			dsSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else
			dsSRVFormat = DXGI_FORMAT_R32_FLOAT;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = dsSRVFormat;

		if (arraySize == 1)
		{
			srvDesc.ViewDimension = multiSamples > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.MostDetailedMip = 0;
		}
		else if (cubeMap)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = 1;
			srvDesc.TextureCube.MostDetailedMip = 0;
		}
		else
		{
			srvDesc.ViewDimension = multiSamples > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = arraySize;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
		}

		DXCall(device->CreateShaderResourceView(Texture, &srvDesc, &SRView));

		SRVArraySlices.clear();
		for (int i = 0; i < arraySize; ++i)
		{
			ID3D11ShaderResourceViewPtr srView;
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			srvDesc.Format = dsSRVFormat;

			if (arraySize == 1)
			{
				if (multiSamples > 1)
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
				}
				else
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = -1;
					srvDesc.Texture2D.MostDetailedMip = 0;
				}
			}
			else
			{
				if (multiSamples > 1)
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
					srvDesc.Texture2DMSArray.ArraySize = 1;
					srvDesc.Texture2DMSArray.FirstArraySlice = i;
				}
				else
				{
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.ArraySize = 1;
					srvDesc.Texture2DArray.FirstArraySlice = i;
					srvDesc.Texture2DArray.MipLevels = -1;
					srvDesc.Texture2DArray.MostDetailedMip = 0;
				}
			}

			DXCall(device->CreateShaderResourceView(Texture, &srvDesc, &srView));
			SRVArraySlices.push_back(srView);
		}
	}
	else
		SRView = nullptr;
}

// == RWBuffer ====================================================================================

RWBuffer::RWBuffer() : Size(0), Stride(0), NumElements(0), Format(DXGI_FORMAT_UNKNOWN), RawBuffer(false)
{

}

void RWBuffer::Initialize(ID3D11Device* device, DXGI_FORMAT format, int stride, int numElements,
	bool rawBuffer, bool vertexBuffer, bool indexBuffer, bool indirectArgs,
	const void* initData)
{
	if (rawBuffer)
	{
		stride = 4;
		Format = DXGI_FORMAT_R32_TYPELESS;
	}

	Format = format;
	Size = stride * numElements;
	Stride = stride;
	NumElements = numElements;
	RawBuffer = rawBuffer;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = stride * numElements;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = rawBuffer ? D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS : 0;
	bufferDesc.StructureByteStride = 0;

	Assert_(vertexBuffer == false || indexBuffer == false);

	if (vertexBuffer)
		bufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;

	if (indexBuffer)
	{
		bufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
		Assert_(Format == DXGI_FORMAT_R32_TYPELESS ||
			Format == DXGI_FORMAT_R32_UINT ||
			Format == DXGI_FORMAT_R16_UINT);
	}

	if (indirectArgs)
		bufferDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

	D3D11_SUBRESOURCE_DATA srData;
	srData.pSysMem = initData;
	srData.SysMemPitch = 0;
	srData.SysMemSlicePitch = 0;

	DXCall(device->CreateBuffer(&bufferDesc, initData ? &srData : nullptr, &Buffer));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = Format;

	if (rawBuffer)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.NumElements = numElements;
		srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	}
	else
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.NumElements = numElements;
	}

	DXCall(device->CreateShaderResourceView(Buffer, &srvDesc, &SRView));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.Flags = rawBuffer ? D3D11_BUFFER_UAV_FLAG_RAW : 0;

	DXCall(device->CreateUnorderedAccessView(Buffer, &uavDesc, &UAView));
}

// == StructuredBuffer ============================================================================

StructuredBuffer::StructuredBuffer() : Size(0), Stride(0), NumElements(0)
{
}

void StructuredBuffer::Initialize(ID3D11Device* device, int stride, int numElements, bool dynamic, bool useAsUAV,
	bool appendConsume, bool hiddenCounter, const void* initData)
{
	Size = stride * numElements;
	Stride = stride;
	NumElements = numElements;

	Assert_(appendConsume == false || hiddenCounter == false);

	if (appendConsume || hiddenCounter)
		useAsUAV = true;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = stride * numElements;
	bufferDesc.Usage = useAsUAV ? D3D11_USAGE_DEFAULT : dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.BindFlags |= useAsUAV ? D3D11_BIND_UNORDERED_ACCESS : 0;
	bufferDesc.CPUAccessFlags = useAsUAV ? 0 : dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = stride;

	D3D11_SUBRESOURCE_DATA subresourceData;
	subresourceData.pSysMem = initData;
	subresourceData.SysMemPitch = 0;
	subresourceData.SysMemSlicePitch = 0;

	DXCall(device->CreateBuffer(&bufferDesc, initData != nullptr ? &subresourceData : nullptr, &Buffer));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = numElements;
	DXCall(device->CreateShaderResourceView(Buffer, &srvDesc, &SRView));

	if (useAsUAV)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.Flags |= appendConsume ? D3D11_BUFFER_UAV_FLAG_APPEND : 0;
		uavDesc.Buffer.Flags |= hiddenCounter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0;
		uavDesc.Buffer.NumElements = numElements;
		DXCall(device->CreateUnorderedAccessView(Buffer, &uavDesc, &UAView));
	}
}


// == StagingBuffer ===============================================================================

StagingBuffer::StagingBuffer() : Size(0)
{
}

void StagingBuffer::Initialize(ID3D11Device* device, int size)
{
	Size = size;

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = Size;
	bufferDesc.Usage = D3D11_USAGE_STAGING;
	bufferDesc.BindFlags = 0;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	DXCall(device->CreateBuffer(&bufferDesc, nullptr, &Buffer));
}

void* StagingBuffer::Map(ID3D11DeviceContext* context)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	DXCall(context->Map(Buffer, 0, D3D11_MAP_READ, 0, &mapped));

	return mapped.pData;
}

void StagingBuffer::Unmap(ID3D11DeviceContext* context)
{
	context->Unmap(Buffer, 0);
}

// == StagingTexture2D ============================================================================

StagingTexture2D::StagingTexture2D() : Width(0),
	Height(0),
	Format(DXGI_FORMAT_UNKNOWN),
	NumMipLevels(0),
	MultiSamples(0),
	MSQuality(0),
	ArraySize(1)
{
}

void StagingTexture2D::Initialize(ID3D11Device* device,
	int width,
	int height,
	DXGI_FORMAT format,
	int numMipLevels,
	int multiSamples,
	int msQuality,
	int arraySize)
{
	D3D11_TEXTURE2D_DESC desc;
	desc.Width = width;
	desc.Height = height;
	desc.ArraySize = arraySize;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Format = format;
	desc.MipLevels = numMipLevels;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = multiSamples;
	desc.SampleDesc.Quality = msQuality;
	desc.Usage = D3D11_USAGE_STAGING;
	DXCall(device->CreateTexture2D(&desc, nullptr, &Texture));


	Width = width;
	Height = height;
	NumMipLevels = numMipLevels;
	MultiSamples = multiSamples;
	Format = format;
	ArraySize = arraySize;
};

void* StagingTexture2D::Map(ID3D11DeviceContext* context, int subResourceIndex, int& pitch)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	DXCall(context->Map(Texture, subResourceIndex, D3D11_MAP_READ, 0, &mapped));
	pitch = mapped.RowPitch;
	return mapped.pData;
}

void StagingTexture2D::Unmap(ID3D11DeviceContext* context, int subResourceIndex)
{
	context->Unmap(Texture, subResourceIndex);
}
