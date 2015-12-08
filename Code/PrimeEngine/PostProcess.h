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

	inline void SetEnableColorCorrection(int tf) { _enableInstagram = tf; }
	inline bool getEnableColorCorrection() {
		return _enableInstagram;
	};

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

	struct DOFConstants
	{
		float projA;
		float projB;
		float GatherBlurSize;
		unsigned int enableInstagram;
		Vector4 DOFDepths;
	};

	ConstantBuffer<DOFConstants> _dofConstants;

	float _nearFocusStart;
	float _nearFocusEnd;
	float _farFocusStart;
	float _farFoucsEnd;

	ID3D11SamplerStatePtr _linearSampler;

	bool _enableInstagram;
};