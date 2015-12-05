#include "ProbeManager.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/Mesh.h"
#include "PrimeEngine/Render/D3D11Renderer.h"

#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"
#include "PrimeEngine/Scene/DrawList.h"
#include "PrimeEngine/Math/CameraOps.h"

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


void ProbeManager::Initialize(PE::GameContext *context, PE::MemoryArena arena)
{
	_pContext = context;
	_arena = arena;

	PE::D3D11Renderer *pD3D11Renderer = static_cast<PE::D3D11Renderer *>(_pContext->getGPUScreen());
	_context = pD3D11Renderer->m_pD3DContext;
	_device = pD3D11Renderer->m_pD3DDevice;

	_cubemapSize = 128;

	_cubemapTarget0.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1,
		1, 0, TRUE, FALSE, 6, TRUE);

	_cubemapTarget1.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 1,
		1, 0, TRUE, FALSE, 6, TRUE);

	_cubemapTarget2.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1,
		1, 0, TRUE, FALSE, 6, TRUE);

	_cubemapFinalTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 1,
		1, 0, TRUE, FALSE, 6, TRUE);

	_cubemapDepthTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_D32_FLOAT, true, 1,
		false, 6, TRUE);
	_cubemapPrefilterTarget.Initialize(_device, _cubemapSize, _cubemapSize, DXGI_FORMAT_R16G16B16A16_FLOAT, 0,
		false, 6, TRUE);

	prepareDefaultCameras();
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

void ProbeManager::Render(int &threadOwnershipMask)
{
	{
		PIXEvent event(L"Render Cubemap GBuffer");
		renderGBuffer(threadOwnershipMask);
	}
}

void ProbeManager::renderGBuffer(int &threadOwnershipMask)
{
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(_cubemapSize);
	viewport.Height = static_cast<float>(_cubemapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	_context->RSSetViewports(1, &viewport);
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
		Matrix4x4 viewProj =  _cameraMatrices[cubeboxFaceIndex];
		Matrix4x4 viewInv = _cameraMatrices[cubeboxFaceIndex].inverse();

		_context->OMSetRenderTargets(3, renderTarget, DSView);

		DrawList::InstanceReadOnly()->optimize();
		_pContext->getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);
		DrawList::InstanceReadOnly()->do_RENDER(*(Events::Event **)&cubeboxFaceIndex, threadOwnershipMask, &viewProj, &viewInv);
		_pContext->getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);
	}

	
	// end render targets
	ID3D11RenderTargetView *nullRenderTarget[3] = { nullptr, nullptr, nullptr };
	_context->OMSetRenderTargets(3, nullRenderTarget, nullptr);
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