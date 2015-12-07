#ifndef BLUR_PS
#define BLUR_PS
#include "ColoredMinimalMesh_Structs.fx"
#include "StandardTextureResources.fx"

float CalcGaussianWeight(int sampleDist, float sigma)
{
    float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
    return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}

float4 Blur(COLORED_MINIMAL_MESH_PS_IN pIn, float2 texScale, float sigma)
{
	float weightSum = 0.0f;
    float4 color = 0;
    for (int i = -10; i < 10; i++)
    {
        float weight = CalcGaussianWeight(i, sigma);
        weightSum += weight;
        float2 texCoord = pIn.iColor;
        texCoord += (i / int2(640, 360)) * texScale; // hard code resolution for now
        float4 sample = gDiffuseMap.Sample(gDiffuseMapSampler, texCoord);
        color += sample * weight;
    }

    color /= weightSum;

    return color;
}

#endif 