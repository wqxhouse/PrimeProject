#ifndef __PYENGINE_2_0_SetClusteredShadingConstants_SHADER_ACTION_H__
#define __PYENGINE_2_0_SetClusteredShadingConstants_SHADER_ACTION_H__

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/Render/IRenderer.h"


#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/Render/D3D9Renderer.h"
#include "PrimeEngine/Render/GLRenderer.h"
#include "PrimeEngine/Math/Matrix4x4.h"
// Sibling/Children includes
#include "ShaderAction.h"

namespace PE {

	namespace Components{
		struct Effect;
	}

	struct SetClusteredShadingConstantsShaderAction : ShaderAction
	{
		SetClusteredShadingConstantsShaderAction(PE::GameContext &context, PE::MemoryArena arena)
		{
			m_arena = arena; m_pContext = &context;
		}

		virtual ~SetClusteredShadingConstantsShaderAction() {}

		virtual void bindToPipeline(Components::Effect *pCurEffect = NULL);
		virtual void unbindFromPipeline(Components::Effect *pCurEffect = NULL);
		virtual void releaseData();
#if PE_PLAT_IS_PSVITA
		static void * s_pBuffer;
#	elif APIABSTRACTION_D3D11
		static ID3D11Buffer * s_pBuffer; // the buffer is first filled with data and then bound to pipeline
		// unlike DX9 where we just set registers
#	endif
		PE::MemoryArena m_arena; PE::GameContext *m_pContext;

		struct cbClusteredShadingConsts
		{
			float cNear;
			float cFar;
			float cProjA;
			float cProjB;
		};

		struct cbDirectionalLight // 2 vec4
		{
			Vector3 cDir;
			float pad;
			Vector3 cColor;
			float pad2;
		};

		struct cbPointLight // 2 vec4
		{
			Vector3 cPos;
			float cRadius;
			Vector3 cColor;
			float cSpecPow;
		};

		struct cbSpotLight // 3 vec4
		{
			Vector3 cPos;
			float cSpotCutOff;
			Vector3 cColor;
			float cSpotPow;
			Vector3 cDir;
			float pad;
		};

		// the actual data that is transferred to video memory
		struct Data {
			cbClusteredShadingConsts csconsts;
			Vector3 camPos;
			float pad;
			Vector3 camZAxisWS;
			unsigned int dirLightNum;

			Vector3 scale;
			float pad2;
			Vector3 bias;
			float pad3;

			cbDirectionalLight dirLights[8];
			cbPointLight pointLights[1024];
			cbSpotLight spotLights[512];

			unsigned int enableIndirectLighting;
			unsigned int enableLocalCubemap;
			unsigned int paddd;
			unsigned int paddd2;

		} m_data;

#	if APIABSTRACTION_OGL
		// we need this struct for OGL implementation with cg
		// because we need to store a list of cgParameters in each effect (vs setting register values in DX)
		struct PerEffectBindIds
		{
#       if APIABSTRACTION_IOS
			GLuint v_gViewProj, f_gViewProj;
			GLuint v_gPreviousViewProjMatrix, f_gPreviousViewProjMatrix;
			GLuint v_gViewProjInverseMatrix, f_gViewProjInverseMatrix;
			GLuint v_gViewInv, f_gViewInv;

			GLuint v_gLightWVP, f_gLightWVP;
			GLuint v_gEyePosW, f_gEyePosW;
			GLuint v_xyzgEyePosW_wDoMotionBlur, f_xyzgEyePosW_wDoMotionBlur;

			struct LightCGParams{
				GLuint pos;
				GLuint dir;

				GLuint ambient;
				GLuint diffuse;
				GLuint specular;

				GLuint atten_spot;
				GLuint range_type;
			};

#       else

			CGparameter v_gViewProj, f_gViewProj;
			CGparameter v_gPreviousViewProjMatrix, f_gPreviousViewProjMatrix;
			CGparameter v_gViewProjInverseMatrix, f_gViewProjInverseMatrix;
			CGparameter v_gViewInv, f_gViewInv;

			CGparameter v_gLightWVP, f_gLightWVP;
			CGparameter v_gEyePosW, f_gEyePosW;
			CGparameter v_xyzgEyePosW_wDoMotionBlur, f_xyzgEyePosW_wDoMotionBlur;

			struct LightCGParams{
				CGparameter pos;
				CGparameter dir;

				CGparameter ambient;
				CGparameter diffuse;
				CGparameter specular;

				CGparameter atten_spot;
				CGparameter range_type;
			};
#       endif
			LightCGParams v_gLight[NUM_LIGHT_SOURCES_DATAS], f_gLight[NUM_LIGHT_SOURCES_DATAS];
			void initialize(Components::Effect *pEffect);
			void initializeLightCGParams(unsigned int index, LightCGParams &out_v, LightCGParams &out_f, Components::Effect *pEffect);
		};
#endif
	};


}; // namespace PE
#endif
