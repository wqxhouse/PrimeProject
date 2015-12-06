#include "PostProcess.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"

int DispatchSize(int tgSize, int numElements)
{
	int dispatchSize = numElements / tgSize;
	dispatchSize += numElements % tgSize > 0 ? 1 : 0;
	return dispatchSize;
}

void PostProcess::Initialize(PE::GameContext *context, PE::MemoryArena arena, ID3D11ShaderResourceView *finalPassSRV)
{
	_pContext = context;
	_arena = arena;

	PE::D3D11Renderer *pD3D11Renderer = static_cast<PE::D3D11Renderer *>(_pContext->getGPUScreen());
	_context = pD3D11Renderer->m_pD3DContext;
	_device = pD3D11Renderer->m_pD3DDevice;

	_finalPassSRV = finalPassSRV;

	// bloom targets - size hard coded for now
	_bloomTarget.Initialize(_device, 1280 / 2, 720 / 2, DXGI_FORMAT_R11G11B10_FLOAT);
	_blurTarget.Initialize(_device, 1280 / 2, 720 / 2, DXGI_FORMAT_R11G11B10_FLOAT);

	// Create luminance reduction targets
	int w = 1280;
	int h = 720;
	while (w > 1 || h > 1)
	{
		w = DispatchSize(16, w);
		h = DispatchSize(16, h);

		RenderTarget2D rt;
		rt.Initialize(_device, w, h, DXGI_FORMAT_R32_FLOAT, 1, 1, 0, false, true);
		_reductionTargets.push_back(rt);
	}

	_adaptedLuminance = _reductionTargets[_reductionTargets.size() - 1].SRView;
}

void PostProcess::Render()
{
	computeAvgLuminance();
}


void PostProcess::computeAvgLuminance()
{
	// Calculate the geometric mean of luminance through reduction
	PIXEvent pixEvent(L"ComputeAvergeLuminance");

	// constantBuffer.SetCS(context, 0);

	ID3D11UnorderedAccessView* uavs[1] = { _reductionTargets[0].UAView };
	_context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

	ID3D11ShaderResourceView* srvs[1] = { _finalPassSRV };
	_context->CSSetShaderResources(0, 1, srvs);

	PE::Components::Effect *firstPass = 
		PE::EffectManager::Instance()->getEffectHandle("GetLumInit").getObject<PE::Components::Effect>();
	firstPass->setCurrent(NULL);
	
	int dispatchX = _reductionTargets[0].Width;
	int dispatchY = _reductionTargets[0].Height;
	_context->Dispatch(dispatchX, dispatchY, 1);

	uavs[0] = NULL;
	_context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

	srvs[0] = NULL;
	_context->CSSetShaderResources(0, 1, srvs);

	PE::Components::Effect *secondPass =
		PE::EffectManager::Instance()->getEffectHandle("GetLumSec").getObject<PE::Components::Effect>();

	PE::Components::Effect *finalPass=
		PE::EffectManager::Instance()->getEffectHandle("GetLumFinal").getObject<PE::Components::Effect>();

	for (int i = 1; i < _reductionTargets.size(); ++i)
	{
		if (i == _reductionTargets.size() - 1)
		{
			finalPass->setCurrent(NULL);
		}
		else
		{
			secondPass->setCurrent(NULL);
		}

		uavs[0] = _reductionTargets[i].UAView;
		_context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

		srvs[0] = _reductionTargets[i - 1].SRView;
		_context->CSSetShaderResources(0, 1, srvs);

		dispatchX = _reductionTargets[i].Width;
		dispatchY = _reductionTargets[i].Height;
		_context->Dispatch(dispatchX, dispatchY, 1);

		uavs[0] = NULL;
		_context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

		srvs[0] = NULL;
		_context->CSSetShaderResources(0, 1, srvs);
	}
}

void PostProcess::renderBloom()
{
	PIXEvent event(L"Bloom");



}
