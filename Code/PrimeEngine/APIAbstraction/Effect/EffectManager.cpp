#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "../../Lua/LuaEnvironment.h"
#include "../../Lua/EventGlue/EventDataCreators.h"

#include "PrimeEngine/Render/ShaderActions/SetPerObjectGroupConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SetPerFrameConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SA_Bind_Resource.h"
#include "PrimeEngine/Render/ShaderActions/SetInstanceControlConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_PerInstanceData.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_SingleObjectAnimationPalette.h"
#include "PrimeEngine/Render/ShaderActions/SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes.h"

// + Deferred
#include "PrimeEngine/Render/ShaderActions/SetClusteredShadingConstantShaderAction.h"
#define MAX_DIRECITONAL_LIGHT 8
#define MAX_POINT_LIGHT	1024
#define MAX_SPOT_LIGHT 512 
// #define MAX_LIGHT_INDICES 32*8*32*2048 // crashes... 
#define MAX_LIGHT_INDICES 32*8*32*20 

#include "PrimeEngine/APIAbstraction/GPUBuffers/AnimSetBufferGPU.h"
#include "PrimeEngine/APIAbstraction/Texture/GPUTextureManager.h"
#include "PrimeEngine/Scene/DrawList.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/Scene/CameraSceneNode.h"
#include "PrimeEngine/Scene/CameraManager.h"
#include "PrimeEngine/Scene/Light.h"
#include "PrimeEngine/Scene/SceneNode.h"

#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/RootSceneNode.h"
#include "PrimeEngine/TestModels.h"

// Sibling/Children includes
#include "EffectManager.h"
#include "Effect.h"


namespace PE {

using namespace Components;

Handle EffectManager::s_myHandle;

EffectManager::EffectManager(PE::GameContext &context, PE::MemoryArena arena)
	: m_map(context, arena, 128)
	, m_pixelShaderSubstitutes(context, arena)
	, m_pCurRenderTarget(NULL)
	, m_pixelShaders(context, arena, 256)
	, m_vertexShaders(context, arena, 256)
#if APIABSTRACTION_D3D11
		, m_cbuffers(context, arena, 64)
	#endif
	, m_doMotionBlur(false)
	, m_glowSeparatedTextureGPU(context, arena)
	, m_2ndGlowTargetTextureGPU(context, arena)
	, m_shadowMapDepthTexture(context, arena)
	, m_frameBufferCopyTexture(context, arena)
{
	m_arena = arena; m_pContext = &context;
	_normalIntensity = 1.0f;
}

void EffectManager::setupConstantBuffersAndShaderResources()
{
#if PE_PLAT_IS_PSVITA
	PSVitaRenderer *pPSVitaRenderer = static_cast<PSVitaRenderer *>(m_pContext->getGPUScreen());
	
	int BufferCount = 1024;
	SceUID shaderActionMemId;
	int shaderActionSize = 
		sizeof( SetPerFrameConstantsShaderAction::Data ) +
		sizeof( SetPerObjectGroupConstantsShaderAction::Data ) +
		sizeof( SetPerObjectConstantsShaderAction::Data ) * BufferCount +
		sizeof( SetPerMaterialConstantsShaderAction::Data ) * BufferCount;
	
	// this will also map memory to gpu
	char *shaderActionData = (char *)graphicsAlloc(
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RWDATA_UNCACHE,
		shaderActionSize,
		4,
		SCE_GXM_MEMORY_ATTRIB_READ, // how to map to gpu
		&shaderActionMemId);

	{
		SetPerFrameConstantsShaderAction::s_pBuffer = shaderActionData;
		shaderActionData += sizeof(SetPerFrameConstantsShaderAction::Data);

		SetPerObjectGroupConstantsShaderAction::s_pBuffer = shaderActionData;
		shaderActionData += sizeof( SetPerObjectGroupConstantsShaderAction::Data );
		
		SetPerObjectConstantsShaderAction::s_pBuffer = shaderActionData;
		shaderActionData += sizeof( SetPerObjectConstantsShaderAction::Data ) * BufferCount;
		
		SetPerMaterialConstantsShaderAction::s_pBuffer = shaderActionData;
		shaderActionData += sizeof( SetPerMaterialConstantsShaderAction::Data );

	}
#elif APIABSTRACTION_D3D9

#	elif APIABSTRACTION_D3D11

		D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
		ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
		ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

		ID3D11Buffer *pBuf;
		{
			//cbuffers
			HRESULT hr;
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC; // can only write to it, can't read
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // make sure we can access it with cpu for writing only
			cbDesc.MiscFlags = 0;
		
			cbDesc.ByteWidth = sizeof( SetPerFrameConstantsShaderAction::Data );

			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf );
			assert(SUCCEEDED(hr));
			SetPerFrameConstantsShaderAction::s_pBuffer = pBuf;
		
			m_cbuffers.add("cbPerFrame", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerObjectGroupConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
		
			SetPerObjectGroupConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerObjectGroup", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerObjectConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetPerObjectConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerObject", pBuf);

			cbDesc.ByteWidth = sizeof( SetPerMaterialConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetPerMaterialConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbPerMaterial", pBuf);

			cbDesc.ByteWidth = sizeof( SetInstanceControlConstantsShaderAction::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetInstanceControlConstantsShaderAction::s_pBuffer = pBuf;
			m_cbuffers.add("cbInstanceControlConstants", pBuf);

			// + Deferred - clustered shading constant buffer
			cbDesc.ByteWidth = sizeof(SetClusteredShadingConstantsShaderAction::Data);
			hr = pDevice->CreateBuffer(&cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SetClusteredShadingConstantsShaderAction::s_pBuffer= pBuf;
			m_cbuffers.add("cbClusteredShadingConsts", pBuf);
		}

		{
			// cbDesc.Usage = D3D11_USAGE_DEFAULT can't have cpu access flag set

			// if need cpu access for writing (using map) usage must be DYNAMIC or STAGING. resource cant be set as output
			// if need cpu access for reading and writing (using map) usage must be STAGING. resource cant be set as output
			// note, map & DYNAMIC is used for resrouces that get updated at least once per frame
			// also note that map allows updating part of resource while gpu might be using other part
			// can use UpdateSubresource with DEFAULT or IMMUTABLE, but suggested to use only with DEFAULT
			
			// if usage is default (gpu reads and writes) then UpdateSubresource() is used to write to it from cpu
			
			
			//tbuffers
			D3D11_BUFFER_DESC cbDesc;
			cbDesc.Usage = D3D11_USAGE_DYNAMIC; // can only write to it, can't read
			cbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; //means will need shader resource view // D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // make sure we can access it with cpu for writing only
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			cbDesc.StructureByteStride = sizeof(Matrix4x4);
#else
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = sizeof(Vector4);
#endif
			cbDesc.ByteWidth = sizeof(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::Data);
			HRESULT hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pBuffer = pBuf;
			

			cbDesc.ByteWidth = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data);
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pBufferSinglePalette = pBuf;


			cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			cbDesc.StructureByteStride = sizeof(SA_SetAndBind_ConstResource_PerInstanceData::PerObjectInstanceData);
			cbDesc.ByteWidth = sizeof( SA_SetAndBind_ConstResource_PerInstanceData::Data );
			hr = pDevice->CreateBuffer( &cbDesc, NULL, &pBuf);
			assert(SUCCEEDED(hr));
			SA_SetAndBind_ConstResource_PerInstanceData::s_pBuffer = pBuf;


			D3D11_SHADER_RESOURCE_VIEW_DESC vdesc;
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			vdesc.Format = DXGI_FORMAT_UNKNOWN;
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::Data) / (4*4*4);
#else
			vdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			vdesc.Buffer.NumElements = sizeof(SetPerObjectAnimationConstantsShaderAction::Data) / (4*4);
#endif
			vdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			vdesc.Buffer.ElementOffset = 0;
			
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pBuffer, &vdesc, &SA_SetAndBind_ConstResource_InstancedObjectsAnimationPalettes::s_pShaderResourceView);
			assert(SUCCEEDED(hr));
			
#if PE_DX11_USE_STRUCTURED_BUFFER_INSTEAD_OF_TBUFFER
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data) / (4*4*4);
#else
			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::Data) / (4*4);
#endif
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pBufferSinglePalette, &vdesc, &SA_SetAndBind_ConstResource_SingleObjectAnimationPalette::s_pShaderResourceViewSinglePalette);
			assert(SUCCEEDED(hr));


			vdesc.Buffer.NumElements = sizeof(SA_SetAndBind_ConstResource_PerInstanceData::Data) / sizeof(SA_SetAndBind_ConstResource_PerInstanceData::PerObjectInstanceData);
			hr = pDevice->CreateShaderResourceView(SA_SetAndBind_ConstResource_PerInstanceData::s_pBuffer, &vdesc, &SA_SetAndBind_ConstResource_PerInstanceData::s_pShaderResourceView);
			assert(SUCCEEDED(hr));
		}

		AnimSetBufferGPU::createGPUBufferForAnimationCSResult(*m_pContext);

		// + Deferred - hard code cluster World space bound
		m_cMin = Vector3(-1000, -450, -1000);
		m_cMax = Vector3(1000, 450, 1000);

		_dirLights.resize(MAX_DIRECITONAL_LIGHT);
		_pointLights.resize(MAX_POINT_LIGHT);
		_spotLights.resize(MAX_SPOT_LIGHT);
		// _lightIndices.resize(CX*CY*CZ*(MAX_POINT_LIGHT + MAX_SPOT_LIGHT)); // Over 32MB ...
		_lightIndices.resize(MAX_LIGHT_INDICES);
		_dirLightNum = 0;
		_pointLightNum = 0;
		_spotLightNum = 0;

		// + Deferred - create structured buffer to store light indices
		{
			// Figure out a way to use R16_UINT in structured buffer
			// to save GPU memory (or is it even possible???)
			D3D11_BUFFER_DESC bufferDesc;
			ZeroMemory(&bufferDesc, sizeof(bufferDesc));
			bufferDesc.ByteWidth = _lightIndices.size() * sizeof(unsigned int);

			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;

			bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			bufferDesc.StructureByteStride = sizeof(unsigned int);

			HRESULT hr = pDevice->CreateBuffer(&bufferDesc, NULL, &_lightIndicesBuffer);
			assert(SUCCEEDED(hr));

	
			// Too damn small 
			//D3D11_TEXTURE1D_DESC desc;
			//ZeroMemory(&desc, sizeof(desc));
			//desc.Width = _lightIndices.size();
			//desc.MipLevels = 1;
			//desc.ArraySize = 1;
			//desc.Format = DXGI_FORMAT_R16_UINT;
			//desc.Usage = D3D11_USAGE_DYNAMIC;
			//desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			//desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			//desc.MiscFlags = 0;
			//HRESULT hr = pDevice->CreateTexture1D(&desc, NULL, &_lightIndicesTex);
			//assert(SUCCEEDED(hr));

			D3D11_SHADER_RESOURCE_VIEW_DESC srv;
			ZeroMemory(&srv, sizeof(srv));

			// srv.Format = DXGI_FORMAT_R16_UINT;
			srv.Format = DXGI_FORMAT_UNKNOWN;
			srv.Buffer.NumElements = _lightIndices.size();

			// srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
			srv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			srv.Buffer.ElementOffset = 0;

			hr = pDevice->CreateShaderResourceView(
				_lightIndicesBuffer, &srv, 
				&_lightIndicesBufferShaderView);
			//hr = pDevice->CreateShaderResourceView(
			//	_lightIndicesTex, &srv, 
			//	&_lightIndicesBufferShaderView);
			assert(SUCCEEDED(hr));
		}

		// Allocate 3D Texture for clusters
		{
			D3D11_TEXTURE3D_DESC desc;
			desc.Width = CX;
			desc.Height = CY;
			desc.Depth = CZ;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_R32G32_UINT;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			HRESULT hr = pDevice->CreateTexture3D(&desc, NULL, &_clusterTex);
			assert(SUCCEEDED(hr));

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

			srvDesc.Format = DXGI_FORMAT_R32G32_UINT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = 0;
			srvDesc.Texture3D.MipLevels = 1;

			hr = pDevice->CreateShaderResourceView(_clusterTex, &srvDesc, &_clusterTexShaderView);
			assert(SUCCEEDED(hr));
		}

#	endif
}

void EffectManager::assignLightToClusters()
{
	auto &lights = PE::RootSceneNode::Instance()->m_lights;

	// Handle hdirL = Handle("ARRAY", sizeof())
	/*Array<Light *> dirLights;
	dirLights.reset(MAX_DIRECITONAL_LIGHT);
	Array<Light *> pointLights;
	pointLights.reset(MAX_POINT_LIGHT);
	Array<Light *> spotLights;
	spotLights.reset(MAX_SPOT_LIGHT);
	Array<short> lightIndices;
	lightIndices.reset(CX*CY*CZ*(MAX_SPOT_LIGHT + MAX_POINT_LIGHT));*/

	// can be optimized
	//_dirLights.clear();
	//_pointLights.clear();
	//_spotLights.clear();
	//_lightIndices.clear();
	int curDirLight = 0;
	int curPointLight = 0;
	int curSpotLight = 0;
	int curLightIndices = 0;
	memset(&_cluster, 0, sizeof(ClusterData)*CX*CY*CZ);

	int c_list[CZ][CY][CX][20]; // currently allow each cluster store 20 lights at maximum
	short c_list_count[CZ][CY][CX];

	memset(c_list, 0, sizeof(c_list));
	memset(c_list_count, 0, sizeof(c_list_count));

	for (int i = 0; i < lights.m_size; i++)
	{
		Light *l = lights[i].getObject<Light>();
		if (l->_showLight)
		{
			if (l->m_cbuffer.type == 0) // p
			{
				_pointLights[curPointLight++] = l;
			}
			else if (l->m_cbuffer.type == 1)
			{
				_dirLights[curDirLight++] = l;
			}
			else if (l->m_cbuffer.type == 2)
			{
				_spotLights[curSpotLight++] = l;
			}
		}
	}

	Vector3 size = m_cMax - m_cMin;
	Vector3 scale = Vector3(float(CX) / size.m_x, float(CY) / size.m_y, float(CZ) / size.m_z);
	Vector3 inv_scale = Vector3(1.0f / scale.m_x, 1.0f / scale.m_y, 1.0f / scale.m_z);

	// Directional light treat differently - do not assign to clusters

	// Point light assignment
	for (int i = 0; i < curPointLight; i++)
	{
		Light *pl = _pointLights[i];

		const Vector3 p = (pl->m_cbuffer.pos - m_cMin);
		const Vector3 pos = pl->m_cbuffer.pos;
		const Vector3 radiusVec = Vector3(pl->m_cbuffer.range, pl->m_cbuffer.range, pl->m_cbuffer.range);

		Vector3 p_min;
		Vector3 p_max;

		Vector3 p0 = p - radiusVec;
		Vector3 p1 = p + radiusVec;
		p0.m_x *= scale.m_x;
		p0.m_y *= scale.m_y;
		p0.m_z *= scale.m_z;
		p1.m_x *= scale.m_x;
		p1.m_y *= scale.m_y;
		p1.m_z *= scale.m_z;

		p_min = p0;
		p_max = p1;

		// Cluster for the center of the light
		const int px = (int)floorf(p.m_x * scale.m_x);
		const int py = (int)floorf(p.m_y * scale.m_y);
		const int pz = (int)floorf(p.m_z * scale.m_z);

		// Cluster bounds for the light
		const int x0 = max((int)floorf(p_min.m_x), 0);
		const int x1 = min((int)ceilf(p_max.m_x), CX);
		const int y0 = max((int)floorf(p_min.m_y), 0);
		const int y1 = min((int)ceilf(p_max.m_y), CY);
		const int z0 = max((int)floorf(p_min.m_z), 0);
		const int z1 = min((int)ceilf(p_max.m_z), CZ);

		const float radius_sqr = radiusVec.m_x * radiusVec.m_x;

		// Do AABB<->Sphere tests to figure out which clusters are actually intersected by the light
		for (int z = z0; z < z1; z++)
		{
			float dz = (pz == z) ? 0.0f : m_cMin.m_z + (pz < z ? z : z + 1) * inv_scale.m_z - pos.m_z;
			dz *= dz;

			for (int y = y0; y < y1; y++)
			{
				float dy = (py == y) ? 0.0f : m_cMin.m_y + (py < y ? y : y + 1) * inv_scale.m_y - pos.m_y;
				dy *= dy;
				dy += dz;

				for (int x = x0; x < x1; x++)
				{
					float dx = (px == x) ? 0.0f : m_cMin.m_x + (px < x ? x : x + 1) * inv_scale.m_x - pos.m_x;
					dx *= dx;
					dx += dy;

					if (dx < radius_sqr)
					{
						// TOOD: find ways to allocate larger buffer
						// than max limit of d3d11 of texture1D
					/*	if (curLightIndices >= MAX_LIGHT_INDICES)
						{
							char dbg[512];
							sprintf_s(dbg, 512, "curLightIndices reached maximum\n");
							break;
						}
						
*/	
						// printf("Assigned %d, %d, %d\n", x, y, z);

						int curClusterLightCount = c_list_count[z][y][x];
						if (curClusterLightCount >= 20)
						{
							char dbg[512];
							sprintf_s(dbg, 512, "curLightIndices reached maximum\n");
							break;
						}

						//_cluster[z][y][x].offset = curLightIndices;
						 
						// _cluster[z][y][x].pointLightCount++;
						int curPtCount = _cluster[z][y][x].counts >> 16;
						curPtCount++;
						
						// clear original 
						_cluster[z][y][x].counts &= 0xFFFF;
						_cluster[z][y][x].counts |= (curPtCount << 16);

						c_list[z][y][x][c_list_count[z][y][x]++] = i;

						// _lightIndices[curLightIndices++] = i;
					}
				}
			}
		}
	}

	
	// flush c_list to _lightIndices - could cause high overhead if too finely subdivide the cluster
	for (int z = 0; z < CZ; z++)
	{
		for (int y = 0; y < CY; y++)
		{
			for (int x = 0; x < CX; x++)
			{
				_cluster[z][y][x].offset = curLightIndices;

				for (int k = 0; k < c_list_count[z][y][x]; k++)
				{
					_lightIndices[curLightIndices++] = c_list[z][y][x][k];
				}
			}
		}
	}

	// figure out spot light intersection later
	
	ID3D11DeviceContext *context = ((D3D11Renderer *)m_pContext->getGPUScreen())->m_pD3DContext;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// Upload lightIndices structured buffer
	BYTE* mappedData = NULL;
	context->Map(_lightIndicesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	memcpy(mappedData, &_lightIndices[0], sizeof(unsigned int) * _lightIndices.size());
	context->Unmap(_lightIndicesBuffer, 0);

	// Upload 3D Texture
	context->Map(_clusterTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	for (int z = 0; z < CZ; z++)
	{
		for (int y = 0; y < CY; y++)
		{
			memcpy(mappedData + z * mappedResource.DepthPitch 
							  + y * mappedResource.RowPitch, 
							  _cluster[z][y], CX * sizeof(ClusterData));
		}
	}
	context->Unmap(_clusterTex, 0);

	_dirLightNum = curDirLight;
	_pointLightNum = curPointLight;
	_spotLightNum = curSpotLight;
}

void EffectManager::createSetShadowMapShaderValue(PE::Components::DrawList *pDrawList)
{
	Handle &h = pDrawList->nextGlobalShaderValue();
	h = Handle("SA_Bind_Resource", sizeof(SA_Bind_Resource));
	SA_Bind_Resource *p = new(h) SA_Bind_Resource(*m_pContext, m_arena);
	p->set(SHADOWMAP_TEXTURE_2D_SAMPLER_SLOT, m_shadowMapDepthTexture.m_samplerState, API_CHOOSE_DX11_DX9_OGL_PSVITA(m_shadowMapDepthTexture.m_pDepthShaderResourceView, m_shadowMapDepthTexture.m_pTexture, m_shadowMapDepthTexture.m_texture, m_shadowMapDepthTexture.m_texture));
}

void EffectManager::buildFullScreenBoard()
{
	//todo: use createBillboard() functionality of cpu buffers
	PositionBufferCPU vbcpu(*m_pContext, m_arena);
	float fw = (float)(m_pContext->getGPUScreen()->getWidth());
	float fh = (float)(m_pContext->getGPUScreen()->getHeight());
	float fx = -1.0f / fw / 2.0f;
	float fy = 1.0f / fh / 2.0f;

	// vbcpu.createNormalizeBillboardCPUBufferXYWithPtOffsets(fx, fy);

	// + Deferred -- Make the compensation for Deferred depth reconstruction
	vbcpu.createNormalizeBillboardCPUBufferXYWithPtOffsets(0, 0);
	vbcpu.m_values[2] = 1.0f;
	vbcpu.m_values[5] = 1.0f;
	vbcpu.m_values[8] = 1.0f;
	vbcpu.m_values[11] = 1.0f;

	//for (int i = 0; i < vbcpu.m_values.m_size; i++)
	//{
	//	if (i % 3 == 0 && i != 0)
	//	{
	//		printf("\n");
	//	}
	//	printf("%f ", vbcpu.m_values[i]);
	//}
	//

	ColorBufferCPU tcbcpu(*m_pContext, m_arena);
	tcbcpu.m_values.reset(3 * 4);
	#if APIABSTRACTION_OGL
		// flip up vs down in ogl
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
	#else
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(1.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
		tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f); tcbcpu.m_values.add(0.0f);
	#endif
	
	IndexBufferCPU ibcpu(*m_pContext, m_arena);
	ibcpu.createBillboardCPUBuffer();

	m_hVertexBufferGPU = Handle("VERTEX_BUFFER_GPU", sizeof(VertexBufferGPU));
	VertexBufferGPU *pvbgpu = new(m_hVertexBufferGPU) VertexBufferGPU(*m_pContext, m_arena);
	pvbgpu->createGPUBufferFromSource_ColoredMinimalMesh(vbcpu, tcbcpu);

	m_hIndexBufferGPU = Handle("INDEX_BUFFER_GPU", sizeof(IndexBufferGPU));
	IndexBufferGPU *pibgpu = new(m_hIndexBufferGPU) IndexBufferGPU(*m_pContext, m_arena);
	pibgpu->createGPUBuffer(ibcpu);

	m_hFirstGlowPassEffect = getEffectHandle("firstglowpass.fx");
	m_hSecondGlowPassEffect = getEffectHandle("verticalblur.fx");
	m_hGlowSeparationEffect = getEffectHandle("glowseparationpass.fx");
	m_hMotionBlurEffect = getEffectHandle("motionblur.fx");
	m_hColoredMinimalMeshTech = getEffectHandle("ColoredMinimalMesh_Tech");

	m_hAccumulationHDRPassEffect = getEffectHandle("DeferredLightPass_Clustered_Tech");
	m_hfinalHDRPassEffect= getEffectHandle("deferredFinalHDR.fx");
	m_hdebugPassEffect = getEffectHandle("debug_Tech.fx");
	//Liu
	m_hDeferredLightPassEffect = getEffectHandle("DeferredLightPass_Classical_Tech");
	m_hLightMipsPassEffect = getEffectHandle("LightMipsPassTech");
	m_hRayTracingPassEffect = getEffectHandle("RayTracingPassTech");

	m_hGBufferLightPassEffect = getEffectHandle("ColoredMinimalMesh_GBuffer_Tech");

	//Liu
	createSphere(1,20,20);
	createSkyBoxGeom();

	m_hCubemapPrefilterPassEffect = getEffectHandle("CubemapPrefilterTech");
}

void EffectManager::setFrameBufferCopyRenderTarget()
{
	#if APIABSTRACTION_D3D9
		m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_frameBufferCopyTexture, true);
	#elif APIABSTRACTION_OGL
		assert(!"not implemented");
	#elif APIABSTRACTION_D3D11

		m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_frameBufferCopyTexture, true);
	#endif
}

void EffectManager::setShadowMapRenderTarget()
{
	if (m_pCurRenderTarget)
	{
		assert(!"There should be no active render target when we set shadow map as render target!");
	}

#if APIABSTRACTION_D3D9
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#elif APIABSTRACTION_D3D11
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#elif APIABSTRACTION_OGL
	m_pContext->getGPUScreen()->setDepthStencilOnlyRenderTargetAndViewport(&m_shadowMapDepthTexture, true);
	m_pCurRenderTarget = &m_shadowMapDepthTexture;
#endif
}

void EffectManager::endCurrentRenderTarget()
{
	m_pContext->getGPUScreen()->endRenderTarget(m_pCurRenderTarget);
	m_pCurRenderTarget = NULL;
}

void EffectManager::setTextureAndDepthTextureRenderTargetForGBuffer()
{
#if !APIABSTRACTION_D3D11
	assert(false || "Deferred shading only works on D3D11 mode\n");
#endif

	// doesn't matter it deallocates after out of scope
	//const int numRts = 2;
	//TextureGPU* rtts[numRts];
	//rtts[0] = m_halbedoTextureGPU.getObject<TextureGPU>();
	//rtts[1] = m_hnormalTextureGPU.getObject<TextureGPU>();

	if (m_pContext->_renderMode == 0)
	{
		const int numRts = 3; // Liu
		TextureGPU* rtts[numRts];
		rtts[0] = m_halbedoTextureGPU.getObject<TextureGPU>();
		rtts[1] = m_hnormalTextureGPU.getObject<TextureGPU>();
		rtts[2] = m_hmaterialTextureGPU.getObject<TextureGPU>();
		// rtts[3] = m_hpositionTextureGPU.getObject<TextureGPU>();
		
		// TODO: make it work for xbox, now only D3D11
		((D3D11Renderer *)m_pContext->getGPUScreen())->
			setDeferredShadingRTsAndViewportWithDepth(rtts, numRts, m_pCurDepthTarget, true, true);
	}
	else if (m_pContext->_renderMode == 1)
	{
		const int numRts = 3; // Liu
		TextureGPU* rtts[numRts];
		rtts[0] = m_halbedoTextureGPU.getObject<TextureGPU>();
		rtts[1] = m_hnormalTextureGPU.getObject<TextureGPU>();
		rtts[2] = m_hpositionTextureGPU.getObject<TextureGPU>();
		// TODO: make it work for xbox, now only D3D11
		((D3D11Renderer *)m_pContext->getGPUScreen())->
			setDeferredShadingRTsAndViewportWithDepth(rtts, numRts, m_pCurDepthTarget, true, true);
	}

	// really not necessary since we use MRT, not a single render target
	// but for convention... meh...

	// TODO: Actually this seems to be  what debug info resolves to
	m_pCurRenderTarget = m_halbedoTextureGPU.getObject<TextureGPU>();
}

void EffectManager::setLightAccumTextureRenderTarget()
{
	TextureGPU* accumTex = m_haccumHDRTextureGPU.getObject<TextureGPU>();
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(accumTex, true);

	m_pCurRenderTarget = m_haccumHDRTextureGPU.getObject<TextureGPU>();
}

//Liu
void EffectManager::setLightMipsTextureRenderTarget(int level)
{
	//create new buffer	

	HRESULT hr = S_OK;
	D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
	ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
	ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

	TextureGPU* accumTex = m_haccumHDRTextureGPU.getObject<TextureGPU>();
	TextureGPU* tempTex = m_htempMipsTextureGPU.getObject<TextureGPU>();

	//Create a render target view
	D3D11_RENDER_TARGET_VIEW_DESC DescRT;
	DescRT.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DescRT.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	DescRT.Texture2D.MipSlice = level;
	hr = pDevice->CreateRenderTargetView(accumTex->m_pTexture, &DescRT, &accumTex->m_pMipsRenderTargetView);

	
	// Get the mip level and create the SRV
	//ID3D11Resource* MipRes;
	//accumTex->m_pMipsRenderTargetView->GetResource(&MipRes);
	

	// Create the resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
	DescRV.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	DescRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	DescRV.Texture2D.MipLevels = 1;	
	DescRV.Texture2D.MostDetailedMip = 0; //mip0
	
	hr = pDevice->CreateShaderResourceView(accumTex->m_pTexture, &DescRV, &accumTex->m_pMipsShaderResourceView);
	
	//MipRes->Release();

	ID3D11Resource* srcRes;
	ID3D11Resource*	desRes;
	accumTex->m_pMipsShaderResourceView->GetResource(&srcRes);
	tempTex->m_pRenderTargetView->GetResource(&desRes);
	pDeviceContext->CopyResource(desRes, srcRes);
	srcRes->Release(); desRes->Release();


	((D3D11Renderer *)m_pContext->getGPUScreen())->
		setMipsRenderTargetsAndViewportWithNoDepth(accumTex, true);
	m_pCurRenderTarget = m_haccumHDRTextureGPU.getObject<TextureGPU>();

	//m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(clightTex, true);

	//m_pCurRenderTarget = m_hlightTextureGPU.getObject<TextureGPU>();
}
//
//void EffectManager::setFinalLDRTextureRenderTarget()
//{
//	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(m_hfinalLDRTextureGPU.getObject<TextureGPU>(), true);
//	m_pCurRenderTarget = m_hfinalLDRTextureGPU.getObject<TextureGPU>();
//}

void EffectManager::setTextureAndDepthTextureRenderTargetForGlow()
{
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(m_hGlowTargetTextureGPU.getObject<TextureGPU>(), m_hGlowTargetTextureGPU.getObject<TextureGPU>(), true, true);
	m_pCurRenderTarget = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
}

void EffectManager::setTextureAndDepthTextureRenderTargetForDefaultRendering()
{
	// use device back buffer and depth
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
}

void EffectManager::set2ndGlowRenderTarget()
{
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_2ndGlowTargetTextureGPU, true);
	m_pCurRenderTarget = &m_2ndGlowTargetTextureGPU;
}


void EffectManager::drawFullScreenQuad()
{
	Effect &curEffect = *m_hColoredMinimalMeshTech.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *pTex = PE::GPUTextureManager::Instance()->getRandomTexture().getObject<TextureGPU>();

 	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, pTex->m_samplerState, API_CHOOSE_DX11_DX9_OGL_PSVITA(pTex->m_pShaderResourceView, pTex->m_pTexture, pTex->m_texture, pTex->m_texture));
 	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerFrameConstantsShaderAction sa(*m_pContext, m_arena);
	sa.m_data.gGameTime = 1.0f;

	sa.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();

	static float x = 0;
	//objSa.m_data.gW.importScale(0.5f, 1.0f, 1.0f);
	objSa.m_data.gW.setPos(Vector3(x, 0, 1.0f));
	//x+=0.01;
	if (x > 1)
		x = 0;
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	sa.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
}

// this function has to be called right after rendering scene into render target
// the reason why mipmaps have to be generated is because we separate glow
// into a smaller texture, so we need to generate mipmaps so that when we sample glow, it is not aliased
// if mipmaps are not generated, but do exist glow might not work at all since a lower mip will be sampled
void EffectManager::drawGlowSeparationPass()
{
	Effect &curEffect = *m_hGlowSeparationEffect.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;
//todo: generate at least one mipmap on all platforms so that glow downsampling is smoother
// in case it doesnt look smooth enough. as long as we are nto donwsampling to ridiculous amount it should be ok without mipmaps
/*
#		ifndef _XBOX
			m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture->GenerateMipSubLevels();
#		endif

#if APIABSTRACTION_D3D11

			D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
			ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
			ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

			pDeviceContext->GenerateMips(
			m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView);
#endif		
*/
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_glowSeparatedTextureGPU, true);

	m_pCurRenderTarget = &m_glowSeparatedTextureGPU;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);
	
	TextureGPU *t = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture,t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);
	
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
}

void EffectManager::drawFirstGlowPass()
{
	Effect &curEffect = *m_hFirstGlowPassEffect.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(&m_2ndGlowTargetTextureGPU, true);

	m_pCurRenderTarget = &m_2ndGlowTargetTextureGPU;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_glowSeparatedTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_glowSeparatedTextureGPU.m_pShaderResourceView, m_glowSeparatedTextureGPU.m_pTexture, m_glowSeparatedTextureGPU.m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
}

void EffectManager::drawSecondGlowPass()
{
	Effect &curEffect = *m_hSecondGlowPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>(), true);
	
	m_pCurRenderTarget = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	
	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	PE::SA_Bind_Resource setBlurTextureAction(*m_pContext, m_arena, DIFFUSE_BLUR_TEXTURE_2D_SAMPLER_SLOT, m_2ndGlowTargetTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_2ndGlowTargetTextureGPU.m_pShaderResourceView, m_2ndGlowTargetTextureGPU.m_pTexture, m_2ndGlowTargetTextureGPU.m_texture));
	setBlurTextureAction.bindToPipeline(&curEffect);

	TextureGPU *t = m_hGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);


	// the effect blurs vertically the horizontally blurred glow and adds it to source
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setBlurTextureAction.unbindFromPipeline(&curEffect);
	setTextureAction.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
}

void EffectManager::uploadDeferredClusteredConstants(float nearClip, float farClip)
{
	Effect &curEffect = *m_hAccumulationHDRPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	// Quad MVP
	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;
	objSa.bindToPipeline(&curEffect);

	// cbClusteredShadingConsts
	SetClusteredShadingConstantsShaderAction pscs(*m_pContext, m_arena);
	/*CameraSceneNode *csn =
		CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<CameraSceneNode>();*/
	float n = nearClip;
	float f = farClip;
	float projA = f / (f - n);
	float projB = (-f * n) / (f - n);
	// Vector3 camPos = csn->m_base.getPos();
	Vector3 camPos = Vector3();

	// TODO: default camera far plane is 2000 (too large, consider make it small to get better depth-pos reconstruction)
	pscs.m_data.csconsts.cNear = n;
	pscs.m_data.csconsts.cFar = f;
	pscs.m_data.csconsts.cProjA = projA;
	pscs.m_data.csconsts.cProjB = projB;
	pscs.m_data.camPos = camPos;
	pscs.m_data.camZAxisWS = Vector3();

	pscs.m_data.dirLightNum = _dirLightNum;

	Vector3 size = m_cMax - m_cMin;
	Vector3 scale = Vector3(float(CX) / size.m_x, float(CY) / size.m_y, float(CZ) / size.m_z);
	pscs.m_data.scale = scale;
	Vector3 bias;
	bias.m_x = -scale.m_x * m_cMin.m_x;
	bias.m_y = -scale.m_y * m_cMin.m_y;
	bias.m_z = -scale.m_z * m_cMin.m_z;
	pscs.m_data.bias = bias;
	// pscs.m_data.bias = m_cMin;

	// Convert PE Light to shader Light (huge overhead...; 
	// but no time to change the fundamental structure of PE)
	for (unsigned int i = 0; i < _dirLightNum; i++)
	{
		pscs.m_data.dirLights[i].cDir = _dirLights[i]->m_cbuffer.dir;
		pscs.m_data.dirLights[i].cColor = _dirLights[i]->m_cbuffer.diffuse.asVector3Ref();
	}

	for (unsigned int i = 0; i < _pointLightNum; i++)
	{
		pscs.m_data.pointLights[i].cPos = _pointLights[i]->m_cbuffer.pos;
		pscs.m_data.pointLights[i].cColor = _pointLights[i]->m_cbuffer.diffuse.asVector3Ref();
		pscs.m_data.pointLights[i].cRadius = _pointLights[i]->m_cbuffer.range;
		pscs.m_data.pointLights[i].cSpecPow = _pointLights[i]->m_cbuffer.spotPower;
	}

	for (unsigned int i = 0; i < _spotLightNum; i++)
	{
		pscs.m_data.spotLights[i].cPos = _spotLights[i]->m_cbuffer.pos;
		pscs.m_data.spotLights[i].cColor = _spotLights[i]->m_cbuffer.diffuse.asVector3Ref();
		pscs.m_data.spotLights[i].cSpotCutOff = _spotLights[i]->m_cbuffer.range;
		pscs.m_data.spotLights[i].cDir = _spotLights[i]->m_cbuffer.dir;
		pscs.m_data.spotLights[i].cPos = _spotLights[i]->m_cbuffer.pos;
	}

	// memcpy(pscs.m_data.lightIndices, &_lightIndices, sizeof(short) * _lightIndices.size());

	pscs.bindToPipeline(&curEffect);
}

void EffectManager::drawDeferredCubemapLightingQuadOnly(ID3D11ShaderResourceView *depth, ID3D11ShaderResourceView *rt0, ID3D11ShaderResourceView *rt1, ID3D11ShaderResourceView *rt2)
{
	PE::Components::Effect &curEffect =
		*PE::EffectManager::Instance()->getEffectHandle("DeferredCubemapLightingTech").getObject<PE::Components::Effect>();

	if (!curEffect.m_isReady)
		return;

	PE::SA_Bind_Resource setTextureActionAlbedo(
		*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		rt0);
	setTextureActionAlbedo.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionNormal(
		*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded, 
		rt1);
	setTextureActionNormal.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionDepth(
		*m_pContext, m_arena, DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		depth);
	setTextureActionDepth.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionClusters(
		*m_pContext, m_arena, CLUSTERED_TEXTURE_3D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		_clusterTexShaderView);
	setTextureActionClusters.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setLightIndicesStructuredBuffer(
		*m_pContext, m_arena, GpuResourceSlot_ClusteredLightIndexListResource,
		SamplerState_NotNeeded,
		_lightIndicesBufferShaderView);
	setLightIndicesStructuredBuffer.bindToPipeline(&curEffect);


	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrent(pvbGPU);

	// draw quad
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureActionAlbedo.unbindFromPipeline(&curEffect);
	setTextureActionNormal.unbindFromPipeline(&curEffect);
	setTextureActionDepth.unbindFromPipeline(&curEffect);
	setTextureActionClusters.unbindFromPipeline(&curEffect);
}

// + Deferred
void EffectManager::drawClusteredLightHDRPass()
{
	PIXEvent event(L"Render Scene Clustered Deferred");
	Effect &curEffect = *m_hAccumulationHDRPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrent(pvbGPU);

	TextureGPU *albedoTexture = m_halbedoTextureGPU.getObject<TextureGPU>();
	TextureGPU *normalTexture = m_hnormalTextureGPU.getObject<TextureGPU>();
	TextureGPU *rootDepthTexture = m_hrootDepthBufferTextureGPU.getObject<TextureGPU>();
	TextureGPU *materialTexture = m_hmaterialTextureGPU.getObject<TextureGPU>();

	PE::SA_Bind_Resource setTextureActionAlbedo(
		*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		albedoTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(albedoTexture->m_pShaderResourceView, albedoTexture->m_pTexture, albedoTexture->m_texture));
	setTextureActionAlbedo.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionNormal(
		*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		normalTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(normalTexture->m_pShaderResourceView, normalTexture->m_pTexture, normalTexture->m_texture));
	setTextureActionNormal.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionMaterial(
		*m_pContext, m_arena, WIND_TEXTURE_2D_SAMPLER_SLOT,
		materialTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(materialTexture->m_pShaderResourceView, materialTexture->m_pTexture, materialTexture->m_texture));
	setTextureActionMaterial.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionDepth(
		*m_pContext, m_arena, DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		API_CHOOSE_DX11_DX9_OGL(rootDepthTexture->m_pDepthShaderResourceView, rootDepthTexture->m_pTexture, rootDepthTexture->m_texture));
	setTextureActionDepth.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionClusters(
		*m_pContext, m_arena, CLUSTERED_TEXTURE_3D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		_clusterTexShaderView);
	setTextureActionClusters.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setLightIndicesStructuredBuffer(
		*m_pContext, m_arena, GpuResourceSlot_ClusteredLightIndexListResource,
		SamplerState_NotNeeded,
		_lightIndicesBufferShaderView);
	setLightIndicesStructuredBuffer.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setLocalCubemap(
		*m_pContext, m_arena, GpuResourceSlot_LocalSpecularCubemapResource,
		SamplerState_NotNeeded,
		_probeManager.getLocalCubemapPrefilterTargetSRV());
	setLocalCubemap.bindToPipeline(&curEffect);

	if (!_skybox.isSky())
	{
		PE::SA_Bind_Resource setGlobalCubemap(
			*m_pContext, m_arena, GpuResourceSlot_GlobalSpecularCubemapResource,
			SamplerState_NotNeeded,
			_skybox.getGlobalCubemapSRV(_skybox.GetCurrentCubemapId()));
		setGlobalCubemap.bindToPipeline(&curEffect);
	}


	PE::SA_Bind_Resource setIBLLut(
		*m_pContext, m_arena, GpuResourceSlot_SpecularCubemapLUTResource,
		SamplerState_NotNeeded,
		_probeManager.getPbrLutTex());
	setIBLLut.bindToPipeline(&curEffect);

	// TODO: optimize constant buffer - reduce num of const buffers

	// Quad MVP
	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;
	objSa.bindToPipeline(&curEffect);

	// cbClusteredShadingConsts
	SetClusteredShadingConstantsShaderAction pscs(*m_pContext, m_arena);
	CameraSceneNode *csn = 
		CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<CameraSceneNode>();
	float n = csn->m_near;
	float f = csn->m_far;
	float projA = f / (f - n);
	float projB = (-f * n) / (f - n);
	Vector3 camPos = csn->m_base.getPos();

	// TODO: default camera far plane is 2000 (too large, consider make it small to get better depth-pos reconstruction)
	pscs.m_data.csconsts.cNear = n;
	pscs.m_data.csconsts.cFar = f;
	pscs.m_data.csconsts.cProjA = projA;
	pscs.m_data.csconsts.cProjB = projB;
	pscs.m_data.camPos = camPos;
	pscs.m_data.camZAxisWS = csn->m_base.getN();

	pscs.m_data.dirLightNum = _dirLightNum;

	pscs.m_data.enableLocalCubemap = _enableLocalCubemap;
	pscs.m_data.enableIndirectLighting = _enableIndirectLighting;

	Vector3 size = m_cMax - m_cMin;
	Vector3 scale = Vector3(float(CX) / size.m_x, float(CY) / size.m_y, float(CZ) / size.m_z);
	pscs.m_data.scale = scale;
	Vector3 bias;
	bias.m_x = -scale.m_x * m_cMin.m_x;
	bias.m_y = -scale.m_y * m_cMin.m_y;
	bias.m_z = -scale.m_z * m_cMin.m_z;
	pscs.m_data.bias = bias;
	// pscs.m_data.bias = m_cMin;

	// Convert PE Light to shader Light (huge overhead...; 
	// but no time to change the fundamental structure of PE)
	for (unsigned int i = 0; i < _dirLightNum; i++)
	{
		pscs.m_data.dirLights[i].cDir = _dirLights[i]->m_cbuffer.dir;
		pscs.m_data.dirLights[i].cColor = _dirLights[i]->m_cbuffer.diffuse.asVector3Ref();
	}

	for (unsigned int i = 0; i < _pointLightNum; i++)
	{
		pscs.m_data.pointLights[i].cPos = _pointLights[i]->m_cbuffer.pos;
		pscs.m_data.pointLights[i].cColor = _pointLights[i]->m_cbuffer.diffuse.asVector3Ref();
		pscs.m_data.pointLights[i].cRadius = _pointLights[i]->m_cbuffer.range;
		pscs.m_data.pointLights[i].cSpecPow = _pointLights[i]->m_cbuffer.spotPower;
	}

	for (unsigned int i = 0; i < _spotLightNum; i++)
	{
		pscs.m_data.spotLights[i].cPos = _spotLights[i]->m_cbuffer.pos;
		pscs.m_data.spotLights[i].cColor = _spotLights[i]->m_cbuffer.diffuse.asVector3Ref();
		pscs.m_data.spotLights[i].cSpotCutOff = _spotLights[i]->m_cbuffer.range;
		pscs.m_data.spotLights[i].cDir = _spotLights[i]->m_cbuffer.dir;
		pscs.m_data.spotLights[i].cPos = _spotLights[i]->m_cbuffer.pos;
	}

	// memcpy(pscs.m_data.lightIndices, &_lightIndices, sizeof(short) * _lightIndices.size());

	pscs.bindToPipeline(&curEffect);

	// draw quad
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureActionAlbedo.unbindFromPipeline(&curEffect);
	setTextureActionNormal.unbindFromPipeline(&curEffect);
	setTextureActionDepth.unbindFromPipeline(&curEffect);
	setTextureActionClusters.unbindFromPipeline(&curEffect);
	setLocalCubemap.unbindFromPipeline(&curEffect);
	setIBLLut.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	pscs.unbindFromPipeline(&curEffect);
}

void EffectManager::drawDeferredFinalPass()
{
	Effect &curEffect = *m_hfinalHDRPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

#if 0
	// We could do this if more complex postprocessing is used
	m_pContext->getGPUScreen()->
		setRenderTargetsAndViewportWithNoDepth(
		m_hfinalLDRTextureGPU.getObject<TextureGPU>(), true);
#else
	// For simplicity, just draw directly into back buffer
	// TODO: with or without depth?
	// m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);////setRenderTargetsAndViewportWithDepth(0, 0, true, true)
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(m_hfinalHDRTextureGPU.getObject<TextureGPU>(), true);////setRenderTargetsAndViewportWithDepth(0, 0, true, true)
#endif

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *lightPassHDRTex = m_haccumHDRTextureGPU.getObject<TextureGPU>();
	TextureGPU *rootDepthTexture = m_hrootDepthBufferTextureGPU.getObject<TextureGPU>();
	TextureGPU *normalTexture = m_hnormalTextureGPU.getObject<TextureGPU>();
	TextureGPU *materialTexture = m_hmaterialTextureGPU.getObject<TextureGPU>();
	

	TextureGPU *rayTracingTexture = m_pContext->isSSR ? m_hrayTracingTextureGPU.getObject<TextureGPU>() : m_haccumHDRTextureGPU.getObject<TextureGPU>();

	
	//
	//TextureGPU *positionTexture = m_hpositionTextureGPU.getObject<TextureGPU>();

	PE::SA_Bind_Resource setTextureAction(
		*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		SamplerStateCustom0,//lightPassHDRTex->m_samplerState
		API_CHOOSE_DX11_DX9_OGL(lightPassHDRTex->m_pShaderResourceView, lightPassHDRTex->m_pTexture, lightPassHDRTex->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionDepth(
		*m_pContext, m_arena, DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		API_CHOOSE_DX11_DX9_OGL(rootDepthTexture->m_pDepthShaderResourceView, rootDepthTexture->m_pTexture, rootDepthTexture->m_texture));
	setTextureActionDepth.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionNormal(
		*m_pContext, m_arena, BUMPMAP_TEXTURE_2D_SAMPLER_SLOT,
		normalTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(normalTexture->m_pShaderResourceView, normalTexture->m_pTexture, normalTexture->m_texture));
	setTextureActionNormal.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionMaterial(
		*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		materialTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(materialTexture->m_pShaderResourceView, materialTexture->m_pTexture, materialTexture->m_texture));
	setTextureActionMaterial.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionRayTracing(
		*m_pContext, m_arena, WIND_TEXTURE_2D_SAMPLER_SLOT,
		rayTracingTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(rayTracingTexture->m_pShaderResourceView, rayTracingTexture->m_pTexture, rayTracingTexture->m_texture));
	setTextureActionRayTracing.bindToPipeline(&curEffect);

	/*PE::SA_Bind_Resource setTextureActionPosition(
		*m_pContext, m_arena, WIND_TEXTURE_2D_SAMPLER_SLOT,
		positionTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(positionTexture->m_pShaderResourceView, positionTexture->m_pTexture, positionTexture->m_texture));
	setTextureActionPosition.bindToPipeline(&curEffect);*/


	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	SetClusteredShadingConstantsShaderAction pscs(*m_pContext, m_arena);
	CameraSceneNode *csn =
		CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<CameraSceneNode>();
	float n = csn->m_near;
	float f = csn->m_far;
	float projA = f / (f - n);
	float projB = (-f * n) / (f - n);
	Vector3 camPos = csn->m_base.getPos();

	pscs.m_data.csconsts.cNear = n;
	pscs.m_data.csconsts.cFar = f;
	pscs.m_data.csconsts.cProjA = projA;
	pscs.m_data.csconsts.cProjB = projB;
	pscs.m_data.camPos = camPos;
	pscs.m_data.pad3 = 0.5;
	pscs.m_data.camZAxisWS = csn->m_base.getN();
	pscs.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gViewInv = csn->m_worldToViewTransform;
	cb.m_data.gViewProj = csn->m_viewToProjectedTransform;
	cb.m_data.gViewProjInverseMatrix = (csn->m_viewToProjectedTransform * csn->m_worldToViewTransform).inverse();
	cb.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	setTextureActionDepth.unbindFromPipeline(&curEffect);
	setTextureActionNormal.unbindFromPipeline(&curEffect);
	setTextureActionMaterial.unbindFromPipeline(&curEffect);
	setTextureActionRayTracing.unbindFromPipeline(&curEffect);
	//setTextureActionPosition.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	pscs.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
}

void EffectManager::drawMotionBlur()
{
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;
	
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *t = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	//todo: enable this to get motion blur working. need to make shader use depth map, not shadow map
	//PE::SA_Bind_Resource setDepthTextureAction(DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT, API_CHOOSE_DX11_DX9_OGL(m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	//setDepthTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	//todo: enable this to get motion blur working
	//setDepthTextureAction.unbindFromPipeline(&curEffect);
}

void EffectManager::drawFrameBufferCopy()
{
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();

	if (!curEffect.m_isReady)
		return;

	setFrameBufferCopyRenderTarget();

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	
	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	TextureGPU *t = m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>();
	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, t->m_samplerState, API_CHOOSE_DX11_DX9_OGL(t->m_pShaderResourceView, t->m_pTexture, t->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	}

Effect *EffectManager::operator[] (const char *pEffectFilename)
{
	return m_map.findHandle(pEffectFilename).getObject<Effect>();
}

void EffectManager::drawDeferredFinalToBackBuffer()
{
//	// use motion blur for now since it doesnt do anything but draws the surface
//	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();
//	if (!curEffect.m_isReady)
//		return;
//
//#	if APIABSTRACTION_D3D9
//	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
//	// this is called in function above: IRenderer::Instance()->getDevice()->BeginScene();
//#elif APIABSTRACTION_OGL
//	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
//#	elif APIABSTRACTION_D3D11
//	// TODO: with or without depth?
//	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);//setRenderTargetsAndViewportWithDepth(0, 0, true, true)
//#	endif
//
//	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
//	pibGPU->setAsCurrent();
//
//	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
//	pvbGPU->setAsCurrent(&curEffect);
//
//	curEffect.setCurrent(pvbGPU);
//
//	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena);
//	setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, 
//		m_hfinalLDRTextureGPU.getObject<TextureGPU>()->m_samplerState,  
//		API_CHOOSE_DX11_DX9_OGL(m_hfinalHDRTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hfinalLDRTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hfinalLDRTextureGPU.getObject<TextureGPU>()->m_texture));
//	setTextureAction.bindToPipeline(&curEffect);
//
//	PE::SetPerObjectConstantsShaderAction objSa;
//	objSa.m_data.gW = Matrix4x4();
//	objSa.m_data.gW.loadIdentity();
//	objSa.m_data.gWVP = objSa.m_data.gW;
//
//	objSa.bindToPipeline(&curEffect);
//
//	pibGPU->draw(1, 0);
//
//	pibGPU->unbindFromPipeline();
//	pvbGPU->unbindFromPipeline(&curEffect);
//
//	setTextureAction.unbindFromPipeline(&curEffect);
//	objSa.unbindFromPipeline(&curEffect);
}

// + Deferred
void EffectManager::debugDeferredRenderTarget(int which)
{
	// Effect &curEffect = *m_hfinalLDRPassEffect.getObject<Effect>();
	Effect &curEffect = *m_hdebugPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

#	if APIABSTRACTION_D3D9
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
	// this is called in function above: IRenderer::Instance()->getDevice()->BeginScene();
#elif APIABSTRACTION_OGL
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
#	elif APIABSTRACTION_D3D11
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);
#	endif

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena);

	if (which == 0)
	{
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, 
			m_halbedoTextureGPU.getObject<TextureGPU>()->m_samplerState,
			API_CHOOSE_DX11_DX9_OGL(
			m_halbedoTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, 
			m_halbedoTextureGPU.getObject<TextureGPU>()->m_pTexture,
			m_halbedoTextureGPU.getObject<TextureGPU>()->m_texture));
	}
	else if (which == 1)
	{
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			m_hnormalTextureGPU.getObject<TextureGPU>()->m_samplerState, 
			API_CHOOSE_DX11_DX9_OGL(
			m_hnormalTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, 
			m_hnormalTextureGPU.getObject<TextureGPU>()->m_pTexture,
			m_hnormalTextureGPU.getObject<TextureGPU>()->m_texture));
	}
	else if (which == 2)
	{
		// TODO: currently depth texture not working
		if (m_pContext->_renderMode == 0)
		{
			auto tex = m_hrootDepthBufferTextureGPU.getObject<TextureGPU>();
			setTextureAction.set(DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
				//tex->m_samplerState,
				SamplerState_NotNeeded,
				API_CHOOSE_DX11_DX9_OGL(
				tex->m_pDepthShaderResourceView,
				tex->m_pTexture,
				tex->m_texture));
		}
		else
		{
			setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
				m_hpositionTextureGPU.getObject<TextureGPU>()->m_samplerState,
				API_CHOOSE_DX11_DX9_OGL(
				m_hpositionTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView,
				m_hpositionTextureGPU.getObject<TextureGPU>()->m_pTexture,
				m_hpositionTextureGPU.getObject<TextureGPU>()->m_texture));
		}
	}
	else if (which == 3)
	{
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			m_haccumHDRTextureGPU.getObject<TextureGPU>()->m_samplerState,
			API_CHOOSE_DX11_DX9_OGL(
			m_haccumHDRTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView,
			m_haccumHDRTextureGPU.getObject<TextureGPU>()->m_pTexture,
			m_haccumHDRTextureGPU.getObject<TextureGPU>()->m_texture));
	}
	else if (which == 4)
	{
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, 
			m_hfinalHDRTextureGPU.getObject<TextureGPU>()->m_samplerState, 
			API_CHOOSE_DX11_DX9_OGL(
			m_hfinalHDRTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView,
			m_hfinalHDRTextureGPU.getObject<TextureGPU>()->m_pTexture,
			m_hfinalHDRTextureGPU.getObject<TextureGPU>()->m_texture));
	}

	setTextureAction.bindToPipeline(&curEffect);

	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;

	objSa.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);

	/*SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	setTextureAction.unbindFromPipeline(&curEffect);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);*/
}

void EffectManager::debugDrawRenderTarget(bool drawGlowRenderTarget, bool drawSeparatedGlow, bool drawGlow1stPass, bool drawGlow2ndPass, bool drawShadowRenderTarget)
{
	// use motion blur for now since it doesnt do anything but draws the surface
	Effect &curEffect = *m_hMotionBlurEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;
	
#	if APIABSTRACTION_D3D9
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
	// this is called in function above: IRenderer::Instance()->getDevice()->BeginScene();
#elif APIABSTRACTION_OGL
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);
#	elif APIABSTRACTION_D3D11
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);
#	endif

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);

	curEffect.setCurrent(pvbGPU);

	PE::SA_Bind_Resource setTextureAction(*m_pContext, m_arena);
	
	if (drawGlowRenderTarget)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_samplerState,  API_CHOOSE_DX11_DX9_OGL(m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	if (drawSeparatedGlow)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_glowSeparatedTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_glowSeparatedTextureGPU.m_pShaderResourceView, m_glowSeparatedTextureGPU.m_pTexture, m_glowSeparatedTextureGPU.m_texture));
	if (drawGlow1stPass)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_2ndGlowTargetTextureGPU.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_2ndGlowTargetTextureGPU.m_pShaderResourceView, m_2ndGlowTargetTextureGPU.m_pTexture, m_2ndGlowTargetTextureGPU.m_texture));
	if (drawGlow2ndPass)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_pShaderResourceView, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_pTexture, m_hFinishedGlowTargetTextureGPU.getObject<TextureGPU>()->m_texture));
	if (drawShadowRenderTarget)
		setTextureAction.set(DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, m_shadowMapDepthTexture.m_samplerState, API_CHOOSE_DX11_DX9_OGL(m_shadowMapDepthTexture.m_pShaderResourceView, m_shadowMapDepthTexture.m_pTexture, m_shadowMapDepthTexture.m_texture));
	
	setTextureAction.bindToPipeline(&curEffect);
	
	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gPreviousViewProjMatrix = m_previousViewProjMatrix;
	cb.m_data.gViewProjInverseMatrix = m_currentViewProjMatrix.inverse();
	cb.m_data.gDoMotionBlur = m_doMotionBlur;

	cb.bindToPipeline();

	pibGPU->draw(1, 0);
	m_previousViewProjMatrix = m_currentViewProjMatrix;

	setTextureAction.unbindFromPipeline(&curEffect);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);
}

void EffectManager::createSkyBoxGeom()
{
	static const int NumIndices = 36;
	static const int NumVertices = 8;

	Vector3 verts[NumVertices] =
	{
		Vector3(-1, 1, 1),
		Vector3(1, 1, 1),
		Vector3(1, -1, 1),
		Vector3(-1, -1, 1),
		Vector3(1, 1, -1),
		Vector3(-1, 1, -1),
		Vector3(-1, -1, -1),
		Vector3(1, -1, -1),
	};

	unsigned short indices[NumIndices] =
	{
		0, 1, 2, 2, 3, 0,   // Front
		1, 4, 7, 7, 2, 1,   // Right
		4, 5, 6, 6, 7, 4,   // Back
		5, 0, 3, 3, 6, 5,   // Left
		5, 4, 1, 1, 0, 5,   // Top
		3, 2, 7, 7, 6, 3    // Bottom
	};


	PositionBufferCPU vbcpu(*m_pContext, m_arena);
	vbcpu.m_values.reset(NumIndices * 3);
	for (int i = 0; i < NumVertices; i++)
	{
		vbcpu.m_values.add(verts[i].m_x);
		vbcpu.m_values.add(verts[i].m_y);
		vbcpu.m_values.add(verts[i].m_z);
	}

	ColorBufferCPU tcbcpu(*m_pContext, m_arena);
	tcbcpu.m_values.reset(NumIndices);
	for (int i = 0; i < NumIndices; i++)
	{
		tcbcpu.m_values.add(0.5f);
	}

	IndexBufferCPU ibcpu(*m_pContext, m_arena);
	ibcpu.m_values.reset(NumIndices);
	for (int i = 0; i < NumIndices; i++)
	{
		ibcpu.m_values.add(indices[i]);
	}
	ibcpu.m_indexRanges.reset(1);
	ibcpu.m_vertsPerFacePerRange.reset(1);
	IndexRange range(*m_pContext, m_arena);
	range.m_start = 0;
	range.m_end = NumIndices;
	range.m_minVertIndex = 0;
	range.m_maxVertIndex = NumVertices;
	ibcpu.m_indexRanges.add(range);
	ibcpu.m_vertsPerFacePerRange.add(3);
	ibcpu.m_primitiveTopology = PEPrimitveTopology_TRIANGLES;
	ibcpu.m_minVertexIndex = range.m_minVertIndex;
	ibcpu.m_maxVertexIndex = range.m_maxVertIndex;
	ibcpu.m_verticesPerPolygon = 3;

	m_hSkyBoxGeomIBGpu = Handle("INDEX_BUFFER_GPU", sizeof(IndexBufferGPU));
	IndexBufferGPU *pibgpu = new(m_hSkyBoxGeomIBGpu)IndexBufferGPU(*m_pContext, m_arena);
	pibgpu->createGPUBuffer(ibcpu);

	m_hSkyBoxGeomVBGpu = Handle("VERTEX_BUFFER_GPU", sizeof(VertexBufferGPU));
	VertexBufferGPU *pvbgpu = new(m_hSkyBoxGeomVBGpu)VertexBufferGPU(*m_pContext, m_arena);
	pvbgpu->createGPUBufferFromSource_ColoredMinimalMesh(vbcpu, tcbcpu);
}

//Liu
void EffectManager::createSphere(float radius, int sliceCount, int stackCount)
{
	

	PositionBufferCPU vbcpu(*m_pContext, m_arena);
	vbcpu.createSphereForDeferred(radius,sliceCount,stackCount);

	ColorBufferCPU tcbcpu(*m_pContext, m_arena);
	tcbcpu.m_values.reset(3 * 401);
	for (int i=0;i<3*401;i++)
	{
		tcbcpu.m_values.add(0.5f);
	}
	
	IndexBufferCPU ibcpu(*m_pContext, m_arena);
	ibcpu.createSphereCPUBuffer();
		
	m_hLightIndexBufferGPU = Handle("INDEX_BUFFER_GPU", sizeof(IndexBufferGPU));
	IndexBufferGPU *pibgpu = new(m_hLightIndexBufferGPU) IndexBufferGPU(*m_pContext, m_arena);
	pibgpu->createGPUBuffer(ibcpu);
	

	m_hLightVertexBufferGPU = Handle("VERTEX_BUFFER_GPU", sizeof(VertexBufferGPU));
	VertexBufferGPU *pvbgpu = new(m_hLightVertexBufferGPU) VertexBufferGPU(*m_pContext, m_arena);
	pvbgpu->createGPUBufferFromSource_ColoredMinimalMesh(vbcpu, tcbcpu);
}

void EffectManager::renderCubemapConvolutionSphere()
{
	Effect &curEffect = *m_hCubemapPrefilterPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	IndexBufferGPU *pibGPU = m_hLightIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hLightVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrent(pvbGPU);
	pibGPU->draw(1, 0);
	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);
}

void EffectManager::renderSkyboxNewSphere(int isSky)
{
	Handle hSkyboxEffect;
	if (isSky)
	{
		hSkyboxEffect = getEffectHandle("SkyboxNewTech");
	}
	else
	{
		hSkyboxEffect = getEffectHandle("SkyboxNewCubemapTech");
	}

	Effect &curEffect = *hSkyboxEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	IndexBufferGPU *pibGPU = m_hSkyBoxGeomIBGpu.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hSkyBoxGeomVBGpu.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrentShaderOnly(pvbGPU);
	pibGPU->draw(1, 0);
	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);
}
//Liu
void EffectManager::drawLightGbuffer()
{
	auto &lights = PE::RootSceneNode::Instance()->m_lights;

	for (int i = 0; i < lights.m_size; i++)
	{
		Light *l = lights[i].getObject<Light>();
		if (l->m_cbuffer.type != 0) continue;
		float scale = 0.1f;
		Vector3 translation = l->m_base.getPos();

		Effect &curEffect = *m_hGBufferLightPassEffect.getObject<Effect>();
		if (!curEffect.m_isReady)
			return;

		IndexBufferGPU *pibGPU = m_hLightIndexBufferGPU.getObject<IndexBufferGPU>();
		pibGPU->setAsCurrent();


		VertexBufferGPU *pvbGPU = m_hLightVertexBufferGPU.getObject<VertexBufferGPU>();
		pvbGPU->setAsCurrent(&curEffect);

		curEffect.setCurrent(pvbGPU);

		PE::SetPerObjectConstantsShaderAction objSa;
		Matrix4x4 scaleM = Matrix4x4();
		scaleM.loadIdentity();
		scaleM.importScale(scale, scale, scale);

		Matrix4x4 rotationM = Matrix4x4();
		rotationM.loadIdentity();

		Matrix4x4 translationM = Matrix4x4();
		translationM.loadIdentity();
		translationM.setPos(translation );//- Vector3(0, 0f, 0)

		objSa.m_data.gW = Matrix4x4();
		objSa.m_data.gW.loadIdentity();
		objSa.m_data.gW = translationM *rotationM* scaleM;

		objSa.m_data.gWVP = m_currentViewProjMatrix*objSa.m_data.gW;
		objSa.bindToPipeline(&curEffect);

		pibGPU->draw(1, 0);

		pibGPU->unbindFromPipeline();
		pvbGPU->unbindFromPipeline(&curEffect);

		objSa.unbindFromPipeline(&curEffect);
	
	}
}

//Liu
void EffectManager::drawClassicalLightPass(float angle)
{
	auto &lights = PE::RootSceneNode::Instance()->m_lights;


	for (int i = 0; i < lights.m_size; i++)
	{
		Light *l = lights[i].getObject<Light>();
		if (l->m_cbuffer.type != 0) continue;

		
		//Vector3 translation = m_lights[i].pos;
		//Vector4 difuss = Vector4(m_lights[i].color.getX(), m_lights[i].color.getY(), m_lights[i].color.getZ(), 1);
		Vector3 randomAxis;
		//randomizeLight(l, &randomAxis);
		float scale = l->m_cbuffer.range;

		Vector3 translation = l->m_base.getPos();
		Vector4 &difuss = l->m_cbuffer.diffuse;

		Effect &curEffect = *m_hDeferredLightPassEffect.getObject<Effect>();
		if (!curEffect.m_isReady)
			return;

		IndexBufferGPU *pibGPU = m_hLightIndexBufferGPU.getObject<IndexBufferGPU>();
		pibGPU->setAsCurrent();
		

		VertexBufferGPU *pvbGPU = m_hLightVertexBufferGPU.getObject<VertexBufferGPU>();
		pvbGPU->setAsCurrent(&curEffect);
		
		curEffect.setCurrent(pvbGPU);


		TextureGPU *albedoTexture = m_halbedoTextureGPU.getObject<TextureGPU>();
		TextureGPU *normalTexture = m_hnormalTextureGPU.getObject<TextureGPU>();
		TextureGPU *positionTexture = m_hpositionTextureGPU.getObject<TextureGPU>();

		PE::SA_Bind_Resource setTextureActionAlbedo(
			*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			albedoTexture->m_samplerState,
			API_CHOOSE_DX11_DX9_OGL(albedoTexture->m_pShaderResourceView, albedoTexture->m_pTexture, albedoTexture->m_texture));
		setTextureActionAlbedo.bindToPipeline(&curEffect);


		PE::SA_Bind_Resource setTextureActionNormal(
			*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			normalTexture->m_samplerState,
			API_CHOOSE_DX11_DX9_OGL(normalTexture->m_pShaderResourceView, normalTexture->m_pTexture, normalTexture->m_texture));
		setTextureActionNormal.bindToPipeline(&curEffect);

		PE::SA_Bind_Resource setTextureActionPosition(
			*m_pContext, m_arena, WIND_TEXTURE_2D_SAMPLER_SLOT,
			positionTexture->m_samplerState,
			API_CHOOSE_DX11_DX9_OGL(positionTexture->m_pShaderResourceView, positionTexture->m_pTexture, positionTexture->m_texture));
		setTextureActionPosition.bindToPipeline(&curEffect);

		CameraSceneNode *csn = CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<CameraSceneNode>();

		//OutputDebugStringA("");

		PE::SetPerObjectConstantsShaderAction objSa;
		Matrix4x4 scaleM = Matrix4x4();
		scaleM.loadIdentity();
		scaleM.importScale(scale, scale, scale);

		Matrix4x4 rotationM =Matrix4x4();
		rotationM.loadIdentity();
		rotationM.turnAboutAxis(angle, l->m_oribitAxis);
		//l->m_base.
		//l->m_base = rotationM* l->m_base;

		Matrix4x4 translationM = Matrix4x4();
		translationM.loadIdentity();
		translationM.setPos(translation);

		objSa.m_data.gW = Matrix4x4();
		objSa.m_data.gW.loadIdentity();
		//objSa.m_data.gW = rotationM*scaleM*translationM;
		objSa.m_data.gW = translationM *rotationM* scaleM;

		objSa.m_data.gWVP = m_currentViewProjMatrix*objSa.m_data.gW;//csn->m_worldToViewTransform ;//*m_currentViewProjMatrix;//
		//objSa.m_data.gWVPInverse = objSa.m_data.gWVP.inverse();
		objSa.bindToPipeline(&curEffect);

		SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
		// cb.m_data.gLights[0].pos = objSa.m_data.gW.getPos();
		cb.m_data.gLights[0].pos = l->m_base.getPos();
		cb.m_data.gLights[0].diffuse = difuss;
		cb.m_data.gLights[0].range = scale;
		cb.m_data.gLights[0].spotPower = l->m_cbuffer.spotPower;
		cb.bindToPipeline(&curEffect);

		pibGPU->draw(1, 0);

		pibGPU->unbindFromPipeline();
		pvbGPU->unbindFromPipeline(&curEffect);

		setTextureActionAlbedo.unbindFromPipeline(&curEffect);
		setTextureActionNormal.unbindFromPipeline(&curEffect);
		setTextureActionPosition.unbindFromPipeline(&curEffect);

		objSa.unbindFromPipeline(&curEffect);
		cb.unbindFromPipeline(&curEffect);
	}
}

//Liu
void EffectManager::drawLightMipsPass(int curlevel, bool isSecBlur = false)
{

	Effect &curEffect = *m_hLightMipsPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	//m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(0, true);

	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrent(pvbGPU);

	TextureGPU *lightPassHDRTex = m_htempMipsTextureGPU.getObject<TextureGPU>();
	
	PE::SA_Bind_Resource setTextureActionLight(
		*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		lightPassHDRTex->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(lightPassHDRTex->m_pShaderResourceView, lightPassHDRTex->m_pTexture, lightPassHDRTex->m_texture));
	setTextureActionLight.bindToPipeline(&curEffect);

	// Quad MVP
	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;
	objSa.bindToPipeline(&curEffect);

	SetClusteredShadingConstantsShaderAction pscs(*m_pContext, m_arena);
	if (isSecBlur)
	{
		pscs.m_data.csconsts.cNear = 1;
	}
	else
	{
		pscs.m_data.csconsts.cNear = 0;
	}
	pscs.m_data.csconsts.cFar = curlevel;
	pscs.bindToPipeline(&curEffect);

	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureActionLight.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	pscs.unbindFromPipeline(&curEffect);
}

Vector4 CreateInvDeviceZToWorldZTransformA(Matrix4x4 & ProjMatrix)
{
	float DepthMul = ProjMatrix.m[2][2];
	float DepthAdd = ProjMatrix.m[2][3];

	if (DepthAdd == 0.f)
	{
		// Avoid dividing by 0 in this case
		DepthAdd = 0.00000001f;
	}

	float SubtractValue = DepthMul / DepthAdd;

	// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
	// This fixes fog not being applied to the black background in the editor.
	SubtractValue -= 0.00000001f;

	return Vector4(
		0.0f,			// Unused
		0.0f,			// Unused
		1.f / DepthAdd,
		SubtractValue
		);
}

void EffectManager::drawRayTracingPass()
{
	Effect &curEffect = *m_hRayTracingPassEffect.getObject<Effect>();
	if (!curEffect.m_isReady)
		return;

	//setrendertarget
	TextureGPU* rayTracingTex = m_hrayTracingTextureGPU.getObject<TextureGPU>();
	m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithNoDepth(rayTracingTex, true);
	m_pCurRenderTarget = m_haccumHDRTextureGPU.getObject<TextureGPU>();

	//
	IndexBufferGPU *pibGPU = m_hIndexBufferGPU.getObject<IndexBufferGPU>();
	pibGPU->setAsCurrent();

	VertexBufferGPU *pvbGPU = m_hVertexBufferGPU.getObject<VertexBufferGPU>();
	pvbGPU->setAsCurrent(&curEffect);
	curEffect.setCurrent(pvbGPU);

	TextureGPU *lightPassHDRTex = m_haccumHDRTextureGPU.getObject<TextureGPU>();
	TextureGPU *rootDepthTexture = m_hrootDepthBufferTextureGPU.getObject<TextureGPU>();
	TextureGPU *normalTexture = m_hnormalTextureGPU.getObject<TextureGPU>();
	TextureGPU *materialTexture = m_hmaterialTextureGPU.getObject<TextureGPU>();

	PE::SA_Bind_Resource setTextureAction(
		*m_pContext, m_arena, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		lightPassHDRTex->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(lightPassHDRTex->m_pShaderResourceView, lightPassHDRTex->m_pTexture, lightPassHDRTex->m_texture));
	setTextureAction.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionDepth(
		*m_pContext, m_arena, DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
		SamplerState_NotNeeded,
		API_CHOOSE_DX11_DX9_OGL(rootDepthTexture->m_pDepthShaderResourceView, rootDepthTexture->m_pTexture, rootDepthTexture->m_texture));
	setTextureActionDepth.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionNormal(
		*m_pContext, m_arena, BUMPMAP_TEXTURE_2D_SAMPLER_SLOT,
		normalTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(normalTexture->m_pShaderResourceView, normalTexture->m_pTexture, normalTexture->m_texture));
	setTextureActionNormal.bindToPipeline(&curEffect);

	PE::SA_Bind_Resource setTextureActionMaterial(
		*m_pContext, m_arena, ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		materialTexture->m_samplerState,
		API_CHOOSE_DX11_DX9_OGL(materialTexture->m_pShaderResourceView, materialTexture->m_pTexture, materialTexture->m_texture));
	setTextureActionMaterial.bindToPipeline(&curEffect);

	// Quad MVP
	PE::SetPerObjectConstantsShaderAction objSa;
	objSa.m_data.gW = Matrix4x4();
	objSa.m_data.gW.loadIdentity();
	objSa.m_data.gWVP = objSa.m_data.gW;
	objSa.bindToPipeline(&curEffect);

	SetClusteredShadingConstantsShaderAction pscs(*m_pContext, m_arena);
	CameraSceneNode *csn =
		CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<CameraSceneNode>();
	float n = csn->m_near;
	float f = csn->m_far;
	float projA = f / (f - n);
	float projB = (-f * n) / (f - n);
	Vector3 camPos = csn->m_base.getPos();

	pscs.m_data.csconsts.cNear = n;
	pscs.m_data.csconsts.cFar = f;
	pscs.m_data.csconsts.cProjA = projA;
	pscs.m_data.csconsts.cProjB = projB;
	pscs.m_data.camPos = camPos;
	pscs.m_data.camZAxisWS = csn->m_base.getN();
	Vector4 bias = CreateInvDeviceZToWorldZTransformA(csn->m_viewToProjectedTransform);
	pscs.m_data.bias = Vector3(bias.m_x, bias.m_y, bias.m_z);
	pscs.m_data.pad3 = bias.m_w;
	pscs.bindToPipeline(&curEffect);

	SetPerObjectGroupConstantsShaderAction cb(*m_pContext, m_arena);
	cb.m_data.gViewInv = csn->m_worldToViewTransform;
	cb.m_data.gViewProj = csn->m_viewToProjectedTransform;
	cb.m_data.gViewProjInverseMatrix = (csn->m_viewToProjectedTransform * csn->m_worldToViewTransform).inverse();
	cb.bindToPipeline(&curEffect);

	
	pibGPU->draw(1, 0);

	pibGPU->unbindFromPipeline();
	pvbGPU->unbindFromPipeline(&curEffect);

	setTextureAction.unbindFromPipeline(&curEffect);
	setTextureActionDepth.unbindFromPipeline(&curEffect);
	setTextureActionNormal.unbindFromPipeline(&curEffect);
	setTextureActionMaterial.unbindFromPipeline(&curEffect);
	objSa.unbindFromPipeline(&curEffect);
	pscs.unbindFromPipeline(&curEffect);
	cb.unbindFromPipeline(&curEffect);
}

void EffectManager::updateLightDirection(Vector3 sprinkleDir)
{
	auto &lights = PE::RootSceneNode::Instance()->m_lights;

	Vector3 box = Vector3(12,10,6);

	for (int i = 0; i < lights.m_size-1; i++)
	{
		Light *l = lights[i].getObject<Light>();
		if (l->m_cbuffer.type != 0) continue;

		Vector3 pos = l->m_base.getPos();
		Vector3 dir = l->m_oribitAxis;

		l->m_physicsRest += 0.03f;

		if (l->m_physicsRest >= 4.0){
			// Restart animation
			l->m_oribitAxis = sprinkleDir;
			l->m_base.setPos(Vector3(0, 2, 0));
			l->m_physicsRest -= 4.0;
		}
		else {
			// Gravity
			l->m_oribitAxis.m_y -=  25 * 0.03f;//500.0 *

			// Update position
			l->m_base.setPos(l->m_base.getPos()+(0.03f * l->m_oribitAxis));

			Vector3 newdir = Vector3(abs(l->m_oribitAxis.m_x), abs(l->m_oribitAxis.m_y), abs(l->m_oribitAxis.m_z));
			//l->m_oribitAxis.m_y<0 ? -0.8 * abs(l->m_oribitAxis)
			
			l->m_oribitAxis = (pos.m_x > box.m_x || pos.m_y > box.m_y || pos.m_z > box.m_z) ? -0.8f * newdir : l->m_oribitAxis;
			l->m_oribitAxis = (-pos.m_x > box.m_x || -pos.m_y > 0 || -pos.m_z > box.m_z) ? 0.8f * newdir : l->m_oribitAxis;
			
			float a = 0;
		}
	}
}
//Liu
void EffectManager::randomLightInfo(int num)
{
	RootSceneNode::Instance()->m_lights.reset(num + 1,true);
	for (int i=0; i<num; i++)
	{	
		Handle hLight("LIGHT", sizeof(Light));

		Light *pLight = new(hLight) Light(
		*m_pContext, 
		m_arena,
		hLight,
		Vector3(0,-i-100,0), //Position
		Vector3(0,0,0), 
		Vector3(0,0,0), 
		Vector3(0,0,0), //Direction (z-axis)
		Vector4(0,0,0,1), //Ambient
		Vector4(1,0,1,1), //Diffuse
		Vector4(0,0,0,1), //Specular
		Vector3(0.05, 0.05, 0.05), //Attenuation (x, y, z)
		1, // Spot Power
		5, //Range
		false, //Whether or not it casts shadows
		0,//0 = point, 1 = directional, 2 = spot
		Vector3(0,0,0),
		i*(-0.1f)
		);
	
	randomizeLight(pLight,&(pLight->m_oribitAxis) ,i);
	pLight->addDefaultComponents();
	
	// diffuse multiplyer
	pLight->m_cbuffer.diffuse = pLight->m_cbuffer.diffuse * 1000;

	RootSceneNode::Instance()->m_lights.add(hLight);
	RootSceneNode::Instance()->addComponent(hLight);

	}
}

void EffectManager::updateLight()
{
	auto &lights = PE::RootSceneNode::Instance()->m_lights;
	for (int i = 0; i < lights.m_size; i++)
	{
		Handle hl = lights[i];
		Light *l = hl.getObject<Light>();
		if (l->m_cbuffer.type == 1)
		{
			if (_skybox.isSky())
			{
				l->_showLight = true;
				Vector3 dir = _skybox.GetSunDirection();
				Vector3 color = _skybox.GetSunColor() * 20;
				Vector4 colorVec4 = Vector4(color.m_x, color.m_y, color.m_z, 1.0f);
				l->m_cbuffer.dir = dir;
				l->m_cbuffer.diffuse = colorVec4;
			}
			else
			{
				l->_showLight = false;
			}
		}
	}
}

void EffectManager::changeRoughness(int curModel, bool isIncrease)
{

	auto &testModels = RootSceneNode::Instance()->m_testmodels;
	int a = testModels.m_size;
	TestModel *l = testModels[curModel].getObject<TestModel>();
	if (isIncrease)
	{
		l->m_roughness += 0.05f;
		l->m_roughness = min(l->m_roughness, 1.0f);
	}
	else
	{
		l->m_roughness -= 0.05f;
		l->m_roughness = max(l->m_roughness, 0.01f);
	}

}

void EffectManager::changeModel(int curModel, int preModel)
{
	auto &testModels = RootSceneNode::Instance()->m_testmodels;
	int a = testModels.m_size;
	TestModel *l = testModels[curModel].getObject<TestModel>();
	l->m_sceneNode->m_base.setPos(Vector3(0, 0, 0));

	TestModel *l1 = testModels[preModel].getObject<TestModel>();
	l1->m_sceneNode->m_base.setPos(Vector3(0, -100, 0));
}

void EffectManager::randomizeLight(Light *l, Vector3 *axis, int i)
{
	/*if(i<2)
	{
		l->m_base.setPos(Vector3(rand() % 5, rand() % 5, rand() % 5));
	}else
	{
		l->m_base.setPos(Vector3(rand() % 50 - 25, 3, rand() % 50 - 25));
	}*/
	
	l->m_cbuffer.diffuse = Vector4((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), 1.0);
	//*axis = Vector3((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)));
	l->m_cbuffer.range = 1.0f;
	
}
//Liu
void EffectManager::rotateLight(float angle, int counter)
{
	
	int a = PE::RootSceneNode::Instance()->getComponentCount();
	for (int b = 0; b < a;b++)
	{
		if (PE::RootSceneNode::Instance()->getComponentByIndex(b).getObject<SceneNode>() != NULL)
			PE::RootSceneNode::Instance()->getComponentByIndex(b).getObject<SceneNode>()->m_base.turnRight(0.1f);
	}
	//int b = 0;
	/*
	auto &lights = PE::RootSceneNode::Instance()->m_lights;
	for (int i = 0; i < lights.m_size; i++)
	{
		Light *l = lights[i].getObject<Light>();
		if (l->m_cbuffer.type != 0) continue;
		
		Matrix4x4 rotationM =Matrix4x4();
		rotationM.loadIdentity();
		rotationM.turnAboutAxis(angle, l->m_oribitAxis);
		//l->m_base.
		l->m_base = rotationM* l->m_base;
		//if ((counter) % 9 == 0)
		//{
		//	l->m_cbuffer.diffuse = Vector4((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)), 1.0);
		//}
		
	}

	*/
}


}; // namespace PE
