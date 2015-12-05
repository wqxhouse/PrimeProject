#include "ProbeManager.h"

void ProbeManager::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_device = device;
	_context = context;

}