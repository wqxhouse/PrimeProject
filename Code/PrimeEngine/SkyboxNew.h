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

	//inline void SetSunDirection(float theta, float phi) { _sunTheta = theta; _sunPhi = phi; }
	//inline void GetSunDirection(float &theta, float &phi) { theta = _sunTheta; phi = _sunPhi; }
	Vector3 GetSunDirection() { return _lightDirection; }

	void SetSolarTime(float solarTime);
	inline float GetSolarTime() { return _solarTime; }
	Vector3 GetSunColor() { return _LightColor; }

	ID3D11ShaderResourceView *getLocalCubemapSRV();

	void SetSolarZenith(float zenith);
	void SetSolarAzimuth(float azimuth);
	void SetTurbidity(float turbidity);

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

	//struct PSConstants
	//{
	//	Float4Align Vector3 A;
	//	Float4Align Vector3 B;
	//	Float4Align Vector3 C;
	//	Float4Align Vector3 D;
	//	Float4Align Vector3 E;
	//	Float4Align Vector3 Z;
	//	Float4Align Vector3 SunDirection;
	//};

	struct Coefficients
	{
		float A, B, C, D, E, cpad0, cpad1, cpad2;
	};

	struct PSConstants
	{
		float solar_azimuth;
		float solar_zenith;

		float Yz;
		float xz;
		float yz;

		float turbidity;

		float pad0;
		float pad1;

		Coefficients cY;
		Coefficients cx;
		Coefficients cy;

		Vector3 sunColor;
		float night;
	};

	ConstantBuffer<VSConstants> _vsConstants;
	ConstantBuffer<PSConstants> _psConstants;
	ID3D11DepthStencilStatePtr _dsState;
	ID3D11BlendStatePtr _blendState;
	ID3D11RasterizerStatePtr _rasterizerState;
	ID3D11SamplerStatePtr _samplerState;

	ID3D11ShaderResourceViewPtr _nightCubemap;

	// PreethamSky params 
	Vector3 _A, _B, _C, _D, _E, _Z;
	void calcPreetham(float sunTheta, float turbidity, float normalizedSunY);

	float _sunTheta;
	float _sunPhi;

	// ===========================================
	void CalculateZenitalAbsolutes();
	void CalculateCoefficents();
	void CalculateLightColor();
	Vector3 calcRGB(float Y, float x, float y);

	double Perez(float zenith, float gamma, const Coefficients &coeffs);

	Vector3 _LightColor;

	Coefficients _cY;
	Coefficients _cx;
	Coefficients _cy;

	ID3D11SamplerStatePtr _linearSampler;

	float _Yz;
	float _xz;
	float _yz;

	float _turbidity;

	float _solarZenith;
	float _solarAzimuth;

	float _solarDeclination;
	float _solarTime;
	float _latitude;

	Vector3 _lightDirection;

	float PI_;
};