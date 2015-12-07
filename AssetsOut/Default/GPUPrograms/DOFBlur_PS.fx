#ifndef DOFBLUR_PS
#define DOFBLUR_PS
#include "ColoredMinimalMesh_Structs.fx"
#include "StandardTextureResources.fx"

float3 DOFBlur(COLORED_MINIMAL_MESH_PS_IN pIn, float2 texScale)
{
	const uint NumSamples = 5;

	const float Weights[NumSamples] = {
		0.06100f,
		0.24197f,
		0.39894f,
		0.24197f,
		0.06100f
	};

    float3 samples[NumSamples];
    float2 depthSamples[NumSamples];

	[unroll]
    for(int i = -2; i <= 2; ++i) {
        float2 sampleCoord = pIn.iColor;
        sampleCoord += (i / int2(1280, 720)) * texScale;

        samples[i + 2] = gDiffuseMap.Sample(gDiffuseMapSampler, sampleCoord).rgb;
		depthSamples[i + 2] = gAdditionalDiffuseMap.Sample(gAdditionalDiffuseMapSampler, sampleCoord).xy;
    }

    float3 output = 0.0f;
    float centerWeight = Weights[2];
    const float centerDepth = depthSamples[2].x;

	[unroll]
    for(int j = 0; j < 2; ++j) {
        float diff = depthSamples[j].x >= centerDepth;
        diff += depthSamples[j].y;
        diff = saturate(diff);
        float weight = Weights[j] * diff;
        centerWeight += Weights[j] * (1.0f - diff);
        output += weight * samples[j];
    }

	[unroll]
    for(int k = 3; k < 5; ++k) {
        float diff = depthSamples[k].x >= centerDepth;
        diff += depthSamples[k].y;
        diff = saturate(diff);
        float weight = Weights[k] * diff;
        centerWeight += Weights[k] * (1.0f - diff);
        output += weight * samples[k];
    }

    output += samples[2] * centerWeight;

    return output;
}

#endif 