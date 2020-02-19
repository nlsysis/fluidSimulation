#include "LightShaderClass.h"
#include "common/RenderStates.h"

LightShaderClass::LightShaderClass()
	:
	DirLightResourceView(nullptr),
	PointLightResourceView(nullptr),
	SpotLightResourceView(nullptr)
{
}

LightShaderClass::~LightShaderClass()
{
}

bool LightShaderClass::Init(ID3D11Device* device)
{
	DirectionalLight dirLight;
	dirLight.Ambient = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
	dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	dirLight.Direction = XMFLOAT3(0.577f, 0.577f, 0.577f);
	mDirLights.push_back(dirLight);

	PointLight mPointLight;
	// Point light--position is changed every frame to animate in UpdateScene function.
	mPointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mPointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	mPointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	mPointLight.Range = 25.0f;
	mPointLights.push_back(mPointLight);

	SpotLight  mSpotLight;
	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	mSpotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mSpotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	mSpotLight.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mSpotLight.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	mSpotLight.Spot = 96.0f;
	mSpotLight.Range = 10000.0f;
	mSpotLights.push_back(mSpotLight);

	BuildShader(device);
	return true;
}

void LightShaderClass::RenderLight(ID3D11DeviceContext* deviceContext)
{
	SetLightParameters(deviceContext);
	deviceContext->IASetInputLayout(mInputLayout.Get());
	deviceContext->VSSetShader(mVertexShader.Get(), NULL, 0);
	deviceContext->PSSetShader(mPixelShader.Get(), NULL, 0);
}

void LightShaderClass::SetCBLightBufferPara(XMMATRIX mvp, XMFLOAT3 eyePosW, bool skipLighting, XMMATRIX camViewProjInv)
{
	m_matrixBufferType.mvp = mvp;
	m_lightBuffer.gEyePosW = eyePosW;
	m_lightBuffer.gSkipLighting = skipLighting;
	m_lightBuffer.gCamViewProjInv = camViewProjInv;
}

void LightShaderClass::SetLight(ID3D11DeviceContext * deviceContext)
{
	deviceContext->PSSetShaderResources(3, 1, DirLightResourceView.GetAddressOf());
	deviceContext->PSSetShaderResources(4, 1, PointLightResourceView.GetAddressOf());
	deviceContext->PSSetShaderResources(5, 1, SpotLightResourceView.GetAddressOf());
}

void LightShaderClass::BuildShader(ID3D11Device* device)
{
	ID3DBlob* pBlob = NULL;

	SafeCompileShaderFromFile(L".\\shader\\LightShader.hlsl", "LightShaderVS", "vs_5_0", &pBlob);
	HR(device->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, mVertexShader.GetAddressOf()));
	D3D11SetDebugObjectName(mVertexShader.Get(), "LightShaderVS");
	//create InputLayout
	const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	unsigned int numElements = sizeof(vertexDesc) / sizeof(vertexDesc[0]);
	device->CreateInputLayout(vertexDesc, numElements, pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(), mInputLayout.GetAddressOf());

	SAFE_RELEASE(pBlob);

	SafeCompileShaderFromFile(L".\\shader\\LightShader.hlsl", "LightShaderPS", "ps_5_0", &pBlob);
	HR(device->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, mPixelShader.GetAddressOf()));
	D3D11SetDebugObjectName(mPixelShader.Get(), "LightShaderPS");
	SAFE_RELEASE(pBlob);

	// Create Structured Buffers
	HR(CreateStructuredBuffer< DirectionalLight >(device,1, mDirLightBuffer.GetAddressOf(), DirLightResourceView.GetAddressOf(),NULL, mDirLights.data()));
	D3D11SetDebugObjectName(mDirLightBuffer.Get(), "DirLightBuffer");
	D3D11SetDebugObjectName(DirLightResourceView.Get(), "DirLightResourceView SRV");

	HR(CreateStructuredBuffer< PointLight >(device, 1 , mPointLightBuffer.GetAddressOf(), PointLightResourceView.GetAddressOf(), NULL, mPointLights.data()));
	D3D11SetDebugObjectName(mPointLightBuffer.Get(), "DirLightBuffer");
	D3D11SetDebugObjectName(DirLightResourceView.Get(), "DirLightResourceView SRV");
	
	HR(CreateStructuredBuffer< SpotLight >(device, 1 ,mSpotLightBuffer.GetAddressOf(), SpotLightResourceView.GetAddressOf(), NULL, mSpotLights.data()));
	D3D11SetDebugObjectName(mSpotLightBuffer.Get(), "DirLightBuffer");
	D3D11SetDebugObjectName(DirLightResourceView.Get(), "DirLightResourceView SRV");


	// Create Constant Buffers
	HR(CreateConstantBuffer< MatrixBufferType >(device, g_pcbMatrixBufferType.GetAddressOf()));
	HR(CreateConstantBuffer< cbLightBuffer >(device, g_pcbLightBuffer.GetAddressOf()));

}

void LightShaderClass::SetLightParameters(ID3D11DeviceContext* deviceContext)
{
	m_lightBuffer.gDirLightCount = (UINT)mDirLights.size();
	m_lightBuffer.gPointLightCount = (UINT)mPointLights.size();
	m_lightBuffer.gSpotLightCount = (UINT)mSpotLights.size();
	m_lightBuffer.padding = 0;

	deviceContext->UpdateSubresource(g_pcbMatrixBufferType.Get(), 0, NULL, &m_matrixBufferType, 0, 0);
	deviceContext->UpdateSubresource(g_pcbLightBuffer.Get(), 0, NULL, &m_lightBuffer, 0, 0);

	deviceContext->VSSetConstantBuffers(0, 1, g_pcbMatrixBufferType.GetAddressOf());
	deviceContext->PSSetConstantBuffers(1, 1, g_pcbLightBuffer.GetAddressOf());

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->PSSetSamplers(1, 1, RenderStates::SSPointClamped.GetAddressOf());
}

void LightShaderClass::SafeCompileShaderFromFile(const WCHAR * fileName, LPCSTR enterPoint, LPCSTR shaderModel, ID3DBlob ** ppBlob)
{
	ID3D10Blob* compilationMsgs = NULL;
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
#ifdef _DEBUG
	dwShaderFlags |= D3D10_SHADER_DEBUG;
	dwShaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = D3DX11CompileFromFile(fileName, 0, 0, enterPoint, shaderModel, D3D10_SHADER_ENABLE_STRICTNESS,
		0, 0, ppBlob, &compilationMsgs, 0);
	// compilationMsgs can store errors or warnings.
	if (compilationMsgs != 0)
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
		ReleaseCOM(compilationMsgs);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if (FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, L"D3DX11CompileFromFile", true);
	}
}

