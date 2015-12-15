#pragma once

#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/RenderUtils.h"

#include <vector>

class PostProcess
{
public:
	void Initialize(PE::GameContext *context, PE::MemoryArena arena, ID3D11ShaderResourceView *finalPassTarget, ID3D11RenderTargetView *finalPassRTV, ID3D11ShaderResourceView *depthSRV);

	void Render();

	inline void SetEnableColorCorrection(bool tf) { _enableInstagram = tf; }
	inline bool getEnableColorCorrection() {
		return _enableInstagram;
	};

	inline void setEnableManualExposure(bool tf) { _enableManualExposure = tf; }
	inline bool getEnableManualExposure() { return _enableManualExposure; }

	inline void setManualExposure(float manualExposure) { _manualExposure = manualExposure; }
	inline float getManualExposure() { return _manualExposure; }

	inline float getKeyValue() { return _keyValue; }
	inline void setKeyValue(float keyvalue) { _keyValue = keyvalue; }

	inline void setFarFocusStart(float farFocusStart)
	{
		_farFocusStart = farFocusStart;
	}

	inline void setFarFocusEnd(float farFoucsEnd)
	{
		_farFoucsEnd = farFoucsEnd;
	}

	inline float getFarFocusStart()
	{
		return _farFocusStart;
	}

	inline float getFarFoucsEnd() {
		return _farFoucsEnd;
	};

	inline bool getEnableDOF()
	{
		return _enableDOF;
	}

	inline void setEnableDOF(bool tf)
	{
		_enableDOF = tf;
	}


private:
	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	PE::GameContext *_pContext;
	PE::MemoryArena _arena;

	ID3D11ShaderResourceViewPtr _finalPassSRV;
	ID3D11RenderTargetViewPtr _finalPassRTV;
	ID3D11ShaderResourceViewPtr _depthSRV;

	ID3D11ShaderResourceViewPtr _adaptedLuminance;
	std::vector<RenderTarget2D> _reductionTargets;

	// Bloom Targets
	RenderTarget2D _bloomTarget;
	RenderTarget2D _blurTarget;

	RenderTarget2D _depthBlurTarget;
	RenderTarget2D _depthOfFieldFirstPassTarget;

	void renderBloom();
	void renderBlur();
	void renderTonemapping();

	void renderDepthBlur();
	void renderDOFGather();

	void computeAvgLuminance();

	void uploadConstants();

	struct DOFConstants
	{
		float projA;
		float projB;
		float GatherBlurSize;
		unsigned int enableInstagram;
		Vector4 DOFDepths;

		float KeyValue;
		unsigned int enableManualExposure;
		float manualExposure;
		float pad1;
	};

	ConstantBuffer<DOFConstants> _dofConstants;

	float _nearFocusStart;
	float _nearFocusEnd;
	float _farFocusStart;
	float _farFoucsEnd;

	ID3D11SamplerStatePtr _linearSampler;

	bool _enableInstagram;
	bool _enableManualExposure;
	float _manualExposure;
	float _keyValue;

	bool _enableDOF;
};