#ifndef HLSL_DEFERREDLIGHTCOMMON
#define HLSL_DEFERREDLIGHTCOMMON

#if APIABSTRACTION_D3D11

#define MAX_DIR_LIGHTS 8
#define MAX_POINT_LIGHTS 1024
#define MAX_SPOT_LIGHTS 512

struct cbDirectionalLight // 2 vec4
{
	float3 cDir;
	float pad;
	float3 cColor;
	float pad2;
};

struct cbPointLight // 2 vec4
{
	float3 cPos;
	float cRadius;
	float3 cColor;
	float cSpecPow;
};

struct cbSpotLight // 3 vec4
{
	float3 cPos;
	float cSpotCutOff;
	float3 cColor;
	float cSpotPow;
	float3 cDir;
	float pad;
};

struct ClusterData
{
	uint offset;
	uint counts; // pointlight | spotLight
};

// We use b4 since API_BUFFER_INDEX is up to 3 in StandardConstants.fx
cbuffer cbClusteredShadingConsts : register(b4)
{
	float cs_near      : packoffset(c0.x);
    float cs_far       : packoffset(c0.y);
    float cs_projA     : packoffset(c0.z);
    float cs_projB     : packoffset(c0.w);     
	float3 cs_camPos   : packoffset(c1);
	float cs_pad	   : packoffset(c1.w);
	float3 cs_camZAxis : packoffset(c2);
	uint cs_dirLightNum : packoffset(c2.w);
	
	float3 cs_scale		: packoffset(c3);
	float cs_pad2		: packoffset(c3.w);
	float3 cs_bias		: packoffset(c4);
	float cs_pad3		: packoffset(c4.w);
	
	cbDirectionalLight cs_dirLights[MAX_DIR_LIGHTS] : packoffset(c5); 	 // 2 * 8 = 16 | 16 + 5 = 21
	cbPointLight cs_pointLights[MAX_POINT_LIGHTS] 	: packoffset(c21);	 // 2 * 1024 = 2048 | 2048 + 21 = 2069
	cbSpotLight cs_spotLights[MAX_SPOT_LIGHTS] 		: packoffset(c2069); // 3 * 512 = 1536 | 1536 + 2069 = 3605 < 4096 -> ok
};

// texture register maxed t70 used in animations
StructuredBuffer<uint> gLightIndices : register(t71);

// 3D texture for clusters
Texture3D<ClusterData> gClusters : register(t8);

#elif APIABSTRACTION_D3D9
#define MAX_POINT_LIGHTS 3

struct cbPointLight // 2 vec4
{
	float3 cPos;
	float cRadius;
	float3 cColor;
	float cSpecPow;
};

struct ClusterData
{
	float offset;
	float counts;
};

// clustered forward shading data
sampler3D gClusters : register(s8);
sampler gLightIndices : register(s9);

float4 cs_scale : register(c163);
float4 cs_bias  : register(c164);
cbPointLight cs_pointLights[MAX_POINT_LIGHTS] : register(c165);

// 224 - 165 = 159 / 2 = 79 > 78 - ok

#endif // D3D9

#endif