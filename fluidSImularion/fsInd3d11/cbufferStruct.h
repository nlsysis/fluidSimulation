#pragma once
#include "common/d3dUtil.h"

struct Particle
{
	XMFLOAT3 vPosition;
	XMFLOAT3 vVelocity;
};

struct ParticleDensity
{
	float fDensity;
};

struct ParticleForces
{
	XMFLOAT3 vAcceleration;
};

struct UINT2
{
	UINT x;
	UINT y;
};
struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
struct MatrixBufferType
{
	XMMATRIX mvp;
};

//constant buffer layout
#pragma warning(push)
#pragma warning(disable:4324)   //structure was padded due to _declspec(align())
_DECLSPEC_ALIGN_16_ struct CBSimulationConstants3D
{
	UINT iNumParticles;
	float fTimeStep;
	float fSmoothlen;
	float fPressureStiffness;
	float fRestDensity;
	float fDensityCoef;
	float fGradPressureCoef;
	float fLapViscosityCoef;
	float fWallStiffness;

	XMFLOAT4  vGravity;
	XMFLOAT4  vGridDim;
	XMFLOAT4  vGridSize;
	XMFLOAT4  originPos;
};

struct CBTest
{
	UINT iNumParticles;
	XMFLOAT3  vGravity;
	float fTimeStep;
	float fSmoothlen;
	float fPressureStiffness;
	float fRestDensity;
	float fDensityCoef;
	float fGradPressureCoef;
	float fLapViscosityCoef;
	float fWallStiffness;
	
	float fParticleRadius;
	XMFLOAT3  vGridDim;

	XMFLOAT3  vGridSize;
	float boundaryDampening;    //two sphere collisiton
	XMFLOAT4  originPos;
};

_DECLSPEC_ALIGN_16_ struct CBPlanes
{
	XMFLOAT4  vPlanes[6];
};

_DECLSPEC_ALIGN_16_ struct CBRenderConstants
{
	XMFLOAT4X4 mViewProjection;
	float fParticleSize;
};

_DECLSPEC_ALIGN_16_ struct SortCB
{
	UINT iLevel;
	UINT iLevelMask;
	UINT iWidth;
	UINT iHeight;
};

_DECLSPEC_ALIGN_16_ struct CBEyePos
{
	XMFLOAT3 eyePosW;
	float padding = 0;
};
#pragma warning(pop)
// --------------------------------------------------------------------------------------
// Helper for creating constant buffers
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateConstantBuffer(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(T);
	HR(pd3dDevice->CreateBuffer(&Desc, NULL, ppCB));

	return hr;
}

template <class T>
HRESULT CreateConstantBuffer2(ID3D11Device* pd3dDevice, ID3D11Buffer** ppCB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.ByteWidth = sizeof(T);
	HR(pd3dDevice->CreateBuffer(&Desc, NULL, ppCB));

	return hr;
}

//--------------------------------------------------------------------------------------
// Helper for creating structured buffers with an SRV and UAV
//--------------------------------------------------------------------------------------
template <class T>
HRESULT CreateStructuredBuffer(ID3D11Device* pd3dDevice, UINT iNumElements, ID3D11Buffer** ppBuffer, ID3D11ShaderResourceView** ppSRV, ID3D11UnorderedAccessView** ppUAV, const T* pInitialData = NULL)
{
	HRESULT hr = S_OK;

	// Create SB
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.ByteWidth = iNumElements * sizeof(T);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufferDesc.StructureByteStride = sizeof(T);

	D3D11_SUBRESOURCE_DATA bufferInitData;
	ZeroMemory(&bufferInitData, sizeof(bufferInitData));
	bufferInitData.pSysMem = pInitialData;
	HR(pd3dDevice->CreateBuffer(&bufferDesc, (pInitialData) ? &bufferInitData : NULL, ppBuffer));

	// Create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementWidth = iNumElements;
	HR(pd3dDevice->CreateShaderResourceView(*ppBuffer, &srvDesc, ppSRV));

	// Create UAV
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = iNumElements;
	HR(pd3dDevice->CreateUnorderedAccessView(*ppBuffer, &uavDesc, ppUAV));

	return hr;
}
