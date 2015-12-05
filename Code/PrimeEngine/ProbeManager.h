#pragma once
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/RenderUtils.h"
#include "PrimeEngine/GameObjectModel/Camera.h"


namespace PE
{
	namespace Components
	{
		struct Camera;
	}
}

class ProbeManager
{
public:
	void Initialize(PE::GameContext *context, PE::MemoryArena arena);
	void Render(int &threadOwnershipMask);


private:
	void createSphere(float radius, int sliceCount, int stackCount);
	void renderGBuffer(int &threadOwnershipMask);
	void renderLight(int &threadOwnershipMask);

	void prepareDefaultCameras();

	Matrix4x4 genCamWorldMatrix(const Vector3 &eye, const Vector3 &lookAt, Vector3 up);

	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	PE::GameContext *_pContext;
	PE::MemoryArena _arena; 

	// Sphere
	PE::Handle _sphereVertexBuf;
	PE::Handle _sphereIndexBuf;

	// cube map
	struct CameraStruct{
		Vector3 LookAt;
		Vector3	Up;
	} CubemapCameraStruct[6];
	static const CameraStruct DefaultCubemapCameraStruct[6];
	static PE::Handle DefaultCameras[6];

	struct VSConstant
	{
		Matrix4x4 world;
		Float4Align Matrix4x4 View;
		Float4Align Matrix4x4 WorldViewProjection;
	};

	ConstantBuffer<VSConstant> _VSConstants;

	RenderTarget2D _cubemapTarget0;
	RenderTarget2D _cubemapTarget1;
	RenderTarget2D _cubemapTarget2;
	RenderTarget2D _cubemapFinalTarget;
	RenderTarget2D _cubemapPrefilterTarget;
	DepthStencilBuffer _cubemapDepthTarget;

	int _cubemapSize;
};