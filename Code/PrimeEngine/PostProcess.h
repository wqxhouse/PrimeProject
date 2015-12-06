#pragma once

#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/RenderUtils.h"

#include <vector>

class PostProcess
{
public:
	void Initialize(PE::GameContext *context, PE::MemoryArena arena, ID3D11ShaderResourceView *finalPassTarget);

	void Render();

private:
	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	PE::GameContext *_pContext;
	PE::MemoryArena _arena;

	ID3D11ShaderResourceViewPtr _finalPassSRV;

	ID3D11ShaderResourceViewPtr _adaptedLuminance;
	std::vector<RenderTarget2D> _reductionTargets;

	// Bloom Targets
	RenderTarget2D _bloomTarget;
	RenderTarget2D _blurTarget;

	void renderBloom();
	void renderBlur();
	void renderTonemapping();

	void computeAvgLuminance();

	struct PostProcessConstants
	{
		Float4Align float deltaSec;
	};
	
};