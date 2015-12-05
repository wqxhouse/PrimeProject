#include "ProbeManager.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/Mesh.h"
#include "PrimeEngine/Render/D3D11Renderer.h"

#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/Scene/DrawList.h"
#include "PrimeEngine/Math/CameraOps.h"

#include <math.h>
#include <DirectXMath.h>

using namespace PE;
using namespace PE::Components;

ProbeManager::CameraStruct ProbeManager::DefaultCubemapCameraStruct[6] = {
		{ Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) },//Left
		{ Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) },//Right
		{ Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) },//Up
		{ Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },//Down
		{ Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) }, //Center
		{ Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) },//Back
};

Matrix4x4 ProbeManager::_cameraMatrices[7];

inline UINT NumMipLevels(UINT width, UINT height)
{
	UINT numMips = 0;
	UINT size = std::max(width, height);
	while (1U << numMips <= size)
		++numMips;

	if (1U << numMips < size)
		++numMips;

	return numMips;
}

void ProbeManager::Initialize(PE::GameContext *context, PE::MemoryArena arena)
{
	_pContext = context;
	_arena = arena;

	PE::D3D11Renderer *pD3D11Renderer = static_cast<PE::D3D11Renderer *>(_pContext->getGPUScreen());
	_context = pD3D11Renderer->m_pD3DContext;
	_device = pD3D11Renderer->m_pD3DDevice;

	_cubemapSize = 128;

	_cubemapTarget0.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1,
		1, 0, FALSE, FALSE, 6, TRUE);

	_cubemapTarget1.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 1,
		1, 0, FALSE, FALSE, 6, TRUE);

	_cubemapTarget2.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1,
		1, 0, FALSE, FALSE, 6, TRUE);

	_cubemapFinalTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 1,
		1, 0, FALSE, FALSE, 6, TRUE);

	_cubemapDepthTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_D32_FLOAT, true, 1,
		false, 6, TRUE);

	_cubemapPrefilterTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 0,
		1, 0, TRUE, FALSE, 6, TRUE);

	_convolvePSConsts.Initialize(_device);

	prepareDefaultCameras();

	D3D11_SAMPLER_DESC sampDesc;

	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.MipLODBias = 0.0f;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.BorderColor[0] = sampDesc.BorderColor[1] = sampDesc.BorderColor[2] = sampDesc.BorderColor[3] = 0;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	DXCall(_device->CreateSamplerState(&sampDesc, &_linearSampler));

	_probePosition = Vector3(0, 2, 0);

	int numMips = NumMipLevels(_cubemapSize, _cubemapSize);
	_convolveMipmapRTs.clear();
	_convolveMipmapRTs.resize(6);
	for (int i = 0; i < 6; i++)
	{
		_convolveMipmapRTs[i].resize(numMips);
	}

	for (int j = 0; j < 6; j++)
	{
		for (int i = 0; i < numMips; i++)
		{
			ID3D11RenderTargetViewPtr rtView;
			D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
			rtDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtDesc.Texture2DArray.ArraySize = 1;
			rtDesc.Texture2DArray.FirstArraySlice = j;
			rtDesc.Texture2DArray.MipSlice = i;

			DXCall(_device->CreateRenderTargetView(_cubemapPrefilterTarget.Texture, &rtDesc, &rtView));
			_convolveMipmapRTs[j][i] = rtView;
		}
	}
}

void ProbeManager::prepareDefaultCameras()
{
	Handle hSN("SceneNode", sizeof(SceneNode));
	SceneNode *sn = new(hSN)SceneNode(*_pContext, _arena, hSN);
	sn->addDefaultComponents();

	for (int i = 0; i < 6; i++)
	{
		_cameraMatrices[i] = genCamViewMatrix(Vector3(0, 0, 0), DefaultCubemapCameraStruct[i].LookAt, DefaultCubemapCameraStruct[i].Up);
	}

	_cameraMatrices[6] = genCamProjMatrix();
}

void ProbeManager::SetProbePosition(const Vector3 &pos)
{
	_probePosition = pos;
}

void ProbeManager::Render(int &threadOwnershipMask)
{
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(_cubemapSize);
	viewport.Height = static_cast<float>(_cubemapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	_context->RSSetViewports(1, &viewport);

	{
		PIXEvent event(L"Render Cubemap GBuffer");
		renderGBuffer(threadOwnershipMask);
	}

	{
		PIXEvent event(L"Render convolution");
		renderConvolution();
	}
}

void ProbeManager::renderConvolution()
{
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// srv
	ID3D11ShaderResourceView *srv[1] = { _cubemapFinalTarget.SRView };
	_context->PSSetShaderResources(0, 1, srv);

	ID3D11SamplerState* sampStates[1] = {
		_linearSampler
	};

	_context->PSSetSamplers(0, 1, sampStates);

	int numMips = NumMipLevels(_cubemapSize, _cubemapSize);
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(_cubemapSize);
	viewport.Height = static_cast<float>(_cubemapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;


	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		viewport.Width = static_cast<float>(_cubemapSize);
		viewport.Height= static_cast<float>(_cubemapSize);
			
		for (int mipLevel = 0; mipLevel < numMips; mipLevel++)
		{
			_context->RSSetViewports(1, &viewport);

			// ID3D11RenderTargetViewPtr RTView = _cubemapPrefilterTarget.RTVArraySlices.at(cubeboxFaceIndex);
			ID3D11RenderTargetViewPtr RTView = _convolveMipmapRTs[cubeboxFaceIndex][mipLevel];
			ID3D11DepthStencilViewPtr DSView = _cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);

			_context->ClearRenderTargetView(RTView, clearColor);
			_context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

			ID3D11RenderTargetView *renderTarget[1] = { RTView };
			Matrix4x4 viewProj = _cameraMatrices[6] * _cameraMatrices[cubeboxFaceIndex];
			Matrix4x4 viewInv = _cameraMatrices[cubeboxFaceIndex].inverse();

			_context->OMSetRenderTargets(1, renderTarget, DSView);

			Handle convolutionEffect = EffectManager::Instance()->getEffectHandle("CubemapPrefilterTech");
			Effect *effect = convolutionEffect.getObject<Effect>();
			PE::SetPerObjectConstantsShaderAction objSa;
			objSa.m_data.gWVP = _cameraMatrices[6] * _cameraMatrices[cubeboxFaceIndex];
			objSa.bindToPipeline(effect);

			_convolvePSConsts.Data.mipLevel = mipLevel;
			_convolvePSConsts.Data.numMips = numMips;
			_convolvePSConsts.ApplyChanges(_context);
			_convolvePSConsts.SetPS(_context, 0);

			EffectManager::Instance()->renderCubemapConvolutionSphere();

			viewport.Width = static_cast<float>((int)viewport.Width >> 1);
			viewport.Height = viewport.Width;
		}
	}

	srv[0] = nullptr;
	_context->PSSetShaderResources(0, 1, srv);

	sampStates[0] = nullptr;
	_context->PSSetSamplers(0, 1, sampStates);

	// end render targets
	ID3D11RenderTargetView *nullRenderTarget[1] = { nullptr };
	_context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
}

void ProbeManager::renderGBuffer(int &threadOwnershipMask)
{
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		ID3D11RenderTargetViewPtr RTView0 = _cubemapTarget0.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11RenderTargetViewPtr RTView1 = _cubemapTarget1.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11RenderTargetViewPtr RTView2 = _cubemapTarget2.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = _cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);

		_context->ClearRenderTargetView(RTView0, clearColor);
		_context->ClearRenderTargetView(RTView1, clearColor);
		_context->ClearRenderTargetView(RTView2, clearColor);
		_context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		
		ID3D11RenderTargetView *renderTarget[3] = { RTView0, RTView1, RTView2 };

		Matrix4x4 viewInv = _cameraMatrices[cubeboxFaceIndex].inverse();
		Matrix4x4 translate;
		translate.setPos(_probePosition);
		Matrix4x4 camWorld = translate * viewInv;
		Matrix4x4 view = camWorld.inverse();
		Matrix4x4 viewProj = _cameraMatrices[6] * view;
		viewInv = camWorld;

		_context->OMSetRenderTargets(3, renderTarget, DSView);

		DrawList::InstanceReadOnly()->optimize();
		_pContext->getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);
		DrawList::InstanceReadOnly()->do_RENDER(*(Events::Event **)&cubeboxFaceIndex, threadOwnershipMask, &viewProj, &viewInv);
		_pContext->getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);

		// end render targets
		ID3D11RenderTargetView *nullRenderTarget[3] = { nullptr, nullptr, nullptr };
		_context->OMSetRenderTargets(3, nullRenderTarget, nullptr);

		// Shading
		ID3D11RenderTargetViewPtr RTView = _cubemapFinalTarget.RTVArraySlices.at(cubeboxFaceIndex);
		_context->ClearRenderTargetView(RTView, clearColor);
		ID3D11RenderTargetView *renderTarget2[1] = { RTView };
		_context->OMSetRenderTargets(1, renderTarget2, nullptr);

		// TODO: this is huge overhead
		EffectManager::Instance()->uploadDeferredClusteredConstants(0.1f, 500.0f);
		EffectManager::Instance()->drawClusteredQuadOnly(
			_cubemapDepthTarget.SRVArraySlices[cubeboxFaceIndex],
			_cubemapTarget0.SRVArraySlices[cubeboxFaceIndex], 
			_cubemapTarget1.SRVArraySlices[cubeboxFaceIndex],
			_cubemapTarget2.SRVArraySlices[cubeboxFaceIndex]);
	}

	
	// end render targets
	ID3D11RenderTargetView *nullRenderTarget[1] = { nullptr  };
	_context->OMSetRenderTargets(1, nullRenderTarget, nullptr);
}


void ProbeManager::createSphere(float radius, int sliceCount, int stackCount)
{
	PositionBufferCPU vbcpu(*_pContext, _arena);
	vbcpu.createSphereForDeferred(radius, sliceCount, stackCount);

	IndexBufferCPU ibcpu(*_pContext, _arena);
	ibcpu.createSphereCPUBuffer();

	ColorBufferCPU tcbcpu(*_pContext, _arena);
	tcbcpu.m_values.reset(3 * 401);
	for (int i = 0; i < 3 * 401; i++)
	{
		tcbcpu.m_values.add(1.0f);
	}

	_sphereIndexBuf = PE::Handle("INDEX_BUFFER_GPU", sizeof(PE::IndexBufferGPU));
	PE::IndexBufferGPU *pibgpu = new(_sphereIndexBuf)PE::IndexBufferGPU(*_pContext, _arena);
	pibgpu->createGPUBuffer(ibcpu);


	_sphereVertexBuf = PE::Handle("VERTEX_BUFFER_GPU", sizeof(PE::VertexBufferGPU));
	PE::VertexBufferGPU *pvbgpu = new(_sphereVertexBuf)PE::VertexBufferGPU(*_pContext, _arena);
	pvbgpu->createGPUBufferFromSource_ColoredMinimalMesh(vbcpu, tcbcpu);
}

Matrix4x4 ProbeManager::genCamViewMatrix(Vector3 &eye, Vector3 &lookAt, Vector3 up)
{
	//Matrix4x4 world;
	//Vector3 z = lookAt - eye;
	//Vector3 x = up.crossProduct(z);
	//Vector3 y = z.crossProduct(x);

	//world.setU(x);
	//world.setV(y);
	//world.setN(z);
	//
	//// return world.inverse();
	//return world;

	// return CameraOps::CreateViewMatrix(eye, lookAt, up);
	DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(eye.m_x, eye.m_y, eye.m_z, 1.0f);
	DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(lookAt.m_x, lookAt.m_y, lookAt.m_z, 1.0f);
	DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(up.m_x, up.m_y, up.m_z, 0.0f);
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	Matrix4x4 viewMat;
	memcpy(&viewMat, &view, sizeof(Matrix4x4));
	return viewMat.transpose();
}

Matrix4x4 ProbeManager::genCamProjMatrix()
{
	// return CameraOps::CreateProjectionMatrix(90, 1, 0.01, 100);

	DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(90.0f), 1.0f, 0.1f, 500.0f);
	Matrix4x4 projMat;
	memcpy(&projMat, &proj, sizeof(Matrix4x4));
	return projMat.transpose();
}