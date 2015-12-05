#pragma once
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"
#include "PrimeEngine/Render/D3D11Renderer.h"
#include "PrimeEngine/MemoryManagement/Handle.h"
#include "PrimeEngine/RenderUtils.h"

class ProbeManager
{
public:
	ProbeManager();
	
	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context);



private:
	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
};