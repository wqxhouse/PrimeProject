#pragma once
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/RenderUtils.h"

class SkyboxNew
{
public:
	void Initialize(PE::GameContext *context, PE::MemoryArena arena);
	void Render(const Matrix4x4 &viewMat, const Matrix4x4 &projMat, ID3D11RenderTargetView *rtView, ID3D11DepthStencilView *dsView);

	inline void SetSunDirection(float theta, float phi) { _sunTheta = theta; _sunPhi = phi; }
	inline void GetSunDirection(float &theta, float &phi) { theta = _sunTheta; phi = _sunPhi; }
	Vector3 GetSunDirection();

private:
	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	PE::GameContext *_pContext;
	PE::MemoryArena _arena;

	struct VSConstants
	{
		Matrix4x4 View;
		Matrix4x4 Proj;
	};

	struct PSConstants
	{
		Float4Align Vector3 A;
		Float4Align Vector3 B;
		Float4Align Vector3 C;
		Float4Align Vector3 D;
		Float4Align Vector3 E;
		Float4Align Vector3 Z;
		Float4Align Vector3 SunDirection;
	};

	ConstantBuffer<VSConstants> _vsConstants;
	ConstantBuffer<PSConstants> _psConstants;
	ID3D11DepthStencilStatePtr _dsState;
	ID3D11BlendStatePtr _blendState;
	ID3D11RasterizerStatePtr _rasterizerState;
	ID3D11SamplerStatePtr _samplerState;

	// PreethamSky params 
	Vector3 _A, _B, _C, _D, _E, _Z;
	void calcPreetham(float sunTheta, float turbidity, float normalizedSunY);

	float _sunTheta;
	float _sunPhi;
};