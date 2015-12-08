#include "SkyboxNew.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"
#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"

#include "PrimeEngine/DDSTextureLoader.h"

void SkyboxNew::Initialize(PE::GameContext *context, PE::MemoryArena arena)
{
	PI_ = 3.14159265359f;

	_pContext = context;
	_arena = arena;

	_sunTheta = 60;
	_sunPhi = 200;

	// _turbidity = 4.0;
	_latitude = 40 * (float)PI_ / 180;
	_solarDeclination = 0;
	_solarTime = 0;

	SetTurbidity(4.0f);
	SetSolarTime(0.0f);


	PE::D3D11Renderer *pD3D11Renderer = static_cast<PE::D3D11Renderer *>(_pContext->getGPUScreen());
	_context = pD3D11Renderer->m_pD3DContext;
	_device = pD3D11Renderer->m_pD3DDevice;

	_vsConstants.Initialize(_device);
	_psConstants.Initialize(_device);

	// Create a depth-stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dsDesc.StencilEnable = false;
	dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dsDesc.BackFace = dsDesc.FrontFace;
	DXCall(_device->CreateDepthStencilState(&dsDesc, &_dsState));

	// Create a blend state
	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	for (UINT i = 0; i < 8; ++i)
	{
		blendDesc.RenderTarget[i].BlendEnable = false;
		blendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
	}
	DXCall(_device->CreateBlendState(&blendDesc, &_blendState));

	// Create a rasterizer state
	D3D11_RASTERIZER_DESC rastDesc;
	rastDesc.AntialiasedLineEnable = false;
	rastDesc.CullMode = D3D11_CULL_NONE;
	rastDesc.DepthBias = 0;
	rastDesc.DepthBiasClamp = 0.0f;
	rastDesc.DepthClipEnable = true;
	rastDesc.FillMode = D3D11_FILL_SOLID;
	rastDesc.FrontCounterClockwise = false;
	rastDesc.MultisampleEnable = false;
	rastDesc.ScissorEnable = false;
	rastDesc.SlopeScaledDepthBias = 0;
	DXCall(_device->CreateRasterizerState(&rastDesc, &_rasterizerState));

	D3D11_SAMPLER_DESC sampDesc;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.BorderColor[0] = 0;
	sampDesc.BorderColor[1] = 0;
	sampDesc.BorderColor[2] = 0;
	sampDesc.BorderColor[3] = 0;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.MaxAnisotropy = 1;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.MinLOD = 0;
	sampDesc.MipLODBias = 0;
	DXCall(_device->CreateSamplerState(&sampDesc, &_samplerState));

	ID3D11ResourcePtr resource;
	DXCall(DirectX::CreateDDSTextureFromFileEx(_device, L"AssetsOut\\FinalProjectAssets\\Milkyway.dds", 0, D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
		&resource, &_nightCubemap, nullptr));

	_isSky = true;
	_curCubemap = -1;
}

Vector3 sphericalConv(float theta, float phi)
{
	return Vector3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
}

float deg2rad(float deg)
{
	return deg * 3.14159265359f / 180.0f;
}

//Vector3 SkyboxNew::GetSunDirection()
//{
//	return -sphericalConv(deg2rad(_sunTheta), deg2rad(_sunPhi));
//}

void SkyboxNew::RenderSky(const Matrix4x4 &viewMat, const Matrix4x4 &projMat, ID3D11RenderTargetView *rtView, ID3D11DepthStencilView *dsView)
{
	float blendFactor[4] = { 1, 1, 1, 1 };
	_context->RSSetState(_rasterizerState);
	_context->OMSetBlendState(_blendState, blendFactor, 0xFFFFFFFF);
	_context->OMSetDepthStencilState(_dsState, 0);
	_context->PSSetSamplers(0, 1, &(_samplerState.GetInterfacePtr()));

	ID3D11RenderTargetView *rtvs[1] = { rtView };
	_context->OMSetRenderTargets(1, rtvs, dsView);

	UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	_context->RSGetViewports(&numViewports, oldViewports);

	// Set a viewport with MinZ pushed back
	D3D11_VIEWPORT newViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (UINT i = 0; i < numViewports; ++i)
	{
		newViewports[i] = oldViewports[0];
		newViewports[i].MinDepth = 1.0f;
		newViewports[i].MaxDepth = 1.0f;
	}
	_context->RSSetViewports(numViewports, newViewports);

	_vsConstants.Data.View = viewMat;
	_vsConstants.Data.Proj = projMat;
	_vsConstants.ApplyChanges(_context);
	_vsConstants.SetVS(_context, 0);

	//_psConstants.Data.A = _A;
	//_psConstants.Data.B = _B;
	//_psConstants.Data.C = _C;
	//_psConstants.Data.D = _D;
	//_psConstants.Data.E = _E;
	//_psConstants.Data.Z = _Z;
	//_psConstants.Data.SunDirection = -sphericalConv(deg2rad(_sunTheta), deg2rad(_sunPhi));

	_psConstants.Data.cY = _cY;
	_psConstants.Data.cx = _cx;
	_psConstants.Data.cy = _cy;
	_psConstants.Data.solar_azimuth = _solarAzimuth;
	_psConstants.Data.solar_zenith = _solarZenith;
	_psConstants.Data.turbidity = _turbidity;
	_psConstants.Data.Yz = _Yz;
	_psConstants.Data.xz = _xz;
	_psConstants.Data.yz = _yz;
	_psConstants.Data.sunColor = _LightColor;
	_psConstants.Data.night = _solarZenith > PI_ / 2 ? 1.0f : -1.0f;

	_psConstants.ApplyChanges(_context);
	_psConstants.SetPS(_context, 0);

	_nightCubemap;

	ID3D11ShaderResourceView *srvs[1] = { _nightCubemap };
	_context->PSSetShaderResources(0, 1, srvs);


	PE::EffectManager::Instance()->renderSkyboxNewSphere(1);

	ID3D11SamplerState *sampStates[1] = { nullptr };
	_context->PSSetSamplers(0, 1, sampStates);

	srvs[0] = nullptr;
	_context->PSSetShaderResources(0, 1, srvs);

	// Set the viewport back to what it was
	_context->RSSetViewports(numViewports, oldViewports);

	rtvs[0] = nullptr;
	_context->OMSetRenderTargets(1, rtvs, nullptr);
}

void SkyboxNew::RenderSkyCubemap(const Matrix4x4 &viewMat, const Matrix4x4 &projMat, ID3D11RenderTargetView *rtView, ID3D11DepthStencilView *dsView)
{
	float blendFactor[4] = { 1, 1, 1, 1 };
	_context->RSSetState(_rasterizerState);
	_context->OMSetBlendState(_blendState, blendFactor, 0xFFFFFFFF);
	_context->OMSetDepthStencilState(_dsState, 0);
	_context->PSSetSamplers(0, 1, &(_samplerState.GetInterfacePtr()));

	ID3D11RenderTargetView *rtvs[1] = { rtView };
	_context->OMSetRenderTargets(1, rtvs, dsView);

	UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	_context->RSGetViewports(&numViewports, oldViewports);

	// Set a viewport with MinZ pushed back
	D3D11_VIEWPORT newViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	for (UINT i = 0; i < numViewports; ++i)
	{
		newViewports[i] = oldViewports[0];
		newViewports[i].MinDepth = 1.0f;
		newViewports[i].MaxDepth = 1.0f;
	}
	_context->RSSetViewports(numViewports, newViewports);

	ID3D11ShaderResourceView *srvs[1] = { _skyCubemaps[_curCubemap] };
	_context->PSSetShaderResources(0, 1, srvs);

	_vsConstants.Data.View = viewMat;
	_vsConstants.Data.Proj = projMat;
	_vsConstants.ApplyChanges(_context);
	_vsConstants.SetVS(_context, 0);

	PE::EffectManager::Instance()->renderSkyboxNewSphere(0);

	ID3D11SamplerState *sampStates[1] = { nullptr };
	_context->PSSetSamplers(0, 1, sampStates);

	srvs[0] = nullptr;
	_context->PSSetShaderResources(0, 1, srvs);

	// Set the viewport back to what it was
	_context->RSSetViewports(numViewports, oldViewports);

	rtvs[0] = nullptr;
	_context->OMSetRenderTargets(1, rtvs, nullptr);
}

void SkyboxNew::Render(const Matrix4x4 &viewMat, const Matrix4x4 &projMat, ID3D11RenderTargetView *rtView, ID3D11DepthStencilView *dsView)
{
	PIXEvent event(L"Render skybox");
	// calcPreetham(deg2rad(_sunTheta), 4.0f, 1.1f);

	if (_isSky)
	{
		RenderSky(viewMat, projMat, rtView, dsView);
	}
	else
	{
		RenderSkyCubemap(viewMat, projMat, rtView, dsView);
	}

}


static float perez(float theta, float gamma, float A, float B, float C, float D, float E)
{
	return (1.f + A * exp(B / (cos(theta) + 0.01))) * (1.f + C * exp(D * gamma) + E * cos(gamma) * cos(gamma));
}

static float zenithLuminance(float sunTheta, float turbidity)
{
	float chi = (4.f / 9.f - turbidity / 120) * (3.14159265359 - 2 * sunTheta);

	return (4.0453 * turbidity - 4.9710) * tan(chi) - 0.2155 * turbidity + 2.4192;
}

static float zenithChromacity(const Vector4& c0, const Vector4& c1, const Vector4& c2, float sunTheta, float turbidity)
{
	Vector4 thetav = Vector4(sunTheta * sunTheta * sunTheta, sunTheta * sunTheta, sunTheta, 1.0);
	float dot1 = thetav.dotProduct(c0);
	float dot2 = thetav.dotProduct(c1);
	float dot3 = thetav.dotProduct(c2);
	Vector3 turbidityVec = Vector3(turbidity * turbidity, turbidity, 1);
	float res = turbidityVec.dotProduct(Vector3(dot1, dot2, dot3));
	return res;
}

void SkyboxNew::calcPreetham(float sunTheta, float turbidity, float normalizedSunY)
{
	// Assert_(sunTheta >= 0 && sunTheta <= 3.14159265359 / 2);
	Assert_(turbidity >= 1);

	if (sunTheta < 0) _sunTheta = 0;
	if (sunTheta > 3.14159265359 / 2) _sunTheta = 3.14159265359 / 2;

	// A.2 Skylight Distribution Coefficients and Zenith Values: compute Perez distribution coefficients
	//Vector3 A = Vector3(-0.0193, -0.0167, 0.1787) * turbidity + Vector3(-0.2592, -0.2608, -1.4630);
	//Vector3 B = Vector3(-0.0665, -0.0950, -0.3554) * turbidity + Vector3(0.0008, 0.0092, 0.4275);
	//Vector3 C = Vector3(-0.0004, -0.0079, -0.0227) * turbidity + Vector3(0.2125, 0.2102, 5.3251);
	//Vector3 D = Vector3(-0.0641, -0.0441, 0.1206) * turbidity + Vector3(-0.8989, -1.6537, -2.5771);
	//Vector3 E = Vector3(-0.0033, -0.0109, -0.0670) * turbidity + Vector3(0.0452, 0.0529, 0.3703);

	Vector3 A = Vector3(-0.0193, -0.0167, 0.1787) * turbidity + Vector3(-0.2592, -0.2608, -1.4630);
	Vector3 B = Vector3(-0.0665, -0.0950, -0.3554) * turbidity + Vector3(0.0008, 0.0092, 0.4275);
	Vector3 C = Vector3(-0.0004, -0.0079, -0.0227) * turbidity + Vector3(0.2125, 0.2102, 5.3251);
	Vector3 D = Vector3(-0.0641, -0.0441, 0.1206) * turbidity + Vector3(-0.8989, -1.6537, -2.5771);
	Vector3 E = Vector3(-0.0033, -0.0109, -0.0670) * turbidity + Vector3(0.0452, 0.0529, 0.3703);

	// A.2 Skylight Distribution Coefficients and Zenith Values: compute zenith color
	Vector3 Z;
	Z.m_x = zenithChromacity(Vector4(0.00166, -0.00375, 0.00209, 0), Vector4(-0.02903, 0.06377, -0.03202, 0.00394), Vector4(0.11693, -0.21196, 0.06052, 0.25886), sunTheta, turbidity);
	Z.m_y = zenithChromacity(Vector4(0.00275, -0.00610, 0.00317, 0), Vector4(-0.04214, 0.08970, -0.04153, 0.00516), Vector4(0.15346, -0.26756, 0.06670, 0.26688), sunTheta, turbidity);
	Z.m_z = zenithLuminance(sunTheta, turbidity);
	// Z.m_z *= 1000; // conversion from kcd/m^2 to cd/m^2
	Z.m_z *= 10; 

	// 3.2 Skylight Model: pre-divide zenith color by distribution denominator
	Z.m_x /= perez(0, sunTheta, A.m_x, B.m_x, C.m_x, D.m_x, E.m_x);
	Z.m_y /= perez(0, sunTheta, A.m_y, B.m_y, C.m_y, D.m_y, E.m_y);
	Z.m_z /= perez(0, sunTheta, A.m_z, B.m_z, C.m_z, D.m_z, E.m_z);

	// For low dynamic range simulation, normalize luminance to have a fixed value for sun
	//if (normalizedSunY)
	//{
	//	Z.m_z = normalizedSunY / perez(sunTheta, 0, A.m_z, B.m_z, C.m_z, D.m_z, E.m_z);
	//}

	_A = A; _B = B; _C = C; _D = D; _E = E; _Z = Z;
}

double SkyboxNew::Perez(float zenith, float gamma, const Coefficients &coeffs)
{
	return (1 + coeffs.A * exp(coeffs.B / cos(zenith))) *
		(1 + coeffs.C * exp(coeffs.D * gamma) + coeffs.E * pow(cos(gamma), 2));
}


void SkyboxNew::CalculateZenitalAbsolutes()
{
	float Yz = (4.0453 * _turbidity - 4.9710) * tan((4.0 / 9 - _turbidity / 120.0f) * (PI_ - 2 * _solarZenith)) - 0.2155 * _turbidity + 2.4192;
	float Y0 = (4.0453 * _turbidity - 4.9710) * tan((4.0 / 9 - _turbidity / 120.0f) * PI_)-0.2155 * _turbidity + 2.4192;
	_Yz = (float)(Yz / Y0);

	float z3 = (float)pow(_solarZenith, 3);
	float z2 = _solarZenith * _solarZenith;
	float z = _solarZenith;
	Vector3 T_vec(_turbidity * _turbidity, _turbidity, 1);

	Vector3 x = Vector3(
		0.00166f * z3 - 0.00375f * z2 + 0.00209f * z,
		-0.02903f * z3 + 0.06377f * z2 - 0.03202f * z + 0.00394f,
		0.11693f * z3 - 0.21196f * z2 + 0.06052f * z + 0.25886f);
	float xz = T_vec.dotProduct(x);
	_xz = xz;

	Vector3 y = Vector3(
		0.00275f * z3 - 0.00610f * z2 + 0.00317f * z,
		-0.04214f * z3 + 0.08970f * z2 - 0.04153f * z + 0.00516f,
		0.15346f * z3 - 0.26756f * z2 + 0.06670f * z + 0.26688f);
	float yz = T_vec.dotProduct(y);
	_yz = yz;
}

void SkyboxNew::CalculateCoefficents()
{
	Coefficients coeffsY;
	coeffsY.A = 0.1787f * _turbidity - 1.4630f;
	coeffsY.B = -0.3554f * _turbidity + 0.4275f;
	coeffsY.C = -0.0227f * _turbidity + 5.3251f;
	coeffsY.D = 0.1206f * _turbidity - 2.5771f;
	coeffsY.E = -0.0670f * _turbidity + 0.3703f;
	_cY = coeffsY;

	Coefficients coeffsx;
	coeffsx.A = -0.0193f * _turbidity - 0.2592f;
	coeffsx.B = -0.0665f * _turbidity + 0.0008f;
	coeffsx.C = -0.0004f * _turbidity + 0.2125f;
	coeffsx.D = -0.0641f * _turbidity - 0.8989f;
	coeffsx.E = -0.0033f * _turbidity + 0.0452f;
	_cx = coeffsx;

	Coefficients coeffsy;
	coeffsy.A = -0.0167f * _turbidity - 0.2608f;
	coeffsy.B = -0.0950f * _turbidity + 0.0092f;
	coeffsy.C = -0.0079f * _turbidity + 0.2102f;
	coeffsy.D = -0.0441f * _turbidity - 1.6537f;
	coeffsy.E = -0.0109f * _turbidity + 0.0529f;
	_cy = coeffsy;
}

void SkyboxNew::CalculateLightColor()
{
	Vector3 nightColor(0.01f, 0.01f, 0.03f);
	if (_solarZenith > PI_ / 2)
	{
		_LightColor = nightColor;
		return;
	}

	float Ys = _Yz * Perez(_solarZenith, 0, _cY) / Perez(0, _solarZenith, _cY);
	float xs = _xz * Perez(_solarZenith, 0, _cx) / Perez(0, _solarZenith, _cx);
	float ys = _yz * Perez(_solarZenith, 0, _cy) / Perez(0, _solarZenith, _cy);
	Vector3 dayColor = calcRGB((float)Ys, (float)xs, (float)ys);

	float interpolation = (float)max(0.0f, min(1.0f, (_solarZenith - PI_ / 2 + 0.2f) / 0.2f));
	printf("interpo: %.2f\n", interpolation);
	_LightColor = (interpolation * nightColor + (1 - interpolation) * dayColor);
}

Vector3 SkyboxNew::calcRGB(float Y, float x, float y)
{
	float X = x / y * Y;
	float Z = (1 - x - y) / y * Y;
	Vector3 rgb;
	rgb.m_x = max(0.0f, min(1.0f, 3.2406f * X - 1.5372f * Y - 0.4986f * Z));
	rgb.m_y = max(0.0f, min(1.0f, -0.9689f * X + 1.8758f * Y + 0.0415f * Z));
	rgb.m_z = max(0.0f, min(1.0f, 0.0557f * X - 0.2040f * Y + 1.0570f * Z));
	return rgb;
}

void SkyboxNew::SetTurbidity(float turbitidy)
{
	_turbidity = turbitidy;
	CalculateZenitalAbsolutes();
	CalculateCoefficents();
	CalculateLightColor();
}

void SkyboxNew::SetSolarZenith(float zenith)
{
	zenith = min(zenith, (float)PI_ / 2 + 0.3f);
	_solarZenith = zenith;
	CalculateZenitalAbsolutes();
	CalculateLightColor();
}

void SkyboxNew::SetSolarAzimuth(float azimuth)
{
	_solarAzimuth = azimuth;
}

void SkyboxNew::SetSolarTime(float solarTime)
{
	_solarTime = solarTime;
	float solarZenith = (float)acos(sin(_latitude) * sin(_solarDeclination) + cos(_latitude) * cos(_solarDeclination) * cos(solarTime));
	float lightZenith = min(solarZenith, PI_ / 2 - 0.2f);
	float solarAzimuth = solarTime;

	SetSolarZenith(solarZenith);
	SetSolarAzimuth(solarAzimuth);

	Vector3 lightDir = Vector3(
		(float)(sin(solarAzimuth) * sin(lightZenith)),
		(float)(cos(lightZenith)),
		(float)(cos(solarAzimuth) * sin(lightZenith)));

	_lightDirection = lightDir;
}

int SkyboxNew::AddCubemap(const std::wstring &filepath)
{
	ID3D11ShaderResourceViewPtr srv;
	ID3D11ResourcePtr resource;
	DXCall(DirectX::CreateDDSTextureFromFileEx(_device, filepath.c_str(), 0, D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE, 0, 0, false,
		&resource, &srv, nullptr));

	_skyCubemaps.push_back(srv);

	return _skyCubemaps.size() - 1;
}

void SkyboxNew::SetCubemap(int index)
{
	if (index < _skyCubemaps.size())
	{
		_curCubemap = index;
		_isSky = false;
	}
}

void SkyboxNew::SetSky()
{
	_isSky = true;
}
