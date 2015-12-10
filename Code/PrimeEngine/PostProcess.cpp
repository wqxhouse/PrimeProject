#include "PostProcess.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"

#include "PrimeEngine/Scene/CameraSceneNode.h"
#include "PrimeEngine/Scene/CameraManager.h"

int DispatchSize(int tgSize, int numElements)
{
	int dispatchSize = numElements / tgSize;
	dispatchSize += numElements % tgSize > 0 ? 1 : 0;
	return dispatchSize;
}

void PostProcess::Initialize(PE::GameContext *context, PE::MemoryArena arena, ID3D11ShaderResourceView *finalPassSRV, ID3D11RenderTargetView *finalPassRTV, ID3D11ShaderResourceView *depthSRV)
{
	_pContext = context;
	_arena = arena;

	PE::D3D11Renderer *pD3D11Renderer = static_cast<PE::D3D11Renderer *>(_pContext->getGPUScreen());
	_context = pD3D11Renderer->m_pD3DContext;
	_device = pD3D11Renderer->m_pD3DDevice;

	_finalPassSRV = finalPassSRV;
	_finalPassRTV = finalPassRTV;
	_depthSRV = depthSRV;

	_depthBlurTarget.Initialize(_device, 1280, 720, DXGI_FORMAT_R16G16_FLOAT);
	_depthOfFieldFirstPassTarget.Initialize(_device, 1280, 720, DXGI_FORMAT_R16G16B16A16_FLOAT);

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

	_dofConstants.Initialize(_device);

	_nearFocusStart = 0.01f;
	_nearFocusEnd = 0.01f;

	_farFocusStart = 20.0f;
	_farFoucsEnd = 100.0f;


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

	_enableInstagram = true;
	_enableManualExposure = false;
	_manualExposure = 0.1f;
	_keyValue = 0.4f;
}

void PostProcess::Render()
{
	uploadConstants();

	//renderDepthBlur();
	//renderDOFGather();

	computeAvgLuminance();
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(1280 / 2);
	viewport.Height = static_cast<float>(720 / 2);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;

	_context->RSSetViewports(1, &viewport);
	renderBloom();
	renderBlur();

	renderTonemapping();
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

	PE::Components::Effect *bloomPass =
		PE::EffectManager::Instance()->getEffectHandle("BloomTech").getObject<PE::Components::Effect>();

	PE::SA_Bind_Resource setInputTex(
		*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NotNeeded,
		_finalPassSRV);
	setInputTex.bindToPipeline(bloomPass);

	PE::SA_Bind_Resource setAvgLumTex(
		*_pContext, _arena, PE::ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NotNeeded,
		_adaptedLuminance);
	setAvgLumTex.bindToPipeline(bloomPass);

	ID3D11RenderTargetView *rtvs[1] = { _bloomTarget.RTView };
	_context->OMSetRenderTargets(1, rtvs, nullptr);

	PE::IndexBufferGPU *pibGPU = PE::EffectManager::Instance()->m_hIndexBufferGPU.getObject<PE::IndexBufferGPU>();
	pibGPU->setAsCurrent();

	PE::VertexBufferGPU *pvbGPU = PE::EffectManager::Instance()->m_hVertexBufferGPU.getObject<PE::VertexBufferGPU>();
	pvbGPU->setAsCurrent(bloomPass);
	bloomPass->setCurrent(pvbGPU);

	// draw quad
	pibGPU->draw(1, 0);

	setInputTex.unbindFromPipeline(bloomPass);
	setAvgLumTex.unbindFromPipeline(bloomPass);
	rtvs[0] = nullptr;
	_context->OMSetRenderTargets(1, rtvs, nullptr);
}

void PostProcess::renderBlur()
{
	PIXEvent event(L"RenderBlur");

	PE::Components::Effect *blurHPass  =
		PE::EffectManager::Instance()->getEffectHandle("BlurH").getObject<PE::Components::Effect>();

	PE::Components::Effect *blurVPass  =
		PE::EffectManager::Instance()->getEffectHandle("BlurV").getObject<PE::Components::Effect>();

	PE::IndexBufferGPU *pibGPU = PE::EffectManager::Instance()->m_hIndexBufferGPU.getObject<PE::IndexBufferGPU>();
	PE::VertexBufferGPU *pvbGPU = PE::EffectManager::Instance()->m_hVertexBufferGPU.getObject<PE::VertexBufferGPU>();

	for (int i = 0; i < 2; ++i)
	{
		// horizontal - input bloomtar - output blurtar
		ID3D11RenderTargetView *rtvs[1] = { _blurTarget.RTView };
		_context->OMSetRenderTargets(1, rtvs, nullptr);

		PE::SA_Bind_Resource setInputTex(
			*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
			_bloomTarget.SRView);
		setInputTex.bindToPipeline(blurHPass);
		pibGPU->setAsCurrent();
		pvbGPU->setAsCurrent(blurHPass);
		blurHPass->setCurrent(pvbGPU);
		pibGPU->draw(1, 0);

		setInputTex.unbindFromPipeline(blurHPass);

		// vertical - input blurtar - output bloomtar
		rtvs[0] = _bloomTarget.RTView;
		_context->OMSetRenderTargets(1, rtvs, nullptr);

		PE::SA_Bind_Resource setInputTexVertical(
			*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
			PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
			_blurTarget.SRView);
		setInputTexVertical.bindToPipeline(blurVPass);
		pibGPU->setAsCurrent();
		pvbGPU->setAsCurrent(blurVPass);
		blurVPass->setCurrent(pvbGPU);
		pibGPU->draw(1, 0);
		setInputTexVertical.unbindFromPipeline(blurVPass);
	}

	ID3D11RenderTargetView *nullRTVs[1] = { nullptr };
	_context->OMSetRenderTargets(1, nullRTVs, nullptr);
}

void PostProcess::renderTonemapping()
{
	PIXEvent event(L"Tonemapping");

	PE::Components::Effect *tonemappingPass =
		PE::EffectManager::Instance()->getEffectHandle("TonemappingTech").getObject<PE::Components::Effect>();

	PE::IndexBufferGPU *pibGPU = PE::EffectManager::Instance()->m_hIndexBufferGPU.getObject<PE::IndexBufferGPU>();
	PE::VertexBufferGPU *pvbGPU = PE::EffectManager::Instance()->m_hVertexBufferGPU.getObject<PE::VertexBufferGPU>();

	PE::SA_Bind_Resource setInputTex(
		*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
		_finalPassSRV);
	setInputTex.bindToPipeline(tonemappingPass);

	PE::SA_Bind_Resource setAvgLumTex(
		*_pContext, _arena, PE::ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NotNeeded,
		_adaptedLuminance);
	setAvgLumTex.bindToPipeline(tonemappingPass);

	PE::SA_Bind_Resource setBloomTex(
		*_pContext, _arena, PE::GLOW_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Clamp,
		_bloomTarget.SRView);
	setBloomTex.bindToPipeline(tonemappingPass);

	// Render directly to back buffer
	_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, true, true);

	pibGPU->setAsCurrent();
	pvbGPU->setAsCurrent(tonemappingPass);
	tonemappingPass->setCurrent(pvbGPU);
	pibGPU->draw(1, 0);

	setInputTex.unbindFromPipeline(tonemappingPass);
	setAvgLumTex.unbindFromPipeline(tonemappingPass);
	setBloomTex.unbindFromPipeline(tonemappingPass);
}

void PostProcess::uploadConstants()
{
	PE::Components::CameraSceneNode *csn =
		PE::Components::CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<PE::Components::CameraSceneNode>();
	float n = csn->m_near;
	float f = csn->m_far;
	float projA = f / (f - n);
	float projB = (-f * n) / (f - n);

	_dofConstants.Data.projA = projA;
	_dofConstants.Data.projB = projB;
	_dofConstants.Data.GatherBlurSize = 16;
	_dofConstants.Data.enableInstagram = _enableInstagram;
	_dofConstants.Data.DOFDepths.m_x = _nearFocusStart;
	_dofConstants.Data.DOFDepths.m_y = _nearFocusEnd;
	_dofConstants.Data.DOFDepths.m_z = _pContext->_farFocusStart > 9 ? _pContext->_farFocusStart : 9;
	_dofConstants.Data.DOFDepths.m_w = _pContext->_farFocusEnd > 9 ? _pContext->_farFocusEnd : 9;
	_dofConstants.Data.enableManualExposure = _enableManualExposure;
	_dofConstants.Data.manualExposure = _manualExposure;
	_dofConstants.Data.KeyValue = _keyValue;

	_dofConstants.ApplyChanges(_context);
	_dofConstants.SetPS(_context, 0);
}

void PostProcess::renderDepthBlur()
{
	PIXEvent event(L"Depth Blur");

	PE::Components::Effect *depthBlurPass=
		PE::EffectManager::Instance()->getEffectHandle("DepthBlurTech").getObject<PE::Components::Effect>();

	PE::SA_Bind_Resource setInputTex(
		*_pContext, _arena, PE::DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NotNeeded,
		_depthSRV);
	setInputTex.bindToPipeline(depthBlurPass);

	ID3D11RenderTargetView *rtvs[1] = { _depthBlurTarget.RTView };
	_context->OMSetRenderTargets(1, rtvs, nullptr);

	PE::IndexBufferGPU *pibGPU = PE::EffectManager::Instance()->m_hIndexBufferGPU.getObject<PE::IndexBufferGPU>();
	pibGPU->setAsCurrent();

	PE::VertexBufferGPU *pvbGPU = PE::EffectManager::Instance()->m_hVertexBufferGPU.getObject<PE::VertexBufferGPU>();
	pvbGPU->setAsCurrent(depthBlurPass);
	depthBlurPass->setCurrent(pvbGPU);

	// set cb
	//PE::Components::CameraSceneNode *csn =
	//	PE::Components::CameraManager::Instance()->getActiveCamera()->m_hCameraSceneNode.getObject<PE::Components::CameraSceneNode>();
	//float n = csn->m_near;
	//float f = csn->m_far;
	//float projA = f / (f - n);
	//float projB = (-f * n) / (f - n);

	//_dofConstants.Data.projA = projA;
	//_dofConstants.Data.projB = projB;
	//_dofConstants.Data.GatherBlurSize = 16;
	//_dofConstants.Data.enableInstagram = _enableInstagram;
	//_dofConstants.Data.DOFDepths.m_x = _nearFocusStart;
	//_dofConstants.Data.DOFDepths.m_y = _nearFocusEnd;
	//_dofConstants.Data.DOFDepths.m_z = _pContext->_farFocusStart>9 ? _pContext->_farFocusStart : 9;
	//_dofConstants.Data.DOFDepths.m_w = _pContext->_farFocusEnd>9 ? _pContext->_farFocusEnd : 9;
	//_dofConstants.ApplyChanges(_context);
	//_dofConstants.SetPS(_context, 0);

	// draw quad
	pibGPU->draw(1, 0);

	setInputTex.unbindFromPipeline(depthBlurPass);

	rtvs[0] = nullptr;
	_context->OMSetRenderTargets(1, rtvs, nullptr);
}

void PostProcess::renderDOFGather()
{
	PIXEvent event(L"DOF Gather");
	PE::Components::Effect *DOFGatherPass =
		PE::EffectManager::Instance()->getEffectHandle("DOFGatherTech").getObject<PE::Components::Effect>();

	PE::IndexBufferGPU *pibGPU = PE::EffectManager::Instance()->m_hIndexBufferGPU.getObject<PE::IndexBufferGPU>();
	PE::VertexBufferGPU *pvbGPU = PE::EffectManager::Instance()->m_hVertexBufferGPU.getObject<PE::VertexBufferGPU>();

	ID3D11SamplerState *samplerStates[1] = { _linearSampler };
	_context->PSSetSamplers(9, 1, samplerStates);

	// First pass
	ID3D11RenderTargetView *rtvs[1] = { _depthOfFieldFirstPassTarget.RTView };
	_context->OMSetRenderTargets(1, rtvs, nullptr);

	PE::SA_Bind_Resource setInputTex(
		*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
		_finalPassSRV);

	PE::SA_Bind_Resource setInputDepthTex(
		*_pContext, _arena, PE::ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
		_depthBlurTarget.SRView);

	setInputTex.bindToPipeline();
	setInputDepthTex.bindToPipeline();
	pibGPU->setAsCurrent();
	pvbGPU->setAsCurrent(DOFGatherPass);
	DOFGatherPass->setCurrent(pvbGPU);
	pibGPU->draw(1, 0);

	setInputTex.unbindFromPipeline(DOFGatherPass);
	// setInputDepthTex.unbindFromPipeline(DOFGatherPass);

	// Second pass
	rtvs[0] = _finalPassRTV;
	_context->OMSetRenderTargets(1, rtvs, nullptr);

	PE::SA_Bind_Resource setInputFirstPassTex(
		*_pContext, _arena, PE::DIFFUSE_TEXTURE_2D_SAMPLER_SLOT,
		PE::SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp,
		_depthOfFieldFirstPassTarget.SRView);
	setInputFirstPassTex.bindToPipeline(DOFGatherPass);
	pibGPU->setAsCurrent();
	pvbGPU->setAsCurrent(DOFGatherPass);
	DOFGatherPass->setCurrent(pvbGPU);
	pibGPU->draw(1, 0);
	setInputFirstPassTex.unbindFromPipeline(DOFGatherPass);
	setInputDepthTex.unbindFromPipeline(DOFGatherPass);


	rtvs[0] = nullptr;
	_context->OMSetRenderTargets(1, rtvs, nullptr);

	samplerStates[0] = nullptr;
	_context->PSSetSamplers(9, 1, samplerStates);
}
