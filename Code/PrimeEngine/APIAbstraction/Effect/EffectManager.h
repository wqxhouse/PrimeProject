#ifndef __PYENGINE_2_0_EFFECT_MANAGER__
#define __PYENGINE_2_0_EFFECT_MANAGER__



// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes

#include "PrimeEngine/Render/IRenderer.h"
#include "PrimeEngine/Math/Matrix4x4.h"
#include "../../Utils/StrToHandleMap.h"
#include "../../Utils/PEMap.h"
#include "../Texture/Texture.h"
#include "../GPUBuffers/VertexBufferGPU.h"
#include "../GPUBuffers/IndexBufferGPU.h"
//Liu
#include "PrimeEngine/Render/D3D11Renderer.h"

// Sibling/Children includes
#include "EffectEnums.h"
#include "PEAlphaBlendState.h"
#include "PERasterizerState.h"
#include "PEDepthStencilState.h"


#include <vector>
#include <D3DCommon.h>
#include <D3DCompiler.h>
#include <d3d11.h>

#include "PrimeEngine/ProbeManager.h"
#include "PrimeEngine/SkyboxNew.h"
#include "PrimeEngine/PostProcess.h"

#include "math.h"

namespace PE {
namespace Components{
struct Effect;
struct DrawList;
struct Light;
};

struct EffectManager : public PE::PEAllocatableAndDefragmentable
{
	EffectManager(PE::GameContext &context, PE::MemoryArena arena);

	void loadDefaultEffects();
	void setupConstantBuffersAndShaderResources();
	
	Components::Effect *operator[] (const char *pEffectFilename);

	Handle getEffectHandle(const char *pEffectFilename)
	{
		return m_map.findHandle(pEffectFilename);
	}

		// Singleton
	static void Construct(PE::GameContext &context, PE::MemoryArena arena)
	{
		Handle handle("EFFECT_MANAGER", sizeof(EffectManager));
		/* EffectManager *p = */ new(handle) EffectManager(context, arena);
		
		// Singleton
		SetInstanceHandle(handle);

	}

	static EffectManager *Instance()
	{
		return s_myHandle.getObject<EffectManager>();
	}

	static Handle InstanceHandle()
	{
		return s_myHandle;
	}

	static void SetInstanceHandle(const Handle &handle)
	{
		// Singleton
		EffectManager::s_myHandle = handle;
	}

	inline ProbeManager *getProbeManagerPtr() { return &_probeManager; }
	inline SkyboxNew *getSkybox() { return &_skybox; }

	// + Deferred
	void setTextureAndDepthTextureRenderTargetForGBuffer();
	void setLightAccumTextureRenderTarget();

	//Liu
	// void EffectManager::setClassicalLightTextureRenderTarget();
	void drawClassicalLightPass(float angle);
	void createSphere(float radius, int sliceCount, int stackCount);
	void createSkyBoxGeom();

	void renderCubemapConvolutionSphere();
	void renderSkyboxNewSphere();
	void randomLightInfo(int num);
	void randomizeLight(PE::Components::Light *l, Vector3 *axis,int i);
	void rotateLight(float angle,int counter);
	void drawLightGbuffer();
	void updateLightDirection(Vector3 sprinkleDir);
	//Liu
	void drawRayTracingPass();
	void drawLightMipsPass(int curlevel, bool isSecBlur);
	void setLightMipsTextureRenderTarget(int level);
	
	void updateLight();

	void setFinalLDRTextureRenderTarget();

	void setTextureAndDepthTextureRenderTargetForDefaultRendering();
	void setTextureAndDepthTextureRenderTargetForGlow();
	void set2ndGlowRenderTarget();

	void setFrameBufferCopyRenderTarget();

	void setShadowMapRenderTarget();
	void endCurrentRenderTarget();

	void createSetShadowMapShaderValue(PE::Components::DrawList *pDrawList);

	void buildFullScreenBoard();
	void drawFullScreenQuad();
	void drawFirstGlowPass();
	void drawGlowSeparationPass();
	void drawSecondGlowPass();
	void drawMotionBlur();
	void drawFrameBufferCopy();

	// + Deferred
	void uploadDeferredClusteredConstants(float nearClip, float farClip);
	void drawClusteredQuadOnly(ID3D11ShaderResourceView *depth, ID3D11ShaderResourceView *rt0, ID3D11ShaderResourceView *rt1, ID3D11ShaderResourceView *rt2);
	void drawClusteredLightHDRPass();
	void drawDeferredFinalPass();
	void drawDeferredFinalToBackBuffer();

	void assignLightToClusters();

	void debugDrawRenderTarget(bool drawGlowRenderTarget, bool drawSeparatedGlow, bool drawGlow1stPass, bool drawGlow2ndPass, bool drawShadowRenderTarget);
	void debugDeferredRenderTarget(int which);

public:
	// Singleton
	static Handle s_myHandle;

	// Member vars
	StrToHandleMap m_map;

	TextureGPU m_glowSeparatedTextureGPU;
	Handle m_hGlowTargetTextureGPU;
	TextureGPU m_2ndGlowTargetTextureGPU;

	TextureGPU *m_pCurRenderTarget;
	TextureGPU *m_pCurDepthTarget;

	Handle m_hFinishedGlowTargetTextureGPU;
public:
	TextureGPU m_shadowMapDepthTexture;
	
	TextureGPU m_frameBufferCopyTexture;
#if 0
	// enable when readable textures for DX 9 are available
	TextureGPU m_readableTexture;
#endif
	// + Deferred
	Handle m_halbedoTextureGPU;
	Handle m_hnormalTextureGPU;
	Handle m_haccumHDRTextureGPU;
	Handle m_hfinalHDRTextureGPU;
	Handle m_hrootDepthBufferTextureGPU;
	

	//Liu
	Handle m_hpositionTextureGPU;
	Handle m_hmaterialTextureGPU;
	Handle m_htempMipsTextureGPU;
	Handle m_hrayTracingTextureGPU;
	// Handle m_hlightTextureGPU;
	struct LightInfo
	{
		Vector3 pos;
		Vector3 color;
		Vector3 obritAxis;
	};
	//Liu
	// Array<LightInfo> m_lights;

	// + End deferred 
	
	Matrix4x4 m_currentViewProjMatrix;
	Matrix4x4 m_previousViewProjMatrix;

	Handle m_hVertexBufferGPU;
	Handle m_hIndexBufferGPU;
	//Liu
	Handle m_hLightVertexBufferGPU;
	Handle m_hLightIndexBufferGPU;

	Handle m_hSkyBoxGeomVBGpu;
	Handle m_hSkyBoxGeomIBGpu;

	Handle m_hFirstGlowPassEffect;
	Handle m_hSecondGlowPassEffect;
	
	Handle m_hGlowSeparationEffect;
	Handle m_hMotionBlurEffect;
	Handle m_hColoredMinimalMeshTech;

	// + Deferred
	Handle m_hAccumulationHDRPassEffect;
	Handle m_hfinalHDRPassEffect;

	Handle m_hdebugPassEffect;

	// + Deferred cluster data - hard coded cluster size
	struct ClusterData
	{
		unsigned int offset;
		unsigned int counts;
		// short pointLightCount;
		// short spotLightCount;
	};

	Vector3 m_cMin;
	Vector3 m_cMax;
	static const int CX = 32;
	static const int CY = 8;
	static const int CZ = 32;
	std::vector<PE::Components::Light *> _dirLights;
	std::vector<PE::Components::Light *> _pointLights;
	std::vector<PE::Components::Light *> _spotLights;
	int _dirLightNum, _pointLightNum, _spotLightNum;
	// std::vector<short> _lightIndices;
	std::vector<unsigned int> _lightIndices;
	ClusterData _cluster[CZ][CY][CX];

	// DX11 resources - did not bother impl for TextureCPU
	ID3D11Texture3D *_clusterTex;
	ID3D11ShaderResourceView *_clusterTexShaderView;
	ID3D11Buffer *_lightIndicesBuffer;
	// ID3D11Texture1D *_lightIndicesTex;
	ID3D11ShaderResourceView *_lightIndicesBufferShaderView;

	// + End

	//Liu
	Handle m_hDeferredLightPassEffect;
	Handle m_hLightMipsPassEffect;
	Handle m_hRayTracingPassEffect;
	Handle m_hGBufferLightPassEffect;

	Handle m_hCubemapPrefilterPassEffect;

	Array<Handle> m_pixelShaderSubstitutes;
#	if APIABSTRACTION_D3D11
		PEMap<ID3D11PixelShader *> m_pixelShaders;
		PEMap<ID3D11VertexShader *> m_vertexShaders;
#	elif APIABSTRACTION_D3D9
		PEMap<IDirect3DPixelShader9*> m_pixelShaders;
		PEMap<IDirect3DVertexShader9*> m_vertexShaders;
#	elif APIABSTRACTION_OGL
	PEMap<GLuint> m_vertexShaders;
	PEMap<GLuint> m_pixelShaders;
#	elif PE_PLAT_IS_PSVITA
	PEMap<SceGxmVertexProgram *> m_vertexShaders;
	PEMap<SceGxmFragmentProgram *> m_pixelShaders;
#	endif

#	if APIABSTRACTION_D3D11
		PEMap<ID3D11Buffer *> m_cbuffers; // only DX 11 has constant buffers. DX9 just has constant registers
#	endif

	bool m_doMotionBlur;

	PE::MemoryArena m_arena; PE::GameContext *m_pContext;

	ProbeManager _probeManager;
	SkyboxNew _skybox;
	PostProcess _postProcess;
}; // class EffectManager

}; // namespace PE

#endif
