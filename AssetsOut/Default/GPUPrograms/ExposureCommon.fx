#ifndef EXPOSURE_COMMON
#define EXPOSURE_COMMON

cbuffer DepthBlurConsts : register(b0)
{
	float2 ProjectionAB;
	float GatherBlurSize;
	uint enableInstagram;
	float4 DOFDepths;
	float KeyValue;
	uint enableManualExposure;
	float manualExposure;
	float pad1;
}

float3 CalcExposedColor(float3 color, float avgLuminance, float offset, out float exposure)
{
    // Use geometric mean
    avgLuminance = max(avgLuminance, 0.001f);
	
	if (enableManualExposure > 0)
	{
	    exposure = manualExposure;
	}
	else
	{
		float linearExposure = (KeyValue / avgLuminance);
		exposure = log2(max(linearExposure, 0.0001f));
	}
	
    exposure += offset;
    // exposure -= ExposureScale;
	
    return exp2(exposure) * color;
}

float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}



#endif