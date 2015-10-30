
#define NOMINMAX

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/Lua/LuaEnvironment.h"
#include "../../Lua/EventGlue/EventDataCreators.h"
	
// Sibling/Children includes
#include "EffectManager.h"
#include "Effect.h"

namespace PE {
    
    using namespace Components;
    void EffectManager::loadDefaultEffects()
    {
        PEINFO("PE: PROGRESS: EffectManager::loadDefaultEffects() Entry\n");

		// + Deferred
		m_halbedoTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
		TextureGPU *pAlbedoRTTGPU = new (m_halbedoTextureGPU)TextureGPU(*m_pContext, m_arena);
		pAlbedoRTTGPU->createDrawableIntoColorTexture(
			m_pContext->getGPUScreen()->getWidth(),
			m_pContext->getGPUScreen()->getHeight(),
			SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Clamp);

		// 64bit float
		m_hnormalTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
		TextureGPU *pNormalRTTGPU = new (m_hnormalTextureGPU)TextureGPU(*m_pContext, m_arena);
		pNormalRTTGPU->createDrawableIntoColorTexture(
			m_pContext->getGPUScreen()->getWidth(),
			m_pContext->getGPUScreen()->getHeight(),
			SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp, 1); // mipmap for normal ?

		// 64bit float - TODO: mipmap or not? 
		// (mipmap can create possible wrong samples at depth discontinuity)
		m_haccumHDRTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
		TextureGPU *pAccumTextureGPU = new (m_haccumHDRTextureGPU)TextureGPU(*m_pContext, m_arena);
		pAccumTextureGPU->createDrawableIntoColorTexture(
			m_pContext->getGPUScreen()->getWidth(),
			m_pContext->getGPUScreen()->getHeight(),
			SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Clamp, 1);

		// 32bit unorm
		m_hfinalLDRTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
		TextureGPU *pFinalRTTGPU = new (m_hfinalLDRTextureGPU)TextureGPU(*m_pContext, m_arena);
		pFinalRTTGPU->createDrawableIntoColorTexture(
			m_pContext->getGPUScreen()->getWidth(),
			m_pContext->getGPUScreen()->getHeight(),
			SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Clamp);

		// Root Depth buffer TextureGPU
		m_hrootDepthBufferTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
		TextureGPU *pRootDepthBufferGPU = new (m_hrootDepthBufferTextureGPU)TextureGPU(*m_pContext, m_arena);

#if APIABSTRACTION_D3D11
		D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(m_pContext->getGPUScreen());
		pRootDepthBufferGPU->m_samplerState = SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp;
		pRootDepthBufferGPU->m_viewport.TopLeftX = 0;
		pRootDepthBufferGPU->m_viewport.TopLeftY = 0;
		pRootDepthBufferGPU->m_viewport.Width = pD3D11Renderer->getWidth();
		pRootDepthBufferGPU->m_viewport.Height = pD3D11Renderer->getHeight();
		pRootDepthBufferGPU->m_viewport.MinDepth = 0.0f;
		pRootDepthBufferGPU->m_viewport.MaxDepth = 1.0f;
		pRootDepthBufferGPU->m_DepthStencilView = pD3D11Renderer->m_pDepthStencilView;
		pRootDepthBufferGPU->m_pDepthShaderResourceView = pD3D11Renderer->m_pDepthStencilShaderView;
#else 
		assert(false);
#endif
		// + End

// 
//		m_hGlowTargetTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
//		TextureGPU *pGlowTargetTextureGPU = new(m_hGlowTargetTextureGPU) TextureGPU(*m_pContext, m_arena);
//		//todo: once mipmaps are generated for this RT enable SamplerState_MipLerp_MinTexelLerp_MagTexelLerp_Clamp
//		// if donwsampling doesnt look good
//		// for now stick with because we dont have cross platform mipmap generation, should work fine
//		pGlowTargetTextureGPU->createDrawableIntoColorTextureWithDepth(m_pContext->getGPUScreen()->getWidth(), m_pContext->getGPUScreen()->getHeight(), SamplerState_NoMips_MinTexelLerp_NoMagTexelLerp_Clamp);
//#ifndef _XBOX
//		m_frameBufferCopyTexture.createDrawableIntoColorTexture(m_pContext->getGPUScreen()->getWidth(), m_pContext->getGPUScreen()->getHeight(), SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp);
//#endif
//#if 0
//		// enable when readable textures for DX 9 are available
//		m_readableTexture.createReadableColorTexture(IRenderer::Instance()->getWidth(), IRenderer::Instance()->getHeight());
//#endif
//
//		// 1/4 scren texture blur
//		PEINFO("PE: PROGRESS: Creating m_glowSeparatedTextureGPU\n");
//		// no need to magnification filtering for this texture since we render it into same size texture
//		m_glowSeparatedTextureGPU.createDrawableIntoColorTexture(m_pContext->getGPUScreen()->getWidth() / 2, m_pContext->getGPUScreen()->getHeight() / 2, SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp);
//
//		PEINFO("PE: PROGRESS: Creating m_2ndGlowTargetTextureGPU\n");
//		// need magnification filtering since we render this small texture onto larger result RT
//		m_2ndGlowTargetTextureGPU.createDrawableIntoColorTexture(m_pContext->getGPUScreen()->getWidth() / 2, m_pContext->getGPUScreen()->getHeight() / 2, SamplerState_NoMips_NoMinTexelLerp_MagTexelLerp_Clamp);
//		
//		PEINFO("PE: PROGRESS: Creating pFinishedGlowTargetTextureGPU\n");
//		m_hFinishedGlowTargetTextureGPU = Handle("TEXTURE_GPU", sizeof(TextureGPU));
//		TextureGPU *pFinishedGlowTargetTextureGPU = new(m_hFinishedGlowTargetTextureGPU) TextureGPU(*m_pContext, m_arena);
//		// need to use mag texel lerp since will be upscaled and put on result
//		pFinishedGlowTargetTextureGPU->createDrawableIntoColorTexture(m_pContext->getGPUScreen()->getWidth(), m_pContext->getGPUScreen()->getHeight(), SamplerState_NoMips_NoMinTexelLerp_NoMagTexelLerp_Clamp);
//
//#if APIABSTRACTION_IOS || APIABSTRACTION_X360
//		PEINFO("PE: PROGRESS: Creating m_shadowMapDepthTexture 128x128\n");
//		// need to lerp minimization and at this point we dont lerp magnification
//		m_shadowMapDepthTexture.createDrawableIntoDepthTexture(128, 128, SamplerState_NoMips_MinTexelLerp_NoMagTexelLerp_Clamp);
//#else
//		PEINFO("PE: PROGRESS: Creating m_shadowMapDepthTexture 2048x2048\n");
//		// need to lerp minimization and at this point we dont lerp magnification
//		m_shadowMapDepthTexture.createDrawableIntoDepthTexture(2048,2048, SamplerState_NoMips_MinTexelLerp_NoMagTexelLerp_Clamp);
//#endif

		m_pixelShaderSubstitutes.reset(EffectPSInputFamily::Count);

        for (PrimitiveTypes::UInt32 i = 0; i < m_pixelShaderSubstitutes.m_capacity; ++i)
        {
            m_pixelShaderSubstitutes.add(Handle());
        }


#	if 0 && (APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11)
        Handle hSkyFx("EFFECT", sizeof(Effect));
        Effect *pSkyFx = new(hSkyFx) Effect(*m_pContext, m_arena, hSkyFx);
        {
            pSkyFx->loadTechnique(
                                  "SkyBox.fx", "Sky_VS", 
                                  NULL, NULL, // geometry shader
                                  "SkyBox.fx", "Sky_PS", 
                                  NULL, NULL, // compute shader
								  PERasterizerState_SolidTriNoCull,
								  PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
								  "Sky_Tech");
            pSkyFx->m_effectDrawOrder = EffectDrawOrder::Manual;
            pSkyFx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_3DTC_PS_IN;
        }
        m_map.add("Sky_Tech", hSkyFx);
#	endif

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"ColoredMinimalMesh_VS", "main",
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_A_0_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"ColoredMinimalMesh_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add("ColoredMinimalMesh_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"ColoredMinimalMesh_VS", "main",
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_A_0_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_Line,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"ColoredMinimalMesh_Line_A_0_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add("ColoredMinimalMesh_Line_A_0_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"ColoredMinimalMesh_VS", "main",
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_A_0_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"ColoredMinimalMesh_2D_A_0_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add("ColoredMinimalMesh_2D_A_0_Tech", hEffect);
	}
	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
				"StdMesh_Shadowed_VS", "main",
				NULL, NULL, // geometry shader
				"StdMesh_Shadowed_A_0_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
            PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
            "StdMesh_Shadowed_A_0_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::First;

		m_map.add("StdMesh_Shadowed_A_0_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"StdMesh_ZOnly_VS", "main",
			NULL, NULL, // geometry shader
			API_CHOOSE_DX11_ELSE(NULL, "StdMesh_ZOnly_PS"), "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"StdMesh_ZOnly_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::First;

		m_map.add("StdMesh_ZOnly_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"StdMesh_2D_VS", "main",
			NULL, NULL, // geometry shader
			"StdMesh_Diffuse_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"StdMesh_2D_Diffuse_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add("StdMesh_2D_Diffuse_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"StdMesh_2D_VS", "main",
			NULL, NULL, // geometry shader
			"StdMesh_Diffuse_A_0_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"StdMesh_2D_Diffuse_A_0_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add("StdMesh_2D_Diffuse_A_0_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"StdMesh_2D_VS", "main",
			NULL, NULL, // geometry shader
			"StdMesh_Diffuse_A_RGBIntensity_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"StdMesh_2D_Diffuse_A_RGBIntensity_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::Last;

		m_map.add(pEffect->m_techName, hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"StdMesh_VS", "main",
			NULL, NULL, // geometry shader
			"StdMesh_Diffuse_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"StdMesh_Diffuse_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::STD_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::First;

		m_map.add("StdMesh_Diffuse_Tech", hEffect);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
				"DetailedMesh_Shadowed_VS", "main",
				NULL, NULL, // geometry shader
				"DetailedMesh_Shadowed_A_Glow_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedMesh_Shadowed_A_Glow_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::First;
		m_map.add("DetailedMesh_Shadowed_A_Glow_Tech", hEffect);
	}

	// + Deferred GBuffer
	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect)Effect(*m_pContext, m_arena, hEffect);
		pEffect->loadTechnique(
			"DetailedMesh_Shadowed_VS", "main",
			NULL, NULL,
			"DetailedMesh_GBuffer_PS", "main",
			NULL, NULL,
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer,
			PEAlphaBlendState_NoBlend, 
			"DetailedMesh_GBuffer_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		pEffect->m_effectDrawOrder = EffectDrawOrder::First;
		m_map.add("DetailedMesh_GBuffer_Tech", hEffect);
	}

	// TODO: Light Pass - classical deferred and clustered will be different here
	// Classical Deferred pass render sphere/cone geometry + additive blending
	// Clustered Deferred pass render quad... (no additive blending, implied in one-pass shader)
	{
		Handle hLightHDRClusteredFx("EFFECT", sizeof(Effect));
		Effect *pLightHDRClusteredFx = new(hLightHDRClusteredFx)Effect(*m_pContext, m_arena, hLightHDRClusteredFx);
		pLightHDRClusteredFx->loadTechnique(
			"ColoredMinimalMesh_VS", "main",
			NULL, NULL, // geometry shader
			"Deferred_LightPassClustered_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DeferredLightPass_Clustered_Tech");
		pLightHDRClusteredFx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("DeferredLightPass_Clustered_Tech", hLightHDRClusteredFx);
	}

	// + Deferred final LDR Pass
	{
		Handle hFinalLDRFx("EFFECT", sizeof(Effect));
		Effect *pFinalLDRFx = new(hFinalLDRFx)Effect(*m_pContext, m_arena, hFinalLDRFx);
		pFinalLDRFx->loadTechnique(
			"ColoredMinimalMesh_VS", "main",
			NULL, NULL, // geometry shader
			"DeferredFinalPass_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"finalLDRPassTech");
		pFinalLDRFx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("deferredFinalLDR.fx", hFinalLDRFx);
	}

	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
	
		//this is work in progress. right now trying to get this working on PC GL, and leaving as was for PC
		//this will not work on ps3, ios
		pEffect->loadTechnique(
			"DetailedSkin_Shadowed_VS", "main",
			NULL, NULL, // geometry shader
			"DetailedMesh_Shadowed_A_Glow_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedSkin_Shadowed_A_Glow_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		m_map.add("DetailedSkin_Shadowed_A_Glow_Tech", hEffect);

	}
	#if PE_API_IS_D3D11
	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
	
		pEffect->loadTechnique(
			"DetailedSkin_Shadowed_Instanced_VS", "main",
			NULL, NULL, // geometry shader
			"DetailedMesh_Shadowed_A_Glow_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedSkin_Shadowed_Instanced_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		m_map.add("DetailedSkin_Shadowed_Instanced_Tech", hEffect);

	}
	#endif
	#if PE_API_IS_D3D11
	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
	
		pEffect->loadTechnique(
			NULL, NULL,
			NULL, NULL, // geometry shader
			NULL, NULL, 
			"DetailedSkin_Instanced_CS", "main", // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedSkin_Instanced_CS_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		m_map.add("DetailedSkin_Instanced_CS_Tech", hEffect);
	}
	#endif

	#if PE_API_IS_D3D11
	{
		Handle hEffect("EFFECT", sizeof(Effect));
		Effect *pEffect = new(hEffect) Effect(*m_pContext, m_arena, hEffect);
	
		pEffect->loadTechnique(
			NULL, NULL,
			NULL, NULL, // geometry shader
			NULL, NULL, 
			"DetailedSkin_Reduce_Instanced_CS", "main", // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedSkin_Reduce_Instanced_CS_Tech");

		pEffect->m_psInputFamily = EffectPSInputFamily::DETAILED_MESH_PS_IN;
		m_map.add("DetailedSkin_Reduce_Instanced_CS_Tech", hEffect);
	}
	#endif


	{
		Handle htexturedskinzonly("EFFECT", sizeof(Effect));
		Effect *ptexturedskinzonly = new(htexturedskinzonly) Effect(*m_pContext, m_arena, htexturedskinzonly);
		ptexturedskinzonly->loadTechnique(
			"DetailedSkin_ZOnly_VS", "main",
			NULL, NULL, // geometry shader
			API_CHOOSE_DX11_ELSE(NULL, "DetailedMesh_ZOnly_PS"), "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedSkin_ZOnly_Tech");
		ptexturedskinzonly->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("DetailedSkin_ZOnly_Tech", htexturedskinzonly);
	}

	{
		Handle htexturedskinzonly("EFFECT", sizeof(Effect));
		Effect *ptexturedskinzonly = new(htexturedskinzonly) Effect(*m_pContext, m_arena, htexturedskinzonly);
		ptexturedskinzonly->loadTechnique(
			"DetailedMesh_ZOnly_VS", "main",
			NULL, NULL, // geometry shader
			API_CHOOSE_DX11_ELSE(NULL, "DetailedMesh_ZOnly_PS"), "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriBackCull,
			PEDepthStencilState_ZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"DetailedMesh_ZOnly_Tech");
		ptexturedskinzonly->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("DetailedMesh_ZOnly_Tech", htexturedskinzonly);
	}


	{
		Handle hflattex2dfx("EFFECT", sizeof(Effect));
		Effect *pflattex2dfx = new(hflattex2dfx) Effect(*m_pContext, m_arena, hflattex2dfx);
		pflattex2dfx->loadTechnique(
			"ColoredMinimalMesh_VS", "main", 
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_VertBlurCombine_PS", "main",
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"verticalblurTech");
		pflattex2dfx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("verticalblur.fx", hflattex2dfx);
	}

	{
		Handle hglowseparationpassfx("EFFECT", sizeof(Effect));
		Effect *pglowseparationpassfx = new(hglowseparationpassfx) Effect(*m_pContext, m_arena, hglowseparationpassfx);

		pglowseparationpassfx->loadTechnique(
			"ColoredMinimalMesh_VS", "main", 
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_GlowSeparation_PS", "main", 
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"glowseparationpassTech");
		pglowseparationpassfx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;
		m_map.add("glowseparationpass.fx", hglowseparationpassfx);
	}

	{
		Handle hfirstglowpassfx("EFFECT", sizeof(Effect));
		Effect *pfirstglowpassfx = new(hfirstglowpassfx) Effect(*m_pContext, m_arena, hfirstglowpassfx);

		pfirstglowpassfx->loadTechnique(
			"ColoredMinimalMesh_VS", "main", 
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_HorizontalBlur_PS", "main", 
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"firstglowpassTech");
		pfirstglowpassfx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;

		m_map.add("firstglowpass.fx", hfirstglowpassfx);
	}

	{
		Handle hMotionBlurfx("EFFECT", sizeof(Effect));
		Effect *pMotionBlurfx = new(hMotionBlurfx) Effect(*m_pContext, m_arena, hMotionBlurfx);
		pMotionBlurfx->loadTechnique(
			"ColoredMinimalMesh_VS", "main", 
			NULL, NULL, // geometry shader
			"ColoredMinimalMesh_MotionBlur_PS", "main", 
			NULL, NULL, // compute shader
			PERasterizerState_SolidTriNoCull,
			PEDepthStencilState_NoZBuffer, PEAlphaBlendState_NoBlend, // depth stencil, blend states
			"motionBlurTech");
		pMotionBlurfx->m_psInputFamily = EffectPSInputFamily::REDUCED_MESH_PS_IN;
	
		m_map.add("motionblur.fx", hMotionBlurfx);
	}

	buildFullScreenBoard();

	setupConstantBuffersAndShaderResources();
}
}; // namespace PE
