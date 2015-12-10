#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "RenderJob.h"
#include "PrimeEngine/Scene/DrawList.h"

#if APIABSTRACTION_IOS
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>
#endif

extern "C"{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

#include "PrimeEngine/Game/Client/ClientGame.h"

namespace PE {

using namespace Events;
using namespace Components;

//Liu physics
#define B 0x1000
#define BM 0xff

#define N 0x1000
#define NP 12
#define NM 0xfff

static int p[B + B + 2];
static float g3[B + B + 2][3];
static float g2[B + B + 2][2];
static float g1[B + B + 2];

#define s_curve(t) (t * t * (3 - 2 * t))


#define lerp(t, a, b) (a + t * (b - a))

#define setup(i,b0,b1,r0,r1)\
	t = i + N;\
	b0 = ((int) t) & BM;\
	b1 = (b0 + 1) & BM;\
	r0 = t - (int) t;\
	r1 = r0 - 1;

void drawThreadFunctionJob(void *params)
{
	GameContext *pContext = static_cast<PE::GameContext *>(params);
	#if APIABSTRACTION_X360
		XSetThreadProcessor( GetCurrentThread(), 2 );
	#endif
	g_drawThreadInitializationLock.lock();
	//initialization here..
	g_drawThreadInitialized = true;
	g_drawThreadExited = false;
	g_drawThreadInitializationLock.unlock();

	//acquire rendring thread lock so that we can sleep on it until game thread wakes us up
	g_drawThreadLock.lock();

	// now we can signal main thread that this thread is initialized
	g_drawThreadInitializedCV.signal();
    while (1)
    {
		runDrawThreadSingleFrameThreaded(*pContext);
		if (g_drawThreadExited)
			return;
    }
    return;
}

void runDrawThreadSingleFrameThreaded(PE::GameContext &ctx)
{
	while (!g_drawThreadCanStart && !g_drawThreadShouldExit)
    {
		bool success = g_drawCanStartCV.sleep();
        assert(success);
    }
	g_drawThreadCanStart = false; // set to false for next frame

	//PEINFO("Draw thread got g_drawThreadLock\n");

	if (g_drawThreadShouldExit)
	{
		//right now game thread is waiting on this thread to finish
		g_drawThreadLock.unlock();
		g_drawThreadExited = true;
		return;
	}

	runDrawThreadSingleFrame(ctx);
}
//liu physics
float noise1(const float x){
	int bx0, bx1;
	float rx0, rx1, sx, t, u, v;

	setup(x, bx0, bx1, rx0, rx1);

	sx = s_curve(rx0);

	u = rx0 * g1[p[bx0]];
	v = rx1 * g1[p[bx1]];

	return lerp(sx, u, v);
}
//liu physics
static void normalize2(float v[2]){
	float s = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1]);
	v[0] *= s;
	v[1] *= s;
}

static void normalize3(float v[3]){
	float s = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= s;
	v[1] *= s;
	v[2] *= s;
}

void initNoise(){
	int i, j, k;

	for (i = 0; i < B; i++) {
		p[i] = i;

		g1[i] = (float)((rand() % (B + B)) - B) / B;

		for (j = 0; j < 2; j++)
			g2[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize2(g2[i]);

		for (j = 0; j < 3; j++)
			g3[i][j] = (float)((rand() % (B + B)) - B) / B;
		normalize3(g3[i]);
	}

	while (--i) {
		k = p[i];
		p[i] = p[j = rand() % B];
		p[j] = k;
	}

	for (i = 0; i < B + 2; i++) {
		p[B + i] = p[i];
		g1[B + i] = g1[i];
		for (j = 0; j < 2; j++)
			g2[B + i][j] = g2[i][j];
		for (j = 0; j < 3; j++)
			g3[B + i][j] = g3[i][j];
	}
}

//Liu
float m_angle = 0;


void runDrawThreadSingleFrame(PE::GameContext &ctx)
{
	int threadOwnershipMask = 0;
	
	ctx.getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);

	#if PE_ENABLE_GPU_PROFILING
	Timer t;
	PE::Profiling::Profiler::Instance()->startEventQuery(Profiling::Group_DrawThread, IRenderer::Instance()->getDevice(), t.GetTime(), "DrawThread");
	#endif

	#if APIABSTRACTION_D3D9
		//IRenderer::Instance()->getDevice()->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	#elif APIABSTRACTION_D3D11
		D3D11Renderer *pD3D11Renderer = static_cast<D3D11Renderer *>(ctx.getGPUScreen());
		ID3D11Device *pDevice = pD3D11Renderer->m_pD3DDevice;
		ID3D11DeviceContext *pDeviceContext = pD3D11Renderer->m_pD3DContext;

		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	#endif

	// hack 
	// CameraManager::Instance()->setCamera(CameraManager::PLAYER, CameraManager::Instance()->getActiveCameraHandle());

	IRenderer::checkForErrors("renderjob update start\n");

	Matrix4x4 viewProjMat;
	viewProjMat.loadIdentity();
	Matrix4x4 viewMat;
	viewMat.loadIdentity();
	Matrix4x4 projMat;
	projMat.loadIdentity();


	static float t = 0;
	
	float ft = 0.03f;//min(ctx.getDefaultGameControls()->m_frameTime, 0.05f);

	t += ft * powf(2.0f, 3);
	Vector3 sprinkleDir = Vector3(
		30 *  noise1(1.3f * t),
		36 * (noise1(t + 21.5823f) * 0.5f + 0.5f),
		30 *  noise1(-t)
		);
	//sprinkleDir.normalize();

	if(ctx.isSmallBalls)
		EffectManager::Instance()->updateLightDirection(sprinkleDir);

	// TODO: since we don't have proj mat in constant buf at hand; we grab from cameramanager
	// However, this will be off by one frame; but since proj mat doesn't always change, 
	// it is convenient for now to use this instead of passing an additional proj mat in const buf
	projMat = CameraManager::Instance()->getActiveCamera()->getCamSceneNode()->m_viewToProjectedTransform;


	auto &globalShaderValues = DrawList::InstanceReadOnly()->m_globalShaderValues;
	for (PrimitiveTypes::UInt32 isv = 0; isv < globalShaderValues.m_size; isv++)
	{
		ShaderAction *sv = globalShaderValues[isv].getObject<ShaderAction>();
		if (globalShaderValues[isv].getDbgName() == "RAW_DATA_PER_OBJECTGROUP")
		{
			SetPerObjectGroupConstantsShaderAction *orig = (SetPerObjectGroupConstantsShaderAction *)sv;
			viewProjMat = orig->m_data.gViewProj;
			viewMat = orig->m_data.gViewInv.inverse();
		}
	}

	IRenderer::RenderMode renderMode = ctx.getGPUScreen()->m_renderMode;
	bool disableScreenSpaceEffects = renderMode == IRenderer::RenderMode_DefaultNoPostProcess;
	if (!disableScreenSpaceEffects)
    {
		EffectManager::Instance()->updateLight();
		// -1) Assign light to clusters
		EffectManager::Instance()->assignLightToClusters();

		// printf("PRE: %d\n", (int)CameraManager::Instance()->getActiveCameraType());
		// 0) Render cubemap
		ProbeManager *pm = EffectManager::Instance()->getProbeManagerPtr();
		pm->Render(threadOwnershipMask);

		// 1) Fill GBuffer
		EffectManager::Instance()->setTextureAndDepthTextureRenderTargetForGBuffer();
		{
			PIXEvent event(L"Render Scene GBuffer");
			assert(DrawList::InstanceReadOnly() != DrawList::Instance());
			DrawList::InstanceReadOnly()->optimize();

			// TODO: skip shadow maps for now, since shadows are implemented differently
			// in deferred pipeline.
			
			ctx.getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);

			int x = -1;
			DrawList::InstanceReadOnly()->do_RENDER(*(Events::Event **)&x, threadOwnershipMask, NULL, NULL);
			if (ctx.isSmallBalls)
				EffectManager::Instance()->drawLightGbuffer();
			ctx.getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);
		}
		
		EffectManager::Instance()->endCurrentRenderTarget();
		// printf("POST: %d\n", (int)CameraManager::Instance()->getActiveCameraType());



		//}
#if APIABSTRACTION_D3D11
		pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
#else
		assert(false);
#endif
		//Liu
		m_angle += 1;

		//EffectManager::Instance()->rotateLight(0.01, m_angle);
		
		if (ctx._renderMode == 0)
		{
			
			// 2.1) shared with cube map lighting - moved to front
			// 2.2) Render lights
			EffectManager::Instance()->setLightAccumTextureRenderTarget();
			EffectManager::Instance()->drawClusteredLightHDRPass();

			ID3D11RenderTargetView *lightClusterSRV = EffectManager::Instance()->m_pCurRenderTarget->m_pRenderTargetView;
			ID3D11DepthStencilView *geomPassDepth = EffectManager::Instance()->m_hrootDepthBufferTextureGPU.getObject<TextureGPU>()->m_DepthStencilView;

			EffectManager::Instance()->getSkybox()->Render(viewMat, projMat, lightClusterSRV, geomPassDepth);
			EffectManager::Instance()->endCurrentRenderTarget();
		}
		else
		{
			// EffectManager::Instance()->setClassicalLightTextureRenderTarget();
			//EffectManager::Instance()->rotateLight(0.01);
			EffectManager::Instance()->setLightAccumTextureRenderTarget();
			EffectManager::Instance()->drawClassicalLightPass(0.01); //0.01
			//EffectManager::Instance()->drawClassicalLightPass(Vector3(2,0,1),5,Vector4(1,1,0,1),m_angle);
			EffectManager::Instance()->endCurrentRenderTarget();
		}

		////create mipmaps for lightTexture
		//{
		//	for (int i = 1; i < 11; i++)
		//	{
		//		EffectManager::Instance()->setLightMipsTextureRenderTarget(i);
		//		EffectManager::Instance()->drawLightMipsPass(i, false);
		//		EffectManager::Instance()->endCurrentRenderTarget();

		//		EffectManager::Instance()->setLightMipsTextureRenderTarget(i);
		//		EffectManager::Instance()->drawLightMipsPass(i, true);
		//		EffectManager::Instance()->endCurrentRenderTarget();
		//	}
		//	
		//}

		//create raybuffer
		if (ctx.isSSR)
		{
			PIXEvent event(L"SSR Pass");
			// EffectManager::Instance()->setLightAccumTextureRenderTarget();
			EffectManager::Instance()->drawRayTracingPass();
			EffectManager::Instance()->endCurrentRenderTarget();
		}


		// 3) Render post process & final pass
		// EffectManager::Instance()->drawDeferredFinalToBackBuffer();
		if (ctx._debugMode == 0)
		{
			{
				PIXEvent event(L"Render Final Pass");
				EffectManager::Instance()->drawDeferredFinalPass();
				EffectManager::Instance()->endCurrentRenderTarget();
			}

			{
				PIXEvent event(L"Post processing");
				EffectManager::Instance()->_postProcess.Render();
			}
		}
		else
		{
			EffectManager::Instance()->debugDeferredRenderTarget(ctx._debugMode - 1);
		}

		// Draw text & hud & etc.
		{
			PIXEvent event(L"Render UI");
			// clear by color target and depth by tonemapping in postprocessing
			EffectManager::Instance()->m_pContext->getGPUScreen()->setRenderTargetsAndViewportWithDepth(0, 0, false, false);
			DrawList::ZOnlyInstanceReadOnly()->optimize();
			ctx.getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);
			DrawList::ZOnlyInstanceReadOnly()->do_RENDER(NULL, threadOwnershipMask, NULL, NULL);
			ctx.getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);
		}

    }
    else
    {
        // use simple rendering
        // the render target here is the same as end result of motion blur
        EffectManager::Instance()->setTextureAndDepthTextureRenderTargetForDefaultRendering();
                
        assert(DrawList::InstanceReadOnly() != DrawList::Instance());
                
		DrawList::InstanceReadOnly()->optimize();
		
		#if PE_PLAT_IS_PSVITA
			EffectManager::Instance()->drawFullScreenQuad();
		#endif
		
		// set global shader value (applied to all draw calls) for shadow map texture
			/*	if (renderShadowMap)
					EffectManager::Instance()->createSetShadowMapShaderValue(DrawList::InstanceReadOnly());*/

		ctx.getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);

        DrawList::InstanceReadOnly()->do_RENDER(NULL, threadOwnershipMask, NULL, NULL);

		ctx.getGPUScreen()->AcquireRenderContextOwnership(threadOwnershipMask);

        // Required when rendering to backbuffer directly. also done in drawMotionBlur_EndScene() since it is the last step of post process
		EffectManager::Instance()->endCurrentRenderTarget();
    }
            
	ctx.getGPUScreen()->endFrame();

    // Flip screen
	ctx.getGPUScreen()->swap(false);
    PE::IRenderer::checkForErrors("");

			
	#if PE_ENABLE_GPU_PROFILING
		Timer::TimeType time = t.TickAndGetCurrentTime();
		// only perform flush when gpu profiling is enabled, swap() above will call present which should flush when PE_DETAILED_GPU_PROFILING = 0 == vsync enabled.
		Profiling::Profiler::Instance()->update(Profiling::Group_DrawCalls, PE_DETAILED_GPU_PROFILING, true, time, t);
		Profiling::Profiler::Instance()->update(Profiling::Group_DrawThread, PE_DETAILED_GPU_PROFILING, true, time, t);
	#endif

	ctx.getGPUScreen()->ReleaseRenderContextOwnership(threadOwnershipMask);
}


}; // namespace PE

