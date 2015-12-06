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

	void SetProbePosition(const Vector3 &pos);

private:
	void createSphere(float radius, int sliceCount, int stackCount);
	void renderGBuffer(int &threadOwnershipMask);
	void renderConvolution();

	void prepareDefaultCameras();

	Matrix4x4 genCamViewMatrix(Vector3 &eye, Vector3 &lookAt, Vector3 up);
	Matrix4x4 genCamProjMatrix();

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
	static CameraStruct DefaultCubemapCameraStruct[6];

	//struct ConvolutionVSConstant
	//{
	//	Float4Align Matrix4x4 WorldViewProjection;
	//};
	//ConstantBuffer<ConvolutionVSConstant> _convolutionVSConstants;

	static Matrix4x4 _cameraMatrices[7];

	struct ConvolutionPSConsts
	{
		long mipLevel;
		long numMips;
		long pad0;
		long pad1;
	};

	ConstantBuffer<ConvolutionPSConsts> _convolvePSConsts;

	RenderTarget2D _cubemapTarget0;
	RenderTarget2D _cubemapTarget1;
	RenderTarget2D _cubemapTarget2;
	RenderTarget2D _cubemapFinalTarget;
	RenderTarget2D _cubemapPrefilterTarget;
	DepthStencilBuffer _cubemapDepthTarget;

	ID3D11SamplerStatePtr _linearSampler;

	Vector3 _probePosition;

	std::vector<std::vector<ID3D11RenderTargetViewPtr> > _convolveMipmapRTs;
	std::vector<std::vector<ID3D11DepthStencilViewPtr> > _convolveMipmapDepthRTs;

	int _cubemapSize;
};